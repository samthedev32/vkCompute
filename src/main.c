#include <stdio.h>
#include <vkCompute.h>

int main() {
  VkCompute comp;

  int err = vkCompute_init(&comp);

  if (err)
    printf("Init terminated with exit code %i\n", err);

  err = vkCompute_new(&comp);

  if (err)
    printf("id: %i\n", err);

  return 0;
}