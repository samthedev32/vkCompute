#include <stdlib.h>
#include <string.h>
#include <vkCompute.h>

#include <stdio.h>

#include <vulkan/vulkan_core.h>

bool enableValidationLayers = true;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);

extern VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger);

extern void
DestroyDebugUtilsMessengerEXT(VkInstance instance,
                              VkDebugUtilsMessengerEXT debugMessenger,
                              const VkAllocationCallbacks *pAllocator);

extern void populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT *createInfo);

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties, VkBuffer *buffer,
                  VkDeviceMemory *bufferMemory);

void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

// Actual Stuff

bool vkCompute_validate(VkCompute *comp) {
  return comp->instance &&
         (comp->physicalDevice && comp->device && comp->queue);
}

int vkCompute_init(VkCompute *comp) {
  if (!comp)
    return 1;

  struct {
    bool instance;
    bool physicalDevice, device, queue;
  } had;

  had.instance = comp->instance;

  had.physicalDevice = comp->physicalDevice;
  had.device = comp->device;
  had.queue = comp->queue;

  /* Check Provided Stuff */ {
    if (!had.instance && had.physicalDevice) {
      printf("can't use provided physical device\n");
      had.physicalDevice = false;
    }

    if (!had.physicalDevice && had.device) {
      printf("can't use provided logical device\n");
      had.device = false;
    }

    if (!had.device && had.queue) {
      printf("can't use provided queue\n");
      had.queue = false;
    }
  }

  const char *validationLayer = "VK_LAYER_KHRONOS_validation";

  // Create Instance
  if (!had.instance) {
    if (enableValidationLayers) {
      uint32_t layerCount;
      vkEnumerateInstanceLayerProperties(&layerCount, NULL);

      VkLayerProperties availableLayers[layerCount];
      vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

      enableValidationLayers = false;
      for (uint32_t i = 0; i < layerCount; i++) {
        if (strcmp(validationLayer, availableLayers[i].layerName) == 0) {
          enableValidationLayers = true;
          break;
        }
      }
    }

    VkApplicationInfo appInfo = {0};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Compute Test";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "vkCompute";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    const char *ext = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = &ext;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {0};
    if (enableValidationLayers) {
      createInfo.enabledLayerCount = 1;

      createInfo.ppEnabledLayerNames = &validationLayer;

      /* Populate Debug Messenger Create Info */ {
        debugCreateInfo.sType =
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity =
            // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugCallback;
      }

      createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    } else {
      createInfo.enabledLayerCount = 0;

      createInfo.pNext = NULL;
    }

    if (vkCreateInstance(&createInfo, NULL, &comp->instance) != VK_SUCCESS)
      return 2;

    if (enableValidationLayers) {
      VkDebugUtilsMessengerCreateInfoEXT createInfo = {0};
      populateDebugMessengerCreateInfo(&createInfo);

      if (CreateDebugUtilsMessengerEXT(comp->instance, &createInfo, NULL,
                                       &comp->debugMessenger) != VK_SUCCESS) {
        printf("failed to set up debug messenger\n");
      }
    }
  }

  // Create Physical Device
  if (!had.physicalDevice) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(comp->instance, &deviceCount, NULL);

    if (deviceCount == 0)
      return 3;

    VkPhysicalDevice devices[deviceCount];
    vkEnumeratePhysicalDevices(comp->instance, &deviceCount, devices);

    for (uint32_t i = 0; i < deviceCount; i++) {
      VkPhysicalDevice device = devices[i];

      /* Find Queue Family */ {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                                 NULL);

        VkQueueFamilyProperties queueFamily[queueFamilyCount];
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                                 queueFamily);

        for (uint32_t j = 0; j < queueFamilyCount; j++) {
          if (queueFamily[j].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            comp->computeFamily = j;
            comp->physicalDevice = device;
            break;
          }
        }
      }
    }

    if (comp->physicalDevice == NULL)
      return 4;
  }

  // Create Logical Device
  if (!had.device) {
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {0};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = comp->computeFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures = {0};

    VkDeviceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = 0;
    createInfo.ppEnabledExtensionNames = VK_NULL_HANDLE;

    if (enableValidationLayers) {
      createInfo.enabledLayerCount = 1;
      createInfo.ppEnabledLayerNames = &validationLayer;
    } else {
      createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(comp->physicalDevice, &createInfo, NULL,
                       &comp->device) != VK_SUCCESS)
      return 5;

    vkGetDeviceQueue(comp->device, comp->computeFamily, 0, &comp->queue);
  }

  return 0;
}

