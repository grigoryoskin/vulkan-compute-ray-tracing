#pragma once

#include <vector>
#include "../utils/vulkan.h"
#include "Mesh.h"
#include "../memory/Buffer.h"
#include "ComputeMaterial.h"

namespace mcvkp
{

    class ComputeModel
    {
    public:
        ComputeModel(std::shared_ptr<ComputeMaterial> material);

        std::shared_ptr<ComputeMaterial> getMaterial();
        void computeCommand(VkCommandBuffer &commandBuffer, size_t currentFrame, size_t x, size_t y, size_t z);

    private:
        std::shared_ptr<ComputeMaterial> m_material;
    };
}
