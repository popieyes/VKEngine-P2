#include <Windows.h>
#include "vulkan/windowVK.h"
#include "vulkan/rendererVK.h"
#include "vulkan/deviceVK.h"
#include "common.h"

using namespace MiniEngine;


WindowVK::WindowVK( const RendererVK& i_renderer, const std::string& i_name, const uint32_t i_width, const uint32_t i_height ) :
    m_renderer           ( i_renderer                        ),
    m_name               ( i_name                            ),
    m_width              ( i_width                           ),
    m_height             ( i_height                          ),
    m_window             ( nullptr                           ),
    m_color_format       ( VK_FORMAT_B8G8R8A8_SRGB           ),
    m_depth_format       ( VK_FORMAT_D32_SFLOAT_S8_UINT      ),
    m_color_space        ( VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ),
    m_fullscreen         ( false                             ),
    m_prepared           ( false                             ),              
    m_surface            ( VK_NULL_HANDLE                    ),
    m_swap_chain         ( VK_NULL_HANDLE                    ),
    m_image_index        ( 0                                 ),
    m_image_count        ( 0                                 )
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_window = glfwCreateWindow( m_width, m_height, "MiniEngine", nullptr, nullptr);

}


uint32 WindowVK::prepareFrame( VkSemaphore i_presentation_semaphore )
{
    // Acquire the next image from the swap chain
    VkResult result = vkAcquireNextImageKHR( m_renderer.getDevice()->getLogicalDevice(), m_swap_chain, UINT64_MAX, i_presentation_semaphore, ( VkFence )nullptr, &m_image_index );

    return result;
}


uint32 WindowVK::renderFrame( VkSemaphore i_presentation_semaphore )
{
    return queuePresent( i_presentation_semaphore );
}


