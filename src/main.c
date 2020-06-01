// TODO: Move into CMAKE
#define QQ_DEBUG

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

// Standard library stuff
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <qq.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#ifdef QQ_DEBUG
const b32 debugModeEnabled = QQ_TRUE;
#else
const b32 debugModeEnabled = QQ_FALSE;
#endif

#ifndef NOMINMAX

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#endif

// GLOBALS
// TODO: Put into structure

// GLFW window
GLFWwindow *window;

// Vulkan info
VkLayerProperties *vulkanLayers;
VkInstance vulkanInstance;
u32 vulkanPhysicalDeviceRating = 0;
VkPhysicalDevice vulkanPhysicalDevice = VK_NULL_HANDLE;
VkDevice vulkanDevice;

// Vulkan swap chain related stuff
VkSwapchainKHR vulkanSwapChain;
u32 vulkanSwapChainImageCount = 0;
VkFormat vulkanSwapChainImageFormat;
VkExtent2D vulkanSwapChainExtent;
VkImage* vulkanSwapChainImages;
VkQueue vulkanGraphicsQueue;
VkQueue vulkanPresentQueue;

VkImageView* vulkanSwapChainImageViews;

// Rendering surface
VkSurfaceKHR vulkanSurface;

// Rendering pass and pipeline layout
VkRenderPass vulkanRenderPass;
VkPipelineLayout vulkanPipelineLayout;

// Rendering pipeline
VkPipeline vulkanGraphicsPipeline;

// Framebuffers
VkFramebuffer* vulkanSwapChainFramebuffers;

// Command pool
VkCommandPool vulkanCommandPool;

// Command buffers
VkCommandBuffer* vulkanCommandBuffers;


typedef struct {
    u8* data;
    i64 size;
} VulkanShaderCode;

// Vulkan swap chain properties
struct VulkanSwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    
    u32 formatCount; 
    VkSurfaceFormatKHR* formats;
    
    u32 presentModeCount;
    VkPresentModeKHR* presentModes;
};

// Vulkan queue families
struct VulkanQueueFamilyIndices {
    // Graphics
    b32 isGraphicsSet;
    u32 graphics;

    // Presentation
    b32 isPresentSet;
    u32 present;
};

// Required validation layers
const char *requiredVulkanLayers[1] = { "VK_LAYER_KHRONOS_validation" };

// Reuired device extensions
u32 requiredVulkanDeviceExtensionCount = 1;
const char *reqVulkanDeviceExtensions[1] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// ------ END GLOBALS

// Fetches best suface format
VkSurfaceFormatKHR chooseSwapSurfaceFormat(VkSurfaceFormatKHR* formats, u32 count) {
    for (u32 i = 0; i < count; i++) {
        if (
            formats[i].format == VK_FORMAT_B8G8R8A8_SRGB
            && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        ) {
            // TODO: Segfault alert
            return formats[i];
        }
    }
    // TODO: Check if count != 0
    return formats[0];
}

// Determines presentation mode
VkPresentModeKHR chooseSwapPresentMode(VkPresentModeKHR* modes, u32 count) {
    for (u32 i = 0; i < count; i++) {
        if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            // Preferred display mode
            return VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }

    // This mode should be always available
    return VK_PRESENT_MODE_FIFO_KHR;
}

// Determines resolution of swap chain images
VkExtent2D chooseSwapExtent(VkSurfaceCapabilitiesKHR capabilities) {
    if (capabilities.currentExtent.width != U32_MAX) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = {WINDOW_WIDTH, WINDOW_HEIGHT};
        actualExtent.width = max(
            capabilities.minImageExtent.width,
            min(
                capabilities.maxImageExtent.width,
                actualExtent.width
            )
        );
        actualExtent.height = max(
            capabilities.minImageExtent.height,
            min(
                capabilities.maxImageExtent.height,
                actualExtent.height
            )
        );
        return actualExtent;
    }
}

