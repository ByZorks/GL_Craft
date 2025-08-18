#include "World.h"

#include <iostream>
#include <ranges>
#include <thread>
#include <unordered_set>

#include "../render/Camera.h"
#include "../render/Renderer.h"
#include "../utils/ThreadSafeQueue.h"
#include "surfaceFeatures/flowers/Allium.h"
#include "surfaceFeatures/flowers/Cornflower.h"
#include "surfaceFeatures/flowers/Poppy.h"
#include "surfaceFeatures/grass/ShortGrass.h"

World::World() : m_threadPool(std::max(1u, std::thread::hardware_concurrency())) {
    m_grassRenderer.init(ShortGrass(0, 0, 0));
    m_poppyRenderer.init(Poppy(0, 0, 0));
    m_cornflowerRenderer.init(Cornflower(0, 0, 0));
    m_alliumRenderer.init(Allium(0, 0, 0));

    m_chunksData.loadedMeshes.reserve(static_cast<size_t>(Renderer::s_renderDistance * Renderer::s_renderDistance * Renderer::s_renderDistance * 0.5f));
    m_tempKeysToProcess.reserve(100);

    const int r = static_cast<int>(Renderer::s_renderDistance / static_cast<float>(Chunk::SIZE));
    const int r2 = r * r;

    m_renderDistanceOffsets.reserve(static_cast<size_t>(std::numbers::pi * static_cast<double>(r2)));
    for (int x = -r; x <= r; ++x) {
        for (int z = -r; z <= r; ++z) {
            if (const int d2 = x*x + z*z; d2 <= r2) {
                m_renderDistanceOffsets.push_back({x, z,
                    static_cast<int>(std::floor(std::sqrt(static_cast<float>(r2 - d2))))});
            }
        }
    }

    std::sort(m_renderDistanceOffsets.begin(), m_renderDistanceOffsets.end(),
              [](const auto &a, const auto &b) {
                  return a.x*a.x + a.z*a.z < b.x*b.x + b.z*b.z;
              });
}

void World::updateChunks(const Camera &camera) {
    const int cameraWorldX = static_cast<int>(std::floor(camera.getPos().x / static_cast<float>(Chunk::SIZE))) * static_cast<int>(Chunk::SIZE);
    const int cameraWorldY = static_cast<int>(std::floor(camera.getPos().y / static_cast<float>(Chunk::SIZE))) * static_cast<int>(Chunk::SIZE);
    const int cameraWorldZ = static_cast<int>(std::floor(camera.getPos().z / static_cast<float>(Chunk::SIZE))) * static_cast<int>(Chunk::SIZE);
    const glm::vec3 cameraChunkPos(cameraWorldX, cameraWorldY, cameraWorldZ);

    unloadDistantMeshes(cameraChunkPos);
    m_threadPool.enqueue_no_future([this, cameraWorldX, cameraWorldY, cameraWorldZ] {
        generateChunksPositions(cameraWorldX, cameraWorldY, cameraWorldZ);
    });
}

