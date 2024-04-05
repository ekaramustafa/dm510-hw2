#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/wait.h> 


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


// TEST 2: When the buffer is empty, make one process wait for another
// one to write to it and then viceversa
int test_two() {
  int fd_1, fd_2;
  char wr_txt_1[20] = "Test number one ins";
  char wr_txt_2[20] = "Test number two ins";
  char read_txt_1[20], read_txt_2[20];
  
  int pid = fork();

  if(pid == 0) {
    fd_1 = open("/dev/dm510-0", O_RDONLY);
    read(fd_1, read_txt_1, 20);
    close(fd_1);
  } else {
    fd_2 = open("/dev/dm510-1", O_WRONLY);
    write(fd_2, wr_txt_1, 20);
    close(fd_2);
  }

  if(pid == 0) {
    fd_1 = open("/dev/dm510-0", O_WRONLY);
    write(fd_1, wr_txt_2, 20);
    close(fd_1);
  } else {
    fd_2 = open("/dev/dm510-1", O_RDONLY);
    read(fd_2, read_txt_2, 20);
    close(fd_2);
  }

  if(pid == 0) {
    printf("\nTest 2. expected first result: %s\n", wr_txt_1);
    printf("Test 2. first result: %s\n", read_txt_1);
    exit(strcmp(wr_txt_1, read_txt_1));
  } else {
    int status;
    wait(&status);

    printf("Test 2. expected second result: %s\n", wr_txt_2);
    printf("Test 2. second result: %s\n", read_txt_2);

    if(WIFEXITED(status)) {
      int status_code = WEXITSTATUS(status);
      if(status_code == 0) {
        return strcmp(wr_txt_2, read_txt_2);
      }
    }
  }

  return -1;
}


// TEST 3: Make multiple writes and reads to the two devices to see
// the expected behavior
int test_three() {
  int fd;
  char wr_txt[5] = "Test";
  char read_txt[15], read_txt_1[10];

  fd = open("/dev/dm510-0", O_WRONLY);
  for(int i = 0; i < 3; i++) {
    write(fd, wr_txt, strlen(wr_txt) + 1);
    perror("write");
  }
  close(fd);

  fd = open("/dev/dm510-1", O_RDWR);
  read(fd, read_txt, 15);

  for(int i = 0; i < 2; i++) {
    write(fd, wr_txt, strlen(wr_txt) + 1);
    perror("write");
  }

  close(fd);

  fd = open("/dev/dm510-0", O_RDONLY);
  read(fd, read_txt_1, 10);

  close(fd);

  printf("Test 3. first expected result: Test Test Test\n");
  printf("Test 3. first result: ");
  for(int i = 0; i < 3; i++) {
    if(strcmp(read_txt + i * 5, "Test")) return -1;
    printf("%s ", read_txt + i * 5);
  }
  printf("\n");

  printf("Test 3. second expected result: Test Test\n");
  printf("Test 3. second result: ");
  for(int i = 0; i < 2; i++) {
    if(strcmp(read_txt_1 + i * 5, "Test")) return -1;
    printf("%s ", read_txt_1 + i * 5);
  }
  printf("\n");

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
  char *txt = "test test";

  fd = open("/dev/dm510-0", O_WRONLY | O_NONBLOCK);
  errno = 0;

  // Change the buffer size
  ioctl(fd, 1, buffer_size);
  perror("set size ioctl");
  errno = 0;

  // Get the buffer size
  int size = ioctl(fd, 0);
  perror("get size ioctl");
  errno = 0;

  printf("New buffer size: %d\n", size);

  // Fill up the buffer
  write(fd, txt, strlen(txt));
  perror("first write");
  errno = 0;

  // Try to write again
  write(fd, txt, strlen(txt));
  perror("second write");
  int err = errno;
  errno = 0;

  close(fd);

  if(err != EAGAIN || size != buffer_size) return -1;

  return 0;
}


// TEST 6: Try to make the buffer size bigger 
// and see the correct error
int test_six() {
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

  printf("Test 6. expected result: %d\n", EINVAL);
  printf("Test 6. result: %d\n", err);

  if(err != EINVAL) return -1;

  return 0;
}


// TEST 7: Set a different number of readers and retrieve it
int test_seven() {
  int fd;
  unsigned long max_readers = 60;

  fd = open("/dev/dm510-1", O_RDWR);
  perror("open");
  errno = 0;

  // Change the max number of readers
  ioctl(fd, 201, max_readers);
  perror("set max ioctl");
  errno = 0;

  // Get the max number of readers
  int readers = ioctl(fd, 200);
  perror("get max ioctl");
  errno = 0;

  close(fd);

  printf("Test 7. expected result: %lu\n", max_readers);
  printf("Test 7. result: %d\n", readers);

  if(readers != max_readers) return -1;

  return 0;
}


// TEST 8: If two processes try to grab the write permission
// to a device one is getting rejected.
int test_eight() {
  int fd;
  int pid = fork();

  fd = open("/dev/dm510-0", O_WRONLY);
  if(pid != 0) {
    perror("parent open");

    int status;
    wait(&status);

    if(WIFEXITED(status)) {
      int status_code = WEXITSTATUS(status);
      if(status_code == 0 || errno == 512) {
        return 0;
      }
    }
  } else {
    perror("child open");
    if(errno == 512) {
      exit(0);
    } else {
      exit(-1);
    }
  }

  return -1;
}


int main(int argc, char const *argv[]) {
  // For all the tests, the return value is 1 if everything went 
  // correctly, otherwise something did not work as expected

  printf("\n");
  printf("TEST 1 ------------------\n\n");
  if(test_one() == 0) {
    printf("\nTest Succeeded\n");
  } else {
    printf("\nTest Failed\n");
  }
  printf("-------------------------\n\n"); 

  printf("TEST 2 ------------------\n\n");
  if(test_two() == 0) {
    printf("\nTest Succeeded\n");
  } else {
    printf("\nTest Failed\n");
  }
  printf("-------------------------\n\n"); 

  printf("TEST 3 ------------------\n\n");
  if(test_three() == 0) {
    printf("\nTest Succeeded\n");
  } else {
    printf("\nTest Failed\n");
  }
  printf("-------------------------\n\n");

  printf("TEST 4 ------------------\n\n");
  if(test_four() == 0) {
    printf("\nTest Succeeded\n");
  } else {
    printf("\nTest Failed\n");
  }
  printf("-------------------------\n\n");

  printf("TEST 5 ------------------\n\n");
  if(test_five() == 0) {
    printf("\nTest Succeeded\n");
  } else {
    printf("\nTest Failed\n");
  }
  printf("-------------------------\n\n");

  printf("TEST 6 ------------------\n\n");
  if(test_six() == 0) {
    printf("\nTest Succeeded\n");
  } else {
    printf("\nTest Failed\n");
  }
  printf("-------------------------\n\n");

  printf("TEST 7 ------------------\n\n");
  if(test_seven() == 0) {
    printf("\nTest Succeeded\n");
  } else {
    printf("\nTest Failed\n");
  }
  printf("-------------------------\n\n");

  printf("TEST 8 ------------------\n\n");
  if(test_eight() == 0) {
    printf("\nTest Succeeded\n");
  } else {
    printf("\nTest Failed\n");
  }
  printf("-------------------------\n\n");
  printf("\n"); 

  return 0;
}