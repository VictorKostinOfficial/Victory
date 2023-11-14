#pragma once

#include <vulkan/vulkan.hpp>

vk::Semaphore CreateSemaphore(vk::Device device_);
vk::Fence CreateFence(vk::Device device_);