struct VulkanSwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
    struct VulkanSwapChainSupportDetails details;
    details.formatCount = 0;
    details.presentModeCount = 0;

    // Fetch capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, vulkanSurface, &details.capabilities);

    // Fetch format count
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, vulkanSurface, &details.formatCount, NULL);
    if (details.formatCount != 0) {
        details.formats = (VkSurfaceFormatKHR*)malloc(
            details.formatCount * sizeof(VkSurfaceFormatKHR)
        );
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            device,
            vulkanSurface,
            &details.formatCount,
            details.formats
        );
    }

    // Fetch present modes
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device,
        vulkanSurface,
        &details.presentModeCount,
        NULL
    );
    if (details.presentModeCount != 0) {
        details.presentModes = (VkPresentModeKHR*)malloc(
            details.presentModeCount * sizeof(VkPresentModeKHR)
        );
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            device,
            vulkanSurface,
            &details.presentModeCount,
            details.presentModes
        );
    }

    // TODO: MEMORY LEAK WARNING!!!!
    // formats and presentModes should be cleared after allocation

    return details;
}

struct VulkanQueueFamilyIndices findVulkanQueueFamilies(VkPhysicalDevice device) {
    printf("Checking for required queue families\n");

    struct VulkanQueueFamilyIndices indices = {
        .isGraphicsSet = QQ_FALSE,
        .isPresentSet = QQ_FALSE
    };
    
    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

    VkQueueFamilyProperties* queueFamilies = (VkQueueFamilyProperties *)malloc(
        queueFamilyCount * sizeof(VkQueueFamilyProperties)
    );

    for (u32 i = 0; i < queueFamilyCount; i++) {
        if (
            indices.isGraphicsSet == QQ_FALSE
            && queueFamilies[i].queueFlags
            && VK_QUEUE_GRAPHICS_BIT
        ) {
            // Graphics queue is supported
            indices.isGraphicsSet = QQ_TRUE;
            indices.graphics = i;
            continue;
        }

        VkBool32 presentSupport = QQ_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vulkanSurface, &presentSupport);
        if (indices.isPresentSet == QQ_FALSE && presentSupport != QQ_FALSE) {
            indices.present = i;
            indices.isPresentSet = QQ_TRUE;
            continue;
        }

        if (
            indices.isGraphicsSet == QQ_TRUE
            && indices.isPresentSet == QQ_TRUE
        ) {
            break;
        }
    }


    free(queueFamilies);

    return indices;
}

void createSwapChain() {
    printf("Creating swapchain\n");

    struct VulkanSwapChainSupportDetails swapChainSupport = querySwapChainSupport(
        vulkanPhysicalDevice
    );

    // Determine properties
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(
        swapChainSupport.formats,
        swapChainSupport.formatCount
    );
    VkPresentModeKHR presentMode = chooseSwapPresentMode(
        swapChainSupport.presentModes,
        swapChainSupport.presentModeCount
    );
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    // Request minimum allowed images in swap chain + 1
    u32 imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (
        swapChainSupport.capabilities.maxImageCount > 0
        && imageCount > swapChainSupport.capabilities.maxImageCount
    ) {
        // Cap to max, if nessesary
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    // Actually create swap chain
    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vulkanSurface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,

        // Amount of image layers each image consist of
        // Always one, unless developing stereoscopic rendering
        .imageArrayLayers = 1,

        // Define operation types on images
        // In this case, render directly to them
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
    };
    
    
    // Determine how to handle swap chain images
    struct VulkanQueueFamilyIndices indices = findVulkanQueueFamilies(vulkanPhysicalDevice);
    u32 queueFamilyIndices[2] = {
        indices.graphics,
        indices.present
    };
    if (indices.graphics != indices.present) {
        // Multiple queues present, allow concurrent processing
        // (can be used in multiple queues, without ownership transfer)
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;

        // Declare queues, which will share resources
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        // Only 1 queue present, make working with images exclusive
        // (must be transferred before usage in another queue)
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = NULL;
    }

    // Specify additional image ransformation
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

    // Declare alpha channel, which should be used for blending with other windows
    // Ignore alpha in this case
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    // Specify present mode
    // Don't case about pixels obscured by another window
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    // Specify previous swap chain (in case of recreation)
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    
    // Create swapchain
    if (
        vkCreateSwapchainKHR(vulkanDevice, &createInfo, NULL, &vulkanSwapChain) != VK_SUCCESS
    ) {
        printf("[ERR] Failed to create vulkan swapchain");
    }

    // Get swap chain images
    vkGetSwapchainImagesKHR(
        vulkanDevice,
        vulkanSwapChain,
        &vulkanSwapChainImageCount,
        NULL
    );

    // Allocate memory to store images
    vulkanSwapChainImages = (VkImage*)malloc(
        vulkanSwapChainImageCount * sizeof(VkImage)
    );

    // Store images
    vkGetSwapchainImagesKHR(
        vulkanDevice,
        vulkanSwapChain,
        &vulkanSwapChainImageCount,
        vulkanSwapChainImages
    );

    // Save format and extent
    vulkanSwapChainImageFormat = surfaceFormat.format;
    vulkanSwapChainExtent = extent;

    // Free resources
    if (swapChainSupport.formatCount != 0) {
        free(swapChainSupport.formats);
    }

    if (swapChainSupport.presentModeCount != 0) {
        free(swapChainSupport.presentModes);
    }
}


