#ifndef _VKCOMPUTE_H_
#define _VKCOMPUTE_H_

#include <stdbool.h>
#include <vulkan/vulkan.h>

typedef struct {
  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger;

  VkPhysicalDevice physicalDevice;
  VkDevice device;

  uint32_t computeFamily;
  VkQueue queue;

  struct {
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    VkCommandPool commandPool;

    VkBuffer shaderStorageBuffers;
    VkDeviceMemory shaderStorageBuffersMemory;

    VkBuffer uniformBuffers;
    VkDeviceMemory uniformBuffersMemory;
    void *uniformBuffersMapped;

    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSets;

    VkCommandBuffer commandBuffers;

    VkSemaphore finishedSemaphores;
    VkFence inFlightFences;
  } *pipeline;
  size_t pipelines;
} VkCompute;

typedef struct {

} asd;

// Initialize vkCompute Engine
int vkCompute_init(VkCompute *comp);

// Create new Compute Pipeline
int vkCompute_new(VkCompute *comp, const char *path, uint32_t bindingCount,
                  VkDescriptorType *bindings);

#endif