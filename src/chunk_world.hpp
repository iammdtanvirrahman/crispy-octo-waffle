#pragma once

#include "voxel_engine.hpp"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace voxel {

struct ChunkCoord {
    int x{};
    int z{};
    friend bool operator==(const ChunkCoord&, const ChunkCoord&) = default;
};

struct ChunkCoordHash {
    std::size_t operator()(const ChunkCoord& c) const noexcept {
        const std::uint64_t x = static_cast<std::uint32_t>(c.x);
        const std::uint64_t z = static_cast<std::uint32_t>(c.z);
        return static_cast<std::size_t>((x << 32U) ^ z);
    }
};

class ChunkWorld {
public:
    explicit ChunkWorld(std::uint64_t seed);

    Chunk& ensureChunk(int cx, int cz);
    void streamAround(int centerX, int centerZ, int radius);
    [[nodiscard]] Block getWorld(int worldX, int y, int worldZ) const noexcept;
    [[nodiscard]] const std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash>& chunks() const noexcept { return chunks_; }
    [[nodiscard]] std::size_t loadedChunkCount() const noexcept { return chunks_.size(); }

    void rebuildMeshes();
    [[nodiscard]] const Mesh* meshFor(int cx, int cz) const noexcept;

private:
    std::uint64_t seed_;
    WorldGenerator generator_;
    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> chunks_;
    std::unordered_map<ChunkCoord, Mesh, ChunkCoordHash> meshes_;

    static int floorDiv(int value, int divisor) noexcept;
    static int positiveMod(int value, int divisor) noexcept;
    [[nodiscard]] Block getChunkAware(const Chunk& chunk, int x, int y, int z) const noexcept;
};

} // namespace voxel
