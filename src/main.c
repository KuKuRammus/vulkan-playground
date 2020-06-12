// TODO: Move into CMAKE
#define QQ_DEBUG
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Image loading by STB library
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Math
#include <cglm/vec2.h>
#include <cglm/vec3.h>
#include <cglm/mat4.h>
#include <cglm/affine.h>
#include <cglm/cam.h>

// Standard library stuff
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <qq.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define MAX_FRAMES_IN_FLIGHT 2

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

// Program start time
f64 frameDeltaTime = 0.0f;

// GLFW window
GLFWwindow *window;

// Vulkan info
VkLayerProperties *layerProperties;
VkInstance instance;
u32 physicalDeviceRating = 0;
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
VkDevice logicalDevice;

// Vulkan swap chain related stuff
VkSwapchainKHR swapchain;
u32 swapchainImageCount = 0;
VkFormat swapchainImageFormat;
VkExtent2D swapchainExtent;
VkImage* swapchainImages;
VkQueue graphicsQueue;
VkQueue presentQueue;

VkImageView* swapchainImageViews;

// Rendering surface
VkSurfaceKHR surface;

// Descriptor set layout
VkDescriptorSetLayout descriptorSetLayout;

// Rendering pass and pipeline layout
VkRenderPass renderPass;
VkPipelineLayout pipelineLayout;

// Rendering pipeline
VkPipeline graphicsPipeline;

// Framebuffers
VkFramebuffer* swapchainFramebuffers;

// Command pool
VkCommandPool commandPool;

// Vertex buffer and memory for it
VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;

// Index buffer and memory for it
VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;

// Descriptor pool
VkDescriptorPool descriptorPool;
VkDescriptorSet* descriptorSets;

// Uniform buffers
VkBuffer* uniformBuffers;
VkDeviceMemory* uniformBuffersMemory;

// Command buffers
VkCommandBuffer* commandBuffers;

// Loaded image handle and memory
VkImage textureImage;
VkDeviceMemory textureImageMemory;

// Texture image view
VkImageView textureImageView;

// Current frame index
u32 currentFrame = 0;

// Flag to handle resize explicitly
u32 framebufferResized = QQ_FALSE;

// Semaphores and fence
VkFence* inFlightFences;
VkFence* imagesInFlight;
VkSemaphore* imageAvailableSemaphores;
VkSemaphore* renderFinishedSemaphores;

// Required validation layers
const char *requiredVulkanLayers[1] = { "VK_LAYER_KHRONOS_validation" };

// Reuired device extensions
u32 requiredVulkanDeviceExtensionCount = 1;
const char *reqVulkanDeviceExtensions[1] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Describe verticies (unique points of mesh)
u32 verticesCount = 8;
Vertex verticies[8] = {
    { {-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f} },
    { {-1.0f, -1.0f,  1.0f}, {1.0f, 0.5f, 0.0f} },
    { { 1.0f, -1.0f,  1.0f}, {1.0f, 1.0f, 0.0f} },
    { { 1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f} },
    { {-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, 1.0f} },
    { {-1.0f,  1.0f,  1.0f}, {0.3f, 0.0f, 0.5f} },
    { { 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 0.0f} },
    { { 1.0f,  1.0f, -1.0f}, {1.0f, 1.0f, 1.0f} },
};

// Describe indices (groups of verticies indexes)
u32 indicesCount = 36;
u32 indices[36] = {
    3, 0, 1, // BACK
    3, 1, 2,
    0, 3, 7, // BOT
    0, 7, 4,
    0, 5, 1, // RIGHT
    0, 4, 5,
    3, 7, 6, // LEFT
    3, 6, 2,
    2, 6, 5, // TOP
    2, 5, 1,
    7, 4, 6, // FRONT
    6, 4, 5
};

// ------ END GLOBALS


// ------ VERTEX HELPERS
// All functions below are related to Vertex struct (ideally, methods in the class)
VkVertexInputBindingDescription getVertexBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {
        // Positional index
        .binding = 0,

        // Distance between each entry
        .stride = sizeof(Vertex),

        // Move to the next data entry after each vertex
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    return bindingDescription;
}

VkVertexInputAttributeDescription* getVertexAttributeDescription() {
    // TODO: Not sure if allocating memory here is a good idea
    VkVertexInputAttributeDescription* attributeDescription = malloc(
        2 * sizeof(VkVertexInputAttributeDescription)
    );

    // Provide position data location
    attributeDescription[0].binding = 0;
    attributeDescription[0].location = 0; // Location directive in shader
    attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescription[0].offset = offsetof(Vertex, position);

    // Provide color data location
    attributeDescription[1].binding = 0;
    attributeDescription[1].location = 1; // Location directive in shader
    attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescription[1].offset = offsetof(Vertex, color);

    return attributeDescription;
}
// ------ END VERTEX HELPERS

// Helper to create command buffer and start recoring
VkCommandBuffer beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = commandPool,
        .commandBufferCount = 1
    };

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);

    // Begin buffer recording
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

