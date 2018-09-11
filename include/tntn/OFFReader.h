#pragma once

#include "tntn/geometrix.h"
#include "tntn/Mesh.h"
#include "tntn/File.h"

#include <vector>
#include <string>
#include <memory>

namespace tntn {

class OFFReader
{
  public:
    OFFReader() {}

    void clear()
    {
        m_vertices.clear();
        m_facades.clear();
        m_num_vertices = 0;
        m_num_faces = 0;
        m_ne = 0;
    }

    bool readFile(FileLike& f) { return parse(f); }

    Vertex* getVertices() { return m_vertices.data(); }

    Face* getFacades() { return m_facades.data(); }

    int getNumVertices() const { return m_num_vertices; }

    int getNumTriangles() const { return m_num_faces; }

    std::unique_ptr<Mesh> convertToMesh();

    void findXYBounds(double& xmin, double& ymin, double& xmax, double& ymax);

  private:
    void readDimensions(std::string line, std::vector<std::string>& tokens);
    bool readVertex(std::string line, Vertex& v, std::vector<std::string>& tokens);
    bool readFacade(std::string line, Face& f, std::vector<std::string>& tokens);
    bool parse(FileLike& f);

  private:
    int m_num_vertices = 0;
    int m_num_faces = 0;
    int m_ne = 0;

    std::vector<Vertex> m_vertices;
    std::vector<Face> m_facades;
};

} //namespace tntn
