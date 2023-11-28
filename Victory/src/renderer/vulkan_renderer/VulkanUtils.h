#pragma once

class VulkanContext;

namespace VulkanUtils {

bool CreateSemaphore(VkDevice device_, VkSemaphore* semaphore_);
bool CreateFence(VkDevice device_, VkFence* fence_);

} // namespace VulkanUtils