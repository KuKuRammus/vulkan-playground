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
VkQueue vulkanGraphicsQueue;

// Vulkan queue families
struct VulkanQueueFamilyIndices {
    b32 isGraphicsSet;
    u32 graphics;
};

// Required validation layers
const char *requiredVulkanLayers[1] = { "VK_LAYER_KHRONOS_validation" };

// ------ END GLOBALS

struct VulkanQueueFamilyIndices findVulkanQueueFamilies(VkPhysicalDevice device) {
    printf("Checking for required queue families\n");

    struct VulkanQueueFamilyIndices indices;

    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

    VkQueueFamilyProperties* queueFamilies = (VkQueueFamilyProperties *)malloc(
        queueFamilyCount * sizeof(VkQueueFamilyProperties)
    );

    for (u32 i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags && VK_QUEUE_GRAPHICS_BIT) {
            // Graphics queue is supported
            indices.isGraphicsSet = QQ_TRUE;
            indices.graphics = i;
        }
    }

    free(queueFamilies);

    return indices;
}

void createVulkanLogicalDevice() {
    printf("Creating vulkan logical device\n");
    struct VulkanQueueFamilyIndices indices = findVulkanQueueFamilies(vulkanPhysicalDevice);

    // Create required queues (already checked for availability)
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indices.graphics;
    queueCreateInfo.queueCount = 1;

    // Declare queue priority
    f32 queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    // Declare required device features (already checked for availability)
    // Empty for now
    VkPhysicalDeviceFeatures deviceFeatures = {};

    // Create logical device
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;
    // TODO: Research about virtual device layers/extensions (is it deprecated?)

    if (
        vkCreateDevice(vulkanPhysicalDevice, &createInfo, NULL, &vulkanDevice) != VK_SUCCESS
    ) {
        printf("[ERR] Failed to create vulkan virtual device");
    }

    // Get handle for graphics queue
    vkGetDeviceQueue(vulkanDevice, indices.graphics, 0, &vulkanGraphicsQueue);
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

    return (allLayersIsFound == QQ_FALSE);
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
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "qq";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Structure describing requirements from Vulkan instance
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Describe extensions required
    // For now, just fetch all extensions, required by GLFW
    u32 glfwExtensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;

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

    printf("PING\n");
}

void initVulkan() {
    printf("Initializing Vulkan\n");
    createVulkanInstance();
    pickVulkanPhysicalDevice();
    createVulkanLogicalDevice();
}

void shutdownVulkan() {
    printf("Shutting down Vulkan\n");
    vkDestroyDevice(vulkanDevice, NULL);
    
    printg("Destroying Vulkan virtual device\n");
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
