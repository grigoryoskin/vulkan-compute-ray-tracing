#pragma once

#include <vector>
#include "../utils/vulkan.h"
#include "Mesh.h"
#include "../memory/Buffer.h"
#include "Material.h"
#include "DrawableModel.h"

namespace mcvkp
{
    DrawableModel::DrawableModel(std::shared_ptr<Material> material,
                                 std::string modelPath) : m_material(material)
    {
        Mesh m(modelPath);

        initVertexBuffer(m);
        initIndexBuffer(m);
    }

    DrawableModel::DrawableModel(std::shared_ptr<Material> material,
                                 MeshType type) : m_material(material)
    {
        Mesh m(type);

        initVertexBuffer(m);
        initIndexBuffer(m);
    }

    std::shared_ptr<Material> DrawableModel::getMaterial()
    {
        return m_material;
    }

    void DrawableModel::drawCommand(VkCommandBuffer &commandBuffer, size_t currentFrame)
    {
        // mcvkp::Buffer<VkDrawIndexedIndirectCommand> indirectBuffer;
        // VkDrawIndexedIndirectCommand indirectCommand;
        // indirectCommand.indexCount = static_cast<uint32_t>(mesh.indices.size());
        // indirectCommand.instanceCount = 1;
        // indirectCommand.firstIndex = 0;
        // indirectCommand.vertexOffset = 0;
        // indirectCommand.firstInstance = 0;

        // indirectBuffer.create(&indirectCommand, sizeof(VkDrawIndirectCommand), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        m_material->bind(commandBuffer, currentFrame);
        VkBuffer vertexBuffers[] = {m_vertexBuffer.buffer};
        VkDeviceSize offsets[] = {0};

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        VkDeviceSize indirect_offset = 0;
        uint32_t draw_stride = sizeof(VkDrawIndirectCommand);

        // execute the draw command buffer on each section as defined by the array of draws
        // vkCmdDrawIndexedIndirect(commandBuffer, indirectBuffer.buffer, indirect_offset, 1, draw_stride);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_numIndices), 1, 0, 0, 0);
    }

    void DrawableModel::initVertexBuffer(const Mesh &mesh)
    {
        BufferUtils::create<Vertex>(&m_vertexBuffer, mesh.vertices.data(), mesh.vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    }

    void DrawableModel::initIndexBuffer(const Mesh &mesh)
    {
        m_numIndices = mesh.indices.size();
        BufferUtils::create<uint32_t>(&m_indexBuffer, mesh.indices.data(), mesh.indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    }
}