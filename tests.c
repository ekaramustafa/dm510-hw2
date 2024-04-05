#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>


// TEST 1: Basic write and read from the same process to the different devices
int test_one() {
  int fd;  
  char test_txt[16] = "This is a test."; 
  char read_txt[16];
  
  fd = open("/dev/dm510-0", O_WRONLY);
	perror("w open");
  errno = 0;

  write(fd, test_txt, 16);
  close(fd);

  fd = open("/dev/dm510-1", O_RDONLY);
	perror("r open");
  errno = 0;

  read(fd, read_txt, 16);
  close(fd);

  printf("Test 1. expected result: %s\n", test_txt);
  printf("Test 1. result: %s\n", read_txt);

  return strcmp(test_txt, read_txt);
}


// TEST 2: When it is 
int test_two() {
  int fd;

  return 0;
}


// TEST 3: 
int test_three() {
  return 0;
}


// TEST 4: Process returns immediately when trying to read in NON-BLOCKING mode
// because there is nothing in the buffer
int test_four() {
  int fd;
  char read_txt[10];

  fd = open("/dev/dm510-0", O_RDONLY | O_NONBLOCK);
  perror("r open");

  read(fd, read_txt, 10);
  perror("fd read");
  int err = errno;
  errno = 0;

  close(fd);

  if(err != EAGAIN) return -1;

  return 0;
}


// TEST 5: Change the buffer size to something smaller with ioctl, 
// fill it up and if the process tries to write again in NON-BLOCKING 
// mode it will return immediately
int test_five() {
  int fd;
  unsigned long buffer_size = 10;
  char *read_txt = "test test";

  fd = open("/dev/dm510-0", O_WRONLY | O_NONBLOCK);
  perror("open");
  errno = 0;

  // Change the buffer size
  ioctl(fd, 1, buffer_size);
  perror("set size ioctl");
  errno = 0;

  // Get the buffer size
  int size = ioctl(fd, 0);
  perror("get size ioctl");
  errno = 0;

  // Fill up the buffer
  write(fd, read_txt, buffer_size);
  perror("write");
  errno = 0;

  // Try to write again
  write(fd, read_txt, buffer_size);
  perror("write");
  int err = errno;
  errno = 0;

  close(fd);

  if(err != EAGAIN || size != 10) return -1;

  return 0;
}


// TEST 6: Change the buffer size to something smaller with ioctl,
// fill it up and if the process tries to write again in BLOCKING
// mode it will wait until the child process reads from the 
// other device
int test_six() {
  int fd;

  return 0;
}

// TEST 7: Try to change the buffer size even bigger and see
// the correct error
int test_seven() {
  int fd;
  unsigned long buffer_size = 5000;

  fd = open("/dev/dm510-0", O_WRONLY | O_NONBLOCK);
  perror("open");
  errno = 0;

  // Change the buffer size
  ioctl(fd, 1, buffer_size);
  perror("set size ioctl");
  int err = errno;
  errno = 0;

  close(fd);

  if(err != EINVAL) return -1;

  return 0;
}


// TEST 8: Set a different number of readers and retrieve it
int test_eight() {
  int fd;
  unsigned long max_readers = 60;

  fd = open("/dev/dm510-1", O_RDWR);
  perror("open");
  errno = 0;

  // Change the max number of readers
  ioctl(fd, 3, max_readers);
  perror("set max ioctl");
  errno = 0;

  // Get the max number of readers
  int readers = ioctl(fd, 2);
  perror("get max ioctl");
  errno = 0;

  close(fd);

  printf("Test 8. expected result: %lu\n", max_readers);
  printf("Test 8. result: %d\n", readers);

  if(readers != max_readers) return -1;

  return 0;
}


int main(int argc, char const *argv[]) {
  // For all the tests, the return value is 1 if everything went 
  // correctly, otherwise something did not work as expected

  printf("\n");
  /* printf("TEST 1 ------------------\n");
  if(test_one() == 0) {
    printf("\nTest Succeeded\n");
  } else {
    printf("\nTest Failed\n");
  }
  printf("-------------------------\n\n");

  printf("TEST 2 ------------------\n");
  if(test_two() == 0) {
    printf("\nTest Succeeded\n");
  } else {
    printf("\nTest Failed\n");
  }
  printf("-------------------------\n\n");

  printf("TEST 3 ------------------\n");
  if(test_three() == 0) {
    printf("\nTest Succeeded\n");
  } else {
    printf("\nTest Failed\n");
  }
  printf("-------------------------\n\n");

  printf("TEST 4 ------------------\n");
  if(test_four() == 0) {
    printf("\nTest Succeeded\n");
  } else {
    printf("\nTest Failed\n");
  }
  printf("-------------------------\n\n");
  printf("\n"); */

  /* printf("TEST 5 ------------------\n");
  if(test_five() == 0) {
    printf("\nTest Succeeded\n");
  } else {
    printf("\nTest Failed\n");
  }
  printf("-------------------------\n\n");
  printf("\n"); */

  /* printf("TEST 6 ------------------\n");
  if(test_six() == 0) {
    printf("\nTest Succeeded\n");
  } else {
    printf("\nTest Failed\n");
  }
  printf("-------------------------\n\n");
  printf("\n");

  printf("TEST 7 ------------------\n");
  if(test_seven() == 0) {
    printf("\nTest Succeeded\n");
  } else {
    printf("\nTest Failed\n");
  }
  printf("-------------------------\n\n");
  printf("\n");*/

  printf("TEST 8 ------------------\n");
  if(test_eight() == 0) {
    printf("\nTest Succeeded\n");
  } else {
    printf("\nTest Failed\n");
  }
  printf("-------------------------\n\n");
  printf("\n"); 

  return 0;
}