void WindowVK::createSwapChain()
{
    // Store the current swap chain handle so we can use it later on to ease up recreation
    VkSwapchainKHR old_swap_chain = m_swap_chain;

    // Get physical device surface properties and formats
    VkSurfaceCapabilitiesKHR surf_caps;
    if(  vkGetPhysicalDeviceSurfaceCapabilitiesKHR( m_renderer.getDevice()->getPhysicalDevice(), m_surface, &surf_caps ) )
    {
        throw MiniEngineException("Error getting capabilities");
    }

    // Get available present modes
    uint32_t present_mode_count = 0;
    
    if( vkGetPhysicalDeviceSurfacePresentModesKHR( m_renderer.getDevice()->getPhysicalDevice(), m_surface, &present_mode_count, 0 ) )
    {
        throw MiniEngineException( "Error getting present modes" );
    }

    std::vector<VkPresentModeKHR> present_modes( present_mode_count );
    
    if( vkGetPhysicalDeviceSurfacePresentModesKHR( m_renderer.getDevice()->getPhysicalDevice(), m_surface, &present_mode_count, present_modes.data() ) )
    {
        throw MiniEngineException( "Error getting present modes" );
    }

    VkExtent2D swapchain_extent = {};
    // If width (and height) equals the special value 0xFFFFFFFF, the size of the surface will be set by the swapchain
    if( surf_caps.currentExtent.width == ( uint32_t )-1 )
    {
        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        swapchain_extent.width  = m_width;
        swapchain_extent.height = m_height;
    }
    else
    {
        // If the surface size is defined, the swap chain size must match
        swapchain_extent = surf_caps.currentExtent;
        m_width  = surf_caps.currentExtent.width;
        m_height = surf_caps.currentExtent.height;
    }


    // Select a present mode for the swapchain

    // The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
    // This mode waits for the vertical blank ("v-sync")
    VkPresentModeKHR swapchain_resent_mode = VK_PRESENT_MODE_FIFO_KHR;

#ifndef VSYNC
    // If v-sync is not requested, try to find a mailbox mode
    // It's the lowest latency non-tearing present mode available
    
    for( size_t i = 0; i < present_modes.size(); i++ )
    {
        if( present_modes[ i ] == VK_PRESENT_MODE_MAILBOX_KHR )
        {
            swapchain_resent_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
        if( present_modes[ i ] == VK_PRESENT_MODE_IMMEDIATE_KHR )
        {
            swapchain_resent_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }
#endif

    // Determine the number of images
    uint32_t desired_number_of_swapchain_images = surf_caps.minImageCount + 1;
    if( ( surf_caps.maxImageCount > 0 ) && ( desired_number_of_swapchain_images > surf_caps.maxImageCount ) )
    {
        desired_number_of_swapchain_images = surf_caps.maxImageCount;
    }

    // Find the transformation of the surface
    VkSurfaceTransformFlagsKHR pre_transform;
    if( surf_caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR )
    {
        // We prefer a non-rotated transform
        pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else
    {
        pre_transform = surf_caps.currentTransform;
    }

    // Find a supported composite alpha format (not all devices support alpha opaque)
    VkCompositeAlphaFlagBitsKHR composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    // Simply select the first composite alpha format available
    std::vector<VkCompositeAlphaFlagBitsKHR> composite_alpha_flags = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };
    for( auto& composite_alpha_flag : composite_alpha_flags )
    {
        if( surf_caps.supportedCompositeAlpha & composite_alpha_flag )
        {
            composite_alpha = composite_alpha_flag;
            break;
        };
    }

    VkSwapchainCreateInfoKHR swapchain_CI{};
    swapchain_CI.sType                  = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_CI.surface                = m_surface;
    swapchain_CI.minImageCount          = desired_number_of_swapchain_images;
    swapchain_CI.imageFormat            = m_color_format;
    swapchain_CI.imageColorSpace        = m_color_space;
    swapchain_CI.imageExtent =          { swapchain_extent.width, swapchain_extent.height };
    swapchain_CI.imageUsage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_CI.preTransform           = ( VkSurfaceTransformFlagBitsKHR )pre_transform;
    swapchain_CI.imageArrayLayers       = 1;
    swapchain_CI.imageSharingMode       = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_CI.queueFamilyIndexCount  = 0;
    swapchain_CI.presentMode            = swapchain_resent_mode;
    // Setting oldSwapChain to the saved handle of the previous swapchain aids in resource reuse and makes sure that we can still present already acquired images
    swapchain_CI.oldSwapchain           = old_swap_chain;
    // Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
    swapchain_CI.clipped                = VK_TRUE;
    swapchain_CI.compositeAlpha         = composite_alpha;

    // Enable transfer source on swap chain images if supported
    if( surf_caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT )
    {
        swapchain_CI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    // Enable transfer destination on swap chain images if supported
    if( surf_caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT )
    {
        swapchain_CI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    if( vkCreateSwapchainKHR( m_renderer.getDevice()->getLogicalDevice(), &swapchain_CI, nullptr, &m_swap_chain ) )
    {
        throw MiniEngineException("cannot create swap chain");
    }

    // If an existing swap chain is re-created, destroy the old swap chain
    // This also cleans up all the presentable images
    if( old_swap_chain != VK_NULL_HANDLE )
    {
        vkDestroySwapchainKHR( m_renderer.getDevice()->getLogicalDevice(), old_swap_chain, nullptr );
    }
    
    if( vkGetSwapchainImagesKHR( m_renderer.getDevice()->getLogicalDevice(), m_swap_chain, &m_image_count, NULL ) )
    {
        throw MiniEngineException( "cannot get swap chain images" );
    }

    // Get the swap chain images
    std::vector<VkImage> images( m_image_count );
    if( vkGetSwapchainImagesKHR( m_renderer.getDevice()->getLogicalDevice(), m_swap_chain, &m_image_count, images.data() ) )
    {
        throw MiniEngineException( "cannot get swap chain images" );
    }


    // Get the swap chain buffers containing the image and imageview
    for( uint32_t i = 0; i < m_image_count; i++ )
    {
        VkImageViewCreateInfo colorAttachmentView = {};
        colorAttachmentView.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        colorAttachmentView.pNext                           = NULL;
        colorAttachmentView.format                          = m_swap_chain_images[ i ].m_format = m_color_format;
        colorAttachmentView.components                      = {
                                                                   VK_COMPONENT_SWIZZLE_R,
                                                                   VK_COMPONENT_SWIZZLE_G,
                                                                   VK_COMPONENT_SWIZZLE_B,
                                                                   VK_COMPONENT_SWIZZLE_A
                                                               };
        colorAttachmentView.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        colorAttachmentView.subresourceRange.baseMipLevel   = 0;
        colorAttachmentView.subresourceRange.levelCount     = 1;
        colorAttachmentView.subresourceRange.baseArrayLayer = 0;
        colorAttachmentView.subresourceRange.layerCount     = 1;
        colorAttachmentView.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        colorAttachmentView.flags                           = 0;
        colorAttachmentView.image                           =  m_swap_chain_images[ i ].m_image = images[ i ];

        if( vkCreateImageView( m_renderer.getDevice()->getLogicalDevice(), &colorAttachmentView, nullptr, &m_swap_chain_images[ i ].m_image_view ) )
        {
            throw MiniEngineException( "cannot create swap chain image views" );
        }
    }
}



VkResult WindowVK::queuePresent( VkSemaphore i_presentation_semaphore )
{
    VkPresentInfoKHR present_info{};
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext              = VK_NULL_HANDLE;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &m_swap_chain;
    present_info.pImageIndices      = &m_image_index;
  
    // Check if a wait semaphore has been specified to wait for before presenting the image
    if( i_presentation_semaphore != VK_NULL_HANDLE )
    {
        present_info.pWaitSemaphores = &i_presentation_semaphore;
        present_info.waitSemaphoreCount = 1;
    }

    return vkQueuePresentKHR( m_renderer.getDevice()->getGraphicsQueue(), &present_info );
}



void WindowVK::createSurface()
{   
    if( glfwCreateWindowSurface( m_renderer.getInstance(), m_window, nullptr, &m_surface ) )
    if( glfwCreateWindowSurface( m_renderer.getInstance(), m_window, nullptr, &m_surface ) )
    {
        throw MiniEngineException( "cannot create the surface" );
    }
}



bool WindowVK::loop()
{
    glfwPollEvents();
    return !glfwWindowShouldClose( m_window );
}


void WindowVK::wait() 
{
    //wait till minimize finishes
    int32 height = 0, width = 0;
    while( width == 0 || height == 0 ) 
    {
        glfwGetFramebufferSize( m_window, reinterpret_cast<int32*>( &width ), reinterpret_cast<int32*>( &height ) );
        glfwWaitEvents();
    }
}


void WindowVK::resize()
{
    for( uint32_t idx = 0; idx < m_image_count; idx++ )
    {
        vkDestroyImageView( m_renderer.getDevice()->getLogicalDevice(), m_swap_chain_images [ idx ].m_image_view , nullptr );
    }

    glfwGetFramebufferSize( m_window, reinterpret_cast<int32*>( &m_width ), reinterpret_cast<int32*>( &m_height ));

    glfwSetWindowSize( m_window, static_cast<int32_t>( m_width ), static_cast<int32_t>( m_height ) );
        
    createSwapChain   ();
}



void WindowVK::resize( const uint32_t i_width, const uint32_t i_height )
{
    glfwSetWindowSize( m_window, i_width, i_height );    
}


void WindowVK::destroySwapChain()
{
    for( uint32_t idx = 0; idx < m_image_count; idx++ )
    {
        vkDestroyImageView( m_renderer.getDevice()->getLogicalDevice(), m_swap_chain_images[ idx ].m_image_view, nullptr );
    }

    vkDestroySwapchainKHR( m_renderer.getDevice()->getLogicalDevice(), m_swap_chain, nullptr );
}



void WindowVK::destroySurface()
{
    vkDestroySurfaceKHR( m_renderer.getInstance(), m_surface, nullptr );
}