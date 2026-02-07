# AGENTS.md

This file provides guidance to agents when working with code in this repository.

## Build, Lint, and Test Commands
- **Build**: Use `cmake` to configure and build the project. Example:
  ```bash
  mkdir build && cd build
  cmake ..
  make
  ```
- **Run Client**: Execute the `appClient` binary generated in the build directory.
- **Run Server**: Execute the `voxel-server` binary generated in the build directory.

## Code Style Guidelines
- **C++ Standard**: The project uses C++17.
- **Header Guards**: All headers use `#ifndef`/`#define` guards.
- **Mutex Usage**: Shared and exclusive locks are used for thread safety in `Region` and `World` classes.
- **Hashing**: Custom hash functions (e.g., `RegionPosHash`) use golden ratio hashing for better bit dispersion.
- **Chunk Mesh Generation**: Use `ChunkMeshGenerator::generateChunkMesh` for creating chunk meshes.
- **Shader Management**: Shaders are loaded dynamically using `QShader`.

## Project-Specific Patterns
- **Region Locking**: Use `claimReadLock` for reading and `claimWriteLock` for writing to regions.
- **Game Loop**: The game loop runs at 20 ticks per second (50ms per tick) with spiral-of-death protection.
- **Chunk Updates**: Chunk updates are queued and processed asynchronously in `RHIRender`.
- **Perlin Noise**: Chunk generation uses 2D Perlin noise for terrain height calculation.
- **Relative Position Encoding**: Relative chunk positions are encoded in 32-bit integers for rendering.

## Testing
- No dedicated test framework or test files were found in the repository. Testing appears to be manual or integrated into the build process.

## Additional Notes
- **Qt Integration**: The project heavily relies on Qt6 for GUI, QML, and RHI rendering.
- **Directory Structure**: Shared code is in `shared/`, client-specific code in `client/`, and server-specific code in `server/`.
- **Shader Compilation**: Shaders are precompiled and managed via `qt_add_shaders` in the `CMakeLists.txt`.