void createVulkanLogicalDevice() {
    printf("Creating vulkan logical device\n");
    struct VulkanQueueFamilyIndices indices = findVulkanQueueFamilies(vulkanPhysicalDevice);

    // Create list of queue create infos
    u32 queueCount = 2;
    u32 queues[2] = { indices.graphics, indices.present };
    VkDeviceQueueCreateInfo* queueCreateInfos = (VkDeviceQueueCreateInfo*)malloc(
        queueCount * sizeof(VkDeviceQueueCreateInfo)
    );

    // Priority for all queues
    f32 queuePriority = 1.0f;
    for (u32 i = 0; i < queueCount; i++) {
        VkDeviceQueueCreateInfo queueCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queues[i],
            .queueCount = 1,
            .pQueuePriorities = &queuePriority
        };

        queueCreateInfos[i] = queueCreateInfo;
    }

    // Declare required device features (already checked for availability)
    // Empty for now
    VkPhysicalDeviceFeatures deviceFeatures = {};

    // Create logical device
    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = queueCreateInfos,
        .queueCreateInfoCount = queueCount,
        .pEnabledFeatures = &deviceFeatures,
        // TODO: Research about virtual device layers/extensions (is it deprecated?)
        .enabledExtensionCount = requiredVulkanDeviceExtensionCount,
        .ppEnabledExtensionNames = reqVulkanDeviceExtensions
    };

    if (
        vkCreateDevice(vulkanPhysicalDevice, &createInfo, NULL, &vulkanDevice) != VK_SUCCESS
    ) {
        printf("[ERR] Failed to create vulkan virtual device");
    }

    // Get handle for graphics queue
    vkGetDeviceQueue(vulkanDevice, indices.graphics, 0, &vulkanGraphicsQueue);
    vkGetDeviceQueue(vulkanDevice, indices.present, 0, &vulkanPresentQueue);

    // Free queue infos
    free(queueCreateInfos);
}

b32 checkDeviceExtensionSupport(VkPhysicalDevice device) {
    u32 extensionCount;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);

    VkExtensionProperties* availableExtensions = (VkExtensionProperties*)malloc(
        extensionCount * sizeof(VkExtensionProperties)
    );
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, availableExtensions);

    b32 allExtensionsFound = QQ_TRUE;
    for (u32 i = 0; i < requiredVulkanDeviceExtensionCount; i++) {
        b32 isFound = QQ_FALSE;
        for (u32 j = 0; j < extensionCount; j++) {
            if (strcmp(reqVulkanDeviceExtensions[i], availableExtensions[j].extensionName)) {
                isFound = QQ_TRUE;
                break;
            }
        }

        if (isFound == QQ_FALSE) {
            printf("Extension %s wasn't found\n", reqVulkanDeviceExtensions[i]);
            allExtensionsFound = QQ_FALSE;
        }
    }

    if (allExtensionsFound == QQ_TRUE) {
        printf(" - All required extensions are present\n");
    }

    free(availableExtensions);

    return (allExtensionsFound == QQ_TRUE);

}

