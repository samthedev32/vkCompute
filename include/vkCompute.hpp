#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <set>
#include <stdexcept>
#include <vector>

const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

const std::vector<const char *> deviceExtensions = {};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

// VkResult CreateDebugUtilsMessengerEXT(
//     VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT
//     *pCreateInfo, const VkAllocationCallbacks *pAllocator,
//     VkDebugUtilsMessengerEXT *pDebugMessenger);

// void DestroyDebugUtilsMessengerEXT(VkInstance instance,
//                                    VkDebugUtilsMessengerEXT debugMessenger,
//                                    const VkAllocationCallbacks *pAllocator);

struct QueueFamilyIndices {
  std::optional<uint32_t> computeFamily;

  bool isComplete() { return computeFamily.has_value(); }
};

struct UniformBufferObject {
  float deltaTime = 1.0f;
};

struct Layer {
  float data;
};

class vkCompute {
public:
  vkCompute();
  ~vkCompute();

  void run();

private:
  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger;

  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device;

  VkQueue computeQueue;

  VkDescriptorSetLayout computeDescriptorSetLayout;
  VkPipelineLayout computePipelineLayout;
  VkPipeline computePipeline;

  VkCommandPool commandPool;

  VkBuffer shaderStorageBuffers;
  VkDeviceMemory shaderStorageBuffersMemory;

  VkBuffer uniformBuffers;
  VkDeviceMemory uniformBuffersMemory;
  void *uniformBuffersMapped;

  VkDescriptorPool descriptorPool;
  VkDescriptorSet computeDescriptorSets;

  VkCommandBuffer computeCommandBuffers;

  VkSemaphore computeFinishedSemaphores;
  VkFence computeInFlightFences;

  void createInstance();

  void populateDebugMessengerCreateInfo(
      VkDebugUtilsMessengerCreateInfoEXT &createInfo);

  void setupDebugMessenger();

  void pickPhysicalDevice();

  void createLogicalDevice();

  void createComputeDescriptorSetLayout();

  void createComputePipeline();

  void createCommandPool();

  void createShaderStorageBuffers();

  void createUniformBuffers();

  void createDescriptorPool();

  void createComputeDescriptorSets();

  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties, VkBuffer &buffer,
                    VkDeviceMemory &bufferMemory);

  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

  uint32_t findMemoryType(uint32_t typeFilter,
                          VkMemoryPropertyFlags properties);

  void createComputeCommandBuffers();

  void recordComputeCommandBuffer(VkCommandBuffer commandBuffer);
  void createSyncObjects();
  void updateUniformBuffer(uint32_t currentImage);

  VkShaderModule createShaderModule(const std::vector<char> &code);

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);

  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);

  bool isDeviceSuitable(VkPhysicalDevice device);

  bool checkDeviceExtensionSupport(VkPhysicalDevice device);

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
  bool checkValidationLayerSupport();

  static std::vector<char> readFile(const std::string &filename);

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData);
};