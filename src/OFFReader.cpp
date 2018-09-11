#include "tntn/OFFReader.h"
#include "tntn/logging.h"
#include "tntn/util.h"

#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string>
#include <limits>

namespace tntn {

std::unique_ptr<Mesh> OFFReader::convertToMesh()
{
    auto mesh = std::make_unique<Mesh>();
    mesh->from_decomposed(std::move(m_vertices), std::move(m_facades));
    clear();
    return mesh;
}

void OFFReader::findXYBounds(double& xmin, double& ymin, double& xmax, double& ymax)
{
    xmin = std::numeric_limits<double>::max();
    ymin = std::numeric_limits<double>::max();

    xmax = std::numeric_limits<double>::min();
    ymax = std::numeric_limits<double>::min();

    for(int i = 0; i < m_num_vertices; i++)
    {
        Vertex& v = m_vertices[i];
        if(v.x < xmin) xmin = v.x;
        if(v.x > xmax) xmax = v.x;

        if(v.y < ymin) ymin = v.y;
        if(v.y > ymax) ymax = v.y;
    }
}

void OFFReader::readDimensions(std::string line, std::vector<std::string>& tokens)
{
    tokenize(line, tokens);
    if(tokens.size() == 3)
    {
        m_num_vertices = std::stoi(tokens[0]);
        m_num_faces = std::stoi(tokens[1]);
        m_ne = std::stoi(tokens[2]);
    }
}

bool OFFReader::readVertex(std::string line, Vertex& v, std::vector<std::string>& tokens)
{
    tokenize(line, tokens);

    if(tokens.size() >= 3 && tokens[0][0] != '#')
    {
        v.x = std::stod(tokens[0]);
        v.y = std::stod(tokens[1]);
        v.z = std::stod(tokens[2]);

        return true;
    }
    else
    {
        return false;
    }
}

bool OFFReader::readFacade(std::string line, Face& f, std::vector<std::string>& tokens)
{
    tokenize(line, tokens);

    if(tokens.size() >= 3 && tokens[0][0] != '#')
    {
        if(tokens.size() > 3)
        {
            int n = std::stoi(tokens[0]);

            if(n == 3 && tokens.size() >= 4)
            {
                f[0] = std::stoi(tokens[1]);
                f[1] = std::stoi(tokens[2]);
                f[2] = std::stoi(tokens[3]);
            }
            else
            {
                TNTN_LOG_WARN(
                    "incorrect facade format, vertices per facade: {} (only supporing n=3) ", n);
            }
        }
        return true;
    }
    else
    {
        return false;
    }
}

// Parser
bool OFFReader::parse(FileLike& in)
{
    std::vector<std::string> tokens;
    if(!in.is_good())
    {
        TNTN_LOG_ERROR("infile is not open/in a good state");
        return false;
    }
    // Container holding last line read
    std::string readLine;
    // Containers for delimiter positions
    //int delimiterPos_1, delimiterPos_2, delimiterPos_3, delimiterPos_4;

    FileLike::position_type from_offset = 0;
    // Check if file is in OFF format
    from_offset = getline(from_offset, in, readLine);
    if(readLine != "OFF")
    {
        TNTN_LOG_ERROR("The file to read is not in OFF format.");
        return false;
    }
    else
    {
        TNTN_LOG_DEBUG("file is OFF format");
    }

    while(true)
    {
        from_offset = getline(from_offset, in, readLine);

        TNTN_LOG_TRACE("line: {}", readLine);

        tokenize(readLine, tokens);

        for(auto s : tokens)
        {
            TNTN_LOG_TRACE("s: {}", s);
        }

        if(tokens.size() > 0)
        {
            if(tokens[0][0] != '#' && tokens.size() == 3)
            {
                TNTN_LOG_TRACE("found dimension line");

                //read dimensions
                readDimensions(readLine, tokens);

                TNTN_LOG_DEBUG("vertices: {}", m_num_vertices);
                TNTN_LOG_DEBUG("facades: {}", m_num_faces);

                break;
            }
        }
    }

    if(m_num_vertices > 0 && m_num_faces > 0)
    {
        // Read the vertices
        m_vertices.resize(m_num_vertices);

        while(true)
        {
            from_offset = getline(from_offset, in, readLine);

            tokenize(readLine, tokens);

            if(tokens.size() > 0)
            {
                if(tokens[0][0] != '#' && tokens.size() == 3)
                {
                    TNTN_LOG_DEBUG("found first vertex line");
                    break;
                }
            }
        }

        TNTN_LOG_INFO("reading vertices");

        int vread = 0;

        for(int i = 0; i < m_num_vertices; i++)
        {
            Vertex& v = m_vertices[i];
            if(readVertex(readLine, v, tokens) != true)
            {
                TNTN_LOG_WARN("could not read vertex {} line: {}", i, readLine);
            }
            else
            {
                vread++;
            }
            from_offset = getline(from_offset, in, readLine);
            if(i % (1 + m_num_vertices / 10) == 0)
            {
                float p = i / (float)m_num_vertices;
                TNTN_LOG_INFO("{} %", p * 100.0);
            }
        }

        // Read the facades
        m_facades.resize(m_num_faces);

        TNTN_LOG_INFO("reading faces");

        int fread = 0;

        for(int i = 0; i < m_num_faces; i++)
        {

            Face& f = m_facades[i];
            if(readFacade(readLine, f, tokens) != true)
            {
                TNTN_LOG_WARN("could not read facade {}", i);
            }
            else
            {
                fread++;
            }

            from_offset = getline(from_offset, in, readLine);

            if((i % (1 + m_num_faces / 10)) == 0)
            {
                float p = i / (float)m_num_faces;
                TNTN_LOG_INFO("{} %", p * 100.0);
            }
        }

        if(vread != m_num_vertices)
        {
            TNTN_LOG_ERROR(
                "not all vertices read. m_num_vertices = {} vread = {}", m_num_vertices, vread);
            return false;
        }

        if(fread != m_num_faces)
        {
            TNTN_LOG_ERROR("not all facades read. nf = {} fread = {}", m_num_faces, fread);
            return false;
        }

        TNTN_LOG_DEBUG("reading OFF file completed");
    }

    return true;
}

} //namespace tntn
