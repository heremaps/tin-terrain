#pragma once

#include "tntn/Mesh.h"
#include "tntn/MeshWriter.h"

#include <memory>

namespace tntn {

class TileMaker
{
    std::unique_ptr<Mesh> m_mesh;
	bool m_normals_enabled;
	
  public:
    TileMaker(bool normals_enabled=false) : m_mesh(std::make_unique<Mesh>()),
											m_normals_enabled(normals_enabled) {}

	bool normals_enabled() const;
    void setMeshWriter(MeshWriter* w);
    bool loadObj(const char* filename);
    void loadMesh(std::unique_ptr<Mesh> mesh);
    // void dumpTile(int tx, int ty, int zoom, const char* filename);
    bool dumpTile(int tx, int ty, int zoom, const char* filename, MeshWriter& mw);
};

} //namespace tntn
