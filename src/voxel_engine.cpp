#include "voxel_engine.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace voxel {
namespace {

constexpr std::array<std::array<int, 3>, 6> kDirections{{
    {{1, 0, 0}}, {{-1, 0, 0}}, {{0, 1, 0}},
    {{0, -1, 0}}, {{0, 0, 1}}, {{0, 0, -1}}
}};

constexpr std::array<std::array<std::array<float, 3>, 4>, 6> kFaceNormals{{
    {{{1.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, {1.f, 0.f, 0.f}}},
    {{{-1.f, 0.f, 0.f}, {-1.f, 0.f, 0.f}, {-1.f, 0.f, 0.f}, {-1.f, 0.f, 0.f}}},
    {{{0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}}},
    {{{0.f, -1.f, 0.f}, {0.f, -1.f, 0.f}, {0.f, -1.f, 0.f}, {0.f, -1.f, 0.f}}},
    {{{0.f, 0.f, 1.f}, {0.f, 0.f, 1.f}, {0.f, 0.f, 1.f}, {0.f, 0.f, 1.f}}},
    {{{0.f, 0.f, -1.f}, {0.f, 0.f, -1.f}, {0.f, 0.f, -1.f}, {0.f, 0.f, -1.f}}}
}};

constexpr std::array<std::array<std::array<float, 3>, 4>, 6> kFaceVerts{{
    {{{1,0,0},{1,1,0},{1,1,1},{1,0,1}}},
    {{{0,0,1},{0,1,1},{0,1,0},{0,0,0}}},
    {{{0,1,1},{1,1,1},{1,1,0},{0,1,0}}},
    {{{0,0,0},{1,0,0},{1,0,1},{0,0,1}}},
    {{{1,0,1},{1,1,1},{0,1,1},{0,0,1}}},
    {{{0,0,0},{0,1,0},{1,1,0},{1,0,0}}}
}};

bool opaque(Block block) noexcept {
    return block != Block::Air && block != Block::Glass;
}

} // namespace

Chunk::Chunk(int cx, int cz) : cx_(cx), cz_(cz) {
    blocks_.fill(static_cast<BlockId>(Block::Air));
}

std::size_t Chunk::indexOf(int x, int y, int z) noexcept {
    return static_cast<std::size_t>(y * CHUNK_SIZE * CHUNK_SIZE + z * CHUNK_SIZE + x);
}

Block Chunk::get(int x, int y, int z) const noexcept {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE) {
        return Block::Air;
    }
    return static_cast<Block>(blocks_[indexOf(x, y, z)]);
}

void Chunk::set(int x, int y, int z, Block block) noexcept {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE) {
        return;
    }
    blocks_[indexOf(x, y, z)] = static_cast<BlockId>(block);
}

Mesh Chunk::buildVisibleMesh() const {
    Mesh mesh;
    mesh.vertices.reserve(CHUNK_VOLUME / 2);
    mesh.indices.reserve(CHUNK_VOLUME);

    for (int y = 0; y < CHUNK_HEIGHT; ++y) {
        for (int z = 0; z < CHUNK_SIZE; ++z) {
            for (int x = 0; x < CHUNK_SIZE; ++x) {
                const Block block = get(x, y, z);
                if (block == Block::Air) continue;

                for (int face = 0; face < 6; ++face) {
                    const auto& d = kDirections[face];
                    const Block neighbor = get(x + d[0], y + d[1], z + d[2]);
                    if (opaque(neighbor)) continue;

                    const auto base = static_cast<std::uint32_t>(mesh.vertices.size());
                    for (int i = 0; i < 4; ++i) {
                        const auto& p = kFaceVerts[face][i];
                        const auto& n = kFaceNormals[face][i];
                        const float u = (i == 1 || i == 2) ? 1.f : 0.f;
                        const float v = (i >= 2) ? 1.f : 0.f;
                        mesh.vertices.push_back(Vertex{
                            static_cast<float>(cx_ * CHUNK_SIZE + x) + p[0],
                            static_cast<float>(y) + p[1],
                            static_cast<float>(cz_ * CHUNK_SIZE + z) + p[2],
                            n[0], n[1], n[2], u, v,
                            static_cast<BlockId>(block)
                        });
                    }
                    mesh.indices.insert(mesh.indices.end(), {
                        base, base + 1, base + 2,
                        base, base + 2, base + 3
                    });
                }
            }
        }
    }
    return mesh;
}

WorldGenerator::WorldGenerator(std::uint64_t seed) : seed_(seed) {}

float WorldGenerator::noise2D(int x, int z) const noexcept {
    std::uint64_t h = seed_;
    h ^= static_cast<std::uint64_t>(x) * 0x9E3779B185EBCA87ULL;
    h ^= static_cast<std::uint64_t>(z) * 0xC2B2AE3D27D4EB4FULL;
    h ^= h >> 30;
    h *= 0xBF58476D1CE4E5B9ULL;
    h ^= h >> 27;
    h *= 0x94D049BB133111EBULL;
    h ^= h >> 31;
    return static_cast<float>((h & 0xFFFFFFu) / 16777215.0 * 2.0 - 1.0);
}

int WorldGenerator::heightAt(int worldX, int worldZ) const noexcept {
    const float broad = noise2D(worldX / 32, worldZ / 32);
    const float medium = noise2D(worldX / 8, worldZ / 8);
    const float fine = noise2D(worldX / 2, worldZ / 2);
    const float h = 30.f + broad * 13.f + medium * 5.f + fine * 2.f;
    return std::clamp(static_cast<int>(std::lround(h)), 3, CHUNK_HEIGHT - 2);
}

void WorldGenerator::generate(Chunk& chunk) const {
    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int z = 0; z < CHUNK_SIZE; ++z) {
            const int worldX = chunk.x() * CHUNK_SIZE + x;
            const int worldZ = chunk.z() * CHUNK_SIZE + z;
            const int top = heightAt(worldX, worldZ);

            for (int y = 0; y <= top; ++y) {
                Block block = Block::Stone;
                if (y == top) block = (top < 7) ? Block::Sand : Block::Grass;
                else if (y >= top - 3) block = Block::Dirt;
                else if (y > 4 && (noise2D(worldX + y * 13, worldZ - y * 7) > 0.82f)) block = Block::CoalOre;
                chunk.set(x, y, z, block);
            }
        }
    }
}

} // namespace voxel
