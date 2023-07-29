#include <vulkan/vulkan.h>

typedef struct {
  VkInstance instance;

  VkPhysicalDevice physicalDevice;
  VkDevice device;

  VkQueue queue;
} VkCompute;

// Initialize vkCompute Engine
int vkCompute_init(VkCompute *comp);