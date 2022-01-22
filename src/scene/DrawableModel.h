#pragma once

#include <vector>
#include "../utils/vulkan.h"
#include "Mesh.h"
#include "../memory/Buffer.h"
#include "Material.h"

namespace mcvkp
{

    class DrawableModel
    {
    public:
        DrawableModel(std::shared_ptr<Material> material,
                      std::string modelPath);

        DrawableModel(std::shared_ptr<Material> material,
                      MeshType type);

        std::shared_ptr<Material> getMaterial();
        void drawCommand(VkCommandBuffer &commandBuffer, size_t currentFrame);

    private:
        std::shared_ptr<Material> m_material;
        mcvkp::Buffer m_vertexBuffer;
        mcvkp::Buffer m_indexBuffer;
        uint32_t m_numIndices;

        void initVertexBuffer(const Mesh &mesh);

        void initIndexBuffer(const Mesh &mesh);
    };
}
