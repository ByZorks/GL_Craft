#include "World.h"

#include <iostream>
#include <ranges>
#include <thread>
#include <unordered_set>

#include "../render/Camera.h"
#include "../render/Renderer.h"
#include "../utils/ThreadSafeQueue.h"
#include "vegetations/grass/ShortGrass.h"
#include "vegetations/trees/Tree.h"

World::World() : m_threadPool(std::max(1u, std::thread::hardware_concurrency())) {
    m_terrainHeightGenerator.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_terrainHeightGenerator.SetFrequency(.0055f);
    m_terrainHeightGenerator.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_terrainHeightGenerator.SetFractalOctaves(6);
    m_terrainHeightGenerator.SetFractalLacunarity(2.2f);

    m_surfaceVegetationGenerator.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_surfaceVegetationGenerator.SetFrequency(.5f);
    m_surfaceVegetationGenerator.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_surfaceVegetationGenerator.SetFractalOctaves(6);

    m_caveGenerator.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_caveGenerator.SetFrequency(.018f);
    m_caveGenerator.SetFractalType(FastNoiseLite::FractalType_Ridged);
    m_caveGenerator.SetFractalOctaves(6);
    m_caveGenerator.SetFractalLacunarity(1.29f);
    m_caveGenerator.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2Reduced);
    m_caveGenerator.SetDomainWarpAmp(20.f);

    m_grassData.renderer.init();
    m_poppyData.renderer.init();
    m_cornflowerData.renderer.init();
    m_alliumData.renderer.init();

    m_chunksData.loadedMeshes.reserve(static_cast<size_t>(Renderer::m_renderDistance * Renderer::m_renderDistance * Renderer::m_renderDistance * 0.5f));

    m_vegetationsData.loadedMeshes.reserve(static_cast<size_t>(Renderer::m_renderDistance * Renderer::m_renderDistance * 0.5f));

    m_heightMap.reserve(static_cast<size_t>(Renderer::m_renderDistance * Renderer::m_renderDistance * 0.5f));
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

void World::drawChunks(const Camera &camera, const Frustum &frustum, Shader &shader, unsigned int &visibleChunksCount) {
    // Remove chunks that are no longer needed, generate voxel and mesh for new chunks, store them in m_chunksData.loadedMeshes
    processChunks();

    // Setup buffers for chunks that are ready to be rendered
    m_displayedNormalMeshes.clear();
    m_displayedTransparentMeshes.clear();
    for (const auto& chunk : m_chunksData.loadedMeshes | std::views::values) {
        if (chunk->m_status1() == Status::MESH_GENERATED) chunk->setupBuffers();
        if (chunk->m_status1() == Status::BUFFERS_SETUP) m_displayedNormalMeshes.push_back(chunk);
    }

    for (const auto& mesh : m_displayedNormalMeshes) {
        auto weak_mesh = mesh.lock();
        if (!weak_mesh) continue;
        if (camera.distanceToCamera(*weak_mesh) > Renderer::m_renderDistance) continue;
        if (!frustum.isAABBInFrustum(weak_mesh->m_box1())) continue;
        shader.setUniform3f("u_Offset",
                            static_cast<float>(weak_mesh->m_x1()),
                            static_cast<float>(weak_mesh->m_y1()),
                            static_cast<float>(weak_mesh->m_z1()));

        weak_mesh->draw();
        if (weak_mesh->hasTransparentFaces()) m_displayedTransparentMeshes.push_back(weak_mesh);
        visibleChunksCount++;
    }

    if (!m_displayedTransparentMeshes.empty()) {
        Renderer::disableDepthMask();
        for (const auto& mesh : m_displayedTransparentMeshes) {
            const auto weak_mesh = mesh.lock();
            if (!weak_mesh) continue;
            shader.setUniform3f("u_Offset",
                                static_cast<float>(weak_mesh->m_x1()),
                                static_cast<float>(weak_mesh->m_y1()),
                                static_cast<float>(weak_mesh->m_z1()));

            weak_mesh->drawTransparent();
        }
        Renderer::enableDepthMask();
    }
}

