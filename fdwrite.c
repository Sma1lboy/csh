#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fdwrite.h"
ssize_t fdwrite(int fd, const char *str)
{
  size_t len = strlen(str);
  ssize_t written = write(fd, str, len);
  if (written == -1)
  {
    perror("write");
  }
  return written;
}