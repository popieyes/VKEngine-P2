#include "common.h"
#include "vulkan/utilsVK.h"
#include "vulkan/deviceVK.h"
#include "vulkan/rendererVK.h"
#include "vulkan/windowVK.h"

using namespace MiniEngine;

void DestroyDebugUtilsMessengerEXT( VkInstance instance, VkDebugUtilsMessengerEXT* pDebugMessenger, const VkAllocationCallbacks* pAllocator )
{
    auto func = ( PFN_vkDestroyDebugUtilsMessengerEXT )vkGetInstanceProcAddr( instance, "vkDestroyDebugUtilsMessengerEXT" );
    if( func != nullptr )
    {
        func( instance, *pDebugMessenger, pAllocator );
    }
}


VkResult CreateDebugUtilsMessengerEXT( VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger )
{
    auto func = ( PFN_vkCreateDebugUtilsMessengerEXT )vkGetInstanceProcAddr( instance, "vkCreateDebugUtilsMessengerEXT" );
    if( func != nullptr )
    {
        return func( instance, pCreateInfo, pAllocator, pDebugMessenger );
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT( VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator )
{
    auto func = ( PFN_vkDestroyDebugUtilsMessengerEXT )vkGetInstanceProcAddr( instance, "vkDestroyDebugUtilsMessengerEXT" );
    if( func != nullptr )
    {
        func( instance, debugMessenger, pAllocator );
    }
}

VkDebugUtilsMessengerEXT            debugUtilsMessenger;

VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData )
{
    // Select prefix depending on flags passed to the callback
    std::string prefix( "" );

    if( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT )
    {
        prefix = "VERBOSE: ";
    }
    else if( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT )
    {
        prefix = "INFO: ";
    }
    else if( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )
    {
        prefix = "WARNING: ";
    }
    else if( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT )
    {
        prefix = "ERROR: ";
    }
    
    std::cerr << prefix << " validation layer: " << pCallbackData->pMessage << std::endl;
     
    return VK_FALSE;
}



bool RendererVK::initialize()
{
    //we create the glfw window
    m_window = std::make_unique<WindowVK>( *this, "Mini Engine", 800, 800  );
    m_device = std::make_shared<DeviceVK>( *this );

    //get the vulkan instance
    createInstance                  ();
    m_window->createSurface         ();
    m_device->createPhysicalDevice  ();
    m_device->createDevice          ();
    m_window->createSwapChain       ();
    m_device->createCommandPool     (); 

    return true;
}


void RendererVK::shutdown()
{
    m_device->destroyCommandPool();
    m_window->destroySwapChain  ();
    m_window->destroySurface    ();
    m_device->destroyDevice     ();

    DestroyDebugUtilsMessengerEXT( m_instance, debugUtilsMessenger, nullptr );

    vkDestroyInstance( m_instance, nullptr );
}


void RendererVK::createInstance()
{
    // Validation can also be forced via a define
#if defined(DEBUG)
    bool enable_validation_layers = true;
#else
    bool enable_validation_layers = false;
#endif

    VkApplicationInfo app_info = {};
    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName   = "Practica 5";
    app_info.pEngineName        = "Mini Engine";
    app_info.apiVersion         = VK_MAKE_VERSION( 1, 2, 0 );

    std::vector<const char*> extensions;
    extensions.push_back            ( VK_KHR_SURFACE_EXTENSION_NAME       );
    extensions.push_back            ( VK_KHR_WIN32_SURFACE_EXTENSION_NAME );

    // Get extensions supported by the instance and store for later use
    uint32_t count = 0;
    vkEnumerateInstanceExtensionProperties( nullptr, &count, nullptr );

    if( count > 0 )
    {
        std::vector<VkExtensionProperties> instance_extensions;
        instance_extensions.resize( count );

        if( vkEnumerateInstanceExtensionProperties( nullptr, &count, &instance_extensions.front() ) == VK_SUCCESS )
        {
            for( size_t idx = 0; idx < instance_extensions.size(); idx++ )
            {
                m_extensions_supported.push_back( instance_extensions[ idx ].extensionName );
            }
        }
    }

    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pNext = NULL;
    instance_create_info.pApplicationInfo = &app_info;

    if( extensions.size() > 0 )
    {
        if( enable_validation_layers )
        {
            extensions.push_back            ( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
        }
        instance_create_info.enabledExtensionCount   = ( uint32_t )extensions.size();
        instance_create_info.ppEnabledExtensionNames = extensions.data();
    }

    // The VK_LAYER_KHRONOS_validation contains all current validation functionality
    const char* validation_layer_name = "VK_LAYER_KHRONOS_validation";
    if( enable_validation_layers )
    {
        // Check if this layer is available at instance level
        uint32_t instance_layer_count;
        vkEnumerateInstanceLayerProperties( &instance_layer_count, nullptr );
        std::vector<VkLayerProperties> instance_layer_properties( instance_layer_count );
        vkEnumerateInstanceLayerProperties( &instance_layer_count, instance_layer_properties.data() );
        bool validation_layer_present = false;

        for( VkLayerProperties layer : instance_layer_properties )
        {
            if( strcmp( layer.layerName, validation_layer_name ) == 0 )
            {
                validation_layer_present = true;
                break;
            }
        }
        if( validation_layer_present )
        {
            instance_create_info.ppEnabledLayerNames = &validation_layer_name;
            instance_create_info.enabledLayerCount   = 1;
        }
        else
        {
            std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled";
        }
    }
    
    if( vkCreateInstance( &instance_create_info, nullptr, &m_instance ) )
    {
        throw MiniEngineException("Error creating Vulkan instance");
    }


#ifdef DEBUG
    VkDebugUtilsMessengerCreateInfoEXT create_info;
    create_info.sType               = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity     = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType         = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback     = debugUtilsMessengerCallback;
    create_info.pNext               = NULL;
    create_info.flags               = 0;


    if( CreateDebugUtilsMessengerEXT( m_instance, &create_info, nullptr, &debugUtilsMessenger ) != VK_SUCCESS )
    {
        throw MiniEngineException( "failed to set debug messages" );
    }



#endif
}