void World::drawVegetations(const Camera &camera, const Frustum &frustum, Shader &shader, unsigned int &visibleVegetationsCount) {
    // Remove vegetations that are no longer needed, generate voxel and mesh, store them
    processVegetations();

    // Setup buffers for chunks that are ready to be rendered
    m_displayedNormalMeshes.clear();
    m_displayedTransparentMeshes.clear();
    m_displayedBillboardsMeshes.clear();
    for (const auto& vegetation : m_vegetationsData.loadedMeshes | std::views::values) {
        // New meshes
        if (vegetation->m_status1() == Status::MESH_GENERATED) vegetation->setupBuffers();

        // Existing meshes
        if (vegetation->m_status1() == Status::BUFFERS_SETUP) {
            if (vegetation->isBillboard()) {
                m_displayedBillboardsMeshes.push_back(vegetation);
            } else {
                m_displayedNormalMeshes.push_back(vegetation);
            }
        }
    }

    for (const auto& mesh : m_displayedNormalMeshes) {
        auto weak_mesh = mesh.lock();
        if (!weak_mesh) continue;
        if (camera.distanceToCamera(*weak_mesh) > Renderer::m_renderDistance) continue;
        if (!frustum.isAABBInFrustum(weak_mesh->m_box1())) continue;
        shader.setUniform3f("u_Offset",
                            static_cast<float>(weak_mesh->m_x1()),
                            static_cast<float>(weak_mesh->m_y1()),
                            static_cast<float>(weak_mesh->m_z1()));

        weak_mesh->draw();
        if (weak_mesh->hasTransparentFaces()) m_displayedTransparentMeshes.push_back(weak_mesh);
        visibleVegetationsCount++;
    }

    if (!m_displayedTransparentMeshes.empty()) {
        for (const auto& mesh : m_displayedTransparentMeshes) {
            const auto weak_mesh = mesh.lock();
            if (!weak_mesh) continue;
            shader.setUniform3f("u_Offset",
                            static_cast<float>(weak_mesh->m_x1()),
                            static_cast<float>(weak_mesh->m_y1()),
                            static_cast<float>(weak_mesh->m_z1()));

            weak_mesh->drawTransparent();
        }
    }

    // Prevent unnecessary OpenGL calls
    if (!m_displayedBillboardsMeshes.empty()) {
        // Batch all billboards together to reduce OpenGL calls
        Renderer::disableBackFaceCulling();
        for (const auto& mesh : m_displayedBillboardsMeshes) {
            auto weak_mesh = mesh.lock();
            if (!weak_mesh) continue; // Skip if the mesh has been deleted
            if (camera.distanceToCamera(*weak_mesh) > Renderer::m_renderDistance) continue;
            if (!frustum.isAABBInFrustum(weak_mesh->m_box1())) continue;
            shader.setUniform3f("u_Offset",
                                static_cast<float>(weak_mesh->m_x1()),
                                static_cast<float>(weak_mesh->m_y1()),
                                static_cast<float>(weak_mesh->m_z1()));

            weak_mesh->draw();
            visibleVegetationsCount++;
        }
        Renderer::enableBackFaceCulling();
    }
}

void World::drawInstances(const Camera &camera, const Frustum &frustum, unsigned int &visibleVegetationsCount) {
    if (camera.hasCameraChangedDirection() || camera.hasCameraChangedChunk()) {
        m_grassData.renderer.resetInstances();
        m_poppyData.renderer.resetInstances();
        m_cornflowerData.renderer.resetInstances();
        m_alliumData.renderer.resetInstances();

        for (const auto &pos : m_grassData.instances) {
            if (camera.distanceToCamera(pos) <= Renderer::m_renderDistance &&
                frustum.isPointInFrustum(pos)) {
                m_grassData.renderer.addInstance(pos);
                visibleVegetationsCount++;
            }
        }

        m_grassData.renderer.updateInstanceBuffer();

        for (const auto &pos : m_poppyData.instances) {
            if (camera.distanceToCamera(pos) <= Renderer::m_renderDistance &&
                frustum.isPointInFrustum(pos)) {
                m_poppyData.renderer.addInstance(pos);
                visibleVegetationsCount++;
            }
        }

        m_poppyData.renderer.updateInstanceBuffer();

        for (const auto &pos : m_cornflowerData.instances) {
            if (camera.distanceToCamera(pos) <= Renderer::m_renderDistance &&
                frustum.isPointInFrustum(pos)) {
                m_cornflowerData.renderer.addInstance(pos);
                visibleVegetationsCount++;
                }
        }

        m_cornflowerData.renderer.updateInstanceBuffer();

        for (const auto &pos : m_alliumData.instances) {
            if (camera.distanceToCamera(pos) <= Renderer::m_renderDistance &&
                frustum.isPointInFrustum(pos)) {
                m_alliumData.renderer.addInstance(pos);
                visibleVegetationsCount++;
                }
        }

        m_alliumData.renderer.updateInstanceBuffer();

    }

    if (m_grassData.renderer.m_instance_count() > 0) {
        Renderer::disableBackFaceCulling();
        m_grassData.renderer.draw();
        Renderer::enableBackFaceCulling();
    }

    if (m_poppyData.renderer.m_instance_count() > 0) {
        Renderer::disableBackFaceCulling();
        m_poppyData.renderer.draw();
        Renderer::enableBackFaceCulling();
    }

    if (m_cornflowerData.renderer.m_instance_count() > 0) {
        Renderer::disableBackFaceCulling();
        m_cornflowerData.renderer.draw();
        Renderer::enableBackFaceCulling();
    }

    if (m_alliumData.renderer.m_instance_count() > 0) {
        Renderer::disableBackFaceCulling();
        m_alliumData.renderer.draw();
        Renderer::enableBackFaceCulling();
    }
}

