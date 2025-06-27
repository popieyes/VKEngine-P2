#pragma once

#include "camera.h"
#include "common.h"
#include "transform.h"

namespace MiniEngine
{
struct Runtime;

class Light final
{
  public:
    enum class LightType : uint32_t
    {
        Directional,
        Point,
        Ambient,
        Count,
        Undefined
    };

    explicit Light(const Runtime &i_runtime) : m_runtime(i_runtime) {};
    ~Light() = default;

    bool initialize()
    {
        return true;
    }

    void shutdown()
    {
    }

    static std::shared_ptr<Light> createLight(const Runtime &i_runtime, const pugi::xml_node &emitter);
    static Matrix4f getLightSpaceMatrix(std::shared_ptr<Light> i_light, Camera &i_camera);

    // we use this structure to define the light uniform buffer
    struct LightData
    {
        LightType m_type;
        Vector3f m_radiance;
        Vector3f m_position;
        Vector3f m_attenuation;
        // For shadows
        Vector3f m_target = {0, 0, 0};
        float m_fov = 75.0f;
        float m_near = 0.01f;
        float m_far = 10.0f;
    };

    LightData m_data;

  private:
    Light(const Light &) = delete;
    Light &operator=(const Light &) = delete;

    const Runtime &m_runtime;
};
}; // namespace MiniEngine