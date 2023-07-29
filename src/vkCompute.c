#include <vkCompute.h>

#include <stdbool.h>
#include <stdio.h>

#include <vulkan/vulkan_core.h>

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

  if (!had.instance) {
    // if (enableValidationLayers && !checkValidationLayerSupport()) {
    //   throw std::runtime_error("validation layers requested, but not
    //   available");
    // }

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
    // if (enableValidationLayers) {
    //   createInfo.enabledLayerCount =
    //       static_cast<uint32_t>(validationLayers.size());
    //   createInfo.ppEnabledLayerNames = validationLayers.data();

    //   populateDebugMessengerCreateInfo(debugCreateInfo);
    //   createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT
    //   *)&debugCreateInfo;
    // } else {
    createInfo.enabledLayerCount = 0;

    createInfo.pNext = NULL;
    // }

    if (vkCreateInstance(&createInfo, NULL, &comp->instance) != VK_SUCCESS)
      return 2;
  }

  uint32_t computeFamily = 0;
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
            computeFamily = j + 1;
            break;
          }
        }
      }

      if (computeFamily != 0) {
        comp->physicalDevice = device;
        break;
      }
    }

    if (comp->physicalDevice == NULL)
      return 4;
  }

  if (!had.device) {
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {0};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = computeFamily;
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

    // if (enableValidationLayers) {
    //   createInfo.enabledLayerCount =
    //       static_cast<uint32_t>(validationLayers.size());
    //   createInfo.ppEnabledLayerNames = validationLayers.data();
    // } else {
    createInfo.enabledLayerCount = 0;
    // }

    if (vkCreateDevice(comp->physicalDevice, &createInfo, NULL,
                       &comp->device) != VK_SUCCESS)
      return 5;

    vkGetDeviceQueue(comp->device, computeFamily, 0, &comp->queue);
  }

  return 0;
}

bool findQueueFamil(VkPhysicalDevice device) {
  uint32_t computeFamily;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

  VkQueueFamilyProperties queueFamily[queueFamilyCount];
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamily);

  for (uint32_t i = 0; i < queueFamilyCount; i++) {
    if (queueFamily[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
      computeFamily = i;
      return true;
    }
  }

  return false;
}