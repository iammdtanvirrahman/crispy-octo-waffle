#include "chunk_world.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace voxel {

ChunkWorld::ChunkWorld(std::uint64_t seed) : seed_(seed), generator_(seed) {}

int ChunkWorld::floorDiv(int value, int divisor) noexcept {
    int q = value / divisor;
    int r = value % divisor;
    if (r != 0 && ((r < 0) != (divisor < 0))) --q;
    return q;
}

int ChunkWorld::positiveMod(int value, int divisor) noexcept {
    int r = value % divisor;
    if (r < 0) r += divisor;
    return r;
}

Chunk& ChunkWorld::ensureChunk(int cx, int cz) {
    const ChunkCoord coord{cx, cz};
    auto it = chunks_.find(coord);
    if (it != chunks_.end()) return *it->second;

    auto chunk = std::make_unique<Chunk>(cx, cz);
    generator_.generate(*chunk);
    auto [inserted, _] = chunks_.emplace(coord, std::move(chunk));
    return *inserted->second;
}

void ChunkWorld::streamAround(int centerX, int centerZ, int radius) {
    radius = std::max(0, radius);
    for (int dz = -radius; dz <= radius; ++dz) {
        for (int dx = -radius; dx <= radius; ++dx) {
            if (dx * dx + dz * dz > radius * radius) continue;
            ensureChunk(centerX + dx, centerZ + dz);
        }
    }

    for (auto it = chunks_.begin(); it != chunks_.end();) {
        const int dx = it->first.x - centerX;
        const int dz = it->first.z - centerZ;
        if (dx * dx + dz * dz > (radius + 1) * (radius + 1)) {
            meshes_.erase(it->first);
            it = chunks_.erase(it);
        } else {
            ++it;
        }
    }
}

Block ChunkWorld::getWorld(int worldX, int y, int worldZ) const noexcept {
    if (y < 0 || y >= CHUNK_HEIGHT) return Block::Air;
    const int cx = floorDiv(worldX, CHUNK_SIZE);
    const int cz = floorDiv(worldZ, CHUNK_SIZE);
    const auto it = chunks_.find(ChunkCoord{cx, cz});
    if (it == chunks_.end()) return Block::Air;
    return it->second->get(positiveMod(worldX, CHUNK_SIZE), y, positiveMod(worldZ, CHUNK_SIZE));
}

Block ChunkWorld::getChunkAware(const Chunk& chunk, int x, int y, int z) const noexcept {
    if (y < 0 || y >= CHUNK_HEIGHT) return Block::Air;
    if (x >= 0 && x < CHUNK_SIZE && z >= 0 && z < CHUNK_SIZE) return chunk.get(x, y, z);
    const int wx = chunk.x() * CHUNK_SIZE + x;
    const int wz = chunk.z() * CHUNK_SIZE + z;
    return getWorld(wx, y, wz);
}

