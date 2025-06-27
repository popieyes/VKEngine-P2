#include "light.h"
#include "meshRegistry.h"

using namespace MiniEngine;

std::shared_ptr<Light> Light::createLight(const Runtime &i_runtime, const pugi::xml_node &emitter)
{
    // for now we convert area lights into pointlights
    auto light = std::make_shared<Light>(i_runtime);

    if (strcmp(emitter.attribute("type").value(), "ambient") == 0)
    {
        light->m_data.m_type = LightType::Ambient;
    }
    else if (strcmp(emitter.attribute("type").value(), "directional") == 0)
    {
        light->m_data.m_type = LightType::Directional;
        auto node = emitter.find_child_by_attribute("name", "direction");

        if (!node)
        {
            throw MiniEngineException("Direction undefinned");
        }

        light->m_data.m_position = normalize(toVector3f(node.attribute("value").value()));
    }
    else if (strcmp(emitter.attribute("type").value(), "point") == 0)
    {
        light->m_data.m_type = LightType::Point;
        auto node = emitter.find_child_by_attribute("name", "attenuation");

        if (node)
        {
            light->m_data.m_attenuation = normalize(toVector3f(node.attribute("value").value()));
        }

        node = emitter.find_child_by_attribute("name", "position");

        if (node)
        {
            light->m_data.m_position = toVector3f(node.attribute("value").value());
        }
    }

    // radiance and transform
    if (!emitter.find_child_by_attribute("name", "radiance"))
    {
        throw MiniEngineException("Radiance undefinned");
    }

    light->m_data.m_radiance =
        toVector3f(emitter.find_child_by_attribute("name", "radiance").attribute("value").value());
    light->initialize();

    return light;
}

Matrix4f MiniEngine::Light::getLightSpaceMatrix(std::shared_ptr<Light> i_light, Camera &i_camera)
{

    switch (i_light->m_data.m_type)
    {
    case LightType::Point:

        Matrix4f projectionMatrix =
            glm::perspective(glm::radians(i_light->m_data.m_fov), 1.0f, i_light->m_data.m_near, i_light->m_data.m_far);
        Matrix4f viewMatrix = glm::lookAt(i_light->m_data.m_position, i_light->m_data.m_target, Vector3f(0, 1, 0));
        return projectionMatrix * viewMatrix;

        break;

    case LightType::Directional:

        // Lambda for getting FRUSTRUM corners
        auto getFrustumCornersWorldSpace = [](const Matrix4f projview) {
            const auto inv = glm::inverse(projview);

            std::vector<Vector4f> frustumCorners;
            for (unsigned int x = 0; x < 2; ++x)
            {
                for (unsigned int y = 0; y < 2; ++y)
                {
                    for (unsigned int z = 0; z < 2; ++z)
                    {
                        const Vector4f pt = inv * Vector4f(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                        frustumCorners.push_back(pt / pt.w);
                    }
                }
            }

            return frustumCorners;
        };

        const auto corners = getFrustumCornersWorldSpace(i_camera.getProjection() * i_camera.getView());

        Vector3f center = Vector3f(0, 0, 0);
        for (const auto &v : corners)
        {
            center += Vector3f(v);
        }
        center /= corners.size();

        const auto lightView = glm::lookAt(center - i_light->m_data.m_position, center, Vector3f(0.0f, 1.0f, 0.0f));

        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();
        float minZ = std::numeric_limits<float>::max();
        float maxZ = std::numeric_limits<float>::lowest();
        for (const auto &v : corners)
        {
            const auto trf = lightView * v;
            minX = std::min(minX, trf.x);
            maxX = std::max(maxX, trf.x);
            minY = std::min(minY, trf.y);
            maxY = std::max(maxY, trf.y);
            minZ = std::min(minZ, trf.z);
            maxZ = std::max(maxZ, trf.z);
        }

        // Tune this parameter according to the scene
        constexpr float zMult = 5.0f;
        if (minZ < 0)
        {
            minZ *= zMult;
        }
        else
        {
            minZ /= zMult;
        }
        if (maxZ < 0)
        {
            maxZ /= zMult;
        }
        else
        {
            maxZ *= zMult;
        }

        const Matrix4f lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
        return lightProjection * lightView;

        break;
    }
    return Matrix4f();
}
