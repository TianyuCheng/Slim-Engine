#include <slim/slim.hpp>

using namespace slim;

struct Vertex {
    glm::vec3 position;
};

inline Mesh* CreateMesh(Context *context) {
    Mesh* mesh = new Mesh();
    context->Execute([=](CommandBuffer *commandBuffer) {
        // prepare vertex data
        std::vector<glm::vec3> positions = {
            // position
            glm::vec3(-0.5f, -0.5f,  0.5f),
            glm::vec3( 0.5f, -0.5f,  0.5f),
            glm::vec3( 0.5f,  0.5f,  0.5f),
            glm::vec3(-0.5f,  0.5f,  0.5f),

            // position
            glm::vec3(-0.5f, -0.5f, -0.5f),
            glm::vec3( 0.5f, -0.5f, -0.5f),
            glm::vec3( 0.5f,  0.5f, -0.5f),
            glm::vec3(-0.5f,  0.5f, -0.5f),
        };

        // prepare index data
        std::vector<uint32_t> indices = {
            0, 1, 2, 2, 3, 0,
            4, 7, 6, 6, 5, 4,
            3, 7, 4, 4, 0, 3,
            1, 5, 6, 6, 2, 1,
            6, 7, 3, 3, 2, 6,
            4, 5, 1, 1, 0, 4
        };

        mesh->SetVertexAttrib(commandBuffer, positions, 0);
        mesh->SetIndexAttrib(commandBuffer, indices);
    });
    return mesh;
}