// Helper to stop recording of single time buffer
void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
}


// HELPER TO FIND SPECIFIC MEMORY TYPE
u32 findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) {
    // Get list of memory properties
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    // Find memory which is suitable for the buffer
    for (u32 i = 0; i < memProperties.memoryTypeCount; i++) {
        if (
            // Check if corresponding bit is set to 1
            (typeFilter & (1 << i))

            // Ensure have required property support (O_o)
            && (memProperties.memoryTypes[i].propertyFlags & properties) == properties
        ) {
            return i;
        }
    }

    printf("[ERROR] Failed to find suitable memory type\n");
    return 0;
}

// Helper to create and allocate buffer(-s?)
void createBuffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer* buffer,
    VkDeviceMemory* bufferMemory
) {
    // Create buffer
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    if (vkCreateBuffer(logicalDevice, &bufferInfo, NULL, buffer) != VK_SUCCESS) {
        printf("[ERROR] Failed to create buffer\n");
    }

    // Get memory requirements for this type of buffer
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memRequirements);

    // Allocate memory
    // NOTE:
    //      In real application, DO NOT CALL `vkAllocateMemory` for every buffer
    //      because it is easy to hit GPU limit on maximum allowed buffer allocs.
    //      Proper way would be to create single allocation, and use this memory
    //      by providing `offset` value
    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
    };
    if (vkAllocateMemory(logicalDevice, &allocInfo, NULL, bufferMemory) != VK_SUCCESS) {
        printf("[ERROR] Failed to allocate buffer memory\n");
    }

    // Bind buffer memory
    vkBindBufferMemory(logicalDevice, *buffer, *bufferMemory, 0);

}

// Helper for creating image
void createImage(
    u32 width,
    u32 height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkImage* image,
    VkDeviceMemory* imageMemory
) {
    // Create vulkan texture image
    VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .extent = {
            .width = width,
            .height = height,
            .depth = 1
        },
        .mipLevels = 1,
        .arrayLayers = 1,

        // Image format
        .format = format,

        // Image tiling (let driver pick in this case)
        .tiling = tiling,

        // Not usable by GPU, first transition will discard the texels
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,

        // Image used as a destination, and can be accessed from shaders
        .usage = usage,

        // Image will be only used by 1 queue family
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,

        // Not using multisampling for now
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .flags = 0
    };

    if (vkCreateImage(logicalDevice, &imageInfo, NULL, image) != VK_SUCCESS) {
        printf("[ERROR] Failed to create image\n");
    }

    // Get requirements and allocate memory for the image
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(logicalDevice, *image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(
            memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        )
    };
    if (vkAllocateMemory(logicalDevice, &allocInfo, NULL, imageMemory) != VK_SUCCESS) {
        printf("[ERROR] Failed to allocate memory for the image");
    }

    vkBindImageMemory(logicalDevice, *image, *imageMemory, 0);
}


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
        
        u32 actualWidth, actualHeight;
        glfwGetFramebufferSize(window, &actualWidth, &actualHeight);

        VkExtent2D actualExtent = {actualWidth, actualHeight};
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

SwapchainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
    SwapchainSupportDetails details;
    details.formatCount = 0;
    details.presentModeCount = 0;

    // Fetch capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    // Fetch format count
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formatCount, NULL);
    if (details.formatCount != 0) {
        details.formats = (VkSurfaceFormatKHR*)malloc(
            details.formatCount * sizeof(VkSurfaceFormatKHR)
        );
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            device,
            surface,
            &details.formatCount,
            details.formats
        );
    }

    // Fetch present modes
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device,
        surface,
        &details.presentModeCount,
        NULL
    );
    if (details.presentModeCount != 0) {
        details.presentModes = (VkPresentModeKHR*)malloc(
            details.presentModeCount * sizeof(VkPresentModeKHR)
        );
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            device,
            surface,
            &details.presentModeCount,
            details.presentModes
        );
    }

    // TODO: MEMORY LEAK WARNING!!!!
    // formats and presentModes should be cleared after allocation

    return details;
}

