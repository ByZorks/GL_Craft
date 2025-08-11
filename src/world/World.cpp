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
    m_terrainHeightGenerator.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_terrainHeightGenerator.SetFrequency(.0055f);
    m_terrainHeightGenerator.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_terrainHeightGenerator.SetFractalOctaves(6);
    m_terrainHeightGenerator.SetFractalLacunarity(2.2f);

    m_surfaceFeaturesNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_surfaceFeaturesNoise.SetFrequency(.5f);
    m_surfaceFeaturesNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_surfaceFeaturesNoise.SetFractalOctaves(6);

    m_caveGenerator.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_caveGenerator.SetFrequency(.018f);
    m_caveGenerator.SetFractalType(FastNoiseLite::FractalType_Ridged);
    m_caveGenerator.SetFractalOctaves(6);
    m_caveGenerator.SetFractalLacunarity(1.29f);
    m_caveGenerator.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2Reduced);
    m_caveGenerator.SetDomainWarpAmp(20.f);

    m_grassRenderer.init(ShortGrass(0, 0, 0));
    m_poppyRenderer.init(Poppy(0, 0, 0));
    m_cornflowerRenderer.init(Cornflower(0, 0, 0));
    m_alliumRenderer.init(Allium(0, 0, 0));

    m_chunksData.loadedMeshes.reserve(static_cast<size_t>(Renderer::m_renderDistance * Renderer::m_renderDistance * Renderer::m_renderDistance * 0.5f));

    m_heightMapByChunk.reserve(static_cast<size_t>(Renderer::m_renderDistance * Renderer::m_renderDistance * 0.5f));
}

void World::updateChunks(const Camera &camera) {
    if (!camera.hasCameraChangedChunk()) return;

    const int cameraWorldX = static_cast<int>(std::floor(camera.m_camera_pos().x / static_cast<float>(Chunk::SIZE))) * static_cast<int>(Chunk::SIZE);
    const int cameraWorldY = static_cast<int>(std::floor(camera.m_camera_pos().y / static_cast<float>(Chunk::SIZE))) * static_cast<int>(Chunk::SIZE);
    const int cameraWorldZ = static_cast<int>(std::floor(camera.m_camera_pos().z / static_cast<float>(Chunk::SIZE))) * static_cast<int>(Chunk::SIZE);
    const glm::vec3 cameraChunkPos(cameraWorldX, cameraWorldY, cameraWorldZ);

    unloadDistantMeshes(cameraChunkPos);
    generateDataForEachChunks(cameraWorldX, cameraWorldY, cameraWorldZ);
}

