#include <stdio.h>

#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

static char const* AppName = "Victory Application";
static char const* EngineName = "Victory Engine";

void CollectLayers(std::vector<const char*>& layers_) 
{
#ifdef _DEBUG
    layers_.emplace_back("VK_LAYER_KHRONOS_validation");
#endif
}

void CollectExtensions(std::vector<const char*>& extensions_) 
{
    extensions_.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef _DEBUG
    extensions_.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    extensions_.reserve(glfwExtensionCount + 1);
    for (uint32_t i{0}; i < glfwExtensionCount; ++i) {
        extensions_.emplace_back(glfwExtensions[i]);
    }
}

vk::Instance CreateInstance() 
{
    vk::ApplicationInfo applicationInfo{
        AppName, 
        VK_MAKE_VERSION(1,0,0), 
        EngineName, 
        VK_MAKE_VERSION(1,0,0), 
        VK_API_VERSION_1_0
    };

    std::vector<const char*> layers;
    CollectLayers(layers);

    std::vector<const char*> extensions;
    CollectExtensions(extensions);

    vk::InstanceCreateInfo instanceCreateInfo{{}, &applicationInfo, layers, extensions};

    return vk::createInstance(instanceCreateInfo);
}

#ifdef _DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback( 
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    printf("Validation layer: %s", pCallbackData->pMessage);
    return VK_FALSE;
}

vk::DebugUtilsMessengerEXT RegisterDebugUtilsMessenger(const vk::Instance& instance_) 
{
    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    debugCreateInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    debugCreateInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    debugCreateInfo.pfnUserCallback = DebugReportCallback;
    
    vk::DispatchLoaderDynamic dynamicLoader(instance_, vkGetInstanceProcAddr);
    return instance_.createDebugUtilsMessengerEXT(debugCreateInfo, nullptr, dynamicLoader);
}
#endif

bool supportsPresentation(VkPhysicalDevice physicalDevice, uint32_t familyIndex)
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
	return vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, familyIndex);
#else
	return true;
#endif
}

vk::PhysicalDevice& PickPhysicalDevice(std::vector<vk::PhysicalDevice>& physicalDevices_, uint32_t& graphicsFamilyIndex) 
{
    VkPhysicalDevice discrete = 0;
    VkPhysicalDevice fallback = 0;

    auto&& found = std::find_if(physicalDevices_.begin(), physicalDevices_.end(), 
        [&graphicsFamilyIndex](const vk::PhysicalDevice& physicalDevice) 
        {
            const vk::PhysicalDeviceProperties deviceProperties = physicalDevice.getProperties();
            printf("GPU: %s\n", static_cast<const char*>(deviceProperties.deviceName));
            if (deviceProperties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu) 
            {
                return false;
            }

            //const vk::PhysicalDeviceFeatures deviceFeatures = physicalDevice.getFeatures();
            //const vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();
            const std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
            for (size_t i{0}, n = queueFamilyProperties.size(); i < n; ++i) 
            {
                if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) && 
                    (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eCompute)) 
                {
                    graphicsFamilyIndex = static_cast<uint32_t>(i);
                    break;
                }
                return false;
            }

            if (!supportsPresentation(physicalDevice, graphicsFamilyIndex)) 
            {
                return false;
            }

            return true;
        }
    );

    if (found == physicalDevices_.end()) 
    {
        throw std::runtime_error("Suitable GPU was no found");
    }

    return *found;
}

vk::Device CreateDevice(vk::Instance& instance_, vk::PhysicalDevice& physicalDevice_, uint32_t graphicalFamilyIndex_)
{
    float queuePriorities[] = { 1.0f };

    vk::DeviceQueueCreateInfo queueCreateInfo{
        {},
        graphicalFamilyIndex_,
        1,
        queuePriorities,
    };

    std::vector<const char*> extensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    vk::DeviceCreateInfo deviceCreateInfo{
        {},
        queueCreateInfo,
        {},
        extensions
    };

    return physicalDevice_.createDevice(deviceCreateInfo);
}

vk::SurfaceKHR CreateSurface(vk::Instance& instance_, GLFWwindow* window_, vk::PhysicalDevice& physicalDevice_, uint32_t graphicsFamilyIndex_)
{
    VkSurfaceKHR surface_;
    if (glfwCreateWindowSurface(static_cast<VkInstance>(instance_), window_, nullptr, &surface_) != VK_SUCCESS) {
        glfwDestroyWindow(window_);
        glfwTerminate();
        throw std::runtime_error("glfw surface was not created");
    }

    vk::SurfaceKHR surface = vk::SurfaceKHR(surface_);
    if (!physicalDevice_.getSurfaceSupportKHR(graphicsFamilyIndex_, surface)) {
        throw std::runtime_error("Present is not supported");
    }

    return surface;
}

int main() 
{

    if (!glfwInit()) 
    {
        throw std::runtime_error("GLFW was not inited");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);  // Tell GLFW not to create an OpenGL context
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);   // Window should not be resizable (optional)

    GLFWwindow* window = glfwCreateWindow(1024, 780, AppName, 0, 0);
    if (!window)
    {
        glfwTerminate();
        throw std::runtime_error("GLFW window was not created");
    }

    vk::Instance instance = CreateInstance();

#ifdef _DEBUG
    vk::DebugUtilsMessengerEXT debugUtilsMessenger = RegisterDebugUtilsMessenger(instance);
#endif

    uint32_t graphicsFamilyIndex{0};
    std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
    vk::PhysicalDevice physicalDevice = PickPhysicalDevice(physicalDevices, graphicsFamilyIndex);

    vk::Device device = CreateDevice(instance, physicalDevice, graphicsFamilyIndex);

    vk::SurfaceKHR surface = CreateSurface(instance, window, physicalDevice, graphicsFamilyIndex);


    int windowWidth{0};
    int windowHeight{0};
    glfwGetWindowSize(window, &windowWidth, &windowHeight);



    instance.destroySurfaceKHR(surface);
    device.destroy();

#ifdef _DEBUG
    vk::DispatchLoaderDynamic dynamicLoader(instance, vkGetInstanceProcAddr);
    instance.destroyDebugUtilsMessengerEXT(debugUtilsMessenger, nullptr, dynamicLoader);
#endif

    instance.destroy();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}