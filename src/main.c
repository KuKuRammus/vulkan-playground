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

// Required validation layers


// ------ END GLOBALS

b32 checkVulkanValidationLayerSupport() {
    printf("Checking if required Vulkan layers are present\n");

    // Declare required validation layers
    // TODO: Hard coded array size. Fix required
    const char *requiredLayers[1] = {
            "VK_LAYER_KHRONOS_validation"
    };

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
            if (strcmp(requiredLayers[i], vulkanLayers[j].layerName) == 0) {
                layerIsFound = QQ_TRUE;
                break;
            }
        }

        if (layerIsFound == QQ_FALSE) {
            printf("Layer %s was required, but wasn't found\n", requiredLayers[i]);
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
        createInfo.enabledLayerCount = 0;

//        createInfo.enabledLayerCount = sizeof(vulkanLayers) / sizeof(vulkanLayers[0]);
//        createInfo.ppEnabledLayerNames = vulkanLayers;
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
}

void shutdownVulkan() {
    printf("Shutting down Vulkan\n");
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
