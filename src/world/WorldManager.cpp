#include "WorldManager.h"

#include <iostream>
#include <ranges>

#include "WorldRenderer.h"
#include "../render/Renderer.h"
#include "../utils/ScopedTimer.h"

WorldManager::WorldManager() : m_threadPool(std::max(1u, std::thread::hardware_concurrency())) {
    m_chunksData.loadedMeshes.reserve(
        static_cast<size_t>(Renderer::s_renderDistance * Renderer::s_renderDistance * Renderer::s_renderDistance *
                            2.5f));
    m_tempKeysToProcess.reserve(100);

    const int r = static_cast<int>(std::ceil(Renderer::s_renderDistance / static_cast<float>(Chunk::SIZE)));
    const int r2 = r * r;

    m_renderDistanceOffsets.reserve(static_cast<size_t>(std::numbers::pi * static_cast<double>(r2)) + 1);
    for (int x = -r; x <= r; ++x) {
        for (int z = -r; z <= r; ++z) {
            if (const int d2 = x * x + z * z; d2 <= r2) {
                m_renderDistanceOffsets.push_back({
                    x, z,
                    static_cast<int>(std::floor(std::sqrt(static_cast<float>(r2 - d2))))
                });
            }
        }
    }

    std::sort(m_renderDistanceOffsets.begin(), m_renderDistanceOffsets.end(),
              [](const auto &a, const auto &b) {
                  return a.x * a.x + a.z * a.z < b.x * b.x + b.z * b.z;
              });
}

void WorldManager::updateChunks(const Camera &camera, IndirectRenderer &renderer) {
    if (camera.hasCameraChangedChunk() || m_renderDistanceChanged) {
        const int cameraWorldX = static_cast<int>(std::floor(camera.getPos().x / static_cast<float>(Chunk::SIZE))) *
                                 static_cast<int>(Chunk::SIZE);
        const int cameraWorldY = static_cast<int>(std::floor(camera.getPos().y / static_cast<float>(Chunk::SIZE))) *
                                 static_cast<int>(Chunk::SIZE);
        const int cameraWorldZ = static_cast<int>(std::floor(camera.getPos().z / static_cast<float>(Chunk::SIZE))) *
                                 static_cast<int>(Chunk::SIZE);
        const glm::vec3 cameraChunkPos(cameraWorldX, cameraWorldY, cameraWorldZ);

        getDistantChunks(cameraChunkPos);
        m_threadPool.enqueue_no_future([this, cameraWorldX, cameraWorldY, cameraWorldZ] {
            generateChunksPositions(cameraWorldX, cameraWorldY, cameraWorldZ);
        });
    }

    m_needInstanceUpdate = camera.hasCameraChangedDirection() || camera.hasCameraChangedChunk() ||
                           m_renderDistanceChanged;
    processChunksQueues(renderer);
    m_renderDistanceChanged = false;
}

void WorldManager::updateRenderDistance(Shader &postProcessingShader, const Camera &camera,
                                        IndirectRenderer &renderer) {
    postProcessingShader.use();
    postProcessingShader.setUniform1f("u_RenderDistance", Renderer::s_renderDistance);

    m_renderDistanceOffsets.clear();

    const int r = static_cast<int>(std::ceil(Renderer::s_renderDistance / static_cast<float>(Chunk::SIZE)));
    const int r2 = r * r;

    m_renderDistanceOffsets.reserve(static_cast<size_t>(std::numbers::pi * static_cast<double>(r2)) + 1);
    for (int x = -r; x <= r; ++x) {
        for (int z = -r; z <= r; ++z) {
            if (const int d2 = x * x + z * z; d2 <= r2) {
                m_renderDistanceOffsets.push_back({
                    x, z,
                    static_cast<int>(std::floor(std::sqrt(static_cast<float>(r2 - d2))))
                });
            }
        }
    }

    std::sort(m_renderDistanceOffsets.begin(), m_renderDistanceOffsets.end(),
              [](const auto &a, const auto &b) {
                  return a.x * a.x + a.z * a.z < b.x * b.x + b.z * b.z;
              });

    m_renderDistanceChanged = true;
    updateChunks(camera, renderer);
}

