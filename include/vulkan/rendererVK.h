#pragma once

struct GLFWwindow;

namespace MiniEngine
{
    class DeviceVK;
    class WindowVK;

    class RendererVK
    {
    public:
        explicit RendererVK() : 
            m_instance( VK_NULL_HANDLE ),
            m_app_name( "Mini Engine"  )
        {}
        ~RendererVK() = default;

        bool initialize();
        void shutdown  ();

        VkInstance getInstance() const
        {
            return m_instance;
        }

        std::shared_ptr<DeviceVK> getDevice() const
        {
            return m_device;
        }

        WindowVK& getWindow() const
        {
            return *m_window;
        }

    private:
        RendererVK( const RendererVK& ) = delete;
        RendererVK& operator=(const RendererVK& ) = delete;

        void createInstance();
        std::string                         m_app_name;
        VkInstance                          m_instance;
        std::shared_ptr<DeviceVK>           m_device;
        std::shared_ptr<WindowVK>           m_window;
        std::vector<const char*>            m_extensions_supported;
        
    };
};

