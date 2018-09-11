#include "tntn/MeshWriter.h"
#include "tntn/QuantizedMeshIO.h"
#include "tntn/MeshIO.h"

namespace tntn {

bool ObjMeshWriter::write_mesh_to_file(const char* filename, Mesh& mesh, const BBox3D& bbox)
{
    return write_mesh_as_obj(filename, mesh);
}

std::string ObjMeshWriter::file_extension()
{
    return "obj";
}

bool QuantizedMeshWriter::write_mesh_to_file(const char* filename, Mesh& mesh, const BBox3D& bbox)
{
    return write_mesh_as_qm(filename, mesh, bbox, true);
}

std::string QuantizedMeshWriter::file_extension()
{
    return "terrain";
}
} // namespace tntn