u32 rateVulkanPhysicalDevice(VkPhysicalDevice device) {
    // 0 - Not usable device
    u32 score = 0;

    // Get features and properties
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // All devices should support geometry shader
    if (!deviceFeatures.geometryShader) {
        return 0;
    }

    // Check queue families
    struct VulkanQueueFamilyIndices indices = findVulkanQueueFamilies(device);
    if (indices.isGraphicsSet != QQ_TRUE) {
        // Device must support graphics queue
        return 0;
    }
    if (indices.isPresentSet != QQ_TRUE) {
        // Device must support presentation queue
        return 0;
    }

    b32 extensionsSupported = checkDeviceExtensionSupport(device);
    if (extensionsSupported == QQ_FALSE) {
        // Device should support all required extensions
        return 0;
    }

    struct VulkanSwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
    b32 swapChainWorkingCorrectly = (
        swapChainSupport.formatCount != 0
        && swapChainSupport.presentModeCount != 0
    );
    if (swapChainSupport.formatCount != 0) {
        free(swapChainSupport.formats);
    }
    if (swapChainSupport.presentModeCount != 0) {
        free(swapChainSupport.presentModes);
    }
    if (swapChainWorkingCorrectly == QQ_FALSE) {
        return 0;
    }

    // Prefer discrete GPUs
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    // Bigger supported size of textures is better
    score += deviceProperties.limits.maxImageDimension2D;

    printf("Device %s | score: %d\n", deviceProperties.deviceName, score);

    return score;
}

void pickVulkanPhysicalDevice() {
    printf("Picking vulkan physical device\n");

    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, NULL);

    if (deviceCount == 0) {
        printf(" [ERR] Failed to find GPUs with Vulkan support\n");
    }

    VkPhysicalDevice* devices = (VkPhysicalDevice *)malloc(
        deviceCount * sizeof(VkPhysicalDevice)
    );
    vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, devices);

    for (u32 i = 0; i < deviceCount; i++) {
        u32 score = rateVulkanPhysicalDevice(devices[i]);
        if (score == 0) {
            continue;
        }

        if (score > vulkanPhysicalDeviceRating) {
            vulkanPhysicalDevice = devices[i];
            vulkanPhysicalDeviceRating = score;
        }
   }

    if (vulkanPhysicalDevice == VK_NULL_HANDLE) {
        printf(" [ERR] All devices are failed to meet requirements\n");
    }

    // Free previously allocated memory
    free(devices);
}

b32 checkVulkanValidationLayerSupport() {
    printf("Checking if required Vulkan layers are present\n");

    u32 availableLayerCount;
    vkEnumerateInstanceLayerProperties(&availableLayerCount, NULL);

    // Allocate memory to store all available layers
    vulkanLayers = (VkLayerProperties *) malloc(availableLayerCount * sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&availableLayerCount, vulkanLayers);

    // Iterate over available layers to check if required layers are supported
    b32 allLayersIsFound = QQ_TRUE;
    for (u32 i = 0; i < 1; i++) {
        b32 layerIsFound = QQ_FALSE;

        for (u32 j = 0; j < availableLayerCount; j++) {
            if (strcmp(requiredVulkanLayers[i], vulkanLayers[j].layerName) == 0) {
                layerIsFound = QQ_TRUE;
                break;
            }
        }

        if (layerIsFound == QQ_FALSE) {
            printf("Layer %s was required, but wasn't found\n", requiredVulkanLayers[i]);
            allLayersIsFound = QQ_FALSE;
        }
    }

    if (allLayersIsFound == QQ_TRUE) {
        printf(" - All required layers are present\n");
    }

    return (allLayersIsFound == QQ_TRUE);
}

