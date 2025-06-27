#pragma once

#include "common.h"

namespace MiniEngine
{
    class RendererVK;

    class WindowVK final
    {
    public:
        WindowVK( const RendererVK& i_renderer, const std::string& i_name, const uint32_t i_width, const uint32_t i_height );
        ~WindowVK() = default;

        uint32 prepareFrame( VkSemaphore i_presentation_semaphore );
        uint32 renderFrame ( VkSemaphore i_presentation_semaphore );
        
        GLFWwindow* getWindow() const
        {
            return m_window;
        }

        VkFormat getColorFormat() const
        {
            return m_color_format;
        }

        VkColorSpaceKHR getColorSpace() const
        {
            return m_color_space;
        }

        std::array<ImageBlock, 3> getSwapChainImages() const
        {
            return m_swap_chain_images;
        }

        VkFormat getDepthFormat() const
        {
            return m_depth_format;
        }

        VkSurfaceKHR getSurface() const
        {
            return m_surface;
        }

        uint32_t getCurrentImageId() const
        {
            return m_image_index;
        }

        void getWindowSize( uint32_t& o_width, uint32_t& o_height ) const
        {
            o_width  = m_width;
            o_height = m_height;
        }

        uint32 getImageCount() const
        {
            return m_image_count;
        }

    private:
        WindowVK( const WindowVK& ) = delete;
        WindowVK& operator=(const WindowVK& ) = delete;

        void createSwapChain   ();
        void createSurface     ();

        void destroySwapChain   ();
        void destroySurface     ();

        bool loop   ();
        void wait   ();
        void resize ();
        void resize ( const uint32_t i_width, const uint32_t i_height );

        

        VkResult queuePresent      ( VkSemaphore i_presentation_semaphore );

        const RendererVK&                            m_renderer;
        std::string                                  m_name;
        uint32_t                                     m_width;
        uint32_t                                     m_height;
        VkSwapchainKHR                               m_swap_chain;
        VkSurfaceKHR                                 m_surface;
        VkFormat                                     m_color_format;
        VkFormat                                     m_depth_format;
        VkColorSpaceKHR                              m_color_space;
        uint32_t                                     m_image_count;
        uint32_t                                     m_image_index;
        std::array<ImageBlock,                    3> m_swap_chain_images;
        uint32_t                                     m_queue_node_index = 0xFFFFFFFF;
        bool                                         m_prepared;
        bool                                         m_fullscreen;
        
        //window
        GLFWwindow*      m_window;

        friend class RendererVK;
        friend class Engine;
    };
};