QueueFamilyIndices findVulkanQueueFamilies(VkPhysicalDevice device) {
    printf("Checking for required queue families\n");

    QueueFamilyIndices indices = {
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
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
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

void createSwapchain() {
    printf("Creating swapchain\n");

    SwapchainSupportDetails swapChainSupport = querySwapChainSupport(
        physicalDevice
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
        .surface = surface,
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
    QueueFamilyIndices indices = findVulkanQueueFamilies(physicalDevice);
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
        vkCreateSwapchainKHR(logicalDevice, &createInfo, NULL, &swapchain) != VK_SUCCESS
    ) {
        printf("[ERR] Failed to create vulkan swapchain");
    }

    // Get swap chain images
    vkGetSwapchainImagesKHR(
        logicalDevice,
        swapchain,
        &swapchainImageCount,
        NULL
    );

    // Allocate memory to store images
    swapchainImages = (VkImage*)malloc(
        swapchainImageCount * sizeof(VkImage)
    );

    // Store images
    vkGetSwapchainImagesKHR(
        logicalDevice,
        swapchain,
        &swapchainImageCount,
        swapchainImages
    );

    // Save format and extent
    swapchainImageFormat = surfaceFormat.format;
    swapchainExtent = extent;

    // Free resources
    if (swapChainSupport.formatCount != 0) {
        free(swapChainSupport.formats);
    }

    if (swapChainSupport.presentModeCount != 0) {
        free(swapChainSupport.presentModes);
    }
}


void createLogicalDevice() {
    printf("Creating vulkan logical device\n");
    QueueFamilyIndices indices = findVulkanQueueFamilies(physicalDevice);

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
        vkCreateDevice(physicalDevice, &createInfo, NULL, &logicalDevice) != VK_SUCCESS
    ) {
        printf("[ERR] Failed to create vulkan virtual device");
    }

    // Get handle for graphics queue
    vkGetDeviceQueue(logicalDevice, indices.graphics, 0, &graphicsQueue);
    vkGetDeviceQueue(logicalDevice, indices.present, 0, &presentQueue);

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
    QueueFamilyIndices indices = findVulkanQueueFamilies(device);
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

    SwapchainSupportDetails swapChainSupport = querySwapChainSupport(device);
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

void pickPhysicalDevice() {
    printf("Picking vulkan physical device\n");

    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);

    if (deviceCount == 0) {
        printf(" [ERR] Failed to find GPUs with Vulkan support\n");
    }

    VkPhysicalDevice* devices = (VkPhysicalDevice *)malloc(
        deviceCount * sizeof(VkPhysicalDevice)
    );
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

    for (u32 i = 0; i < deviceCount; i++) {
        u32 score = rateVulkanPhysicalDevice(devices[i]);
        if (score == 0) {
            continue;
        }

        if (score > physicalDeviceRating) {
            physicalDevice = devices[i];
            physicalDeviceRating = score;
        }
   }

    if (physicalDevice == VK_NULL_HANDLE) {
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
    layerProperties = (VkLayerProperties *) malloc(availableLayerCount * sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&availableLayerCount, layerProperties);

    // Iterate over available layers to check if required layers are supported
    b32 allLayersIsFound = QQ_TRUE;
    for (u32 i = 0; i < 1; i++) {
        b32 layerIsFound = QQ_FALSE;

        for (u32 j = 0; j < availableLayerCount; j++) {
            if (strcmp(requiredVulkanLayers[i], layerProperties[j].layerName) == 0) {
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

void createInstance() {
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
    VkResult instanceCreationSuccess = vkCreateInstance(&createInfo, NULL, &instance);
    if (instanceCreationSuccess != VK_SUCCESS) {
        printf("Failed to initialize Vulkan instance\n");
    }
}

void createSurface() {
    printf("Creating KHR surface for window\n");
    if (glfwCreateWindowSurface(instance, window, NULL, &surface)) {
        printf("[ERR] Failed to create window surface\n");
    }
}

// Helper to create vulkan image view
VkImageView createImageView(VkImage image, VkFormat format) {
    VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    VkImageView imageView;
    if (vkCreateImageView(logicalDevice, &viewInfo, NULL, &imageView) != VK_SUCCESS) {
        printf("[ERROR] Failed to create image view\n");
    }

    return imageView;
}

void createImageViews() {
    printf("Creating swap chain image views");

    swapchainImageViews = (VkImageView*)malloc(
        swapchainImageCount * sizeof(VkImageView)
    );

    // Create image views
    for (u32 i = 0; i < swapchainImageCount; i++) {
        swapchainImageViews[i] = createImageView(
            swapchainImages[i],
            swapchainImageFormat
        );
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
    if (vkCreateShaderModule(logicalDevice, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
        printf("Failed to create shader module");
    }

    return shaderModule;
}

/**
 * Descriptor is a way for shaders to freely access resources like buffer or images
 *
 * Descriptor layout specifies types of resources that are going to be accessed by
 * pipeline, just like render pass specifies the types of attachments
 * that will be accessed.
 *
 * Descriptor set specifies the actual buffer or image resources that
 * will be bound to the descriptors
 */
void createDescriptorSetLayout() {
    printf("Creating descriptor set layout\n");

    VkDescriptorSetLayoutBinding uboLayoutBinding = {
        // Binding, used in the shader
        .binding = 0,

        // Type of the descriptor
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,

        // It is possible to pass an array, so we need to specify count manually
        // Passing multiple might be useful for skeletal animation
        .descriptorCount = 1,

        // Which stages will access this descriptor
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,

        // Only relevant for image descriptors
        .pImmutableSamplers = NULL
    };

    // All descriptor binding are combined into DescriptorSetLayout
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &uboLayoutBinding
    };
    if (vkCreateDescriptorSetLayout(
        logicalDevice,
        &layoutInfo,
        NULL,
        &descriptorSetLayout
    ) != VK_SUCCESS) {
        printf("[ERROR] Failed to create descriptor set layout\n");
    }
}

void createGraphicsPipeline() {
    printf("Creating graphics pipeline\n");

    VulkanShaderCode vertShaderCode = loadShaderCodeByPath("./shader/vert.spv");
    VulkanShaderCode fragShaderCode = loadShaderCodeByPath("./shader/frag.spv");

    // Create modules
    printf("Creating shader module\n");
    VkShaderModule vertShaderModule = createVulkanShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createVulkanShaderModule(fragShaderCode);

    // Vertex shader
    printf("Assigning shaders to pipeline states\n");
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShaderModule,
        .pName = "main"
    };

    // Fragment shader
    printf("Frag shader stage create info\n");
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShaderModule,
        .pName = "main"
    };

    // Declare pipeline as an array
    printf("Shader stage create info\n");
    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vertShaderStageInfo,
        fragShaderStageInfo
    };

    // Describe vertex data format
    printf("Binding vertex descriptors\n");
    VkVertexInputBindingDescription bindingDescription = getVertexBindingDescription();
    u32 bindingDescriptionCount = 1;
    u32 vertexAttrDescriptionCount = 2;

    VkVertexInputAttributeDescription* attributeDescriptions = getVertexAttributeDescription();
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        // TODO: This is HARDCODED!!!1 BIG OOOF!1
        .vertexBindingDescriptionCount = bindingDescriptionCount,
        .pVertexBindingDescriptions = &bindingDescription, 
        // TODO: This is HARDCODED!!!1 BIG OOOF!1
        .vertexAttributeDescriptionCount = vertexAttrDescriptionCount,
        .pVertexAttributeDescriptions = attributeDescriptions
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
        .width = (f32)swapchainExtent.width,
        .height = (f32)swapchainExtent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    // Define range, outside of which every geometry will be discarded
    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = swapchainExtent
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
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
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
        .setLayoutCount = 1,
        .pSetLayouts = &descriptorSetLayout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = NULL
    };

    printf("Creating pipeline layout\n");
    VkResult result = vkCreatePipelineLayout(
        logicalDevice,
        &pipelineLayoutInfo,
        NULL,
        &pipelineLayout
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
        .layout = pipelineLayout,
        
        // Reference render pass
        .renderPass = renderPass,
        .subpass = 0,

        // Previous render pipeline
        // (there is no previous pipeline, so null it)
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    VkResult createGraphicsPipelineResult = vkCreateGraphicsPipelines(
        logicalDevice,
        VK_NULL_HANDLE,
        1,
        &pipelineInfo,
        NULL,
        &graphicsPipeline
    );
    if (createGraphicsPipelineResult != VK_SUCCESS) {
        printf("[ERROR] Failed to create graphics pipeline\n");
    }

    // Destroy modules
    vkDestroyShaderModule(logicalDevice, fragShaderModule, NULL);
    vkDestroyShaderModule(logicalDevice, vertShaderModule, NULL);

    // Free loaded shader memory
    unloadShaderCode(vertShaderCode);
    unloadShaderCode(fragShaderCode);

    // Free vertex attribute description info
    free(attributeDescriptions);
}

void createRenderPass() {
    printf("Creating render pass\n");

    // Create color attachment to store color information
    VkAttachmentDescription colorAttachment = {
        .format = swapchainImageFormat,
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

    // Create subpass depency
    VkSubpassDependency dependency = {
        // Dependent subpass and index
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,

        // Operations to wait on
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,

        // Writing info
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    };

    // Create render pass
    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        
        // List of all attachments
        .pAttachments = &colorAttachment,

        // Subpasses
        .subpassCount = 1,
        .pSubpasses = &subpass,

        // Dependencies
        .dependencyCount = 1,
        .pDependencies = &dependency
    };
    VkResult result = vkCreateRenderPass(
        logicalDevice,
        &renderPassInfo,
        NULL,
        &renderPass
    );

    if (result != VK_SUCCESS) {
        printf("[ERROR] Cannot create render pass\n");
    }
}

void createFramebuffers() {
    printf("Creating framebuffers\n");

    // Allocate memory to store frambuffers
    swapchainFramebuffers = (VkFramebuffer*)malloc(
        swapchainImageCount * sizeof(VkFramebuffer)
    );

    // Create framebuffers for each swap chain image
    for (u32 i = 0; i < swapchainImageCount; i++) {
        VkImageView attachments[] = {
            swapchainImageViews[i]
        };

        // Create framebuffer
        VkFramebufferCreateInfo framebufferInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = renderPass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width = swapchainExtent.width,
            .height = swapchainExtent.height,
            .layers = 1
        };
        VkResult result = vkCreateFramebuffer(
            logicalDevice,
            &framebufferInfo,
            NULL,
            &swapchainFramebuffers[i]
        );
        if (result != VK_SUCCESS) {
            printf("[ERROR] Failed to create framebuffer");
        }
    }
}

void createCommandPool() {
    printf("Creating command pool\n");

    QueueFamilyIndices queueFamilyIndices = findVulkanQueueFamilies(
        physicalDevice
    );

    // Create command pool (which will be submitted to GPU for processing)
    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = queueFamilyIndices.graphics,
        .flags = 0
    };
    VkResult result = vkCreateCommandPool(logicalDevice, &poolInfo, NULL, &commandPool);
    if (result != VK_SUCCESS) {
        printf("[ERROR] Failed to create graphics command pool\n");
    }
}

void transitionImageLayout(
    VkImage image,
    VkFormat format,
    VkImageLayout oldLayout,
    VkImageLayout newLayout
) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    // Create memory barrier to sync resource access
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = oldLayout,
        .newLayout = newLayout,

        // Not transferring family ownership
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

        // Specify image and part of the image
        .image = image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        },

        // Specify operations before and after the barrier
        .srcAccessMask = 0,
        .dstAccessMask = 0,
    };

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (
        oldLayout == VK_IMAGE_LAYOUT_UNDEFINED
        && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    ) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (
        oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    ) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        printf("Unsupported layout transition");
    }

    // Submit pipeline barrier
    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage,
        destinationStage,
        0,
        0,
        NULL,
        0,
        NULL,
        1,
        &barrier
    );

    endSingleTimeCommands(commandBuffer);
}