void displaySupportedVulkanExtensions() {
    printf("Loading available Vulkan extensions\n");

    u32 extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);

    printf("%d Vulkan extensions are supported:\n", extensionCount);

    // Allocate memory for list of available extensions
    VkExtensionProperties *availableExtensions = (VkExtensionProperties *) malloc(
        extensionCount * sizeof(VkExtensionProperties)
    );
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, availableExtensions);

    // List available extensions
    for (u32 i = 0; i < extensionCount; i++) {
        printf(
            " - %s (spec: %d)\n",
            availableExtensions[i].extensionName,
            availableExtensions[i].specVersion
        );
    }

    // Free memory
    free(availableExtensions);
}

void createVulkanInstance() {
    printf("Creating Vulkan instance\n");

    // Display supported extensions
    // TODO: Move into into debug module
    displaySupportedVulkanExtensions();

    if (debugModeEnabled == QQ_TRUE) {
        checkVulkanValidationLayerSupport();
    }

    // Structure describing application info
    // This structure is optional, however it is recommended to implement it
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "qq",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0
    };

    // Structure describing requirements from Vulkan instance
    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo
    };

    // Describe extensions required
    // For now, just fetch all extensions, required by GLFW
    u32 glfwExtensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    
    // TODO: These lines are causing segfault
    //       But it is correct declaration (Probably GLFW messes it up)
    // createInfo.enabledExtensionCount = requiredVulkanDeviceExtensionCount;
    // createInfo.ppEnabledExtensionNames = reqVulkanDeviceExtensions;

    // Describe required validation layers
    if (debugModeEnabled) {
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = requiredVulkanLayers;
    } else {
        createInfo.enabledLayerCount = 0;
    }

    // Create Vulkan instance
    VkResult instanceCreationSuccess = vkCreateInstance(&createInfo, NULL, &vulkanInstance);
    if (instanceCreationSuccess != VK_SUCCESS) {
        printf("Failed to initialize Vulkan instance\n");
    }
}

void createWindowSurface() {
    printf("Creating KHR surface for window\n");
    if (glfwCreateWindowSurface(vulkanInstance, window, NULL, &vulkanSurface)) {
        printf("[ERR] Failed to create window surface\n");
    }
}

void createVulkanImageViews() {
    printf("Creating swap chain image views");

    vulkanSwapChainImageViews = (VkImageView*)malloc(
        vulkanSwapChainImageCount * sizeof(VkImageView)
    );

    // Create image views
    for (u32 i = 0; i < vulkanSwapChainImageCount; i++) {
        VkImageViewCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = vulkanSwapChainImages[i],

            // Define how image should be interpreted and it's format
            // in this case: 2d image with default image format
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = vulkanSwapChainImageFormat,

            // Set default values for all color channels
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            
            // Define image purpose and what parts should be accessed
            // In this case: color target without mipmaps and multiple layers
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        // Create image
        if (
            vkCreateImageView(
                vulkanDevice,
                &createInfo,
                NULL,
                &vulkanSwapChainImageViews[i]
            ) != VK_SUCCESS
        ) {
            printf("[ERR] Failed to create image view of index %d", i);
        }
    }
}

void unloadShaderCode(VulkanShaderCode code) {
    // Free shader memory
    free(code.data);
}

VulkanShaderCode loadShaderCodeByPath(const char* path) {
    printf("Loading shader code from file: %s\n", path);

    FILE* file = fopen(path, "rb");

    // Open file
    file = fopen(path, "rb");
    if (file == NULL) {
        printf("Error opening: %s\n", path);
    }
    
    VulkanShaderCode code = {
        .size = 0,
        .data = NULL
    };

    // Get file size
    fseek(file, 0, SEEK_END);
    code.size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory to contain the whole file
    code.data = malloc(code.size + 1);
    fread(code.data, 1, code.size, file);

    fclose(file);

    return code;
}

