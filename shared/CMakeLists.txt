add_library(voxel-shared STATIC
    src/block.cpp
    src/blockdefinitionlookup.cpp
    src/chunk.cpp
    src/chunkgeneration.cpp
    src/chunkmeshgeneration.cpp
    src/region.cpp
)

target_include_directories(voxel-shared PUBLIC include)
target_link_libraries(voxel-shared PUBLIC Qt6::Core )