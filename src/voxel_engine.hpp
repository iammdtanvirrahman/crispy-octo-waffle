#pragma once

#include <array>
#include <cstdint>
#include <random>
#include <vector>

namespace voxel {

constexpr int CHUNK_SIZE = 16;
constexpr int CHUNK_HEIGHT = 96;
constexpr int CHUNK_VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT;

using BlockId = std::uint8_t;

enum class Block : BlockId {
    Air = 0,
    Grass,
    Dirt,
    Stone,
    Sand,
    Wood,
    Leaves,
    Glass,
    CoalOre,
    IronOre,
};

struct Vertex {
    float px, py, pz;
    float nx, ny, nz;
    float u, v;
    BlockId block;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;

    void clear() noexcept {
        vertices.clear();
        indices.clear();
    }
};

class Chunk {
public:
    Chunk(int cx, int cz);

    [[nodiscard]] int x() const noexcept { return cx_; }
    [[nodiscard]] int z() const noexcept { return cz_; }

    [[nodiscard]] Block get(int x, int y, int z) const noexcept;
    void set(int x, int y, int z, Block block) noexcept;

    [[nodiscard]] const std::array<BlockId, CHUNK_VOLUME>& data() const noexcept { return blocks_; }
    [[nodiscard]] Mesh buildVisibleMesh() const;

private:
    static std::size_t indexOf(int x, int y, int z) noexcept;

    int cx_;
    int cz_;
    std::array<BlockId, CHUNK_VOLUME> blocks_{};
};

class WorldGenerator {
public:
    explicit WorldGenerator(std::uint64_t seed);
    void generate(Chunk& chunk) const;

private:
    std::uint64_t seed_;
    [[nodiscard]] int heightAt(int worldX, int worldZ) const noexcept;
    [[nodiscard]] float noise2D(int x, int z) const noexcept;
};

} // namespace voxel
