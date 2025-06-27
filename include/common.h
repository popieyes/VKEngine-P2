#pragma once


#if defined(_MSC_VER)
/* Disable some warnings on MSVC++ */
#pragma warning(disable : 4127 4702 4100 4515 4800 4146 4512)
#define WIN32_LEAN_AND_MEAN     /* Don't ever include MFC on Windows */
#define NOMINMAX                /* Don't override min/max */
#endif

#include "defines.h"

/* "Ray epsilon": relative error threshold for ray intersection computations */
#define Epsilon 1e-4f

/* A few useful constants */
#undef M_PI

#define M_PI         3.14159265358979323846f
#define INV_PI       0.31830988618379067154f
#define INV_TWOPI    0.15915494309189533577f
#define INV_FOURPI   0.07957747154594766788f
#define SQRT_TWO     1.41421356237309504880f
#define INV_SQRT_TWO 0.70710678118654752440f


namespace MiniEngine
{
/// Simple exception class, which stores a human-readable error description
class MiniEngineException : public std::runtime_error 
{
public:
    /// Variadic template constructor to support printf-style arguments
    template <typename... Args> MiniEngineException(const char *fmt, const Args &... args)
     : std::runtime_error(tfm::format(fmt, args...)) { }
};

std::vector<std::string> tokenize(const std::string &string, const std::string &delim = ", ", bool includeEmpty = false);

std::string toLower(const std::string &value);

bool toBool(const std::string &str);

int32 toInt(const std::string &str);

uint32 toUInt(const std::string &str);

float toFloat(const std::string &str);

Vector2f toVector2f(const std::string &str);

Vector3f toVector3f(const std::string &str);

inline float radToDeg(float value) { return value * ( 180.0f / kPI); }

/// Convert degrees to radians
inline float degToRad(float value) { return value * ( kPI / 180.0f); }

struct Vertex
{
    Vector3f m_position;
    Vector3f m_normal;
    Vector2f m_uv;

    Vertex() :
        m_position( { 0.0f, 0.0f, 0.0f } ),
        m_normal  ( { 0.0f, 1.0f, 0.0f } ),
        m_uv      ( { 0.0f, 0.0f       } )
    {}
};

struct ImageBlock
{
    VkImage        m_image      = VK_NULL_HANDLE;
    VkImageView    m_image_view = VK_NULL_HANDLE;
    VkFormat       m_format;
    VkDeviceMemory m_memory     = VK_NULL_HANDLE;
    VkSampler      m_sampler    = VK_NULL_HANDLE;
};

struct Attachments
{
    //GBUFFER
    ImageBlock m_color_attachment;
    ImageBlock m_position_depth_attachment;
    ImageBlock m_normal_attachment;
    ImageBlock m_material_attachment;
    ImageBlock m_depth_attachment;

    //ADDITIONAL RENDER TARGET
    ImageBlock m_ssao_attachment;
    ImageBlock m_ssao_blur_attachment;
};

};