// Helper to copy buffer to image
void copyBufferToImage(VkBuffer buffer, VkImage image, u32 width, u32 height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    // Specify which part of the buffer will be copied to which part of the image
    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,

        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },

        .imageOffset = {0,0,0},
        .imageExtent = {width, height, 1}
    };

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    endSingleTimeCommands(commandBuffer);
}

void createTextureImage() {
    printf("Loading texture\n");

    u32 textureWidth, textureHeight, textureChannels;
    stbi_uc* pixels = stbi_load(
        "texture/animegrill.jpg",
        &textureWidth,
        &textureHeight,
        &textureChannels,
        STBI_rgb_alpha
    );
    VkDeviceSize imageSize = textureWidth * textureHeight * 4;

    if (!pixels) {
        printf("[ERROR] Failed to load texture image");
    }

    // Prepare to load into optimized buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &stagingBuffer,
        &stagingBufferMemory
    );

    // Copy to staging buffer
    void* data;
    vkMapMemory(logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, imageSize);
    vkUnmapMemory(logicalDevice, stagingBufferMemory);

    // Free stb image buffer
    stbi_image_free(pixels);

    // Create image via helper
    createImage(
        textureWidth,
        textureHeight,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &textureImage,
        &textureImageMemory
    );

    // Transition to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    transitionImageLayout(
        textureImage,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );

    // Execute buffer to image copy operation
    copyBufferToImage(
        stagingBuffer,
        textureImage,
        textureWidth,
        textureHeight
    );

    // Prepare image for reading from shader
    transitionImageLayout(
        textureImage,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    // Cleanup staging buffer
    vkDestroyBuffer(logicalDevice, stagingBuffer, NULL);
    vkFreeMemory(logicalDevice, stagingBufferMemory, NULL);
}