void World::drawChunks(const Camera &camera, const Frustum &frustum, Shader &shader, unsigned int &visibleChunksCount, unsigned int &drawCalls) {
    // Remove chunks that are no longer needed, generate voxel and mesh for new chunks, store them in m_chunksData.loadedMeshes
    processChunks();

    // Setup buffers for chunks that are ready to be rendered
    m_displayedNormalMeshes.clear();
    m_displayedTransparentMeshes.clear();
    m_displayedWaterMeshes.clear();
    bool needInstanceUpdate = camera.hasCameraChangedDirection() || camera.hasCameraChangedChunk() ||
        m_renderDistanceChanged || m_instancesChanged;
    m_renderDistanceChanged = false;
    m_instancesChanged = false;
    for (const auto& chunk : m_chunksData.loadedMeshes | std::views::values) {
        const State state = chunk->getState();
        if (state == State::MESH_GENERATED) {
            chunk->createGLBuffers();
            needInstanceUpdate = true;
        }
        if (state == State::READY_TO_DRAW) {
            if (chunk->hasOpaqueFaces()) m_displayedNormalMeshes.push_back(chunk);
            if (chunk->hasTransparentFaces()) m_displayedTransparentMeshes.push_back(chunk);
            if (chunk->hasWaterFaces()) m_displayedWaterMeshes.push_back(chunk);
        }
    }

    if (needInstanceUpdate) {
        m_grassRenderer.resetInstances();
        m_poppyRenderer.resetInstances();
        m_cornflowerRenderer.resetInstances();
        m_alliumRenderer.resetInstances();
    }

    for (const auto& strong_mesh : m_displayedNormalMeshes) {
        if (!frustum.isAABBInFrustum(strong_mesh->getBoundingBox())) continue;
        shader.setUniform3f("u_Offset",
                            static_cast<float>(strong_mesh->getX()),
                            static_cast<float>(strong_mesh->getY()),
                            static_cast<float>(strong_mesh->getZ()));

        strong_mesh->draw();
        ++drawCalls;
        ++visibleChunksCount;

        if (needInstanceUpdate && camera.distanceToCamera(*strong_mesh) < 320.0f) { // They are no longer visible at this distance event if we draw them
            for (const auto& feature : strong_mesh->getSurfaceFeatures()) {
                switch (feature.type) {
                    case SurfaceFeatureType::SHORT_GRASS:
                        m_grassRenderer.addInstance({feature.x - 1, feature.y, feature.z - 1});
                        break;
                    case SurfaceFeatureType::POPPY:
                        m_poppyRenderer.addInstance({feature.x - 1, feature.y, feature.z - 1});
                        break;
                    case SurfaceFeatureType::CORNFLOWER:
                        m_cornflowerRenderer.addInstance({feature.x - 1, feature.y, feature.z - 1});
                        break;
                    case SurfaceFeatureType::ALLIUM:
                        m_alliumRenderer.addInstance({feature.x - 1, feature.y, feature.z - 1});
                        break;
                    default:
                        break;
                }
            }
        }
    }

    if (needInstanceUpdate) {
        m_grassRenderer.updateInstanceBuffer();
        m_poppyRenderer.updateInstanceBuffer();
        m_cornflowerRenderer.updateInstanceBuffer();
        m_alliumRenderer.updateInstanceBuffer();
    }
}

void World::drawTransparentChunks(const Frustum &frustum, Shader &shader, unsigned int &drawCalls) const {
    for (const auto& strong_mesh : m_displayedTransparentMeshes) {
        if (!frustum.isAABBInFrustum(strong_mesh->getBoundingBox())) continue;
        shader.setUniform3f("u_Offset",
                            static_cast<float>(strong_mesh->getX()),
                            static_cast<float>(strong_mesh->getY()),
                            static_cast<float>(strong_mesh->getZ()));

        strong_mesh->drawTransparent();
        ++drawCalls;
    }
}

void World::drawWater(const Frustum &frustum, Shader &shader, unsigned int &drawCalls) const {
    if (!m_displayedWaterMeshes.empty()) {
        Renderer::disableDepthMask();
        for (const auto& strong_mesh : m_displayedWaterMeshes) {
            if (!frustum.isAABBInFrustum(strong_mesh->getBoundingBox())) continue;
            shader.setUniform1f("u_Time", static_cast<float>(glfwGetTime()));
            shader.setUniform3f("u_Offset",
                                static_cast<float>(strong_mesh->getX()),
                                static_cast<float>(strong_mesh->getY()),
                                static_cast<float>(strong_mesh->getZ()));

            strong_mesh->drawWater();
            ++drawCalls;
        }
        Renderer::enableDepthMask();
    }
}

void World::drawInstances(unsigned int &drawCalls) const {
    if (m_grassRenderer.getInstancesCount() == 0 &&
        m_poppyRenderer.getInstancesCount() == 0 &&
        m_cornflowerRenderer.getInstancesCount() == 0 &&
        m_alliumRenderer.getInstancesCount() == 0) {
        return;
    }

    Renderer::disableBackFaceCulling();

    if (m_grassRenderer.getInstancesCount() > 0) {
        m_grassRenderer.draw();
        ++drawCalls;
    }

    if (m_poppyRenderer.getInstancesCount() > 0) {
        m_poppyRenderer.draw();
        ++drawCalls;
    }

    if (m_cornflowerRenderer.getInstancesCount() > 0) {
        m_cornflowerRenderer.draw();
        ++drawCalls;
    }

    if (m_alliumRenderer.getInstancesCount() > 0) {
        m_alliumRenderer.draw();
        ++drawCalls;
    }

    Renderer::enableBackFaceCulling();
}

void World::addPendingBlocks(const std::unordered_map<ChunkPosition, std::vector<PendingBlock>> &blockData) {
    std::lock_guard lock(m_chunksData.m_pendingBlocksMutex);
    for (const auto& [key, blocks] : blockData) {
        auto& targetVector = m_chunksData.m_pendingBlocks[key];
        targetVector.reserve(targetVector.size() + blocks.size());
        targetVector.insert(targetVector.end(), blocks.begin(), blocks.end());
    }
}

