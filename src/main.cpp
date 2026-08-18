#include "voxel_engine.hpp"

#include <cstdint>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    std::uint64_t seed = 123456789ULL;
    if (argc > 1) {
        try {
            seed = std::stoull(argv[1]);
        } catch (...) {
            std::cerr << "Invalid seed. Using default.\n";
        }
    }

    voxel::Chunk chunk(0, 0);
    voxel::WorldGenerator generator(seed);
    generator.generate(chunk);
    const voxel::Mesh mesh = chunk.buildVisibleMesh();

    std::cout << "Voxel Frontier native core\n"
              << "seed=" << seed << '\n'
              << "chunk=" << chunk.x() << ',' << chunk.z() << '\n'
              << "visible_vertices=" << mesh.vertices.size() << '\n'
              << "visible_indices=" << mesh.indices.size() << '\n';

    return 0;
}