void WorldManager::addPendingBlocks(const std::unordered_map<ChunkPosition, std::vector<PendingBlock> > &blockData) {
    std::lock_guard lock(m_chunksData.pendingBlocksMutex);
    for (const auto &[key, blocks]: blockData) {
        auto &targetVector = m_chunksData.pendingBlocks[key];
        targetVector.reserve(targetVector.size() + blocks.size());
        targetVector.insert(targetVector.end(), blocks.begin(), blocks.end());
    }
}

void WorldManager::deleteBlockAndUpdateNeighbors(const RaycastResult &hit) {
    if (hit.blockType == Block::BlockType::BEDROCK) return;

    // TODO: Implement partial mesh update
    m_threadPool.enqueue_no_future([this, hit] {
        ScopedTimer timer("Delete block");

        const Block::BlockType type = hit.blockType;
        const auto &blockLocalPosition = hit.blockLocalPosition;
        const std::shared_ptr<Chunk> chunk = hit.chunk;

        // Update adjacents chunks if the block is at the border of the chunk
        const bool isAtLeftBorder = blockLocalPosition[0] == 0;
        const bool isAtRightBorder = blockLocalPosition[0] == Chunk::SIZE - 1;
        const bool isAtBottomBorder = blockLocalPosition[1] == 0;
        const bool isAtTopBorder = blockLocalPosition[1] == Chunk::SIZE - 1;
        const bool isAtFrontBorder = blockLocalPosition[2] == 0;
        const bool isAtBackBorder = blockLocalPosition[2] == Chunk::SIZE - 1;

        const bool isInstance = Block::isInstance(type);
        if (!isAtLeftBorder && !isAtRightBorder &&
            !isAtBottomBorder && !isAtTopBorder &&
            !isAtFrontBorder && !isAtBackBorder) {
            // If the block is not at the border, we can delete it without updating neighbors
            MeshingResult result;
            chunk->deleteBlock(blockLocalPosition[0], blockLocalPosition[1], blockLocalPosition[2], type,
                               result);

            result.position = {chunk->getX(), chunk->getY(), chunk->getZ()};
            result.needIndirectRendererUpdate = !isInstance;
            result.needInstanceUpdate = isInstance;

            m_chunksData.completedMeshes.push(std::move(result));
            return;
        }

        // Update adjacent chunks if the block is at the border of the chunk
        for (int i = -1; i <= 1; ++i) {
            for (int j = -1; j <= 1; ++j) {
                for (int k = -1; k <= 1; ++k) {
                    if (i == 0 && j == 0 && k == 0) {
                        continue;
                    }

                    if (i == -1 && !isAtLeftBorder || i == 1 && !isAtRightBorder ||
                        j == -1 && !isAtBottomBorder || j == 1 && !isAtTopBorder ||
                        k == -1 && !isAtFrontBorder || k == 1 && !isAtBackBorder) {
                        continue;
                    }

                    constexpr int chunkSize = Chunk::SIZE;
                    if (const auto adjacentChunk = getChunk(
                        chunk->getX() + i * chunkSize,
                        chunk->getY() + j * chunkSize,
                        chunk->getZ() + k * chunkSize)) {
                        constexpr int minBlockPos = -1; // chunk will add +1 when accessing the block
                        constexpr int maxBlockPos = Chunk::SIZE; // chunk will add +1 when accessing the block

                        const int adjX = i == 0 ? blockLocalPosition[0] : i == -1 ? maxBlockPos : minBlockPos;
                        const int adjY = j == 0 ? blockLocalPosition[1] : j == -1 ? maxBlockPos : minBlockPos;
                        const int adjZ = k == 0 ? blockLocalPosition[2] : k == -1 ? maxBlockPos : minBlockPos;

                        MeshingResult result;
                        adjacentChunk->deleteBlock(adjX, adjY, adjZ, type, result);

                        result.position = {adjacentChunk->getX(), adjacentChunk->getY(), adjacentChunk->getZ()};
                        result.needIndirectRendererUpdate = !isInstance;
                        result.needInstanceUpdate = isInstance;

                        m_chunksData.completedMeshes.push(std::move(result));
                    }
                }
            }
        }

        // Delete in current chunk last to avoid popping issues because meshing is too slow
        MeshingResult result;
        chunk->deleteBlock(blockLocalPosition[0], blockLocalPosition[1], blockLocalPosition[2], type, result);

        result.position = {chunk->getX(), chunk->getY(), chunk->getZ()};
        result.needIndirectRendererUpdate = !isInstance;
        result.needInstanceUpdate = isInstance;

        m_chunksData.completedMeshes.push(std::move(result));
    });
}

