#ifndef _VKCOMPUTE_H_
#define _VKCOMPUTE_H_

#include <stdbool.h>
#include <vulkan/vulkan.h>

// bool enableValidationLayers = true;

typedef struct {
  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger;

  VkPhysicalDevice physicalDevice;
  VkDevice device;

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

// Initialize vkCompute Engine
int vkCompute_init(VkCompute *comp);

// Create new Compute Pipeline
int vkCompute_new(VkCompute *comp);

#endif