void World::updateRenderDistance(Shader &postProcessingShader, const Camera &camera) {
    postProcessingShader.setUniform1f("u_RenderDistance", Renderer::s_renderDistance);

    m_renderDistanceOffsets.clear();

    const int r = static_cast<int>(Renderer::s_renderDistance / static_cast<float>(Chunk::SIZE));
    const int r2 = r * r;

    m_renderDistanceOffsets.reserve(static_cast<size_t>(std::numbers::pi * static_cast<double>(r2)));
    for (int x = -r; x <= r; ++x) {
        for (int z = -r; z <= r; ++z) {
            if (const int d2 = x*x + z*z; d2 <= r2) {
                m_renderDistanceOffsets.push_back({x, z,
                    static_cast<int>(std::floor(std::sqrt(static_cast<float>(r2 - d2))))});
            }
        }
    }

    std::sort(m_renderDistanceOffsets.begin(), m_renderDistanceOffsets.end(),
              [](const auto &a, const auto &b) {
                  return a.x*a.x + a.z*a.z < b.x*b.x + b.z*b.z;
              });

    updateChunks(camera);
    m_renderDistanceChanged = true;
}

void World::deleteBlockAndUpdateNeighbors(const RaycastResult &hit) {
    // TODO: Implement partial mesh update
    m_threadPool.enqueue_no_future([this, hit] {
        const auto t1 = std::chrono::high_resolution_clock::now();

        const BlockType type = hit.blockType;
        const auto& blockLocalPosition = hit.blockLocalPosition;
        const std::shared_ptr<Chunk> chunk = hit.chunk;

        // Update adjacents chunks if the block is at the border of the chunk
        const bool isAtLeftBorder = blockLocalPosition[0] == 0;
        const bool isAtRightBorder = blockLocalPosition[0] == Chunk::SIZE - 1;
        const bool isAtBottomBorder = blockLocalPosition[1] == 0;
        const bool isAtTopBorder = blockLocalPosition[1] == Chunk::SIZE - 1;
        const bool isAtFrontBorder = blockLocalPosition[2] == 0;
        const bool isAtBackBorder = blockLocalPosition[2] == Chunk::SIZE - 1;

        constexpr int chunkSize = Chunk::SIZE;
        constexpr int minBlockPos = -1; // chunk will add +1 when accessing the block
        constexpr int maxBlockPos = Chunk::SIZE; // chunk will add +1 when accessing the block

        // Delete in current chunk
        chunk->deleteBlock(blockLocalPosition[0], blockLocalPosition[1], blockLocalPosition[2], type);
        getMeshesToUpdate().push(chunk);

        // Faces: adjacent chunks
        if (isAtLeftBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX() - chunkSize, chunk->getY(), chunk->getZ())) {
                adjacentChunk->deleteBlock(maxBlockPos, blockLocalPosition[1], blockLocalPosition[2], type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (isAtRightBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX() + chunkSize, chunk->getY(), chunk->getZ())) {
                adjacentChunk->deleteBlock(minBlockPos, blockLocalPosition[1], blockLocalPosition[2], type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (isAtBottomBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX(), chunk->getY() - chunkSize, chunk->getZ())) {
                adjacentChunk->deleteBlock(blockLocalPosition[0], maxBlockPos, blockLocalPosition[2], type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (isAtTopBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX(), chunk->getY() + chunkSize, chunk->getZ())) {
                adjacentChunk->deleteBlock(blockLocalPosition[0], minBlockPos, blockLocalPosition[2], type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (isAtFrontBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX(), chunk->getY(), chunk->getZ() - chunkSize)) {
                adjacentChunk->deleteBlock(blockLocalPosition[0], blockLocalPosition[1], maxBlockPos, type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (isAtBackBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX(), chunk->getY(), chunk->getZ() + chunkSize)) {
                adjacentChunk->deleteBlock(blockLocalPosition[0], blockLocalPosition[1], minBlockPos, type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }

        // Edges : adjacent chunks
        if (isAtLeftBorder && isAtBottomBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX() - chunkSize, chunk->getY() - chunkSize, chunk->getZ())) {
                adjacentChunk->deleteBlock(maxBlockPos, maxBlockPos, blockLocalPosition[2], type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (isAtLeftBorder && isAtTopBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX() - chunkSize, chunk->getY() + chunkSize, chunk->getZ())) {
                adjacentChunk->deleteBlock(maxBlockPos, minBlockPos, blockLocalPosition[2], type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (isAtRightBorder && isAtBottomBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX() + chunkSize, chunk->getY() - chunkSize, chunk->getZ())) {
                adjacentChunk->deleteBlock(minBlockPos, maxBlockPos, blockLocalPosition[2], type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (isAtRightBorder && isAtTopBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX() + chunkSize, chunk->getY() + chunkSize, chunk->getZ())) {
                adjacentChunk->deleteBlock(minBlockPos, minBlockPos, blockLocalPosition[2], type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (isAtLeftBorder && isAtFrontBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX() - chunkSize, chunk->getY(), chunk->getZ() - chunkSize)) {
                adjacentChunk->deleteBlock(maxBlockPos, blockLocalPosition[1], maxBlockPos, type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (isAtLeftBorder && isAtBackBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX() - chunkSize, chunk->getY(), chunk->getZ() + chunkSize)) {
                adjacentChunk->deleteBlock(maxBlockPos, blockLocalPosition[1], minBlockPos, type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (isAtRightBorder && isAtFrontBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX() + chunkSize, chunk->getY(), chunk->getZ() - chunkSize)) {
                adjacentChunk->deleteBlock(minBlockPos, blockLocalPosition[1], maxBlockPos, type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (isAtRightBorder && isAtBackBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX() + chunkSize, chunk->getY(), chunk->getZ() + chunkSize)) {
                adjacentChunk->deleteBlock(minBlockPos, blockLocalPosition[1], minBlockPos, type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (isAtBottomBorder && isAtFrontBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX(), chunk->getY() - chunkSize, chunk->getZ() - chunkSize)) {
                adjacentChunk->deleteBlock(blockLocalPosition[0], maxBlockPos, maxBlockPos, type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (isAtBottomBorder && isAtBackBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX(), chunk->getY() - chunkSize, chunk->getZ() + chunkSize)) {
                adjacentChunk->deleteBlock(blockLocalPosition[0], maxBlockPos, minBlockPos, type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (isAtTopBorder && isAtFrontBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX(), chunk->getY() + chunkSize, chunk->getZ() - chunkSize)) {
                adjacentChunk->deleteBlock(blockLocalPosition[0], minBlockPos, maxBlockPos, type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (isAtTopBorder && isAtBackBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX(), chunk->getY() + chunkSize, chunk->getZ() + chunkSize)) {
                adjacentChunk->deleteBlock(blockLocalPosition[0], minBlockPos, minBlockPos, type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }

        // Corners : adjacent chunks
        if (isAtLeftBorder && isAtBottomBorder && isAtFrontBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX() - chunkSize, chunk->getY() - chunkSize, chunk->getZ() - chunkSize)) {
                adjacentChunk->deleteBlock(maxBlockPos, maxBlockPos, maxBlockPos, type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (isAtLeftBorder && isAtBottomBorder && isAtBackBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX() - chunkSize, chunk->getY() - chunkSize, chunk->getZ() + chunkSize)) {
                adjacentChunk->deleteBlock(maxBlockPos, maxBlockPos, minBlockPos, type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (isAtLeftBorder && isAtTopBorder && isAtFrontBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX() - chunkSize, chunk->getY() + chunkSize, chunk->getZ() - chunkSize)) {
                adjacentChunk->deleteBlock(maxBlockPos, minBlockPos, maxBlockPos, type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (isAtLeftBorder && isAtTopBorder && isAtBackBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX() - chunkSize, chunk->getY() + chunkSize, chunk->getZ() + chunkSize)) {
                adjacentChunk->deleteBlock(maxBlockPos, minBlockPos, minBlockPos, type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (isAtRightBorder && isAtBottomBorder && isAtFrontBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX() + chunkSize, chunk->getY() - chunkSize, chunk->getZ() - chunkSize)) {
                adjacentChunk->deleteBlock(minBlockPos, maxBlockPos, maxBlockPos, type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (isAtRightBorder && isAtBottomBorder && isAtBackBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX() + chunkSize, chunk->getY() - chunkSize, chunk->getZ() + chunkSize)) {
                adjacentChunk->deleteBlock(minBlockPos, maxBlockPos, minBlockPos, type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (isAtRightBorder && isAtTopBorder && isAtFrontBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX() + chunkSize, chunk->getY() + chunkSize, chunk->getZ() - chunkSize)) {
                adjacentChunk->deleteBlock(minBlockPos, minBlockPos, maxBlockPos, type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (isAtRightBorder && isAtTopBorder && isAtBackBorder) {
            if (const auto adjacentChunk = getChunk(chunk->getX() + chunkSize, chunk->getY() + chunkSize, chunk->getZ() + chunkSize)) {
                adjacentChunk->deleteBlock(minBlockPos, minBlockPos, minBlockPos, type);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }

        if (type == BlockType::SURFACE_FEATURE_BILLBOARD) {
            setInstancesChanged(true);
        }

        const auto t2 = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        std::cout << "Block deleted in " << duration << " ms" << std::endl;
    });
}

void World::placeBlockAndUpdateNeighbors(const RaycastResult &hit, BlockType blockToPlace) {
    // TODO: Implement partial mesh update
    m_threadPool.enqueue_no_future([this, hit, blockToPlace] {
        const auto t1 = std::chrono::high_resolution_clock::now();

        const std::shared_ptr<Chunk> chunk = hit.chunk;

        // Determine coords based on normal
        const int offsetX = hit.normal.x == 0 ? 0 : hit.normal.x > 0 ? 1 : -1;
        const int offsetY = hit.normal.y == 0 ? 0 : hit.normal.y > 0 ? 1 : -1;
        const int offsetZ = hit.normal.z == 0 ? 0 : hit.normal.z > 0 ? 1 : -1;

        const int newLocalX = hit.blockLocalPosition[0] + offsetX;
        const int newLocalY = hit.blockLocalPosition[1] + offsetY;
        const int newLocalZ = hit.blockLocalPosition[2] + offsetZ;

        constexpr int chunkSize = Chunk::SIZE;
        constexpr int minBlockPos = -1; // chunk will add +1 when accessing the block
        constexpr int maxBlockPos = Chunk::SIZE; // chunk will add +1 when accessing the block

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
        targetChunk->addBlock(targetX, targetY, targetZ, blockToPlace);
        getMeshesToUpdate().push(targetChunk);

        // Check borders for new block position
        const bool willBeAtLeftBorder = targetX == 0;
        const bool willBeAtRightBorder = targetX == Chunk::SIZE - 1;
        const bool willBeAtBottomBorder = targetY == 0;
        const bool willBeAtTopBorder = targetY == Chunk::SIZE - 1;
        const bool willBeAtFrontBorder = targetZ == 0;
        const bool willBeAtBackBorder = targetZ == Chunk::SIZE - 1;

        // Faces: adjacent chunks
        if (willBeAtLeftBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX() - chunkSize, targetChunk->getY(), targetChunk->getZ())) {
                adjacentChunk->addBlock(maxBlockPos, targetY, targetZ, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (willBeAtRightBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX() + chunkSize, targetChunk->getY(), targetChunk->getZ())) {
                adjacentChunk->addBlock(minBlockPos, targetY, targetZ, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (willBeAtBottomBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX(), targetChunk->getY() - chunkSize, targetChunk->getZ())) {
                adjacentChunk->addBlock(targetX, maxBlockPos, targetZ, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (willBeAtTopBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX(), targetChunk->getY() + chunkSize, targetChunk->getZ())) {
                adjacentChunk->addBlock(targetX, minBlockPos, targetZ, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (willBeAtFrontBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX(), targetChunk->getY(), targetChunk->getZ() - chunkSize)) {
                adjacentChunk->addBlock(targetX, targetY, maxBlockPos, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (willBeAtBackBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX(), targetChunk->getY(), targetChunk->getZ() + chunkSize)) {
                adjacentChunk->addBlock(targetX, targetY, minBlockPos, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }

        // Edges : adjacent chunks
        if (willBeAtLeftBorder && willBeAtBottomBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX() - chunkSize, targetChunk->getY() - chunkSize, targetChunk->getZ())) {
                adjacentChunk->addBlock(maxBlockPos, maxBlockPos, targetZ, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (willBeAtLeftBorder && willBeAtTopBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX() - chunkSize, targetChunk->getY() + chunkSize, targetChunk->getZ())) {
                adjacentChunk->addBlock(maxBlockPos, minBlockPos, targetZ, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (willBeAtRightBorder && willBeAtBottomBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX() + chunkSize, targetChunk->getY() - chunkSize, targetChunk->getZ())) {
                adjacentChunk->addBlock(minBlockPos, maxBlockPos, targetZ, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (willBeAtRightBorder && willBeAtTopBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX() + chunkSize, targetChunk->getY() + chunkSize, targetChunk->getZ())) {
                adjacentChunk->addBlock(minBlockPos, minBlockPos, targetZ, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (willBeAtLeftBorder && willBeAtFrontBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX() - chunkSize, targetChunk->getY(), targetChunk->getZ() - chunkSize)) {
                adjacentChunk->addBlock(maxBlockPos, targetY, maxBlockPos, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (willBeAtLeftBorder && willBeAtBackBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX() - chunkSize, targetChunk->getY(), targetChunk->getZ() + chunkSize)) {
                adjacentChunk->addBlock(maxBlockPos, targetY, minBlockPos, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (willBeAtRightBorder && willBeAtFrontBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX() + chunkSize, targetChunk->getY(), targetChunk->getZ() - chunkSize)) {
                adjacentChunk->addBlock(minBlockPos, targetY, maxBlockPos, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (willBeAtRightBorder && willBeAtBackBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX() + chunkSize, targetChunk->getY(), targetChunk->getZ() + chunkSize)) {
                adjacentChunk->addBlock(minBlockPos, targetY, minBlockPos, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (willBeAtBottomBorder && willBeAtFrontBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX(), targetChunk->getY() - chunkSize, targetChunk->getZ() - chunkSize)) {
                adjacentChunk->addBlock(targetX, maxBlockPos, maxBlockPos, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (willBeAtBottomBorder && willBeAtBackBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX(), targetChunk->getY() - chunkSize, targetChunk->getZ() + chunkSize)) {
                adjacentChunk->addBlock(targetX, maxBlockPos, minBlockPos, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (willBeAtTopBorder && willBeAtFrontBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX(), targetChunk->getY() + chunkSize, targetChunk->getZ() - chunkSize)) {
                adjacentChunk->addBlock(targetX, minBlockPos, maxBlockPos, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (willBeAtTopBorder && willBeAtBackBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX(), targetChunk->getY() + chunkSize, targetChunk->getZ() + chunkSize)) {
                adjacentChunk->addBlock(targetX, minBlockPos, minBlockPos, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }

        // Corners : adjacent chunks
        if (willBeAtLeftBorder && willBeAtBottomBorder && willBeAtFrontBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX() - chunkSize, targetChunk->getY() - chunkSize, targetChunk->getZ() - chunkSize)) {
                adjacentChunk->addBlock(maxBlockPos, maxBlockPos, maxBlockPos, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (willBeAtLeftBorder && willBeAtBottomBorder && willBeAtBackBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX() - chunkSize, targetChunk->getY() - chunkSize, targetChunk->getZ() + chunkSize)) {
                adjacentChunk->addBlock(maxBlockPos, maxBlockPos, minBlockPos, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (willBeAtLeftBorder && willBeAtTopBorder && willBeAtFrontBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX() - chunkSize, targetChunk->getY() + chunkSize, targetChunk->getZ() - chunkSize)) {
                adjacentChunk->addBlock(maxBlockPos, minBlockPos, maxBlockPos, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (willBeAtLeftBorder && willBeAtTopBorder && willBeAtBackBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX() - chunkSize, targetChunk->getY() + chunkSize, targetChunk->getZ() + chunkSize)) {
                adjacentChunk->addBlock(maxBlockPos, minBlockPos, minBlockPos, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (willBeAtRightBorder && willBeAtBottomBorder && willBeAtFrontBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX() + chunkSize, targetChunk->getY() - chunkSize, targetChunk->getZ() - chunkSize)) {
                adjacentChunk->addBlock(minBlockPos, maxBlockPos, maxBlockPos, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (willBeAtRightBorder && willBeAtBottomBorder && willBeAtBackBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX() + chunkSize, targetChunk->getY() - chunkSize, targetChunk->getZ() + chunkSize)) {
                adjacentChunk->addBlock(minBlockPos, maxBlockPos, minBlockPos, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (willBeAtRightBorder && willBeAtTopBorder && willBeAtFrontBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX() + chunkSize, targetChunk->getY() + chunkSize, targetChunk->getZ() - chunkSize)) {
                adjacentChunk->addBlock(minBlockPos, minBlockPos, maxBlockPos, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }
        if (willBeAtRightBorder && willBeAtTopBorder && willBeAtBackBorder) {
            if (const auto adjacentChunk = getChunk(targetChunk->getX() + chunkSize, targetChunk->getY() + chunkSize, targetChunk->getZ() + chunkSize)) {
                adjacentChunk->addBlock(minBlockPos, minBlockPos, minBlockPos, blockToPlace);
                getMeshesToUpdate().push(adjacentChunk);
            }
        }

        if (blockToPlace == BlockType::SURFACE_FEATURE_BILLBOARD) {
            setInstancesChanged(true);
        }

        const auto t2 = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        std::cout << "Block placed in " << duration << " ms" << std::endl;
    });
}

int World::getHeight(const int worldX, const int worldZ) {
    constexpr int baseHeight = 58;
    constexpr int maxHeight = 256;

    // 2D noise generation for terrain height
    const float normalizedNoise = (getTerrainNoise().GetNoise(
        static_cast<float>(worldX),
        static_cast<float>(worldZ)
        ) + 1.0f) / 2.0f;

    const float terrainShape = std::pow(normalizedNoise, 4.6f);
    float columnHeight = std::floor(baseHeight + terrainShape * maxHeight);

    // 3D noise generation for cave system
    const float normalized3DNoise = (getCaveNoise().GetNoise(
        static_cast<float>(worldX),
        columnHeight,
        static_cast<float>(worldZ)
        ) + 1.0f) / 2.0f;
    constexpr float baseCaveThreshold = 0.87f;
    const float surfaceModifier = 1.0f - std::clamp((columnHeight - baseHeight) / (maxHeight * 0.7f), 0.0f, 1.0f);
    const float caveThreshold = baseCaveThreshold + surfaceModifier * 0.3f;

    // Adjust column height based on cave noise
    if (std::abs(normalized3DNoise - caveThreshold) < 0.3f) {
        columnHeight -= (normalized3DNoise - (caveThreshold - 0.3f)) * 10.0f;
    }

    return static_cast<int>(columnHeight);
}

bool World::isCave(const int worldX, const int worldY, const int worldZ, const int columnHeight) {
    constexpr int maxHeight = 256;
    constexpr int baseHeight = 58;
    constexpr int waterLevel = 63;

    if (worldY <= 1 || worldY > maxHeight || (worldY >= columnHeight && columnHeight < waterLevel)) return false;

    // 3D noise generation for cave system
    const float normalized3DNoise = (getCaveNoise().GetNoise(static_cast<float>(worldX), static_cast<float>(worldY), static_cast<float>(worldZ)) + 1.0f) / 2.0f; // Normalize to [0, 1]
    constexpr float baseCaveThreshold = 0.87f;
    const float surfaceModifier = 1.0f - std::clamp(static_cast<float>(worldY - baseHeight) / (maxHeight * 0.7f), 0.0f, 1.0f);
    const float caveThreshold = baseCaveThreshold + surfaceModifier * 0.3f; // Increase threshold near surface

    return std::abs(normalized3DNoise - caveThreshold) < 0.3f;
}

std::shared_ptr<Chunk> World::getChunk(const int x, const int y, const int z) const {
    if (const auto it = m_chunksData.loadedMeshes.find({x, y, z}); it != m_chunksData.loadedMeshes.end()) {
        return it->second;
    }

    // Should not happen, but if it does, return a null chunk
    static auto nullChunk = std::make_shared<Chunk>(-1, -1, -1); // Return a null chunk if not found
    std::cerr << "Chunk not found at (" << x << ", " << y << ", " << z << ")\n";
    return nullChunk;
}

const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk>> & World::getLoadedChunks() const {
    return m_chunksData.loadedMeshes;
}

ThreadSafeQueue<std::shared_ptr<Chunk>> & World::getMeshesToUpdate() {
    return m_chunksData.meshesToUpdate;
}

void World::processChunks() {
    const int maxChunksPerFrame = static_cast<int>(0.1 * Renderer::s_renderDistance + 0.2 * static_cast<float>(m_threadPool.getNumberOfThreads()));
    // Remove chunks that are no longer needed
    for (int i = 0; i < maxChunksPerFrame; ++i) {
        if (m_chunksData.meshesToDelete.empty()) break;
        m_chunksData.meshesToDelete.pop();
    }

    // First pass: voxel + mesh generation
    for (int i = 0; i < maxChunksPerFrame; ++i) {
        if (m_chunksData.meshesToGenerateVoxel.empty()) break;

        ChunkPosition key = m_chunksData.meshesToGenerateVoxel.pop();
        m_threadPool.enqueue_no_future([this, key] {
            const auto p_chunk = std::make_shared<Chunk>(key.x, key.y, key.z);
            p_chunk->generateVoxel();
            p_chunk->transferPendingBlocksToWorld(*this);
            p_chunk->generateMesh(true);
            m_chunksData.meshesToRender.push(p_chunk);
        });
    }

    // Second pass: add ready meshes to loaded meshes
    for (int i = 0; i < maxChunksPerFrame; ++i) {
        if (m_chunksData.meshesToRender.empty()) break;
        std::shared_ptr<Chunk> p_chunk = m_chunksData.meshesToRender.pop();
        m_chunksData.loadedMeshes.try_emplace({p_chunk->getX(), p_chunk->getY(), p_chunk->getZ()}, p_chunk);
    }

    // Third pass: generate pending blocks
    if (!m_chunksData.m_pendingBlocks.empty()) {
        m_tempKeysToProcess.clear();
        {
            std::lock_guard lock(m_chunksData.m_pendingBlocksMutex);
            m_tempKeysToProcess.reserve(m_chunksData.m_pendingBlocks.size());
            for (const auto &key: m_chunksData.m_pendingBlocks | std::views::keys) {
                m_tempKeysToProcess.push_back(key);
            }
        }

        for (const auto& key : m_tempKeysToProcess) {
            if (auto it = m_chunksData.loadedMeshes.find(key); it != m_chunksData.loadedMeshes.end()) {
                const std::shared_ptr<Chunk> p_chunk = it->second;
                if (p_chunk->getState() < State::MESH_GENERATED) continue;

                std::vector<PendingBlock> blocks;
                {
                    std::lock_guard lock(m_chunksData.m_pendingBlocksMutex);
                    if (auto pending_it = m_chunksData.m_pendingBlocks.find(key); pending_it != m_chunksData.m_pendingBlocks.end()) {
                        blocks = std::move(pending_it->second);
                        m_chunksData.m_pendingBlocks.erase(pending_it);
                    }
                }

                if (!blocks.empty()) {
                    p_chunk->resetGLBuffers();
                    p_chunk->generatePendingBlocks(blocks);
                }
            }
        }
    }

    // Fourth pass: update GL buffers for chunks that need it
    for (int i = 0; i < maxChunksPerFrame; ++i) {
        if (m_chunksData.meshesToUpdate.empty()) break;
        const std::shared_ptr<Chunk> p_chunk = m_chunksData.meshesToUpdate.pop();
        p_chunk->createNewMeshGLBuffers();
        m_instancesChanged = true;
    }
}

void World::generateChunksPositions(const int cameraWorldX, const int cameraWorldY, const int cameraWorldZ) {
    for (auto [x,z, maxY] : m_renderDistanceOffsets) {
        const int chunkX = cameraWorldX + static_cast<int>(x * Chunk::SIZE);
        const int chunkZ = cameraWorldZ + static_cast<int>(z * Chunk::SIZE);

        for (int y = -maxY; y <= maxY; ++y) {
            const int chunkY = cameraWorldY + static_cast<int>(y * Chunk::SIZE);
            if (chunkY < 0 || chunkY > 256) continue; // World height limit

            const ChunkPosition key = {chunkX, chunkY, chunkZ};
            if (m_chunksData.loadedMeshes.contains(key)) continue;
            m_chunksData.meshesToGenerateVoxel.push(key);
        }
    }
}

void World::unloadDistantMeshes(const glm::vec3 &cameraChunkPos) {
    const float renderDistanceSq = Renderer::s_renderDistance * Renderer::s_renderDistance;

    const float camX = cameraChunkPos.x;
    const float camY = cameraChunkPos.y;
    const float camZ = cameraChunkPos.z;

    std::erase_if(m_chunksData.loadedMeshes, [&](const auto &tuple) {
        auto [x, y, z] = tuple.first;
        const float dx = x - camX;
        const float dy = y - camY;
        const float dz = z - camZ;
        const float distSq = dx * dx + dy * dy + dz * dz;
        return distSq > renderDistanceSq;
    });
}

FastNoiseLite World::makeTerrainNoise() {
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetFrequency(.0055f);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(6);
    noise.SetFractalLacunarity(2.2f);
    return noise;
}

FastNoiseLite World::makeSurfaceFeaturesNoise() {
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(.5f);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(6);
    return noise;
}

FastNoiseLite World::makeCaveNoise() {
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(.018f);
    noise.SetFractalType(FastNoiseLite::FractalType_Ridged);
    noise.SetFractalOctaves(6);
    noise.SetFractalLacunarity(1.29f);
    noise.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2Reduced);
    noise.SetDomainWarpAmp(20.f);
    return noise;
}

FastNoiseLite & World::getTerrainNoise() {
    thread_local FastNoiseLite instance = makeTerrainNoise();
    return instance;
}

FastNoiseLite & World::getSurfaceFeaturesNoise() {
    thread_local FastNoiseLite instance = makeSurfaceFeaturesNoise();
    return instance;
}

void World::setInstancesChanged(const bool m_instances_changed) {
    m_instancesChanged = m_instances_changed;
}

ThreadPool & World::getThreadPool() {
    return m_threadPool;
}

FastNoiseLite & World::getCaveNoise() {
    thread_local FastNoiseLite instance = makeCaveNoise();
    return instance;
}
