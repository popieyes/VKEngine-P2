#pragma once


#if defined(_MSC_VER)
/* Disable some warnings on MSVC++ */
#pragma warning(disable : 4127 4702 4100 4515 4800 4146 4512)
#define WIN32_LEAN_AND_MEAN     /* Don't ever include MFC on Windows */
#define NOMINMAX                /* Don't override min/max */
#endif

#include <stdio.h>
#include <time.h>
#include <algorithm>
#include <iostream>
#include <array>
#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>
#include <fstream>
#include <set>
#include <map>
#include <cstdint>
#include <memory>
#include <iterator>
#include <iostream>
#include <assert.h>
#include <random>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/hash.hpp>
#include <pugixml.hpp>
#include <tinyformat/tinyformat.h>
#include <tiny_obj_loader.h>


// GRAPHIC API
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


namespace MiniEngine
{
    typedef int8_t int8;
    typedef int32_t int32;
    typedef int64_t int64;
    typedef uint8_t uint8;
    typedef uint32_t uint32;
    typedef uint64_t uint64;

    typedef glm::vec2 Vector2f;
    typedef glm::vec3 Vector3f;
    typedef glm::vec4 Vector4f;

    typedef glm::quat Quaternion;

    typedef glm::mat3 Matrix3f;
    typedef glm::mat4 Matrix4f;

    constexpr float kPI = 3.14159265358979323846f;
    constexpr float k2PI = 2.f *  3.14159265358979323846f;
    constexpr float kINV_PI = 1.0f / kPI;
	constexpr float kINV_2PI = 1.0f / ( k2PI );
	constexpr float kINV_4PI = 1.0f / ( 4.f * kPI );
	constexpr float kEPSILON = 1e-8f;
	constexpr float kINFINITY = std::numeric_limits<float>::infinity();
    constexpr float kOFFSET = 0.0001f;
    constexpr float kSQRT_TWO = 1.41421356237309504880f;
    constexpr float kINV_SQRT_TWO = 1.f / kSQRT_TWO;
    constexpr uint32_t kMAX_NUMBER_LIGHTS = 10;
    constexpr uint32_t kMAX_NUMBER_OF_OBJECTS = 10000;
    constexpr uint32_t kMAX_NUMBER_OF_FRAMES = 3;
    constexpr uint32_t kSSAO_KERNEL_SIZE = 64;
    constexpr uint32_t kSSAO_NOISE_DIM = 4;

};