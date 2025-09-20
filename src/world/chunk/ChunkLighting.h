#ifndef GL_CRAFT_CHUNKLIGHTING_H
#define GL_CRAFT_CHUNKLIGHTING_H
#include <cstdint>
#include <list>
#include <queue>
#include <unordered_map>

#include "RGBLight.h"

struct ChunkPosition;
struct MeshingResult;
struct PendingLight;
class WorldManager;
class Chunk;

class ChunkLighting {
public:
    explicit ChunkLighting(Chunk &chunk);

    void propagateLight() const;
    void generatePendingLights(const std::list<PendingLight> &lights, MeshingResult &outResult) const;
    void emitBorderLights() const;

private:
    struct LightPos {
        int x, y, z;
        LightPos(const int x, const int y, const int z) : x(x), y(y), z(z) {}
    };

private:
    void processInitialLightSources(const std::list<PendingLight> &lights,
                                       std::queue<uint32_t> &sunLightQueue,
                                       std::queue<uint32_t> &blockLightQueue,
                                       std::vector<LightPos> &changed) const;
    static bool isValidLightPosition(int x, int y, int z);
    bool updateSunLight(int x, int y, int z, uint8_t sunlight, std::queue<uint32_t> &queue) const;
    bool updateBlockLight(int x, int y, int z, const RGBLight &blockLight, std::queue<uint32_t> &queue) const;
    void propagateSunLightBFS(std::queue<uint32_t> &sunLightQueue, std::vector<LightPos> &changed) const;
    void propagateSunLightToNeighbors(int x, int y, int z, uint8_t currentLevel,
                                         std::queue<uint32_t> &queue, std::vector<LightPos> &changed) const;
    void propagateBlockLightBFS(std::queue<uint32_t> &blockLightQueue, std::vector<LightPos> &changed) const;
    void propagateBlockLightToNeighbors(int x, int y, int z, const RGBLight &currentRGB,
                                           std::queue<uint32_t> &queue, std::vector<LightPos> &changed) const;
    [[nodiscard]] bool tryUpdateBlockLightAt(int x, int y, int z, const RGBLight &sourceRGB) const;
    void emitChangedLightsToNeighbors(const std::vector<LightPos> &changed) const;

    static bool isBorderPosition(int x, int y, int z);
    void addLightToNeighborChunks(int x, int y, int z, const RGBLight &rgb, uint8_t sunlight,
                                     std::unordered_map<ChunkPosition, std::list<PendingLight>> &toEmit) const;

    void propagateSunLight(std::queue<uint32_t> &sunlightQueue) const;
    void propagateBlockLight(std::queue<uint32_t> &blockLightQueue) const;
    void propagateBlockLightFrom(int localX, int localY, int localZ) const;

    static uint32_t packLightPos(int x, int y, int z);
    static std::tuple<int, int, int> unpackLightPos(uint32_t v);

    [[nodiscard]] uint8_t getSunLightLevelAt(int localX, int localY, int localZ) const;
    void setSunLightLevelAt(int localX, int localY, int localZ, uint8_t lightLevel) const;
    [[nodiscard]] RGBLight getBlockLightRGBLevelAt(int localX, int localY, int localZ) const;
    void setBlockLightRGBAt(int localX, int localY, int localZ, const RGBLight &rgb) const;

private:
    friend class Chunk;
    Chunk &m_chunk;
};

#endif //GL_CRAFT_CHUNKLIGHTING_H