int vkCompute_new(VkCompute *comp, const char *path, uint32_t bindingCount,
                  VkDescriptorType *types) {
  if (!vkCompute_validate(comp))
    return 1;

  size_t id;
  /* Append Pipeline List */ {
    if (comp->pipeline == NULL) {
      comp->pipeline = malloc(sizeof(*comp->pipeline));

      if (!comp->pipeline)
        return 2;
      else
        comp->pipelines = 1;
    } else {
      void *tmp = realloc(comp->pipeline,
                          sizeof(*comp->pipeline) * (comp->pipelines + 1));

      if (tmp) {
        comp->pipeline = tmp;
        comp->pipelines++;

      } else
        return 3;
    }
  }

  id = comp->pipelines - 1;

  /* Create Descriptor Set Layout */ {
    VkDescriptorSetLayoutBinding layoutBindings[bindingCount];
    for (uint32_t i = 0; i < bindingCount; i++) {
      layoutBindings[i].binding = i;
      layoutBindings[i].descriptorCount = 1;
      layoutBindings[i].descriptorType = types[i];
      layoutBindings[i].pImmutableSamplers = NULL;
      layoutBindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = bindingCount;
    layoutInfo.pBindings = layoutBindings;

    if (vkCreateDescriptorSetLayout(
            comp->device, &layoutInfo, NULL,
            &(comp->pipeline[id].descriptorSetLayout)) != VK_SUCCESS) {
      return 4;
    }
  }

  /* Create Pipeline */ {
    uint32_t *code;
    size_t len;

    /* Read File */ {
      FILE *f = fopen(path, "rb");

      if (!f)
        return 5;

      fseek(f, 0, SEEK_END);
      long size = ftell(f);
      if (size == -1)
        return 6;

      code = malloc(sizeof(char) * size);

      fseek(f, 0, SEEK_SET);

      len = fread(code, 1, size, f);

      fclose(f);

      if (len != size) {
        free(code);
        return 7;
      }
    }

    VkShaderModule shaderModule;
    /* Create Shader Module */ {
      VkShaderModuleCreateInfo createInfo = {};
      createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
      createInfo.codeSize = len;
      createInfo.pCode = code;

      if (vkCreateShaderModule(comp->device, &createInfo, NULL,
                               &shaderModule) != VK_SUCCESS) {
        return 8;
      }
    }

    VkPipelineShaderStageCreateInfo computeShaderStageInfo = {};
    computeShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStageInfo.module = shaderModule;
    computeShaderStageInfo.pName = "main";

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &comp->pipeline[id].descriptorSetLayout;

    if (vkCreatePipelineLayout(comp->device, &pipelineLayoutInfo, NULL,
                               &comp->pipeline[id].pipelineLayout) !=
        VK_SUCCESS) {
      return 9;
    }

    VkComputePipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = comp->pipeline[id].pipelineLayout;
    pipelineInfo.stage = computeShaderStageInfo;

    if (vkCreateComputePipelines(comp->device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                 NULL,
                                 &comp->pipeline[id].pipeline) != VK_SUCCESS) {
      return 10;
    }

    vkDestroyShaderModule(comp->device, shaderModule, NULL);
  }

  /* Create Command Pool */ {
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = comp->computeFamily;

    if (vkCreateCommandPool(comp->device, &poolInfo, NULL,
                            &comp->pipeline[id].commandPool) != VK_SUCCESS) {
      return 11;
    }
  }

  /* Create Shader Storage Buffers */ {
      // size_t storageBufferCount = 0;
      // for (int i = 0; i < bindingCount; i++) {
      //   switch (types[i]) {
      //   default:
      //     break;

      //   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      //     storageBufferCount++;
      //   }
      // }

      // VkDeviceSize bufferSize = sizeof(Layer) * storageBufferCount;

      // // Create a staging buffer used to upload data to the gpu
      // VkBuffer stagingBuffer;
      // VkDeviceMemory stagingBufferMemory;
      // createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      //              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      //                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      //              &stagingBuffer, &stagingBufferMemory);

      // void *data;
      // vkMapMemory(comp->device, stagingBufferMemory, 0, bufferSize, 0,
      // &data);
      // memcpy(data, layers.data(), (size_t)bufferSize);
      // vkUnmapMemory(comp->device, stagingBufferMemory);

      // // Copy initial particle data to all storage buffers
      // createBuffer(bufferSize,
      //              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
      //                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
      //                  VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      //              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      //              &comp->pipeline[id].shaderStorageBuffers,
      //              &comp->pipeline[id].shaderStorageBuffersMemory);
      // copyBuffer(stagingBuffer, comp->pipeline[id].shaderStorageBuffers,
      //            bufferSize);

      // vkDestroyBuffer(comp->device, stagingBuffer, NULL);
      // vkFreeMemory(comp->device, stagingBufferMemory, NULL);
  }

  /* createUniformBuffers */ {}

  /* createDescriptorPool */ {}

  /* createComputeDescriptorSets */ {}

  /* createComputeCommandBuffers */ {
      // VkCommandBufferAllocateInfo allocInfo = {};
      // allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      // allocInfo.commandPool = comp->commandPool;
      // allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      // allocInfo.commandBufferCount = 1;

      // if (vkAllocateCommandBuffers(device, &allocInfo,
      // &computeCommandBuffers) !=
      //     VK_SUCCESS) {
      //   throw std::runtime_error("failed to allocate compute command
      //   buffers");
      // }
  }

  /* createSyncObjects */ {
    // VkSemaphoreCreateInfo semaphoreInfo = {};
    // semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // VkFenceCreateInfo fenceInfo = {};
    // fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // if (vkCreateSemaphore(comp->device, &semaphoreInfo, NULL,
    //                       &comp->pipeline[id].finishedSemaphores) !=
    //         VK_SUCCESS ||
    //     vkCreateFence(comp->device, &fenceInfo, NULL,
    //                   &comp->pipeline[id].inFlightFences) != VK_SUCCESS) {
    //   printf("failed to create sync objects\n");
    //   // TODO: set return id
    //   return -1;
    // }
  }

  // TODO: finish

  return 0;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {

  printf("\n");

  switch (messageSeverity) {
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    printf(".. ");
    break;

  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    printf("... ");
    break;

  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    printf("!? ");
    break;

  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
    printf("!!! ");

  default:
    break;
  }

  printf("validation layer: %s\n", pCallbackData->pMessage);

  return VK_FALSE;
};

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger) {
  PFN_vkCreateDebugUtilsMessengerEXT func =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance, "vkCreateDebugUtilsMessengerEXT");

  if (func != NULL) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator) {
  PFN_vkDestroyDebugUtilsMessengerEXT func =
      (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != NULL) {
    func(instance, debugMessenger, pAllocator);
  }
}

void populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT *createInfo) {
  createInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo->messageSeverity =
      // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo->pfnUserCallback = debugCallback;
}

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties, VkBuffer &buffer,
                  VkDeviceMemory &bufferMemory) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to create buffer");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      findMemoryType(memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate buffer memory");
  }

  vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  VkBufferCopy copyRegion{};
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(computeQueue);
}

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type");
}