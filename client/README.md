# Structure

## Engine

Connects everything

Has access to:
 - GameLoop

Thread:
 - Main thread

## GameLoop

Handles game logic

Stores:
 - PlayerPos
 - ChunksToRender

Thread:
 - GameLoop thread

## RHI

Handles rendering

Has access to:
 - Engine

Stores:
 - ChunkMeshes

Thread:
 - RHI thread