int World::getHeight(const int worldX, const int worldZ) {
    constexpr int maxHeight = 256;
    constexpr int baseHeight = 60;
    const std::pair coords(worldX, worldZ);

    // Check if height is already cached
    if (const auto it = m_heightMap.find(coords); it != m_heightMap.end()) return it->second;

    // 2D noise generation for terrain height
    const float normalizedNoise = (m_terrainHeightGenerator.GetNoise(static_cast<float>(worldX), static_cast<float>(worldZ)) + 1.0f) / 2.0f; // Normalize to [0, 1]
    const float terrainShape = std::pow(normalizedNoise, 4.6f); // Create more plains and sharper mountains
    float columnHeight = std::floor(baseHeight + terrainShape * maxHeight); // Scale to world height

    // 3D noise generation for cave system
    const float normalized3DNoise = (m_caveGenerator.GetNoise(static_cast<float>(worldX), columnHeight, static_cast<float>(worldZ)) + 1.0f) / 2.0f; // Normalize to [0, 1]
    constexpr float baseCaveThreshold = 0.82f;
    const float surfaceModifier = 1.0f - std::clamp((columnHeight - baseHeight) / (maxHeight * 0.7f), 0.0f, 1.0f);
    const float caveThreshold = baseCaveThreshold + surfaceModifier * 0.15f; // Increase threshold near surface

    // Adjust column height based on cave system
    if (std::abs(normalized3DNoise - caveThreshold) < 0.13) {
        columnHeight -= (normalized3DNoise - (caveThreshold - 0.13f)) * 10.0f;
    }

    {
        std::lock_guard lock(m_heightMapMutex);
        m_heightMap.try_emplace(coords, columnHeight);
    }

    return static_cast<int>(columnHeight);
}

bool World::isCave(const int worldX, const int worldY, const int worldZ) const {
    constexpr int maxHeight = 256;
    constexpr int baseHeight = 60;

    if (worldY <= 1 || worldY > maxHeight) return false;

    // 3D noise generation for cave system
    const float normalized3DNoise = (m_caveGenerator.GetNoise(static_cast<float>(worldX), static_cast<float>(worldY), static_cast<float>(worldZ)) + 1.0f) / 2.0f; // Normalize to [0, 1]
    constexpr float baseCaveThreshold = 0.82f;
    const float surfaceModifier = 1.0f - std::clamp(static_cast<float>(worldY - baseHeight) / (maxHeight * 0.7f), 0.0f, 1.0f);
    const float caveThreshold = baseCaveThreshold + surfaceModifier * 0.15f; // Increase threshold near surface

    return std::abs(normalized3DNoise - caveThreshold) < 0.13f;
}

const FastNoiseLite & World::m_noise_generator() const {
    return m_terrainHeightGenerator;
}

const FastNoiseLite & World::m_surface_vegetation_generator() const {
    return m_surfaceVegetationGenerator;
}

const std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Chunk>> & World::m_loaded_chunks() const {
    return m_chunksData.loadedMeshes;
}

