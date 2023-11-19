#pragma once

#include <vulkan/vulkan_core.h>

bool CreateSemaphore(VkDevice device_, VkSemaphore* semaphore_);
bool CreateFence(VkDevice device_, VkFence* fence_);