#include "vulkan/deviceVK.h"
#include "vulkan/rendererVK.h"
#include "vulkan/windowVK.h"
#include "vulkan/utilsVK.h"
#include "common.h"




using namespace MiniEngine;

DeviceVK::DeviceVK( const RendererVK& i_renderer ) : 
    m_renderer                         ( i_renderer     ),
    m_graphics_queue_index             ( 0              ),
    m_command_pool                     ( VK_NULL_HANDLE ),
    m_graphics_queue                   ( VK_NULL_HANDLE ),
    m_phyisical_device_properties      ( {}             ),
    m_physical_device_features         ( {}             ),
    m_physical_device_memory_properties( {}             )
{}


DeviceVK::~DeviceVK()
{
}


uint32_t DeviceVK::getQueueFamilyIndex( VkQueueFlagBits i_queue_flags ) const
{
    //throw std::runtime_error( "Could not find a matching queue family index" );
    // Search for a graphics and a present queue in the array of queue
	// families, try to find one that supports both
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( m_physical_device, &queue_family_count, nullptr );

    std::vector<VkBool32> supports_present( queue_family_count);
	for (uint32_t i = 0; i < queue_family_count; i++) 
	{
		vkGetPhysicalDeviceSurfaceSupportKHR( m_physical_device, i, m_renderer.getWindow().getSurface(), &supports_present[i]);
	}

	uint32_t graphics_queue_node_index = UINT32_MAX;
	uint32_t present_queue_node_index  = UINT32_MAX;

    std::vector<VkQueueFamilyProperties> queue_props( queue_family_count );
	vkGetPhysicalDeviceQueueFamilyProperties( m_physical_device, &queue_family_count, queue_props.data() );

	for (uint32_t i = 0; i < queue_family_count; i++) 
	{
		if( ( queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 ) 
		{
			if(graphics_queue_node_index == UINT32_MAX) 
			{
				graphics_queue_node_index = i;
			}

			if( supports_present[i] == VK_TRUE ) 
			{
				graphics_queue_node_index = i;
				present_queue_node_index = i;
				break;
			}
		}
	}
	if( present_queue_node_index == UINT32_MAX ) 
	{	
		// If there's no queue that supports both present and graphics
		// try to find a separate present queue
		for( uint32_t i = 0; i < queue_family_count; ++i ) 
		{
			if( supports_present[ i ] == VK_TRUE ) 
			{
				present_queue_node_index = i;
				break;
			}
		}
	}

    return graphics_queue_node_index;
}


void DeviceVK::createPhysicalDevice()
{
    // Physical device
    uint32_t gpu_count = 0;
    // Get number of available physical devices
    if( vkEnumeratePhysicalDevices( m_renderer.getInstance(), &gpu_count, nullptr ) || 0 == gpu_count)
    {
        throw MiniEngineException( "Cannot enumerate phyisical devices" );
    }

    // Enumerate devices
    std::vector<VkPhysicalDevice> physical_devices( gpu_count );
    
    if( vkEnumeratePhysicalDevices( m_renderer.getInstance(), &gpu_count, physical_devices.data() ) )
    {
        throw MiniEngineException( "Cannot enumerate phyisical devices" );
    }

    // GPU selection

    // Select physical device to be used for the Vulkan example
    // Defaults to the first device unless specified by command line
    uint32_t selected_device = 0;

    m_physical_device = physical_devices[ selected_device ];

    m_phyisical_device_properties2.sType       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    m_physical_device_features2.sType          = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    m_physical_device_memory_properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;

    
    m_phyisical_device_properties2.pNext       = nullptr;
    m_physical_device_features2.pNext          = nullptr;
    m_physical_device_memory_properties2.pNext = nullptr;

    // Store properties (including limits), features and memory properties of the physical device (so that examples can check against them)
    vkGetPhysicalDeviceProperties       ( m_physical_device, &m_phyisical_device_properties        );
    vkGetPhysicalDeviceFeatures         ( m_physical_device, &m_physical_device_features           );
    vkGetPhysicalDeviceMemoryProperties ( m_physical_device, &m_physical_device_memory_properties  );
    vkGetPhysicalDeviceProperties2      ( m_physical_device, &m_phyisical_device_properties2       );
    vkGetPhysicalDeviceFeatures2        ( m_physical_device, &m_physical_device_features2          );
    vkGetPhysicalDeviceMemoryProperties2( m_physical_device, &m_physical_device_memory_properties2 );
    
    //carwe can add here new features if we need
    //

    // Queue family properties, used for setting up requested queues upon device creation
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( m_physical_device, &queue_family_count, nullptr );

    if( 0 == queue_family_count )
    {
        throw MiniEngineException("no queues");
    }

    m_queue_family_properties.resize( queue_family_count );
    vkGetPhysicalDeviceQueueFamilyProperties( physical_devices[ selected_device ], &queue_family_count, m_queue_family_properties.data() );
    
    // Get list of supported extensions
    uint32_t ext_count = 0;
    vkEnumerateDeviceExtensionProperties( m_physical_device, nullptr, &ext_count, nullptr );
    if( ext_count > 0 )
    {
        std::vector<VkExtensionProperties> extensions( ext_count );
        if( vkEnumerateDeviceExtensionProperties( m_physical_device, nullptr, &ext_count, &extensions.front() ) == VK_SUCCESS )
        {
            for( auto ext : extensions )
            {
                m_supported_extensions.push_back( ext.extensionName );
            }
        }
    }
}


void DeviceVK::createDevice()
{

    // Desired queues need to be requested upon logical device creation
    // Due to differing queue family configurations of Vulkan implementations this can be a bit tricky, especially if the application
    // requests different queue types
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos{};

    // Get queue family indices for the requested queue family types
    // Note that the indices may overlap depending on the implementation

    const float default_queue_priority( 0.0f );

    // Graphics queue
    m_graphics_queue_index = getQueueFamilyIndex( VK_QUEUE_GRAPHICS_BIT );
    
    VkDeviceQueueCreateInfo queue_info{};
    queue_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = m_graphics_queue_index;
    queue_info.queueCount       = 1;
    queue_info.pQueuePriorities = &default_queue_priority;
    queue_create_infos.push_back( queue_info );
    

    // Create the logical device representation
    // If the device will be used for presenting to a display via a swapchain we need to request the swapchain extension
    m_extensions.push_back( VK_KHR_SWAPCHAIN_EXTENSION_NAME              );
    m_extensions.push_back( VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME );
    m_extensions.push_back( VK_KHR_MAINTENANCE1_EXTENSION_NAME           );

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = static_cast< uint32_t >( queue_create_infos.size() );
    device_create_info.pQueueCreateInfos    = queue_create_infos.data();
    //device_create_info.pEnabledFeatures     = &m_physical_device_features;
    device_create_info.pEnabledFeatures = nullptr; // Ya no usamos esto directamente
    device_create_info.pNext                = &m_physical_device_features2;

    // Enable the debug marker extension if it is present (likely meaning a debugging tool is present)
#ifdef DEBUG
    if( std::find( m_supported_extensions.begin(), m_supported_extensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME ) != m_supported_extensions.end() )
    {
        m_extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
    }
    
    bool is_markers_enabled = false;
    if( std::find( m_supported_extensions.begin(), m_supported_extensions.end(), VK_EXT_DEBUG_MARKER_EXTENSION_NAME ) != m_supported_extensions.end() )
    {
        is_markers_enabled = true;
        m_extensions.push_back( VK_EXT_DEBUG_MARKER_EXTENSION_NAME );
    }
#endif 

    // Add Raytracing Extensions
   // -----------------------------------------------------

    m_physical_device_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    m_physical_device_features2.pNext = nullptr;

    // Start chaining from the last element so we preserve order
    void** pNextHead = &m_physical_device_features2.pNext;
   
    // 1. Buffer Device Address (requerido por Acceleration Structure)
    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures = {};

    if (std::find(m_supported_extensions.begin(), m_supported_extensions.end(),
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) != m_supported_extensions.end())
    {
        m_extensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
        bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
        bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
        *pNextHead = &bufferDeviceAddressFeatures;
        pNextHead = &bufferDeviceAddressFeatures.pNext;
    }

    // 2. Descriptor Indexing (útil para ray tracing)
    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures = {};
    if (std::find(m_supported_extensions.begin(), m_supported_extensions.end(),
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME) != m_supported_extensions.end()) {
        m_extensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
        descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
        descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        descriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
        *pNextHead = &descriptorIndexingFeatures;
        pNextHead = &descriptorIndexingFeatures.pNext;
    }

    // 3. Acceleration Structure
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {};
    if (std::find(m_supported_extensions.begin(), m_supported_extensions.end(),
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) != m_supported_extensions.end()) {
        m_extensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        accelerationStructureFeatures.accelerationStructure = VK_TRUE;
        *pNextHead = &accelerationStructureFeatures;
        pNextHead = &accelerationStructureFeatures.pNext;
    }

    // 4. Ray Tracing Pipeline
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures = {};
    if (std::find(m_supported_extensions.begin(), m_supported_extensions.end(),
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) != m_supported_extensions.end()) {
        m_extensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        rayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
        *pNextHead = &rayTracingPipelineFeatures;
        pNextHead = &rayTracingPipelineFeatures.pNext;
    }

    // 5. Ray Query
    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures = {};
    if (std::find(m_supported_extensions.begin(), m_supported_extensions.end(),
        VK_KHR_RAY_QUERY_EXTENSION_NAME) != m_supported_extensions.end()) {
        m_extensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
        rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
        rayQueryFeatures.rayQuery = VK_TRUE;
        *pNextHead = &rayQueryFeatures;
        pNextHead = &rayQueryFeatures.pNext;
    }

    // 6. Deferred Host Operations (siempre que esté disponible)
      if (std::find(m_supported_extensions.begin(), m_supported_extensions.end(),
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME) != m_supported_extensions.end())
    {
        m_extensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
    }

    // Verifica que todas las extensiones requeridas están disponibles
    std::vector<const char*> requiredExtensions = {
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
    };

    for (const auto& ext : requiredExtensions) {
        if (std::find(m_supported_extensions.begin(), m_supported_extensions.end(), ext) == m_supported_extensions.end()) {
            throw std::runtime_error(std::string("Required extension not supported: ") + ext);
        }
    }
    

    // -----------------------------------------------------
    

    if( m_extensions.size() > 0 )
    {
        for( const char* enabled_extension : m_extensions )
        {
            if( !( std::find( m_supported_extensions.begin(), m_supported_extensions.end(), enabled_extension ) != m_supported_extensions.end() ) )
            {
                std::cerr << "Enabled device extension \"" << enabled_extension << "\" is not present at device level\n";
            }
        }

        device_create_info.enabledExtensionCount    = ( uint32_t )m_extensions.size();
        device_create_info.ppEnabledExtensionNames  = m_extensions.data();
    }

    if( vkCreateDevice( m_physical_device, &device_create_info, nullptr, &m_logical_device ) )
    {
        throw MiniEngineException( "Error creating devices" );
    }

#ifdef DEBUG
    if( is_markers_enabled )
    {
        UtilsVK::setupCallbacks( *this );   
    }
#endif

    vkGetDeviceQueue( m_logical_device, m_graphics_queue_index, 0, &m_graphics_queue );
}


void DeviceVK::createCommandPool()
{
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType             = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex  = m_graphics_queue_index;
    pool_info.flags             = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    
    if( vkCreateCommandPool( m_logical_device, &pool_info, nullptr, &m_command_pool ) )
    {
        throw MiniEngineException("Error creating the global queue");
    }
}


uint32_t DeviceVK::getMemoryTypeIndex( uint32_t typeBits, VkMemoryPropertyFlags properties ) const
{
    // Iterate over all memory types available for the device used in this example
    for( uint32_t i = 0; i < m_physical_device_memory_properties.memoryTypeCount; i++ )
    {
        if( ( typeBits & 1 ) == 1 )
        {
            if( ( m_physical_device_memory_properties.memoryTypes[ i ].propertyFlags & properties ) == properties )
            {
                return i;
            }
        }
        typeBits >>= 1;
    }

    throw "Could not find a suitable memory type!";
}


void DeviceVK::destroyDevice()
{
    vkDestroyCommandPool( m_logical_device, m_command_pool, nullptr );
    vkDestroyDevice( m_logical_device, nullptr );
}


void DeviceVK::destroyCommandPool()
{
}