void World::drawChunks(const Camera &camera, const Frustum &frustum, Shader &shader, unsigned int &visibleChunksCount, unsigned int &drawCalls) {
    // Remove chunks that are no longer needed, generate voxel and mesh for new chunks, store them in m_chunksData.loadedMeshes
    processChunks();

    // Setup buffers for chunks that are ready to be rendered
    m_displayedNormalMeshes.clear();
    m_displayedTransparentMeshes.clear();
    m_displayedWaterMeshes.clear();
    bool needInstanceUpdate = false;
    for (const auto& chunk : m_chunksData.loadedMeshes | std::views::values) {
        if (chunk->m_state1() == State::MESH_GENERATED || chunk->m_state1() == State::NEED_BUFFERS_UPDATE) {
            chunk->setupBuffers();
            needInstanceUpdate = true;
        }
        if (chunk->m_state1() == State::READY_TO_DRAW) {
            if (chunk->hasOpaqueFaces()) m_displayedNormalMeshes.push_back(chunk);
            if (chunk->hasTransparentFaces()) m_displayedTransparentMeshes.push_back(chunk);
            if (chunk->hasWaterFaces()) m_displayedWaterMeshes.push_back(chunk);
        }
    }

    const bool instanceUpdateRequired = camera.hasCameraChangedDirection() || camera.hasCameraChangedChunk() || needInstanceUpdate;
    if (instanceUpdateRequired) {
        m_grassRenderer.resetInstances();
        m_poppyRenderer.resetInstances();
        m_cornflowerRenderer.resetInstances();
        m_alliumRenderer.resetInstances();
    }

    for (const auto& strong_mesh : m_displayedNormalMeshes) {
        if (camera.distanceToCamera(*strong_mesh) > Renderer::m_renderDistance) continue;
        if (!frustum.isAABBInFrustum(strong_mesh->m_box1())) continue;
        shader.setUniform3f("u_Offset",
                            static_cast<float>(strong_mesh->m_x1()),
                            static_cast<float>(strong_mesh->m_y1()),
                            static_cast<float>(strong_mesh->m_z1()));

        strong_mesh->draw();
        drawCalls++;
        visibleChunksCount++;

        if (instanceUpdateRequired) {
            for (const auto& feature : strong_mesh->m_surface_features()) {
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

    if (instanceUpdateRequired) {
        m_grassRenderer.updateInstanceBuffer();
        m_poppyRenderer.updateInstanceBuffer();
        m_cornflowerRenderer.updateInstanceBuffer();
        m_alliumRenderer.updateInstanceBuffer();
    }
}

void World::drawTransparentChunks(Shader &shader, unsigned int &drawCalls) const {
    for (const auto& strong_mesh : m_displayedTransparentMeshes) {
        shader.setUniform3f("u_Offset",
                            static_cast<float>(strong_mesh->m_x1()),
                            static_cast<float>(strong_mesh->m_y1()),
                            static_cast<float>(strong_mesh->m_z1()));

        strong_mesh->drawTransparent();
        drawCalls++;
    }
}

void World::drawWater(Shader &shader, unsigned int &drawCalls) const {
    if (!m_displayedWaterMeshes.empty()) {
        Renderer::disableDepthMask();
        for (const auto& strong_mesh : m_displayedWaterMeshes) {
            shader.setUniform1f("u_Time", static_cast<float>(glfwGetTime()));
            shader.setUniform3f("u_Offset",
                                static_cast<float>(strong_mesh->m_x1()),
                                static_cast<float>(strong_mesh->m_y1()),
                                static_cast<float>(strong_mesh->m_z1()));

            strong_mesh->drawWater();
            drawCalls++;
        }
        Renderer::enableDepthMask();
    }
}

void World::drawInstances(unsigned int &drawCalls) const {
    if (m_grassRenderer.m_instance_count() == 0 &&
        m_poppyRenderer.m_instance_count() == 0 &&
        m_cornflowerRenderer.m_instance_count() == 0 &&
        m_alliumRenderer.m_instance_count() == 0) {
        return;
    }

    Renderer::disableBackFaceCulling();

    if (m_grassRenderer.m_instance_count() > 0) {
        m_grassRenderer.draw();
        drawCalls++;
    }

    if (m_poppyRenderer.m_instance_count() > 0) {
        m_poppyRenderer.draw();
        drawCalls++;
    }

    if (m_cornflowerRenderer.m_instance_count() > 0) {
        m_cornflowerRenderer.draw();
        drawCalls++;
    }

    if (m_alliumRenderer.m_instance_count() > 0) {
        m_alliumRenderer.draw();
        drawCalls++;
    }

    Renderer::enableBackFaceCulling();
}

void World::addPendingBlocks(const std::unordered_map<ChunkPosition, std::vector<std::tuple<int, int, int, BlockType>>> &blockData) {
    std::lock_guard lock(m_chunksData.m_pendingBlocksMutex);
    for (const auto& [key, blocks] : blockData) {
        auto& targetVector = m_chunksData.m_pendingBlocks[key];
        targetVector.reserve(targetVector.size() + blocks.size());
        targetVector.insert(targetVector.end(), blocks.begin(), blocks.end());
    }
}

int World::getHeight(const int worldX, const int worldZ) {
    // static cast have to be used on both coords and size or it will crash
    const int chunkXInHeightMap = static_cast<int>(std::floor(static_cast<double>(worldX) / Chunk::SIZE));
    const int chunkZInHeightMap = static_cast<int>(std::floor(static_cast<double>(worldZ) / Chunk::SIZE));
    const std::pair coordsChunk(chunkXInHeightMap, chunkZInHeightMap);

    const int localXInHeightMap = worldX - chunkXInHeightMap * static_cast<int>(Chunk::SIZE);
    const int localZInHeightMap = worldZ - chunkZInHeightMap * static_cast<int>(Chunk::SIZE);

    // Check cache
    if (const auto it = m_heightMapByChunk.find(coordsChunk); it != m_heightMapByChunk.end()) {
        return it->second.getHeight(localXInHeightMap, localZInHeightMap);
    }

    // Generate heigtmap for the chunk if it doesn't exist
    ChunkHeightmap newHeightMap{};
    for (int i = 0; i < Chunk::SIZE * Chunk::SIZE; ++i) {
        constexpr int baseHeight = 58;
        constexpr int maxHeight = 256;
        const int localXInChunk = i % static_cast<int>(Chunk::SIZE);
        const int localZInChunk = i / static_cast<int>(Chunk::SIZE);
        const int worldXInChunk = chunkXInHeightMap * static_cast<int>(Chunk::SIZE) + localXInChunk;
        const int worldZInChunk = chunkZInHeightMap * static_cast<int>(Chunk::SIZE) + localZInChunk;

        // 2D noise generation for terrain height
        const float normalizedNoise = (m_terrainHeightGenerator.GetNoise(
            static_cast<float>(worldXInChunk),
            static_cast<float>(worldZInChunk)
            ) + 1.0f) / 2.0f;

        const float terrainShape = std::pow(normalizedNoise, 4.6f);
        float columnHeight = std::floor(baseHeight + terrainShape * maxHeight);

        // 3D noise generation for cave system
        const float normalized3DNoise = (m_caveGenerator.GetNoise(
            static_cast<float>(worldXInChunk),
            columnHeight,
            static_cast<float>(worldZInChunk)
            ) + 1.0f) / 2.0f;
        constexpr float baseCaveThreshold = 0.87f;
        const float surfaceModifier = 1.0f - std::clamp((columnHeight - baseHeight) / (maxHeight * 0.7f), 0.0f, 1.0f);
        const float caveThreshold = baseCaveThreshold + surfaceModifier * 0.3f;

        // Adjust column height based on cave noise
        if (std::abs(normalized3DNoise - caveThreshold) < 0.3f) {
            columnHeight -= (normalized3DNoise - (caveThreshold - 0.3f)) * 10.0f;
        }

        newHeightMap.heights[localXInChunk + localZInChunk * Chunk::SIZE] = static_cast<int>(columnHeight);
    }

    {
        std::lock_guard lock(m_heightMapMutex);
        m_heightMapByChunk[coordsChunk] = newHeightMap;
    }

    return m_heightMapByChunk[coordsChunk].getHeight(localXInHeightMap, localZInHeightMap);
}

bool World::isCave(const int worldX, const int worldY, const int worldZ, const int columnHeight) const {
    constexpr int maxHeight = 256;
    constexpr int baseHeight = 58;
    constexpr int waterLevel = 63;

    if (worldY <= 1 || worldY > maxHeight || (worldY >= columnHeight && columnHeight < waterLevel)) return false;

    // 3D noise generation for cave system
    const float normalized3DNoise = (m_caveGenerator.GetNoise(static_cast<float>(worldX), static_cast<float>(worldY), static_cast<float>(worldZ)) + 1.0f) / 2.0f; // Normalize to [0, 1]
    constexpr float baseCaveThreshold = 0.87f;
    const float surfaceModifier = 1.0f - std::clamp(static_cast<float>(worldY - baseHeight) / (maxHeight * 0.7f), 0.0f, 1.0f);
    const float caveThreshold = baseCaveThreshold + surfaceModifier * 0.3f; // Increase threshold near surface

    return std::abs(normalized3DNoise - caveThreshold) < 0.3f;
}

const FastNoiseLite & World::m_noise_generator() const {
    return m_terrainHeightGenerator;
}

const FastNoiseLite & World::m_surface_features_noise() const {
    return m_surfaceFeaturesNoise;
}

const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk>> & World::m_loaded_chunks() const {
    return m_chunksData.loadedMeshes;
}

void World::processChunks() {
    const int maxChunksPerFrame = static_cast<int>(0.15 * Renderer::m_renderDistance + 0.3 * static_cast<float>(m_threadPool.m_num_threads()));
    // Remove chunks that are no longer needed
    for (int i = 0; i < maxChunksPerFrame; ++i) {
        if (m_chunksData.meshesToDelete.empty()) break;
        m_chunksData.meshesToDelete.pop();
    }

    // First pass: voxel + mesh generation
    for (int i = 0; i < maxChunksPerFrame; ++i) {
        if (m_chunksData.meshesToGenerateVoxel.empty()) break;

        ChunkPosition key = m_chunksData.meshesToGenerateVoxel.pop();
        m_threadPool.enqueue([this, key] {
            const auto p_chunk = std::make_shared<Chunk>(key.x, key.y, key.z);
            p_chunk->generateVoxel(*this);
            p_chunk->transferPendingBlocksToWorld(*this);
            p_chunk->generateMesh();
            m_chunksData.meshesToRender.push(p_chunk);
        });
    }

    // Second pass: add ready meshes to loaded meshes
    for (int i = 0; i < maxChunksPerFrame; ++i) {
        if (m_chunksData.meshesToRender.empty()) break;
        std::shared_ptr<Chunk> p_chunk = m_chunksData.meshesToRender.pop();
        m_chunksData.loadedMeshes.try_emplace({p_chunk->m_x1(), p_chunk->m_y1(), p_chunk->m_z1()}, p_chunk);
    }

    // Third pass: generate pending blocks
    if (!m_chunksData.m_pendingBlocks.empty()) {
        std::vector<ChunkPosition> keysToProcess;
        {
            std::lock_guard lock(m_chunksData.m_pendingBlocksMutex);
            for (const auto &key: m_chunksData.m_pendingBlocks | std::views::keys) {
                keysToProcess.push_back(key);
            }
        }

        for (const auto& key : keysToProcess) {
            if (auto it = m_chunksData.loadedMeshes.find(key); it != m_chunksData.loadedMeshes.end()) {
                const std::shared_ptr<Chunk> p_chunk = it->second;
                if (p_chunk->m_state1() < State::MESH_GENERATED) continue;

                std::vector<std::tuple<int, int, int, BlockType>> blocks;
                {
                    std::lock_guard lock(m_chunksData.m_pendingBlocksMutex);
                    if (auto pending_it = m_chunksData.m_pendingBlocks.find(key); pending_it != m_chunksData.m_pendingBlocks.end()) {
                        blocks = std::move(pending_it->second);
                        m_chunksData.m_pendingBlocks.erase(pending_it);
                    }
                }

                if (!blocks.empty()) {
                    p_chunk->resetGLBuffers();
                    p_chunk->resetMesh();
                    p_chunk->generatePendingBlocks(blocks);
                    p_chunk->generateMesh();
                    p_chunk->flagForUpdate();
                }
            }
        }
    }
}

void World::generateDataForEachChunks(const int cameraWorldX, const int cameraWorldY, const int cameraWorldZ) {
    const int r = static_cast<int>(Renderer::m_renderDistance / static_cast<float>(Chunk::SIZE));
    const int r2 = r * r;

    // Pre-compute offsets for a circle of chunks around the camera position
    struct Offset { int x, z, maxY; };
    std::vector<Offset> circleOffsets;
    circleOffsets.reserve(static_cast<std::vector<Offset>::size_type>(std::numbers::pi * static_cast<double>(r2)));
    for (int x = -r; x <= r; x++) {
        for (int z = -r; z <= r; z++) {
            if (const int d2 = x*x + z*z; d2 <= r2) {
                circleOffsets.push_back({x, z,
                    static_cast<int>(std::floor(std::sqrt(static_cast<float>(r2 - d2))))});
            }
        }
    }

    // Sort offsets by distance
    std::sort(circleOffsets.begin(), circleOffsets.end(),
              [&](auto &a, auto &b) {
                  return a.x*a.x + a.z*a.z
                   < b.x*b.x + b.z*b.z;
              });

    // Generate chunks
    for (auto [x,z, maxY] : circleOffsets) {
        int chunkX = cameraWorldX + static_cast<int>(x * Chunk::SIZE);
        int chunkZ = cameraWorldZ + static_cast<int>(z * Chunk::SIZE);

        for (int y = -maxY; y <= maxY; y++) {
            const int chunkY = cameraWorldY + static_cast<int>(y * Chunk::SIZE);
            if (chunkY < 0 || chunkY > 256) continue; // World height limit

            const ChunkPosition key = {chunkX, chunkY, chunkZ};
            if (m_chunksData.loadedMeshes.contains(key)) continue;
            m_chunksData.meshesToGenerateVoxel.push(key);
        }
    }
}

void World::unloadDistantMeshes(const glm::vec3 &cameraChunkPos) {
    const float renderDistanceSq = Renderer::m_renderDistance * Renderer::m_renderDistance;

    std::erase_if(m_chunksData.loadedMeshes, [&](const auto &tuple) {
        auto [x, y, z] = tuple.first;
        const float distSq = (x - cameraChunkPos.x) * (x - cameraChunkPos.x)
                             + (y - cameraChunkPos.y) * (y - cameraChunkPos.y)
                             + (z - cameraChunkPos.z) * (z - cameraChunkPos.z);
        return distSq > renderDistanceSq;
    });

    const int cameraHeightmapX = static_cast<int>(std::floor(cameraChunkPos.x / Chunk::SIZE));
    const int cameraHeightmapZ = static_cast<int>(std::floor(cameraChunkPos.z / Chunk::SIZE));
    const float heightmapRenderDistanceSq = Renderer::m_renderDistance / Chunk::SIZE * (Renderer::m_renderDistance / Chunk::SIZE);
    {
        std::lock_guard lock(m_heightMapMutex);
        std::erase_if(m_heightMapByChunk, [&](const auto &pos) {
            const auto [x, z] = pos.first;
            const float distSq = (x - cameraHeightmapX) * (x - cameraHeightmapX)
                                + (z - cameraHeightmapZ) * (z - cameraHeightmapZ);
            return distSq > heightmapRenderDistanceSq;
        });
    }
}