void ChunkWorld::rebuildMeshes() {
    meshes_.clear();
    for (const auto& [coord, chunk] : chunks_) {
        Mesh mesh;
        mesh.vertices.reserve(CHUNK_VOLUME / 3);
        mesh.indices.reserve(CHUNK_VOLUME / 2);

        // Greedy meshing is intentionally kept in the chunk/world layer so a
        // future GPU renderer only receives already-merged quads.
        for (int axis = 0; axis < 3; ++axis) {
            const int uAxis = (axis + 1) % 3;
            const int vAxis = (axis + 2) % 3;
            const int dimU = (uAxis == 1) ? CHUNK_HEIGHT : CHUNK_SIZE;
            const int dimV = (vAxis == 1) ? CHUNK_HEIGHT : CHUNK_SIZE;
            const int dimD = (axis == 1) ? CHUNK_HEIGHT : CHUNK_SIZE;

            struct Cell { Block a; Block b; };
            std::vector<Cell> mask(static_cast<std::size_t>(dimU * dimV));

            for (int d = -1; d < dimD; ++d) {
                std::fill(mask.begin(), mask.end(), Cell{Block::Air, Block::Air});
                for (int v = 0; v < dimV; ++v) {
                    for (int u = 0; u < dimU; ++u) {
                        int p[3]{0, 0, 0};
                        p[uAxis] = u; p[vAxis] = v; p[axis] = d;
                        int q[3]{p[0], p[1], p[2]}; q[axis] = d + 1;
                        const Block a = getChunkAware(*chunk, p[0], p[1], p[2]);
                        const Block b = getChunkAware(*chunk, q[0], q[1], q[2]);
                        mask[static_cast<std::size_t>(u + v * dimU)] = {a, b};
                    }
                }

                for (int v = 0; v < dimV; ++v) {
                    for (int u = 0; u < dimU;) {
                        const Cell c = mask[static_cast<std::size_t>(u + v * dimU)];
                        const bool positive = c.a != Block::Air && c.b == Block::Air;
                        const bool negative = c.a == Block::Air && c.b != Block::Air;
                        if (!positive && !negative) { ++u; continue; }
                        const Block material = positive ? c.a : c.b;

                        int width = 1;
                        while (u + width < dimU) {
                            const Cell n = mask[static_cast<std::size_t>(u + width + v * dimU)];
                            if ((positive && !(n.a == material && n.b == Block::Air)) ||
                                (negative && !(n.a == Block::Air && n.b == material))) break;
                            ++width;
                        }

                        int height = 1;
                        bool done = false;
                        while (v + height < dimV && !done) {
                            for (int k = 0; k < width; ++k) {
                                const Cell n = mask[static_cast<std::size_t>(u + k + (v + height) * dimU)];
                                if ((positive && !(n.a == material && n.b == Block::Air)) ||
                                    (negative && !(n.a == Block::Air && n.b == material))) {
                                    done = true; break;
                                }
                            }
                            if (!done) ++height;
                        }

                        float origin[3]{0,0,0};
                        float du[3]{0,0,0};
                        float dv[3]{0,0,0};
                        origin[axis] = static_cast<float>(d + 1);
                        origin[uAxis] = static_cast<float>(u);
                        origin[vAxis] = static_cast<float>(v);
                        du[uAxis] = static_cast<float>(width);
                        dv[vAxis] = static_cast<float>(height);
                        if (negative) origin[axis] = static_cast<float>(d + 1);

                        const float normalSign = positive ? 1.0f : -1.0f;
                        const std::uint32_t base = static_cast<std::uint32_t>(mesh.vertices.size());
                        float p0[3]{origin[0],origin[1],origin[2]};
                        float p1[3]{origin[0]+du[0],origin[1]+du[1],origin[2]+du[2]};
                        float p2[3]{origin[0]+du[0]+dv[0],origin[1]+du[1]+dv[1],origin[2]+du[2]+dv[2]};
                        float p3[3]{origin[0]+dv[0],origin[1]+dv[1],origin[2]+dv[2]};
                        if (!positive) std::swap(p1, p3);

                        const float nx = axis == 0 ? normalSign : 0.0f;
                        const float ny = axis == 1 ? normalSign : 0.0f;
                        const float nz = axis == 2 ? normalSign : 0.0f;
                        const float uMax = static_cast<float>(width);
                        const float vMax = static_cast<float>(height);
                        const float worldOffsetX = static_cast<float>(chunk->x() * CHUNK_SIZE);
                        const float worldOffsetZ = static_cast<float>(chunk->z() * CHUNK_SIZE);

                        const float* pts[4]{p0,p1,p2,p3};
                        const float uv[4][2]{{0,0},{uMax,0},{uMax,vMax},{0,vMax}};
                        for (int i = 0; i < 4; ++i) {
                            mesh.vertices.push_back(Vertex{
                                pts[i][0] + worldOffsetX, pts[i][1], pts[i][2] + worldOffsetZ,
                                nx, ny, nz, uv[i][0], uv[i][1], static_cast<BlockId>(material)
                            });
                        }
                        mesh.indices.insert(mesh.indices.end(), {base,base+1,base+2,base,base+2,base+3});

                        for (int yy = 0; yy < height; ++yy)
                            for (int xx = 0; xx < width; ++xx)
                                mask[static_cast<std::size_t>(u + xx + (v + yy) * dimU)] = {Block::Air, Block::Air};
                        u += width;
                    }
                }
            }
        }
        meshes_.emplace(coord, std::move(mesh));
    }
}

const Mesh* ChunkWorld::meshFor(int cx, int cz) const noexcept {
    const auto it = meshes_.find(ChunkCoord{cx, cz});
    return it == meshes_.end() ? nullptr : &it->second;
}

} // namespace voxel