void WorldManager::placeBlockAndUpdateNeighbors(const RaycastResult &hit, Block::BlockType blockToPlace) {
    if (blockToPlace == Block::BlockType::AIR) return;

    // TODO: Implement partial mesh update
    m_threadPool.enqueue_no_future([this, hit, blockToPlace] {
        ScopedTimer timer("Place block");

        const std::shared_ptr<Chunk> chunk = hit.chunk;

        // Determine coords based on normal
        const int offsetX = hit.normal.x == 0 ? 0 : hit.normal.x > 0 ? 1 : -1;
        const int offsetY = hit.normal.y == 0 ? 0 : hit.normal.y > 0 ? 1 : -1;
        const int offsetZ = hit.normal.z == 0 ? 0 : hit.normal.z > 0 ? 1 : -1;

        const int newLocalX = hit.blockLocalPosition[0] + offsetX;
        const int newLocalY = hit.blockLocalPosition[1] + offsetY;
        const int newLocalZ = hit.blockLocalPosition[2] + offsetZ;

        constexpr int chunkSize = Chunk::SIZE;

        std::shared_ptr<Chunk> targetChunk = chunk;
        int targetX = newLocalX;
        int targetY = newLocalY;
        int targetZ = newLocalZ;
        int chunkOffsetX = 0, chunkOffsetY = 0, chunkOffsetZ = 0;

        // If the new block position is at the border of the chunk, we need to adjust the target chunk and position
        if (newLocalX < 0) {
            chunkOffsetX = -chunkSize;
            targetX = chunkSize - 1;
        } else if (newLocalX >= chunkSize) {
            chunkOffsetX = chunkSize;
            targetX = 0;
        }

        if (newLocalY < 0) {
            chunkOffsetY = -chunkSize;
            targetY = chunkSize - 1;
        } else if (newLocalY >= chunkSize) {
            chunkOffsetY = chunkSize;
            targetY = 0;
        }

        if (newLocalZ < 0) {
            chunkOffsetZ = -chunkSize;
            targetZ = chunkSize - 1;
        } else if (newLocalZ >= chunkSize) {
            chunkOffsetZ = chunkSize;
            targetZ = 0;
        }

        if (chunkOffsetX != 0 || chunkOffsetY != 0 || chunkOffsetZ != 0) {
            targetChunk = getChunk(
                chunk->getX() + chunkOffsetX,
                chunk->getY() + chunkOffsetY,
                chunk->getZ() + chunkOffsetZ
            );
        }

        if (!targetChunk) return;

        // Place the block in the target chunk
        const bool isInstance = Block::isInstance(blockToPlace);

        // Scope so IDE does not complain about shadowed variables
        {
            MeshingResult result;
            targetChunk->addBlock(targetX, targetY, targetZ, blockToPlace, result);

            result.position = {targetChunk->getX(), targetChunk->getY(), targetChunk->getZ()};
            result.needIndirectRendererUpdate = !isInstance;
            result.needInstanceUpdate = isInstance;
            m_chunksData.completedMeshes.push(std::move(result));
        }


        // Check borders for new block position
        const bool willBeAtLeftBorder = targetX == 0;
        const bool willBeAtRightBorder = targetX == Chunk::SIZE - 1;
        const bool willBeAtBottomBorder = targetY == 0;
        const bool willBeAtTopBorder = targetY == Chunk::SIZE - 1;
        const bool willBeAtFrontBorder = targetZ == 0;
        const bool willBeAtBackBorder = targetZ == Chunk::SIZE - 1;

        if (!willBeAtLeftBorder && !willBeAtRightBorder &&
            !willBeAtBottomBorder && !willBeAtTopBorder &&
            !willBeAtFrontBorder && !willBeAtBackBorder) {
            return;
        }

        // Update adjacent chunks if the new block is at the border of the chunk
        for (int i = -1; i <= 1; ++i) {
            for (int j = -1; j <= 1; ++j) {
                for (int k = -1; k <= 1; ++k) {
                    if (i == 0 && j == 0 && k == 0) {
                        continue;
                    }

                    if (i == -1 && !willBeAtLeftBorder || i == 1 && !willBeAtRightBorder ||
                        j == -1 && !willBeAtBottomBorder || j == 1 && !willBeAtTopBorder ||
                        k == -1 && !willBeAtFrontBorder || k == 1 && !willBeAtBackBorder) {
                        continue;
                    }

                    if (const auto adjacentChunk = getChunk(
                        targetChunk->getX() + i * chunkSize,
                        targetChunk->getY() + j * chunkSize,
                        targetChunk->getZ() + k * chunkSize)) {
                        constexpr int minBlockPos = -1; // chunk will add +1 when accessing the block
                        constexpr int maxBlockPos = Chunk::SIZE; // chunk will add +1 when accessing the block

                        const int adjX = i == 0 ? targetX : i == -1 ? maxBlockPos : minBlockPos;
                        const int adjY = j == 0 ? targetY : j == -1 ? maxBlockPos : minBlockPos;
                        const int adjZ = k == 0 ? targetZ : k == -1 ? maxBlockPos : minBlockPos;

                        MeshingResult result;
                        adjacentChunk->addBlock(adjX, adjY, adjZ, blockToPlace, result);

                        result.position = {adjacentChunk->getX(), adjacentChunk->getY(), adjacentChunk->getZ()};
                        result.needIndirectRendererUpdate = !isInstance;
                        result.needInstanceUpdate = isInstance;
                        m_chunksData.completedMeshes.push(std::move(result));
                    }
                }
            }
        }
    });
}

