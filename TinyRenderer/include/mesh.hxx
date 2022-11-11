#ifndef TINYRENDERER_MESH_HXX
#define TINYRENDERER_MESH_HXX

#include <Eigen/Dense>

#include <format>
#include <fstream>
#include <optional>
#include <vector>

class Mesh
{
private:
    using Vector3d = Eigen::Vector3d;
    using Vector3i = Eigen::Vector3i;

public:
    Mesh() = default;
    Mesh(const std::vector<Vector3d>& vertices, const std::vector<Vector3i>& faces)
        : vertices_{ vertices }
        , faces_{ faces }
    {}

    Mesh(const Mesh&) = default;
    Mesh& operator=(const Mesh&) = default;
    Mesh(Mesh&&) = default;
    Mesh& operator=(Mesh&&) = default;
    ~Mesh() = default;

    static std::optional<Mesh> load(const std::string& filename)
    {
        std::vector<Vector3d> vertices{};
        std::vector<Vector3i> faces{};
        std::ifstream in;

        in.open(filename, std::ifstream::in);
        if (in.fail()) return {};

        std::string line;
        while (!in.eof()) 
        {
            std::getline(in, line);
            std::istringstream iss{ line.c_str() };
            char trash;

            if (!line.compare(0, 2, "v "))
            {
                iss >> trash;
                Vector3d v;
                for (int i = 0; i < 3; i++) iss >> v[i];
                for (int i = 1; i < 3; i++) v[i] = 0.5 - v[i];
                vertices.push_back(v);
            }
            else if (!line.compare(0, 2, "f "))
            {
                uint8_t vec_idx = 0;
                Vector3i vec;
                int itrash, idx;
                iss >> trash;
                while (iss >> idx >> trash >> itrash >> trash >> itrash)
                {
                    idx--; // in wavefront obj all indices start at 1, not zero
                    vec[vec_idx] = idx;
                    vec_idx++;
                }
                faces.push_back(vec);
            }
        }
        // std::cerr << "# v# " << verts_.size() << " f# " << faces_.size() << std::endl;

        return  Mesh{ vertices, faces };
    }

    size_t get_num_faces() const
    {
        return faces_.size();
    }

    const Vector3d& get_vertex(size_t idx) const
    {
        return vertices_[idx];
    }

    const Vector3i& get_face(size_t idx) const
    {
        return faces_[idx];
    }

private:
    std::vector<Vector3d> vertices_;
    std::vector<Vector3i> faces_;
};

#endif // TINYRENDERER_MESH_HXX