VkShaderModule createVulkanShaderModule(VulkanShaderCode code) {
    printf("Creating shader module\n");
    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size,
        .pCode = (u32*)code.data
    };

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(vulkanDevice, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
        printf("Failed to create shader module");
    }

    return shaderModule;
}

void createGraphicsPipeline() {
    printf("Creating graphics pipeline\n");

    VulkanShaderCode vertShaderCode = loadShaderCodeByPath("./shader/vert.spv");
    VulkanShaderCode fragShaderCode = loadShaderCodeByPath("./shader/frag.spv");

    // Create modules
    printf("Creating shader module\n");
    VkShaderModule vertShaderModule = createVulkanShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createVulkanShaderModule(fragShaderCode);

    printf("Assigning shaders to pipeline states\n");
    // Vertex shader
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShaderModule,
        .pName = "main"
    };

    // Fragment shader
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShaderModule,
        .pName = "main"
    };

    // Declare pipeline as an array
    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vertShaderStageInfo,
        fragShaderStageInfo
    };

    // Describe vertex data format
    // Because now data is hardcoded into shader, don't pass data to it
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = NULL,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = NULL
    };

    // Describe what kind of geometry to draw from provided vertices
    // and if primitive restart should be enabled
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    // Define viewport to render to
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (f32)vulkanSwapChainExtent.width,
        .height = (f32)vulkanSwapChainExtent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    // Define range, outside of which every geometry will be discarded
    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = vulkanSwapChainExtent
    };

    // Combine viewport and scissor into viewport state
    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    // Define rasterizer
    // (thing which will turn geometry into fragments to be colored by frag shader)
    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f
    };

    // Define multisampling
    // used for antialiasing
    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = 1.0f,
        .pSampleMask = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };

    // Define depth testing and stencil
    // TODO: this is skipped for now
    
    // Define how resulting colors should be blended with existing colors
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .colorWriteMask = (
            VK_COLOR_COMPONENT_R_BIT
            | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT
            | VK_COLOR_COMPONENT_A_BIT
        ),
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD
    };

    // Define blend constants
    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}
    };

    // Define dynamic component of pipeline
    // TODO: this is skipped for now

    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pSetLayouts = NULL,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = NULL
    };

    VkResult result = vkCreatePipelineLayout(
        vulkanDevice,
        &pipelineLayoutInfo,
        NULL,
        &vulkanPipelineLayout
    );
    if (result != VK_SUCCESS) {
        printf("[ERROR] Failed to create pipeline layout\n");
    }

    // Creat graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,

        // Combine everything created above
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = NULL,
        .pColorBlendState = &colorBlending,
        .pDynamicState = NULL,

        // Reference fixed function stages
        .layout = vulkanPipelineLayout,
        
        // Reference render pass
        .renderPass = vulkanRenderPass,
        .subpass = 0,

        // Previous render pipeline
        // (there is no previous pipeline, so null it)
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    VkResult createGraphicsPipelineResult = vkCreateGraphicsPipelines(
        vulkanDevice,
        VK_NULL_HANDLE,
        1,
        &pipelineInfo,
        NULL,
        &vulkanGraphicsPipeline
    );
    if (createGraphicsPipelineResult != VK_SUCCESS) {
        printf("[ERROR] Failed to create graphics pipeline\n");
    }

    // Destroy modules
    vkDestroyShaderModule(vulkanDevice, fragShaderModule, NULL);
    vkDestroyShaderModule(vulkanDevice, vertShaderModule, NULL);

    // Free loaded shader memory
    unloadShaderCode(vertShaderCode);
    unloadShaderCode(fragShaderCode);
}

