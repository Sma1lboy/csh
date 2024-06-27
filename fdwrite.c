#include <stdio.h>
#include <stdlib.h>
#include "fdwrite.h"

int main()
{
  const char *message = "Hello, World!\n";
  int fd = STDOUT_FILENO; // 写入标准输出

  if (fdwrite(fd, message) == -1)
  {
    fprintf(stderr, "Failed to write to file descriptor %d\n", fd);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