const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk> > &WorldManager::getLoadedChunks() const {
    return m_chunksData.loadedMeshes;
}

bool WorldManager::getNeedInstanceUpdate() const {
    return m_needInstanceUpdate;
}

void WorldManager::processChunksQueues(IndirectRenderer &renderer) {
    const int maxChunksPerFrame = static_cast<int>(
        0.05f * Renderer::s_renderDistance + 0.2f * static_cast<float>(m_threadPool.getNumberOfThreads()));

    // Destroy chunks that are no longer needed
    for (int i = 0; i < maxChunksPerFrame; ++i) {
        if (m_chunksData.meshesToDelete.empty()) break;
        auto chunk = m_chunksData.meshesToDelete.pop();
        renderer.removeChunk(chunk);
        m_chunksData.loadedMeshes.erase({chunk->getX(), chunk->getY(), chunk->getZ()});
    }

    // First pass: generate voxel and mesh
    for (int i = 0; i < maxChunksPerFrame; ++i) {
        if (m_chunksData.meshesToGenerate.empty()) break;

        const ChunkPosition key = m_chunksData.meshesToGenerate.pop();
        const auto p_chunk = std::make_shared<Chunk>(key.x, key.y, key.z);
        m_chunksData.loadedMeshes.try_emplace(key, p_chunk);

        m_threadPool.enqueue_no_future([this, p_chunk] {
            p_chunk->generateVoxel();
            if (p_chunk->isEmpty()) return;
            // Will be deleted when out of range, don't delete now to avoid it being reloaded immediately

            p_chunk->generateMesh();
            m_needInstanceUpdate.store(true);
            p_chunk->transferPendingBlocksToWorld(*this);
        });
    }

    // Second pass: generate pending blocks
    // Check if no pending blocks have been added/removed since the last frame because needed chunk was not loaded yet
    if (!m_chunksData.pendingBlocks.empty() && m_chunksData.pendingBlocks.size() != m_chunksData.lastPendingBlockSize) {
        m_chunksData.lastPendingBlockSize = m_chunksData.pendingBlocks.size();
        m_tempKeysToProcess.clear(); {
            std::lock_guard lock(m_chunksData.pendingBlocksMutex);
            m_tempKeysToProcess.reserve(m_chunksData.pendingBlocks.size());
            for (const auto &key: m_chunksData.pendingBlocks | std::views::keys) {
                m_tempKeysToProcess.push_back(key);
            }
        }

        for (const auto &key: m_tempKeysToProcess) {
            if (auto it = m_chunksData.loadedMeshes.find(key); it != m_chunksData.loadedMeshes.end()) {
                const std::shared_ptr<Chunk> p_chunk = it->second;
                if (p_chunk->getState() < Mesh::State::VOXEL_GENERATED) continue;

                std::vector<PendingBlock> blocks; {
                    std::lock_guard lock(m_chunksData.pendingBlocksMutex);
                    if (auto pending_it = m_chunksData.pendingBlocks.find(key);
                        pending_it != m_chunksData.pendingBlocks.end()) {
                        blocks = std::move(pending_it->second);
                        m_chunksData.pendingBlocks.erase(pending_it);
                    }
                }

                if (!blocks.empty()) {
                    m_threadPool.enqueue_no_future([this, p_chunk, blocks = std::move(blocks)]() mutable {
                        MeshingResult result;
                        p_chunk->generatePendingBlocks(blocks, result);

                        result.position = {p_chunk->getX(), p_chunk->getY(), p_chunk->getZ()};
                        result.needIndirectRendererUpdate = true;
                        result.needInstanceUpdate = true;

                        m_chunksData.completedMeshes.push(std::move(result));
                    });
                }
            }
        }
    }

    // Third pass: update meshes that needs it
    for (int i = 0; i < maxChunksPerFrame; ++i) {
        if (m_chunksData.completedMeshes.empty()) break;
        auto [opaqueVertices, waterVertices, position,
            hasOpaqueFaces, hasWaterFaces,
            needInstanceUpdate, needIndirectRendererUpdate] = m_chunksData.completedMeshes.pop();

        if (const auto it = m_chunksData.loadedMeshes.find(position);
            it != m_chunksData.loadedMeshes.end()) {
            const std::shared_ptr<Chunk> p_chunk = it->second;

            if (needIndirectRendererUpdate) {
                p_chunk->getOpaqueVertices().swap(opaqueVertices);
                p_chunk->getWaterVertices().swap(waterVertices);
                p_chunk->setHasOpaqueFaces(hasOpaqueFaces);
                p_chunk->setHasWaterFaces(hasWaterFaces);
                p_chunk->updateVertexCount();
                renderer.updateChunk(p_chunk);
            }

            m_needInstanceUpdate.store(m_needInstanceUpdate.load() || needInstanceUpdate);
        }
    }
}