void createVulkanRenderPass() {
    printf("Creating render pass\n");

    // Create color attachment to store color information
    VkAttachmentDescription colorAttachment = {
        .format = vulkanSwapChainImageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,

        // Clear values at the start
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,

        // Render to memory
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,

        // Don't do much with stencil, so dont care what happens there
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,

        // Initial state of image
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,

        // Keep present in the chain
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    // Create color pass reference for subpass
    VkAttachmentReference colorAttachmentRef = {
        // Attachment index
        .attachment = 0,

        // Use color layout (optimal variant)
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    // Create graphics subpass
    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,

        // Specify reference(-s) and links
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef
    };

    // Create render pass
    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        
        // List of all attachments
        .pAttachments = &colorAttachment,

        // Subpasses
        .subpassCount = 1,
        .pSubpasses = &subpass
    };
    VkResult result = vkCreateRenderPass(
        vulkanDevice,
        &renderPassInfo,
        NULL,
        &vulkanRenderPass
    );

    if (result != VK_SUCCESS) {
        printf("[ERROR] Cannot create render pass\n");
    }
}

void createVulkanFramebuffers() {
    printf("Creating framebuffers\n");

    // Allocate memory to store frambuffers
    vulkanSwapChainFramebuffers = (VkFramebuffer*)malloc(
        vulkanSwapChainImageCount * sizeof(VkFramebuffer)
    );

    // Create framebuffers for each swap chain image
    for (u32 i = 0; i < vulkanSwapChainImageCount; i++) {
        VkImageView attachments[] = {
            vulkanSwapChainImageViews[i]
        };

        // Create framebuffer
        VkFramebufferCreateInfo framebufferInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = vulkanRenderPass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width = vulkanSwapChainExtent.width,
            .height = vulkanSwapChainExtent.height,
            .layers = 1
        };
        VkResult result = vkCreateFramebuffer(
            vulkanDevice,
            &framebufferInfo,
            NULL,
            &vulkanSwapChainFramebuffers[i]
        );
        if (result != VK_SUCCESS) {
            printf("[ERROR] Failed to create framebuffer");
        }
    }
}

void createVulkanCommandPool() {
    printf("Creating command pool\n");

    struct VulkanQueueFamilyIndices queueFamilyIndices = findVulkanQueueFamilies(
        vulkanPhysicalDevice
    );

    // Create command pool (which will be submitted to GPU for processing)
    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = queueFamilyIndices.graphics,
        .flags = 0
    };
    VkResult result = vkCreateCommandPool(vulkanDevice, &poolInfo, NULL, &vulkanCommandPool);
    if (result != VK_SUCCESS) {
        printf("[ERROR] Failed to create graphics command pool\n");
    }
}

void createVulkanCommandBuffers() {
    printf("Creating command buffer\n");

    // Allocate memory to contain command buffers
    vulkanCommandBuffers = (VkCommandBuffer*)malloc(
        vulkanSwapChainImageCount * sizeof(VkCommandBuffer)
    );

    // Allocate buffers
    VkCommandBufferAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vulkanCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = vulkanSwapChainImageCount
    };
    VkResult result = vkAllocateCommandBuffers(
        vulkanDevice,
        &allocateInfo,
        vulkanCommandBuffers
    );
    if (result != VK_SUCCESS) {
        printf("[ERROR] Cannot allocate command buffers");
    }

    // Start buffer recording
    for (u32 i = 0; i < vulkanSwapChainImageCount; i++) {
        VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = 0,
            .pInheritanceInfo = NULL
        };

        VkResult beginBufferResult = vkBeginCommandBuffer(
            vulkanCommandBuffers[i], &beginInfo
        );
        if (beginBufferResult != VK_SUCCESS) {
            printf("[ERROR] Failed to begin command buffer");
        }

        // Define clear color
        VkClearValue clearColor = {0.0f,0.0f,0.0f,1.0f};

        // Start render pass
        VkRenderPassBeginInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = vulkanRenderPass,
            .framebuffer = vulkanSwapChainFramebuffers[i],
            .renderArea = {
                .offset = {0, 0},
                .extent = vulkanSwapChainExtent
            },
            .clearValueCount = 1,
            .pClearValues = &clearColor
        };
        vkCmdBeginRenderPass(
            vulkanCommandBuffers[i],
            &renderPassInfo,
            VK_SUBPASS_CONTENTS_INLINE
        );

        // Bind graphics pipeline
        vkCmdBindPipeline(
            vulkanCommandBuffers[i],
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            vulkanGraphicsPipeline
        );

        // FOKEN DRAAAAAWWW!!!
        vkCmdDraw(
            vulkanCommandBuffers[i],
            
            // Vertex count
            3,

            // Instance count
            1,

            // First vertex
            0,
            
            // First instance
            0
        );

        // End render pass
        vkCmdEndRenderPass(vulkanCommandBuffers[i]);

        // Stop recording
        VkResult stopRecordingResult = vkEndCommandBuffer(vulkanCommandBuffers[i]);
        if (stopRecordingResult != VK_SUCCESS) {
            printf("[ERROR] Error while stopping buffer cmd recording!\n");
        }
    }
}

