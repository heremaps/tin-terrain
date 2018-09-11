#pragma once

#include <cstdint>
#include <memory>

#include "tntn/geometrix.h"
#include "tntn/Mesh.h"
#include "tntn/File.h"

namespace tntn {

namespace detail {
//exposed for testing
uint16_t zig_zag_encode(int16_t i);
int16_t zig_zag_decode(uint16_t i);

} //namespace detail

// unsigned int zig_zag_encode(int i);
bool write_mesh_as_qm(const char* filename, const Mesh& m);
bool write_mesh_as_qm(const char* filename,
                      const Mesh& m,
                      const BBox3D& bbox,
                      bool mesh_is_rescaled = false);

bool write_mesh_as_qm(const std::shared_ptr<FileLike>& f, const Mesh& m);
bool write_mesh_as_qm(const std::shared_ptr<FileLike>& f,
                      const Mesh& m,
                      const BBox3D& bbox,
                      bool mesh_is_rescaled = false);

std::unique_ptr<Mesh> load_mesh_from_qm(const char* filename);
std::unique_ptr<Mesh> load_mesh_from_qm(const std::shared_ptr<FileLike>& f);

} // namespace tntn
