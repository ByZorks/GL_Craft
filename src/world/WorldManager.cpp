#include "WorldManager.h"

#include <iostream>
#include <numbers>
#include <ranges>

#include "../render/WorldRenderer.h"
#include "../render/Renderer.h"
#include "../utils/ScopedTimer.h"

WorldManager::WorldManager() : m_threadPool(std::max(1u, std::thread::hardware_concurrency())) {
    m_chunksData.loadedMeshes.reserve(
        static_cast<size_t>(Renderer::s_renderDistance * Renderer::s_renderDistance * Renderer::s_renderDistance *
                            2.5f));

    const auto r = static_cast<int>(std::ceil(Renderer::s_renderDistance / static_cast<float>(Chunk::SIZE)));
    const int r2 = r * r;

    m_renderDistanceOffsets.reserve(static_cast<size_t>(std::numbers::pi * static_cast<double>(r2)) + 1);
    for (int x = -r; x <= r; ++x) {
        for (int z = -r; z <= r; ++z) {
            if (const int d2 = x * x + z * z; d2 <= r2) {
                m_renderDistanceOffsets.emplace_back(
                    x, z, static_cast<int>(std::floor(std::sqrt(static_cast<float>(r2 - d2))))
                );
            }
        }
    }

    std::ranges::sort(m_renderDistanceOffsets,
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

void WorldManager::updateRenderDistance(Shader &postProcessingShader, const Camera &camera, IndirectRenderer &renderer) {
    postProcessingShader.use();
    postProcessingShader.setUniform1f("u_RenderDistance", Renderer::s_renderDistance);

    m_renderDistanceOffsets.clear();

    const auto r = static_cast<int>(std::ceil(Renderer::s_renderDistance / static_cast<float>(Chunk::SIZE)));
    const int r2 = r * r;

    m_renderDistanceOffsets.reserve(static_cast<size_t>(std::numbers::pi * static_cast<double>(r2)) + 1);
    for (int x = -r; x <= r; ++x) {
        for (int z = -r; z <= r; ++z) {
            if (const int d2 = x * x + z * z; d2 <= r2) {
                m_renderDistanceOffsets.emplace_back(
                    x, z, static_cast<int>(std::floor(std::sqrt(static_cast<float>(r2 - d2))))
                );
            }
        }
    }

    std::ranges::sort(m_renderDistanceOffsets,
                      [](const auto &a, const auto &b) {
                          return a.x * a.x + a.z * a.z < b.x * b.x + b.z * b.z;
                      });

    m_renderDistanceChanged = true;
    updateChunks(camera, renderer);
}

void WorldManager::addPendingBlocks(std::unordered_map<ChunkPosition, std::list<PendingBlock> > &blockData) {
    std::lock_guard lock(m_chunksData.pendingBlocksMutex);
    for (auto & [key, blocks] : blockData) {
        m_chunksData.pendingBlocks[key].splice(m_chunksData.pendingBlocks[key].end(), blocks);
    }
}

void WorldManager::addPendingLights(std::unordered_map<ChunkPosition, std::list<PendingLight> > &lightData) {
    std::lock_guard lock(m_chunksData.pendingLightsMutex);
    for (auto &[key, lights]: lightData) {
        m_chunksData.pendingLights[key].splice(m_chunksData.pendingLights[key].end(), lights);
    }
}

void WorldManager::deleteBlockAndUpdateNeighbors(const RaycastResult &hit) {
    if (hit.blockType == Block::BlockType::BEDROCK) return;

    // TODO: Implement partial mesh update
    m_threadPool.enqueue_no_future([this, hit] {
        asyncDeleteBlock(hit);
    });
}

void WorldManager::updateAdjacentChunksAfterDeletion(const std::shared_ptr<Chunk> &chunk, const std::array<int, 3> &blockLocalPos,
                                                     const Block::BlockType blockType,
                                                     std::unordered_set<ChunkPosition> &outProcessedChunks) {
    const bool isAtLeftBorder = blockLocalPos[0] == 0;
    const bool isAtRightBorder = blockLocalPos[0] == Chunk::SIZE - 1;
    const bool isAtBottomBorder = blockLocalPos[1] == 0;
    const bool isAtTopBorder = blockLocalPos[1] == Chunk::SIZE - 1;
    const bool isAtFrontBorder = blockLocalPos[2] == 0;
    const bool isAtBackBorder = blockLocalPos[2] == Chunk::SIZE - 1;

    if (!isAtLeftBorder && !isAtRightBorder && !isAtBottomBorder && !isAtTopBorder && !isAtFrontBorder &&
        !isAtBackBorder) {
        return;
    }

    const bool isInstance = Block::isInstance(blockType);
    const bool isLightEmitter = Block::isLightEmitter(blockType);

    const int startX = isAtLeftBorder ? -1 : 0;
    const int endX = isAtRightBorder ? 1 : 0;
    const int startY = isAtBottomBorder ? -1 : 0;
    const int endY = isAtTopBorder ? 1 : 0;
    const int startZ = isAtFrontBorder ? -1 : 0;
    const int endZ = isAtBackBorder ? 1 : 0;

    for (int i = startX; i <= endX; ++i) {
        for (int j = startY; j <= endY; ++j) {
            for (int k = startZ; k <= endZ; ++k) {
                if (i == 0 && j == 0 && k == 0) continue;

                constexpr int chunkSize = Chunk::SIZE;
                const ChunkPosition neighborPos = {
                    chunk->getX() + i * chunkSize, chunk->getY() + j * chunkSize, chunk->getZ() + k * chunkSize
                };

                processAdjacentChunkOnDeletion(neighborPos, blockLocalPos, {i, j, k}, blockType, isInstance, isLightEmitter,
                                               outProcessedChunks);
            }
        }
    }
}

void WorldManager::processAdjacentChunkOnDeletion(const ChunkPosition &neighborPos, const std::array<int, 3> &blockLocalPos,
                                                  const std::array<int, 3> &neighborOffset, const Block::BlockType blockType,
                                                  const bool isInstance, const bool isLightEmitter,
                                                  std::unordered_set<ChunkPosition> &outProcessedChunks) {
    if (const auto adjacentChunk = getChunk(neighborPos)) {
        constexpr int minBlockPos = -1; // chunk will add +1 when accessing the block
        constexpr int maxBlockPos = Chunk::SIZE; // chunk will add +1 when accessing the block

        int adjX;
        if (neighborOffset[0] == 0) {
            adjX = blockLocalPos[0];
        } else if (neighborOffset[0] == -1) {
            adjX = maxBlockPos;
        } else {
            adjX = minBlockPos;
        }

        int adjY;
        if (neighborOffset[1] == 0) {
            adjY = blockLocalPos[1];
        } else if (neighborOffset[1] == -1) {
            adjY = maxBlockPos;
        } else {
            adjY = minBlockPos;
        }

        int adjZ;
        if (neighborOffset[2] == 0) {
            adjZ = blockLocalPos[2];
        } else if (neighborOffset[2] == -1) {
            adjZ = maxBlockPos;
        } else {
            adjZ = minBlockPos;
        }

        MeshingResult neighborResult;
        neighborResult.opaqueVertices.reserve(adjacentChunk->getOpaqueVertexVectorSize());
        neighborResult.waterVertices.reserve(adjacentChunk->getWaterVertexVectorSize());
        adjacentChunk->deleteBlock(adjX, adjY, adjZ, blockType, neighborResult);

        neighborResult.position = {adjacentChunk->getX(), adjacentChunk->getY(), adjacentChunk->getZ()};
        neighborResult.needIndirectRendererUpdate = !isInstance || isLightEmitter;
        neighborResult.needInstanceUpdate = isInstance;

        m_chunksData.completedMeshes.push(std::move(neighborResult));
        adjacentChunk->transferPendingLightsToWorld(*this);

        outProcessedChunks.emplace(neighborPos);
    }
}

void WorldManager::propagateLightRemoval(const std::shared_ptr<Chunk> &chunk, const std::array<int, 3> &blockLocalPos,
                                         const std::unordered_set<ChunkPosition> &outProcessedChunks) {
    constexpr int lightPropagationRadius = 15;
    constexpr int chunkSize = Chunk::SIZE;
    const auto chunkRadius = static_cast<int>(std::ceil(lightPropagationRadius / static_cast<float>(chunkSize)));
    constexpr int radiusSq = lightPropagationRadius * lightPropagationRadius;

    const int worldBlockX = chunk->getX() + blockLocalPos[0];
    const int worldBlockY = chunk->getY() + blockLocalPos[1];
    const int worldBlockZ = chunk->getZ() + blockLocalPos[2];

    for (int dx = -chunkRadius; dx <= chunkRadius; ++dx) {
        for (int dy = -chunkRadius; dy <= chunkRadius; ++dy) {
            for (int dz = -chunkRadius; dz <= chunkRadius; ++dz) {
                const ChunkPosition pos{
                    chunk->getX() + dx * chunkSize, chunk->getY() + dy * chunkSize, chunk->getZ() + dz * chunkSize
                };

                if (outProcessedChunks.contains(pos)) continue;

                const int minDistX = std::max( 0, std::max(pos.x - worldBlockX, worldBlockX - (pos.x + chunkSize - 1)));
                const int minDistY = std::max(0, std::max(pos.y - worldBlockY, worldBlockY - (pos.y + chunkSize - 1)));
                const int minDistZ = std::max(0, std::max(pos.z - worldBlockZ, worldBlockZ - (pos.z + chunkSize - 1)));

                if (const int minDistSq = minDistX * minDistX + minDistY * minDistY + minDistZ * minDistZ;
                    minDistSq > radiusSq) continue;

                if (const auto adjacentChunk = getChunk(pos)) {
                    MeshingResult neighborResult;
                    neighborResult.opaqueVertices.reserve(adjacentChunk->getOpaqueVertexVectorSize());
                    neighborResult.waterVertices.reserve(adjacentChunk->getWaterVertexVectorSize());
                    adjacentChunk->propagateLight();
                    adjacentChunk->generateNewMesh(neighborResult);

                    neighborResult.position = {adjacentChunk->getX(), adjacentChunk->getY(), adjacentChunk->getZ()};
                    neighborResult.needIndirectRendererUpdate = true;
                    neighborResult.needInstanceUpdate = false;

                    m_chunksData.completedMeshes.push(std::move(neighborResult));
                    adjacentChunk->transferPendingLightsToWorld(*this);
                }
            }
        }
    }
}

void WorldManager::asyncDeleteBlock(const RaycastResult &hit) {
    ScopedTimer timer("Delete block");

    const Block::BlockType type = hit.blockType;
    const auto &blockLocalPosition = hit.blockLocalPosition;
    const std::shared_ptr<Chunk> chunk = hit.chunk;

    const bool isInstance = Block::isInstance(type);
    const bool isLightEmitter = Block::isLightEmitter(type);

    MeshingResult result;
    result.opaqueVertices.reserve(chunk->getOpaqueVertexVectorSize());
    result.waterVertices.reserve(chunk->getWaterVertexVectorSize());
    chunk->deleteBlock(blockLocalPosition[0], blockLocalPosition[1], blockLocalPosition[2], type, result);

    result.position = {chunk->getX(), chunk->getY(), chunk->getZ()};
    result.needIndirectRendererUpdate = !isInstance || isLightEmitter;
    result.needInstanceUpdate = isInstance;

    m_chunksData.completedMeshes.push(std::move(result));

    std::unordered_set<ChunkPosition> processedChunks;
    processedChunks.reserve(7);
    processedChunks.emplace(chunk->getX(), chunk->getY(), chunk->getZ());

    updateAdjacentChunksAfterDeletion(chunk, blockLocalPosition, type, processedChunks);

    if (isLightEmitter) {
        propagateLightRemoval(chunk, blockLocalPosition, processedChunks);
    }

    emitNeighborsBorderLights(chunk);
    chunk->transferPendingLightsToWorld(*this);
}

void WorldManager::placeBlockAndUpdateNeighbors(const RaycastResult &hit, const Block::BlockType blockToPlace) {
    if (blockToPlace == Block::BlockType::AIR) return;

    // TODO: Implement partial mesh update
    m_threadPool.enqueue_no_future([this, hit, blockToPlace] {
        asyncPlaceBlock(hit, blockToPlace);
    });
}

void WorldManager::asyncPlaceBlock(const RaycastResult &hit, const Block::BlockType blockToPlace) {
    ScopedTimer timer("Place block");

    auto [targetChunk, targetLocalPosition] = determineTargetChunkAndPosition(hit);
    if (!targetChunk) return;

    const bool isInstance = Block::isInstance(blockToPlace);
    const bool isLightEmitter = Block::isLightEmitter(blockToPlace);

    MeshingResult result;
    result.opaqueVertices.reserve(targetChunk->getOpaqueVertexVectorSize());
    result.waterVertices.reserve(targetChunk->getWaterVertexVectorSize());
    targetChunk->addBlock(targetLocalPosition[0], targetLocalPosition[1], targetLocalPosition[2], blockToPlace, result);

    result.position = {targetChunk->getX(), targetChunk->getY(), targetChunk->getZ()};
    result.needIndirectRendererUpdate = !isInstance || isLightEmitter;
    result.needInstanceUpdate = isInstance;
    m_chunksData.completedMeshes.push(std::move(result));

    std::unordered_set<ChunkPosition> processedChunks;
    processedChunks.reserve(7);
    processedChunks.emplace(targetChunk->getX(), targetChunk->getY(), targetChunk->getZ());

    updateAdjacentChunksAfterPlacement(targetChunk, targetLocalPosition, blockToPlace, processedChunks);

    if (isLightEmitter) {
        propagateLightAddition(targetChunk, targetLocalPosition, processedChunks);
    }

    emitNeighborsBorderLights(targetChunk);
    targetChunk->transferPendingLightsToWorld(*this);
}

std::pair<std::shared_ptr<Chunk>, std::array<int, 3>> WorldManager::determineTargetChunkAndPosition(const RaycastResult &hit) const {
    auto getOffsetFromNormal = [&](const float normalComponent) {
        if (normalComponent > 0.0f) return 1;
        if (normalComponent < 0.0f) return -1;
        return 0;
    };
    const int offsetX = getOffsetFromNormal(static_cast<const float>(hit.normal.x));
    const int offsetY = getOffsetFromNormal(static_cast<const float>(hit.normal.y));
    const int offsetZ = getOffsetFromNormal(static_cast<const float>(hit.normal.z));

    const int newLocalX = hit.blockLocalPosition[0] + offsetX;
    const int newLocalY = hit.blockLocalPosition[1] + offsetY;
    const int newLocalZ = hit.blockLocalPosition[2] + offsetZ;

    const std::shared_ptr<Chunk> baseChunk = hit.chunk;
    int targetX = newLocalX;
    int targetY = newLocalY;
    int targetZ = newLocalZ;
    int chunkOffsetX = 0;
    int chunkOffsetY = 0;
    int chunkOffsetZ = 0;

    constexpr int chunkSize = Chunk::SIZE;

    if (newLocalX < 0) { chunkOffsetX = -chunkSize; targetX = chunkSize - 1; }
    else if (newLocalX >= chunkSize) { chunkOffsetX = chunkSize; targetX = 0; }

    if (newLocalY < 0) { chunkOffsetY = -chunkSize; targetY = chunkSize - 1; }
    else if (newLocalY >= chunkSize) { chunkOffsetY = chunkSize; targetY = 0; }

    if (newLocalZ < 0) { chunkOffsetZ = -chunkSize; targetZ = chunkSize - 1; }
    else if (newLocalZ >= chunkSize) { chunkOffsetZ = chunkSize; targetZ = 0; }

    std::shared_ptr<Chunk> targetChunk = baseChunk;
    if (chunkOffsetX != 0 || chunkOffsetY != 0 || chunkOffsetZ != 0) {
        targetChunk = getChunk({
            baseChunk->getX() + chunkOffsetX,
            baseChunk->getY() + chunkOffsetY,
            baseChunk->getZ() + chunkOffsetZ
        });
    }

    return {targetChunk, {targetX, targetY, targetZ}};
}

void WorldManager::updateAdjacentChunksAfterPlacement(const std::shared_ptr<Chunk> &chunk, const std::array<int, 3> &blockLocalPos,
                                                      const Block::BlockType blockType,
                                                      std::unordered_set<ChunkPosition> &outProcessedChunks) {
    const bool isAtLeftBorder = blockLocalPos[0] == 0;
    const bool isAtRightBorder = blockLocalPos[0] == Chunk::SIZE - 1;
    const bool isAtBottomBorder = blockLocalPos[1] == 0;
    const bool isAtTopBorder = blockLocalPos[1] == Chunk::SIZE - 1;
    const bool isAtFrontBorder = blockLocalPos[2] == 0;
    const bool isAtBackBorder = blockLocalPos[2] == Chunk::SIZE - 1;

    if (!isAtLeftBorder && !isAtRightBorder && !isAtBottomBorder && !isAtTopBorder && !isAtFrontBorder &&
        !isAtBackBorder) {
        return;
    }

    const bool isInstance = Block::isInstance(blockType);
    const bool isLightEmitter = Block::isLightEmitter(blockType);

    const int startX = isAtLeftBorder ? -1 : 0;
    const int endX = isAtRightBorder ? 1 : 0;
    const int startY = isAtBottomBorder ? -1 : 0;
    const int endY = isAtTopBorder ? 1 : 0;
    const int startZ = isAtFrontBorder ? -1 : 0;
    const int endZ = isAtBackBorder ? 1 : 0;

    for (int i = startX; i <= endX; ++i) {
        for (int j = startY; j <= endY; ++j) {
            for (int k = startZ; k <= endZ; ++k) {
                if (i == 0 && j == 0 && k == 0) continue;

                constexpr int chunkSize = Chunk::SIZE;
                const ChunkPosition neighborPos = {
                    chunk->getX() + i * chunkSize, chunk->getY() + j * chunkSize, chunk->getZ() + k * chunkSize
                };

                processAdjacentChunkOnPlacement(neighborPos, blockLocalPos, {i, j, k}, blockType, isInstance,
                                                isLightEmitter, outProcessedChunks);
            }
        }
    }
}

void WorldManager::processAdjacentChunkOnPlacement(const ChunkPosition &neighborPos, const std::array<int, 3> &blockLocalPos,
                                                   const std::array<int, 3> &neighborOffset, const Block::BlockType blockType,
                                                   const bool isInstance, const bool isLightEmitter,
                                                   std::unordered_set<ChunkPosition> &outProcessedChunks) {
    if (const auto adjacentChunk = getChunk(neighborPos)) {
        constexpr int minBlockPos = -1;
        constexpr int maxBlockPos = Chunk::SIZE;

        auto getAdjChunkPos = [] (const int neighborOffsetCoord, const int localPos) {
            if (neighborOffsetCoord == 0) return localPos;
            if (neighborOffsetCoord == -1) return maxBlockPos;
            return minBlockPos;
        };

        const int adjX = getAdjChunkPos(neighborOffset[0], blockLocalPos[0]);
        const int adjY = getAdjChunkPos(neighborOffset[1], blockLocalPos[1]);
        const int adjZ = getAdjChunkPos(neighborOffset[2], blockLocalPos[2]);

        MeshingResult neighborResult;
        neighborResult.opaqueVertices.reserve(adjacentChunk->getOpaqueVertexVectorSize());
        neighborResult.waterVertices.reserve(adjacentChunk->getWaterVertexVectorSize());
        adjacentChunk->addBlock(adjX, adjY, adjZ, blockType, neighborResult);

        neighborResult.position = {adjacentChunk->getX(), adjacentChunk->getY(), adjacentChunk->getZ()};
        neighborResult.needIndirectRendererUpdate = !isInstance || isLightEmitter;
        neighborResult.needInstanceUpdate = isInstance;

        m_chunksData.completedMeshes.push(std::move(neighborResult));
        adjacentChunk->transferPendingLightsToWorld(*this);

        outProcessedChunks.emplace(neighborPos);
    }
}

void WorldManager::propagateLightAddition(const std::shared_ptr<Chunk> &chunk, const std::array<int, 3> &blockLocalPos,
                                          const std::unordered_set<ChunkPosition> &processedChunks) {
    constexpr int lightPropagationRadius = 15;
    constexpr int chunkSize = Chunk::SIZE;
    const auto chunkRadius = static_cast<int>(std::ceil(lightPropagationRadius / static_cast<float>(chunkSize)));
    constexpr int radiusSq = lightPropagationRadius * lightPropagationRadius;

    const int worldBlockX = chunk->getX() + blockLocalPos[0];
    const int worldBlockY = chunk->getY() + blockLocalPos[1];
    const int worldBlockZ = chunk->getZ() + blockLocalPos[2];

    for (int dx = -chunkRadius; dx <= chunkRadius; ++dx) {
        for (int dy = -chunkRadius; dy <= chunkRadius; ++dy) {
            for (int dz = -chunkRadius; dz <= chunkRadius; ++dz) {
                const ChunkPosition pos{
                    chunk->getX() + dx * chunkSize, chunk->getY() + dy * chunkSize, chunk->getZ() + dz * chunkSize
                };

                if (processedChunks.contains(pos)) continue;

                const int minDistX = std::max(
                    0, std::max(pos.x - worldBlockX, worldBlockX - (pos.x + chunkSize - 1)));
                const int minDistY = std::max(
                    0, std::max(pos.y - worldBlockY, worldBlockY - (pos.y + chunkSize - 1)));
                const int minDistZ = std::max(
                    0, std::max(pos.z - worldBlockZ, worldBlockZ - (pos.z + chunkSize - 1)));

                if (const int minDistSq = minDistX * minDistX + minDistY * minDistY + minDistZ * minDistZ;
                    minDistSq > radiusSq) continue;

                if (const auto adjacentChunk = getChunk(pos)) {
                    MeshingResult neighborResult;
                    neighborResult.opaqueVertices.reserve(adjacentChunk->getOpaqueVertexVectorSize());
                    neighborResult.waterVertices.reserve(adjacentChunk->getWaterVertexVectorSize());
                    adjacentChunk->propagateLight();
                    adjacentChunk->generateNewMesh(neighborResult);

                    neighborResult.position = {adjacentChunk->getX(), adjacentChunk->getY(), adjacentChunk->getZ()};
                    neighborResult.needIndirectRendererUpdate = true;
                    neighborResult.needInstanceUpdate = false;

                    m_chunksData.completedMeshes.push(std::move(neighborResult));
                    adjacentChunk->transferPendingLightsToWorld(*this);
                }
            }
        }
    }
}

const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk> > &WorldManager::getLoadedChunks() const {
    return m_chunksData.loadedMeshes;
}

bool WorldManager::getNeedInstanceUpdate() const {
    return m_needInstanceUpdate;
}

void WorldManager::processChunksQueues(IndirectRenderer &renderer) {
    const unsigned int maxChunksPerFrame = std::max(static_cast<unsigned int>(
        0.05f * Renderer::s_renderDistance + 0.2f * static_cast<float>(m_threadPool.getNumberOfThreads())), 10u);
    const unsigned int maxPendingBlocksPerFrame = std::max(static_cast<unsigned int>(static_cast<float>(maxChunksPerFrame) * 0.5f), 5u);
    const unsigned int maxPendingLightsPerFrame = std::max(static_cast<unsigned int>(static_cast<float>(maxChunksPerFrame) * 0.1f), 2u);
    const unsigned int maxUpdatePerFrame = std::max(static_cast<unsigned int>(static_cast<float>(maxChunksPerFrame) * 0.3f), 4u);

    deleteQueuedChunks(renderer, maxChunksPerFrame);
    generateQueuedChunks(maxChunksPerFrame);
    generateQueuedPendingBlocks(maxPendingBlocksPerFrame);
    generateQueuedPendingLights(maxPendingLightsPerFrame);
    updateQueuedChunks(renderer, maxUpdatePerFrame);

}

void WorldManager::deleteQueuedChunks(IndirectRenderer &renderer, const unsigned int maxProcessPerFrame) {
    for (int i = 0; i < maxProcessPerFrame; ++i) {
        std::shared_ptr<Chunk> chunk;
        if (!m_chunksData.meshesToDelete.try_pop(chunk)) break;
        renderer.removeChunk(chunk);
        m_chunksData.loadedMeshes.erase({chunk->getX(), chunk->getY(), chunk->getZ()});
    }
}

void WorldManager::generateQueuedChunks(const unsigned int maxProcessPerFrame) {
    for (int i = 0; i < maxProcessPerFrame; ++i) {
        ChunkPosition key{};
        if (!m_chunksData.meshesToGenerate.try_pop(key)) break;

        auto [it, inserted] = m_chunksData.loadedMeshes.try_emplace(key, nullptr);
        if (!inserted) continue;

        const auto p_chunk = std::make_shared<Chunk>(key.x, key.y, key.z);
        it->second = p_chunk;

        m_threadPool.enqueue_no_future([this, p_chunk] {
            p_chunk->generateVoxel();
            if (p_chunk->isEmpty()) return;
            // Will be deleted when out of range, don't delete now to avoid it being reloaded immediately

            p_chunk->propagateLight();
            p_chunk->generateMesh();
            m_needInstanceUpdate.store(true);
            p_chunk->transferPendingBlocksToWorld(*this);
            p_chunk->transferPendingLightsToWorld(*this);
        });
    }
}

void WorldManager::generateQueuedPendingBlocks(const unsigned int maxProcessPerFrame) {
    if (!m_chunksData.pendingBlocks.empty()) {
        int processed = 0;
        std::lock_guard lock(m_chunksData.pendingBlocksMutex);
        for (auto it = m_chunksData.pendingBlocks.begin(); it != m_chunksData.pendingBlocks.end() && processed < maxProcessPerFrame;) {
            const ChunkPosition key = it->first;

            if (auto loaded_it = m_chunksData.loadedMeshes.find(key); loaded_it != m_chunksData.loadedMeshes.end()) {
                const std::shared_ptr<Chunk> p_chunk = loaded_it->second;
                if (p_chunk->getState() < Mesh::State::VOXEL_GENERATED) {
                    ++it;
                    continue;
                }

                std::list<PendingBlock> blocks = std::move(it->second);
                it = m_chunksData.pendingBlocks.erase(it);

                if (!blocks.empty()) {
                    ++processed;
                    m_threadPool.enqueue_no_future([this, p_chunk, blocks = std::move(blocks)]() mutable {
                        MeshingResult result;
                        result.opaqueVertices.reserve(p_chunk->getOpaqueVertexVectorSize());
                        result.waterVertices.reserve(p_chunk->getWaterVertexVectorSize());
                        p_chunk->generatePendingBlocks(blocks, result);

                        result.position = {p_chunk->getX(), p_chunk->getY(), p_chunk->getZ()};
                        result.needIndirectRendererUpdate = true;
                        result.needInstanceUpdate = true;

                        m_chunksData.completedMeshes.push(std::move(result));
                        p_chunk->transferPendingLightsToWorld(*this);
                        emitNeighborsBorderLights(p_chunk); // Ensure pendings blocks are lit correctly
                    });
                }
            } else {
                ++it;
            }
        }
    }
}

void WorldManager::generateQueuedPendingLights(const unsigned int maxProcessPerFrame) {
    if (!m_chunksData.pendingLights.empty()) {
        int processed = 0;
        std::lock_guard lock(m_chunksData.pendingLightsMutex);
        for (auto it = m_chunksData.pendingLights.begin(); it != m_chunksData.pendingLights.end() && processed < maxProcessPerFrame; ) {
            const ChunkPosition key = it->first;

            if (auto loaded_it = m_chunksData.loadedMeshes.find(key); loaded_it != m_chunksData.loadedMeshes.end()) {
                const std::shared_ptr<Chunk> p_chunk = loaded_it->second;
                if (p_chunk->getState() < Mesh::State::VOXEL_GENERATED) {
                    ++it;
                    continue;
                }

                std::list<PendingLight> lights = std::move(it->second);
                it = m_chunksData.pendingLights.erase(it);

                if (!lights.empty()) {
                    ++processed;
                    m_threadPool.enqueue_no_future([this, p_chunk, lights = std::move(lights)]() mutable {
                        MeshingResult result;
                        result.opaqueVertices.reserve(p_chunk->getOpaqueVertexVectorSize());
                        result.waterVertices.reserve(p_chunk->getWaterVertexVectorSize());
                        p_chunk->generatePendingLights(lights, result);
                        result.position = {p_chunk->getX(), p_chunk->getY(), p_chunk->getZ()};
                        m_chunksData.completedMeshes.push(std::move(result));
                        p_chunk->transferPendingLightsToWorld(*this);
                        // Note: do NOT emitNeighborsBorderLights here to avoid ping-pong loops
                    });
                }
            } else {
                ++it;
            }
        }
    }
}

void WorldManager::updateQueuedChunks(IndirectRenderer &renderer, const unsigned int maxProcessPerFrame) {
    for (int i = 0; i < maxProcessPerFrame; ++i) {
        MeshingResult result;
        if (!m_chunksData.completedMeshes.try_pop(result)) break;

        if (const auto it = m_chunksData.loadedMeshes.find(result.position);
            it != m_chunksData.loadedMeshes.end()) {
            const std::shared_ptr<Chunk> p_chunk = it->second;

            if (result.needIndirectRendererUpdate) {
                p_chunk->getOpaqueVertices().swap(result.opaqueVertices);
                p_chunk->getWaterVertices().swap(result.waterVertices);
                p_chunk->setHasOpaqueFaces(result.hasOpaqueFaces);
                p_chunk->setHasWaterFaces(result.hasWaterFaces);
                p_chunk->updateVertexCount();
                renderer.updateChunk(p_chunk);
            }

            m_needInstanceUpdate.store(m_needInstanceUpdate.load() || result.needInstanceUpdate);
        }
    }
}

void WorldManager::generateChunksPositions(const int cameraWorldX, const int cameraWorldY, const int cameraWorldZ) {
    for (const auto [x,z, maxY]: m_renderDistanceOffsets) {
        const int chunkX = cameraWorldX + static_cast<int>(x * Chunk::SIZE);
        const int chunkZ = cameraWorldZ + static_cast<int>(z * Chunk::SIZE);

        for (int y = -maxY; y <= maxY; ++y) {
            const int chunkY = cameraWorldY + static_cast<int>(y * Chunk::SIZE);
            if (chunkY < 0 || chunkY > 256) continue; // World height limit

            const ChunkPosition key{chunkX, chunkY, chunkZ};
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

    for (const auto &[position, chunk]: m_chunksData.loadedMeshes) {
        const float dx = static_cast<float>(position.x) - camX;
        const float dy = static_cast<float>(position.y) - camY;
        const float dz = static_cast<float>(position.z) - camZ;
        if (const float distSq = dx * dx + dy * dy + dz * dz;
            distSq > renderDistanceSq) {
            m_chunksData.meshesToDelete.push(chunk);
        }
    }
}

std::shared_ptr<Chunk> WorldManager::getChunk(const ChunkPosition &position) const {
    if (const auto it = m_chunksData.loadedMeshes.find(position); it != m_chunksData.loadedMeshes.end()) {
        return it->second;
    }

    return nullptr;
}

void WorldManager::emitNeighborsBorderLights(const std::shared_ptr<Chunk> &chunk) {
    constexpr int S = Chunk::SIZE;
    const int cx = chunk->getX();
    const int cy = chunk->getY();
    const int cz = chunk->getZ();
    constexpr std::array<std::array<int, 3>, 6> dirs = {
        {
            {-S, 0, 0}, {S, 0, 0},
            {0, -S, 0}, {0, S, 0},
            {0, 0, -S}, {0, 0, S}
        }
    };

    for (const auto &d: dirs) {
        if (const auto neighbor = getChunk({cx + d[0], cy + d[1], cz + d[2]})) {
            neighbor->emitBorderLights();
            neighbor->transferPendingLightsToWorld(*this);
        }
    }
}
