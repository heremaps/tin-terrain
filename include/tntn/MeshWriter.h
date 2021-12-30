#pragma once

#include "tntn/geometrix.h"
#include "tntn/Mesh.h"

namespace tntn {

class MeshWriter
{
  public:
    virtual bool write_mesh_to_file(const char* filename, Mesh& mesh, const BBox3D& bbox) = 0;
    virtual std::string file_extension() = 0;
    virtual ~MeshWriter(){};
};

class ObjMeshWriter : public MeshWriter
{
    virtual bool write_mesh_to_file(const char* filename,
                                    Mesh& mesh,
                                    const BBox3D& bbox) override;

    virtual std::string file_extension() override;
    virtual ~ObjMeshWriter(){};
};


class QuantizedMeshWriter : public MeshWriter
{
  public:
    QuantizedMeshWriter(bool gzipped = false) : m_gzipped(gzipped) {}
    virtual bool write_mesh_to_file(const char* filename,
                                    Mesh& mesh,
                                    const BBox3D& bbox) override;
    virtual std::string file_extension() override;
    virtual ~QuantizedMeshWriter(){};
  private:
    bool m_gzipped = false;
};

} // namespace tntn