void createTextureImageView() {
    printf("Creating texture image view\n");
    textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB);
}

// Helper function to copy data between 2 buffers
void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {

    // Memory transfer operations are executed through command buffers
    // So allocate temp command buffer
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    // Record command for transfering contents of the buffer
    VkBufferCopy copyRegion = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    // Stop recording
    endSingleTimeCommands(commandBuffer);
}



void createVertexBuffer() {
    printf("Creating vertex buffers\n");

    VkDeviceSize bufferSize = sizeof(Vertex) * verticesCount;

    // Create staging (temp) buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &stagingBuffer,
        &stagingBufferMemory
    );

    // Map staging buffer memory into CPU accessible memory
    void* data;
    vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    // Copy data into mapped area
    memcpy(data, verticies, bufferSize);
    // Unmap memory
    vkUnmapMemory(logicalDevice, stagingBufferMemory);
    // TODO: Driver may actualy not copy data to buffer straight away, read about that

    // Create buffer using helper above
    // Data will be copied into this buffer from temp buffer
    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &vertexBuffer,
        &vertexBufferMemory
    );

    // Copy data from staging(CPU accessible memory) to GPU buffer
    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    // Clean temp buffer data
    vkDestroyBuffer(logicalDevice, stagingBuffer, NULL);
    vkFreeMemory(logicalDevice, stagingBufferMemory, NULL);

}

void createIndexBuffer() {
    printf("Creating index buffer\n");

    VkDeviceSize bufferSize = sizeof(indices[0]) * indicesCount;

    // Create temp buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &stagingBuffer,
        &stagingBufferMemory
    );

    // Copy to temp buffer
    void* data;
    vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices, bufferSize);
    vkUnmapMemory(logicalDevice, stagingBufferMemory);

    // Create optimized storage buffer
    createBuffer(
        bufferSize,
        // Using ..._INDEX_BUFFER_BIT this time
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &indexBuffer,
        &indexBufferMemory
    );

    // Copy from temp to optimized(GPU)
    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    // Free temp buffer
    vkDestroyBuffer(logicalDevice, stagingBuffer, NULL);
    vkFreeMemory(logicalDevice, stagingBufferMemory, NULL);
}

void createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    // Allocate memory for buffers and device memory handlers
    uniformBuffers = malloc(sizeof(VkBuffer) * swapchainImageCount);
    uniformBuffersMemory = malloc(sizeof(VkDeviceMemory) * swapchainImageCount);

    // Create buffers with memory
    for (u32 i = 0; i < swapchainImageCount; i++) {
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &uniformBuffers[i],
            &uniformBuffersMemory[i]
        );
    }
}

void createDescriptorPool() {
    VkDescriptorPoolSize poolSize = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = swapchainImageCount
    };

    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize,

        // Maximum about of descriptors that can be allocated
        .maxSets = swapchainImageCount
    };

    if (vkCreateDescriptorPool(logicalDevice, &poolInfo, NULL, &descriptorPool) != VK_SUCCESS) {
        printf("[ERROR] Failed to create descriptor pool\n");
    }
}

void createCommandBuffers() {
    printf("Creating command buffer\n");

    // Allocate memory to contain command buffers
    commandBuffers = (VkCommandBuffer*)malloc(
        swapchainImageCount * sizeof(VkCommandBuffer)
    );

    // Allocate buffers
    VkCommandBufferAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = swapchainImageCount
    };
    VkResult result = vkAllocateCommandBuffers(
        logicalDevice,
        &allocateInfo,
        commandBuffers
    );
    if (result != VK_SUCCESS) {
        printf("[ERROR] Cannot allocate command buffers\n");
    }

    // Start buffer recording
    for (u32 i = 0; i < swapchainImageCount; i++) {
        VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = 0,
            .pInheritanceInfo = NULL
        };

        VkResult beginBufferResult = vkBeginCommandBuffer(
            commandBuffers[i], &beginInfo
        );
        if (beginBufferResult != VK_SUCCESS) {
            printf("[ERROR] Failed to begin command buffer\n");
        }

        // Define clear color
        VkClearValue clearColor = {0.05f,0.05f,0.05f,1.0f};

        // Start render pass
        VkRenderPassBeginInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = renderPass,
            .framebuffer = swapchainFramebuffers[i],
            .renderArea = {
                .offset = {0, 0},
                .extent = swapchainExtent
            },
            .clearValueCount = 1,
            .pClearValues = &clearColor
        };
        vkCmdBeginRenderPass(
            commandBuffers[i],
            &renderPassInfo,
            VK_SUBPASS_CONTENTS_INLINE
        );

        // Bind graphics pipeline
        vkCmdBindPipeline(
            commandBuffers[i],
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            graphicsPipeline
        );

        // Attach vertex buffers
        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

        // Attach index buffers
        vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        // Bind buffer
        vkCmdBindDescriptorSets(
            commandBuffers[i],
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0,
            1,
            &descriptorSets[i],
            0,
            NULL
        );

        // Draw using attached vertex and index buffers
        vkCmdDrawIndexed(
            commandBuffers[i],
            
            // Vertex count
            indicesCount,

            // Instance count
            1,

            // First index in index buffer
            0,
            
            // Offset between indices
            0,

            // Instancing offset (not used)
            0
        );

        // End render pass
        vkCmdEndRenderPass(commandBuffers[i]);

        // Stop recording
        VkResult stopRecordingResult = vkEndCommandBuffer(commandBuffers[i]);
        if (stopRecordingResult != VK_SUCCESS) {
            printf("[ERROR] Error while stopping buffer cmd recording!\n");
        }
    }
}

void createSyncObjects() {
    printf("Creating semaphores\n");

    // Allocate memory for fences
    inFlightFences = (VkFence*)malloc(
        MAX_FRAMES_IN_FLIGHT * sizeof(VkFence)
    );

    // Automatically initialize images in flight with null handles
    imagesInFlight = (VkFence*)malloc(
        swapchainImageCount * sizeof(VkFence)
    );

    for (u32 i = 0; i < swapchainImageCount; i++) {
        imagesInFlight[i] = VK_NULL_HANDLE;
    }

    // Allocate memory for semaphores
    imageAvailableSemaphores = (VkSemaphore*)malloc(
        MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore)
    );
    renderFinishedSemaphores = (VkSemaphore*)malloc(
        MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore)
    );

    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    VkResult result;

    // Create semaphore pairs
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        result = vkCreateSemaphore(
            logicalDevice,
            &semaphoreInfo,
            NULL,
            &imageAvailableSemaphores[i]
        );
        if (result != VK_SUCCESS) {
            printf("[ERROR] Failed to create image available semaphore\n");
        }

        result = vkCreateSemaphore(
            logicalDevice,
            &semaphoreInfo,
            NULL,
            &renderFinishedSemaphores[i]
        );
        if (result != VK_SUCCESS) {
            printf("[ERROR] Failed to create render finished semaphore\n");
        }

        result = vkCreateFence(
            logicalDevice,
            &fenceInfo,
            NULL,
            &inFlightFences[i]
        );
        if (result != VK_SUCCESS) {
            printf("[ERROR] Failed to create fence\n");
        }

    }
}