const std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Vegetation>> & World::m_loaded_vegetations() const {
    return m_vegetationsData.loadedMeshes;
}

void World::processChunks() {
    const int maxChunksPerFrame = static_cast<int>(0.3 * Renderer::m_renderDistance + 0.6 * static_cast<float>(m_threadPool.m_num_threads()));
    // Remove chunks that are no longer needed
    for (int i = 0; i < maxChunksPerFrame; ++i) {
        if (m_chunksData.meshesToDelete.empty()) break;
        m_chunksData.meshesToDelete.pop();
    }

    // Process chunks that are within the render distance
    for (int i = 0; i < maxChunksPerFrame; ++i) {
        if (m_chunksData.meshesToGenerate.empty()) break;

        std::tuple<int, int, int> key = m_chunksData.meshesToGenerate.pop();
        m_threadPool.enqueue([this, key] {
            const auto p_chunk = std::make_shared<Chunk>(std::get<0>(key), std::get<1>(key), std::get<2>(key));
            p_chunk->generateVoxel(*this);
            p_chunk->generateMesh();
            if (!p_chunk->hasVisibleFaces()) {
                m_chunksData.meshesToDelete.push(p_chunk);
                return;
            }
            this->generateVegetationsForEachChunks(p_chunk);
            m_chunksData.meshesToRender.push(p_chunk);
        });
    }

    for (int i = 0; i < maxChunksPerFrame; ++i) {
        if (m_chunksData.meshesToRender.empty()) break;
        std::shared_ptr<Chunk> p_chunk = m_chunksData.meshesToRender.pop();
        std::tuple<int, int, int> key = std::make_tuple(p_chunk->m_x1(), p_chunk->m_y1(), p_chunk->m_z1());
        m_chunksData.loadedMeshes.try_emplace(key, p_chunk);
    }
}

void World::processVegetations() {
    const int maxVegetationsPerFrame = static_cast<int>(0.2 * Renderer::m_renderDistance + 0.4 * static_cast<float>(m_threadPool.m_num_threads()));
    // Remove vegetations that are no longer needed
    for (int i = 0; i < maxVegetationsPerFrame; ++i) {
        if (m_vegetationsData.meshesToDelete.empty()) break;
        m_vegetationsData.meshesToDelete.pop();
    }

    // Process chunks that are within the render distance
    for (int i = 0; i < maxVegetationsPerFrame; ++i) {
        if (m_vegetationsData.meshesToGenerate.empty()) break;

        m_threadPool.enqueue([this] {
            std::tuple<int, int, int> key = m_vegetationsData.meshesToGenerate.pop();
            float vegetationNoise = (this->m_surfaceVegetationGenerator.GetNoise(static_cast<float>(std::get<0>(key)), static_cast<float>(std::get<2>(key))) + 1.0f) * 0.5f;
            std::shared_ptr<Vegetation> p_vegetation;
            if (vegetationNoise > 0.87f) {
                p_vegetation = std::make_shared<Tree>(std::get<0>(key), std::get<1>(key), std::get<2>(key));
            } else if (vegetationNoise > 0.7f) {
                const glm::vec3 position(static_cast<float>(std::get<0>(key) - 1), static_cast<float>(std::get<1>(key)), static_cast<float>(std::get<2>(key) - 1));
                std::lock_guard lock(m_grassData.mutex);
                if (!m_grassData.instances.contains(position)) m_grassData.instances.emplace(position);
            } else if (vegetationNoise > 0.69f) {
                const glm::vec3 position(static_cast<float>(std::get<0>(key) - 1), static_cast<float>(std::get<1>(key)), static_cast<float>(std::get<2>(key) - 1));
                if (!m_poppyData.instances.contains(position) &&
                    !m_cornflowerData.instances.contains(position) &&
                    !m_alliumData.instances.contains(position)) {
                    switch (rand() % 3) {
                        case 0: {
                            std::lock_guard lock(m_poppyData.mutex);
                            m_poppyData.instances.emplace(position);
                        }
                            break;
                        case 1: {
                            std::lock_guard lock(m_cornflowerData.mutex);
                            m_cornflowerData.instances.emplace(position);
                        }
                            break;
                        case 2: {
                            std::lock_guard lock(m_alliumData.mutex);
                            m_alliumData.instances.emplace(position);
                        }
                            break;
                        default: {}
                    }
                }
            }
            if (p_vegetation) {
                p_vegetation->generateVoxel();
                p_vegetation->generateMesh();
                m_vegetationsData.meshesToRender.push(p_vegetation);
            }
        });
    }

    for (int i = 0; i < maxVegetationsPerFrame; ++i) {
        if (m_vegetationsData.meshesToRender.empty()) break;
        std::shared_ptr<Vegetation> p_vegetation = m_vegetationsData.meshesToRender.pop();
        std::tuple<int, int, int> key = std::make_tuple(p_vegetation->m_x1(), p_vegetation->m_y1(), p_vegetation->m_z1());
        m_vegetationsData.loadedMeshes.try_emplace(key, p_vegetation);
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

            const std::tuple<int, int, int> key = std::make_tuple(chunkX, chunkY, chunkZ);
            if (m_chunksData.loadedMeshes.contains(key)) continue;
            m_chunksData.meshesToGenerate.push(key);
        }
    }
}

