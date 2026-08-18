#include "chunk_world.hpp"

#include <cstdint>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    std::uint64_t seed = 123456789ULL;
    int radius = 2;
    if (argc > 1) {
        try { seed = std::stoull(argv[1]); }
        catch (...) { std::cerr << "Invalid seed. Using default.\n"; }
    }
    if (argc > 2) {
        try { radius = std::stoi(argv[2]); }
        catch (...) { std::cerr << "Invalid radius. Using 2.\n"; }
    }

    voxel::ChunkWorld world(seed);
    world.streamAround(0, 0, radius);
    world.rebuildMeshes();

    std::size_t vertices = 0;
    std::size_t indices = 0;
    for (const auto& [coord, chunk] : world.chunks()) {
        if (const voxel::Mesh* mesh = world.meshFor(coord.x, coord.z)) {
            vertices += mesh->vertices.size();
            indices += mesh->indices.size();
        }
    }

    std::cout << "Voxel Frontier native engine\n"
              << "seed=" << seed << "\n"
              << "stream_radius=" << radius << "\n"
              << "loaded_chunks=" << world.loadedChunkCount() << "\n"
              << "merged_vertices=" << vertices << "\n"
              << "merged_indices=" << indices << "\n"
              << "meshing=greedy+cross-chunk-face-culling\n";
    return 0;
}