void initVulkan() {
    printf("Initializing Vulkan\n");
    createVulkanInstance();
    // TODO: Setup debug message pipe
    createWindowSurface();
    pickVulkanPhysicalDevice();
    createVulkanLogicalDevice();
    createSwapChain();
    createVulkanImageViews();
    createVulkanRenderPass();
    createGraphicsPipeline();
    createVulkanFramebuffers();
    createVulkanCommandPool();
    createVulkanCommandBuffers();
}

void shutdownVulkan() {
    // TODO: Shutdown command buffers

    printf("Shutting down command pool\n");
    vkDestroyCommandPool(vulkanDevice, vulkanCommandPool, NULL);

    printf("Shutting down framebuffers\n");
    for (u32 i = 0; i < vulkanSwapChainImageCount; i++) {
        vkDestroyFramebuffer(vulkanDevice, vulkanSwapChainFramebuffers[i], NULL);
    }
    free(vulkanSwapChainFramebuffers);

    printf("Shutting down graphics pipeline\n");
    vkDestroyPipeline(vulkanDevice, vulkanGraphicsPipeline, NULL);

    printf("Shutting down pipeline\n");
    vkDestroyPipelineLayout(vulkanDevice, vulkanPipelineLayout, NULL);

    printf("Shutting down render pass\n");
    vkDestroyRenderPass(vulkanDevice, vulkanRenderPass, NULL);

    printf("Destroying image views\n");
    for (u32 i = 0; i < vulkanSwapChainImageCount; i++) {
        vkDestroyImageView(vulkanDevice, vulkanSwapChainImageViews[i], NULL);
    }

    printf("Freeing image views memory\n");
    free(vulkanSwapChainImageViews);

    printf("Releasing swapchain images info\n");
    free(vulkanSwapChainImages);

    printf("Shutting down swapchain\n");
    vkDestroySwapchainKHR(vulkanDevice, vulkanSwapChain, NULL);

    printf("Shutting down Vulkan\n");
    vkDestroyDevice(vulkanDevice, NULL);

    printf("Destroying window surface\n");
    vkDestroySurfaceKHR(vulkanInstance, vulkanSurface, NULL);

    printf("Destroying Vulkan virtual device\n");
    vkDestroyInstance(vulkanInstance, NULL);

    if (debugModeEnabled) {
        // Free layer info data
        printf("Freeing Vulkan layers info");
        free(vulkanLayers);
    }
}

int main(int argc, const char **argv) {

    // Init GLFW
    glfwInit();

    // Window hints
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // Create GLFW window
    window = glfwCreateWindow(
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        "qq",
        NULL,
        NULL
    );

    // Init vulkan
    initVulkan();

    // Main loop
    f64 lastFrameTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        // Calculate frame delta time
        f64 frameDelta = glfwGetTime() - lastFrameTime;
        lastFrameTime = glfwGetTime();

        glfwPollEvents();
        // printf("%f\n", frameDelta);
    }

    // Shutdown vulkan
    shutdownVulkan();

    // Destroy GLFW window
    glfwDestroyWindow(window);

    // Stop GLFW
    glfwTerminate();

    return 0;
}
