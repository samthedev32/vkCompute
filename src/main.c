#include <stdio.h>
#include <vkCompute.h>

int main() {
  VkCompute comp;

  int err = vkCompute_init(&comp);

  if (err)
    printf("Init terminated with exit code %i\n", err);

  VkDescriptorType binds[4] = {
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};
  err = vkCompute_new(&comp, "comp.spv", 4, binds);

  if (err)
    printf("id: %i\n", err);

  return 0;
}