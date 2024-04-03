#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/kernel.h>


int main(int argc, char const *argv[]) {
  size_t DEVICE_COUNT = 2;
  for (size_t i = 0; i < DEVICE_COUNT; i++) {
    int result = open("/dev/dm510-0", O_RDWR);
    printf("Result %lu : %d ",i,  result);
    if(0 > result){
      printf("invalid result!\n");
      return 0;
    }
    printf("\n");
  }
  return 0;
}
