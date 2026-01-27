#include "engine.h"
#include "region.h"
#include <QtCore/qnamespace.h>

Engine::Engine(QObject *parent) : QObject(parent) {
  m_world.reset(new World());

  // placeholder for actual world generation
  m_world->basicSetup();

  connect(m_rhiView, &RHIView::rhiRenderReady, this,
          &Engine::handleRHIRenderReady);
}

Engine::~Engine() {}

void Engine::setRHIView(RHIView *rhiView) {
  m_rhiView = rhiView;
  connect(m_rhiView, &RHIView::rhiRenderReady, this,
          &Engine::handleRHIRenderReady);
}

void Engine::handleRHIRenderReady() {
  const Region *region = m_world->getRegion({0, 0, 0});
  if (!region) {
    return;
  }

  for (uint8_t x = 0; x < Region::REGION_SIZE; x++) {
    for (uint8_t y = 0; y < Region::REGION_SIZE; y++) {
      for (uint8_t z = 0; z < Region::REGION_SIZE; z++) {
        const Chunk *chunk = region->getChunk(x, y, z);
        if (!chunk) {
          continue;
        }
        m_rhiView->rhiRender()->addChunkMesh(
            {{0, 0, 0}, {x, y, z}},
            ChunkMeshGenerator::generateChunkMesh(*chunk));
      }
    }
  }
}

void Engine::tick() {}