void createDescriptorSets() {
    VkDescriptorSetLayout* layouts = malloc(sizeof(VkDescriptorSetLayout) * swapchainImageCount);
    for (u32 i = 0; i < swapchainImageCount; i++) {
        layouts[i] = descriptorSetLayout;
    }

    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = swapchainImageCount,
        .pSetLayouts = layouts
    };

    // Allocate memory for descriptor sets
    descriptorSets = malloc(sizeof(VkDescriptorSet) * swapchainImageCount);
    if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, descriptorSets) != VK_SUCCESS) {
        printf("[ERROR] Failed to allocate descriptor sets\n");
    }

    // Free temp layouts memory
    free(layouts);

    // Configure descriptor sets
    for (u32 i = 0; i < swapchainImageCount; i++) {
        VkDescriptorBufferInfo bufferInfo = {
            .buffer = uniformBuffers[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject)
        };

        VkWriteDescriptorSet descriptorWrite = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSets[i],
            .dstBinding = 0,
            // Not using as array
            .dstArrayElement = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .pBufferInfo = &bufferInfo,
            .pImageInfo = NULL,
            .pTexelBufferView = NULL
        };

        vkUpdateDescriptorSets(logicalDevice, 1, &descriptorWrite, 0, NULL);
    }
}

void shutdownSwapchain() {
    printf("Shutting down framebuffers\n");
    for (u32 i = 0; i < swapchainImageCount; i++) {
        vkDestroyFramebuffer(logicalDevice, swapchainFramebuffers[i], NULL);
    }
    free(swapchainFramebuffers);

    printf("Freeing command buffers\n");
    u32 commandBufferCount = swapchainImageCount;
    vkFreeCommandBuffers(
        logicalDevice,
        commandPool,
        commandBufferCount,
        commandBuffers
    );

    printf("Freeing uniform buffers\n");
    for (u32 i = 0; i < swapchainImageCount; i++) {
        vkDestroyBuffer(logicalDevice, uniformBuffers[i], NULL);
        vkFreeMemory(logicalDevice, uniformBuffersMemory[i], NULL);
    }
    free(uniformBuffers);
    free(uniformBuffersMemory);

    printf("Shutting down descriptor pool\n");
    vkDestroyDescriptorPool(logicalDevice, descriptorPool, NULL);

    printf("Shutting down graphics pipeline\n");
    vkDestroyPipeline(logicalDevice, graphicsPipeline, NULL);

    printf("Shutting down pipeline\n");
    vkDestroyPipelineLayout(logicalDevice, pipelineLayout, NULL);

    printf("Shutting down render pass\n");
    vkDestroyRenderPass(logicalDevice, renderPass, NULL);

    printf("Destroying image views\n");
    for (u32 i = 0; i < swapchainImageCount; i++) {
        vkDestroyImageView(logicalDevice, swapchainImageViews[i], NULL);
    }

    printf("Freeing image views memory\n");
    free(swapchainImageViews);

    printf("Destroying swapchain\n");
    vkDestroySwapchainKHR(logicalDevice, swapchain, NULL);

}

void recreateSwapchain() {
    printf("[RUNTIME] Recreating swap chain\n");

    // Get window dimentions from glfw
    u32 width;
    u32 height;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    // Wait till currently used resources are free to manage
    vkDeviceWaitIdle(logicalDevice);

    createSwapchain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
}


void initVulkan() {
    printf("Initializing Vulkan\n");
    createInstance();
    // TODO: Setup debug message pipe
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createTextureImage();
    createTextureImageView();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}

void shutdownVulkan() {

    // TODO: There is an errors, when resizing the window
    shutdownSwapchain();

    printf("Shutting down loaded image texture view\n");
    vkDestroyImageView(logicalDevice, textureImageView, NULL);

    printf("Shutting down loaded image texture\n");
    vkDestroyImage(logicalDevice, textureImage, NULL);
    vkFreeMemory(logicalDevice, textureImageMemory, NULL);

    printf("Shutting down descriptor set layout\n");
    vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayout, NULL);

    printf("Shutting down index buffer\n");
    vkDestroyBuffer(logicalDevice, indexBuffer, NULL);
    vkFreeMemory(logicalDevice, indexBufferMemory, NULL);

    printf("Shutting down vertex buffer\n");
    vkDestroyBuffer(logicalDevice, vertexBuffer, NULL);
    vkFreeMemory(logicalDevice, vertexBufferMemory, NULL);

    printf("Shutting down semaphores\n");
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], NULL);
        vkDestroySemaphore(logicalDevice, imageAvailableSemaphores[i], NULL);
        vkDestroyFence(logicalDevice, inFlightFences[i], NULL);
    }
    free(renderFinishedSemaphores);
    free(imageAvailableSemaphores);
    free(inFlightFences);

    printf("Shutting down command pool\n");
    vkDestroyCommandPool(logicalDevice, commandPool, NULL);

    printf("Releasing swapchain images info\n");
    free(swapchainImages);

    printf("Shutting down Vulkan\n");
    vkDestroyDevice(logicalDevice, NULL);

    printf("Destroying window surface\n");
    vkDestroySurfaceKHR(instance, surface, NULL);

    printf("Destroying Vulkan virtual device\n");
    vkDestroyInstance(instance, NULL);

    if (debugModeEnabled) {
        // Free layer info data
        printf("Freeing Vulkan layers info");
        free(layerProperties);
    }
}