void World::generateVegetationsForEachChunks(const std::shared_ptr<Chunk> &chunk) {
    for (int localX = 0; localX < Chunk::SIZE; ++localX) {
        const int worldX = chunk->m_x1() + localX;

        for (int localZ = 0; localZ < Chunk::SIZE; ++localZ) {
            const int worldZ = chunk->m_z1() + localZ;
            const int columnHeight = getHeight(worldX, worldZ);
            if (columnHeight < chunk->m_y1() || columnHeight >= chunk->m_y1() + Chunk::SIZE || isCave(worldX, columnHeight, worldZ)) continue;
            float vegetationNoise = (m_surfaceVegetationGenerator.GetNoise(static_cast<float>(worldX), static_cast<float>(worldZ)) + 1.0f) * 0.5f;
            if (vegetationNoise <= 0.69f) continue;
            std::tuple<int, int, int> key = std::make_tuple(worldX, columnHeight, worldZ);
            m_vegetationsData.meshesToGenerate.push(key);
        }
    }
}

void World::unloadDistantMeshes(const glm::vec3 &cameraChunkPos) {
    const float renderDistanceSq = Renderer::m_renderDistance * Renderer::m_renderDistance;

    std::unordered_set<std::pair<int, int>> toRemoveXZ;

    std::erase_if(m_chunksData.loadedMeshes, [&](const auto &tuple) {
        auto [x, y, z] = tuple.first;
        const glm::vec3 pos(x, y, z);
        float distSq = glm::distance(pos, cameraChunkPos);
        distSq *= distSq;
        if (distSq > renderDistanceSq) {
            toRemoveXZ.emplace(x, z);
            return true;
        }
        return false;
    });

    std::erase_if(m_vegetationsData.loadedMeshes, [&](const auto &tuple) {
        auto [x, y, z] = tuple.first;
        const glm::vec3 pos(x, y, z);
        float distSq = glm::distance(pos, cameraChunkPos);
        distSq *= distSq;
        if (distSq > renderDistanceSq) {
            return true;
        }
        return false;
    });

    std::erase_if(m_grassData.instances, [&](const auto &pos) {
        float distSq = glm::distance(pos, cameraChunkPos);
        distSq *= distSq;
        if (distSq > renderDistanceSq) {
            return true;
        }
        return false;
    });

    std::erase_if(m_poppyData.instances, [&](const auto &pos) {
        float distSq = glm::distance(pos, cameraChunkPos);
        distSq *= distSq;
        if (distSq > renderDistanceSq) {
            return true;
        }
        return false;
    });

    std::erase_if(m_cornflowerData.instances, [&](const auto &pos) {
        float distSq = glm::distance(pos, cameraChunkPos);
        distSq *= distSq;
        if (distSq > renderDistanceSq) {
            return true;
        }
        return false;
    });

    std::erase_if(m_alliumData.instances, [&](const auto &pos) {
        float distSq = glm::distance(pos, cameraChunkPos);
        distSq *= distSq;
        if (distSq > renderDistanceSq) {
            return true;
        }
        return false;
    });

    std::lock_guard lock(m_heightMapMutex);
    for (const auto &pair : toRemoveXZ) {
        m_heightMap.erase(pair);
    }
}