void WorldManager::generateChunksPositions(const int cameraWorldX, const int cameraWorldY, const int cameraWorldZ) {
    for (auto &[x,z, maxY]: m_renderDistanceOffsets) {
        const int chunkX = cameraWorldX + static_cast<int>(x * Chunk::SIZE);
        const int chunkZ = cameraWorldZ + static_cast<int>(z * Chunk::SIZE);

        for (int y = -maxY; y <= maxY; ++y) {
            const int chunkY = cameraWorldY + static_cast<int>(y * Chunk::SIZE);
            if (chunkY < 0 || chunkY > 256) continue; // World height limit

            const ChunkPosition key = {chunkX, chunkY, chunkZ};
            if (m_chunksData.loadedMeshes.contains(key)) continue;
            m_chunksData.meshesToGenerate.push(key);
        }
    }
}

void WorldManager::getDistantChunks(const glm::vec3 &cameraChunkPos) {
    const float renderDistanceSq = Renderer::s_renderDistance * Renderer::s_renderDistance;

    const float camX = cameraChunkPos.x;
    const float camY = cameraChunkPos.y;
    const float camZ = cameraChunkPos.z;

    for (auto &[position, chunk]: m_chunksData.loadedMeshes) {
        const float dx = static_cast<float>(position.x) - camX;
        const float dy = static_cast<float>(position.y) - camY;
        const float dz = static_cast<float>(position.z) - camZ;
        if (const float distSq = dx * dx + dy * dy + dz * dz;
            distSq > renderDistanceSq) {
            m_chunksData.meshesToDelete.push(chunk);
        }
    }
}

std::shared_ptr<Chunk> WorldManager::getChunk(const int x, const int y, const int z) const {
    if (const auto it = m_chunksData.loadedMeshes.find({x, y, z}); it != m_chunksData.loadedMeshes.end()) {
        return it->second;
    }

    // Should not happen, but if it does, return a null chunk
    static auto nullChunk = std::make_shared<Chunk>(-1, -1, -1); // Return a null chunk if not found
    std::cerr << "Chunk not found at (" << x << ", " << y << ", " << z << ")\n";
    return nullChunk;
}
