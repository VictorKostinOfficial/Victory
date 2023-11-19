#include "VulkanSynchronization.h"

bool CreateSemaphore(VkDevice device_, VkSemaphore* semaphore_) {
    if (!semaphore_) {
        return false;
    }

    VkSemaphoreCreateInfo semaphoreCI{};
    semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    return vkCreateSemaphore(device_, &semaphoreCI, nullptr, semaphore_) == VK_SUCCESS;
}

bool CreateFence(VkDevice device_, VkFence* fence_) {
    if (!fence_) {
        return false;
    }

    VkFenceCreateInfo fenceCI{};
    fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    return vkCreateFence(device_, &fenceCI, nullptr, fence_) == VK_SUCCESS;
}
