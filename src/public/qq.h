#pragma once

// Math
#include <cglm/vec2.h>
#include <cglm/vec3.h>
#include <cglm/mat4.h>

// Custom primitive types definitions
#include <stdint.h>

// Signed integers
#define U32_MAX UINT32_MAX
#define U64_MAX UINT64_MAX
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

// Unsigned integers
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// Floats
typedef float f32;
typedef double f64;

// Booleans
typedef uint32_t b32;
typedef uint64_t b64;

// Project-scope boolean values
#define QQ_TRUE 1
#define QQ_FALSE 0

// Descriptor - UniformBufferObject (UBO)
typedef struct {
    mat4 model;
    mat4 view;
    mat4 projection;
} UniformBufferObject;

// New vertex implementation
typedef struct {
    vec3 position;
    vec3 color;
    vec2 uv;
} Vertex;

// Shader code
typedef struct {
    u8* data;
    i64 size;
} VulkanShaderCode;

// Vulkan swap chain properties
typedef struct {
    VkSurfaceCapabilitiesKHR capabilities;
    
    u32 formatCount; 
    VkSurfaceFormatKHR* formats;
    
    u32 presentModeCount;
    VkPresentModeKHR* presentModes;
} SwapchainSupportDetails;

// Vulkan queue families
typedef struct {
    // Graphics
    b32 isGraphicsSet;
    u32 graphics;

    // Presentation
    b32 isPresentSet;
    u32 present;
} QueueFamilyIndices;


