#include "VulkanSynchronization.h"

vk::Semaphore CreateSemaphore(vk::Device device_) {
    vk::SemaphoreCreateInfo semaphoreCI{};
    return device_.createSemaphore(semaphoreCI);
}

vk::Fence CreateFence(vk::Device device_) {
    vk::FenceCreateInfo fenceCI{};
    fenceCI.setFlags(vk::FenceCreateFlagBits::eSignaled);
    return device_.createFence(fenceCI);
}
