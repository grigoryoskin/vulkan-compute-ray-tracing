#pragma once

#include <vector>
#include "../utils/vulkan.h"
#include "Mesh.h"
#include "../memory/Buffer.h"
#include "ComputeModel.h"

namespace mcvkp
{
    ComputeModel::ComputeModel(std::shared_ptr<ComputeMaterial> material) : m_material(material)
    {
        m_material->init();
    }

    std::shared_ptr<ComputeMaterial> ComputeModel::getMaterial()
    {
        return m_material;
    }

    void ComputeModel::computeCommand(VkCommandBuffer &commandBuffer, size_t currentFrame, size_t x, size_t y, size_t z)
    {
        m_material->bind(commandBuffer, currentFrame);
        vkCmdDispatch(commandBuffer, x, y, z);
    }
}
