#pragma once

#include <Tempest/VertexBuffer>
#include <Tempest/IndexBuffer>
#include <Tempest/Device>

#include "resources.h"

class StaticMesh {
  public:
    using Vertex=Resources::Vertex;
    StaticMesh(const ZenLoad::PackedMesh& data);
    StaticMesh(const ZenLoad::PackedSkeletalMesh& data);

    struct SubMesh {
      const Tempest::Texture2d*      texture=nullptr;
      Tempest::IndexBuffer<uint32_t> ibo;
      std::string                    texName;
      };

    Tempest::VertexBuffer<Vertex>  vbo;
    std::vector<SubMesh>           sub;
  };
