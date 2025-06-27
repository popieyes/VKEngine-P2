#pragma once

#include "common.h"

namespace MiniEngine
{
    class RendererVK;

    class DeviceVK final
    {
    public:
        explicit DeviceVK( const RendererVK& i_renderer );
        ~DeviceVK();

        VkPhysicalDevice getPhysicalDevice() const
        {
            return m_physical_device;
        }

        VkDevice getLogicalDevice() const
        {
            return m_logical_device;
        }

        VkQueue getGraphicsQueue() const
        {
            return m_graphics_queue;
        }

        VkCommandPool getCommandPool() const
        {
            return m_command_pool;
        }

        uint32_t getMemoryTypeIndex( uint32_t typeBits, VkMemoryPropertyFlags properties ) const;

    private:
        DeviceVK( const DeviceVK& ) = delete;
        DeviceVK& operator=(const DeviceVK& ) = delete;

        VkPhysicalDevice m_physical_device;
        VkDevice         m_logical_device;
    
        void createPhysicalDevice();
        void createDevice        ();
        void createCommandPool   ();

        void destroyDevice();
        void destroyCommandPool();

        uint32_t getQueueFamilyIndex( VkQueueFlagBits i_queue_flags ) const;
    
        const RendererVK& m_renderer;

        uint32_t                                         m_graphics_queue_index;
        VkCommandPool                                    m_command_pool;
        VkQueue                                          m_graphics_queue;
        VkPhysicalDeviceProperties                       m_phyisical_device_properties;
        VkPhysicalDeviceFeatures                         m_physical_device_features;
        VkPhysicalDeviceMemoryProperties                 m_physical_device_memory_properties;
        VkPhysicalDeviceProperties2                      m_phyisical_device_properties2;
        VkPhysicalDeviceFeatures2                        m_physical_device_features2;
        VkPhysicalDeviceMemoryProperties2                m_physical_device_memory_properties2;
        std::vector<VkQueueFamilyProperties>             m_queue_family_properties;
        std::vector<std::string>                         m_supported_extensions;
        std::vector<const char*>                         m_extensions;

        friend class RendererVK;
    };
}