void updateUniformBuffer(u32 currentImage) {
    UniformBufferObject ubo = {};
    

    f64 rotationAngleRads = 0.174533f;
    mat4 modelMatrix = GLM_MAT4_IDENTITY_INIT;
    glm_rotate_z(
        modelMatrix,
        glfwGetTime() * rotationAngleRads,
        ubo.model
    );

    vec3 eyeVector = {3.0f, 3.0f, 3.0f};
    vec3 lookCenter = {0.0f, 0.0f, 0.0f};
    vec3 lookUp = {0.0f, 0.0f, 1.0f};
    glm_lookat(
        eyeVector,
        lookCenter,
        lookUp,
        ubo.view
    );

    f64 fieldOfViewRads = 0.785398f;
    f64 projectionAspect = (f64)swapchainExtent.width / (f64)swapchainExtent.height;
    glm_perspective(
        fieldOfViewRads,
        projectionAspect,
        0.1f, // Near clipping plane
        10.0f, // Far clipping plane
        ubo.projection
    );

    ubo.projection[1][1] *= -1.0f;

    // Copy data to current uniform buffer
    void* data;
    vkMapMemory(logicalDevice, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(logicalDevice, uniformBuffersMemory[currentImage]);

}

void drawFrame() {
    // Wait for the frame to be finished
    vkWaitForFences(logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, U64_MAX);

    // Aquire image from swap chain
    u32 imageIndex;
    VkResult acquireResult = vkAcquireNextImageKHR(
        // Device and swap chain to aquire image from
        logicalDevice,
        swapchain,

        // Timeout for image to become available (nanoseconds)
        // "max" value disables timeout
        U64_MAX,

        // Objects to signal when image will become available
        imageAvailableSemaphores[currentFrame],
        VK_NULL_HANDLE,

        // Available image index (in swaphChainImages)
        &imageIndex
    );

    // Determine if swapchain needs to be recreated, based on result of image acquiring
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    } else if (acquireResult != VK_SUCCESS) {
        printf("Failed to acquire swap chain image!\n");
    }

    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(logicalDevice, 1, &imagesInFlight[imageIndex], VK_TRUE, U64_MAX);
    }
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    // Update uniform buffer for animation
    updateUniformBuffer(imageIndex);

    // Submit command buffer
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    // Specify which semaphores to wait form and in which pipeline
    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    // Specify which command buffer to submit
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

    // Specify semaphores to trigger when execution is finished
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // Reset fences
    vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);
    
    // Submit command to graphics queue
    if (vkQueueSubmit(
        graphicsQueue,
        1,
        &submitInfo,
        inFlightFences[currentFrame]
    ) != VK_SUCCESS) {
        printf("[ERROR] Failed to submit draw command buffer\n");
    }

    // Swapchains to put images into
    VkSwapchainKHR swapChains[] = {swapchain};

    // Configure presentation
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,

        // Define semaphores to wait
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,

        // Sets swapchain
        .swapchainCount = 1,
        .pSwapchains = swapChains,
        .pImageIndices = &imageIndex,

        .pResults = NULL
    };

    // Queue presentation
    VkResult presentResult = vkQueuePresentKHR(presentQueue, &presentInfo);

    // Recreate chain if image is suboptimal
    if (
        presentResult == VK_ERROR_OUT_OF_DATE_KHR
        || presentResult == VK_SUBOPTIMAL_KHR
        || framebufferResized != QQ_FALSE
    ) {
        framebufferResized = QQ_FALSE;
        recreateSwapchain();
    } else if (presentResult != VK_SUCCESS) {
        printf("[ERROR] Failed to present swap chain data\n");
    }

    // Update current frame sepahore index
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    framebufferResized = QQ_TRUE;
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
    glfwSetFramebufferSizeCallback(window, &framebufferResizeCallback);

    // Init vulkan
    initVulkan();

    // Main loop
    f64 lastFrameTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
        frameDeltaTime = glfwGetTime() - lastFrameTime;
        printf("[RENDER] %f FPS (took: %f sec)\n", (1.0f/frameDeltaTime), frameDeltaTime);
        lastFrameTime = glfwGetTime();
    }

    // Wait till device finished
    vkDeviceWaitIdle(logicalDevice);

    // Shutdown vulkan
    shutdownVulkan();

    // Destroy GLFW window
    glfwDestroyWindow(window);

    // Stop GLFW
    glfwTerminate();

    return 0;
}
