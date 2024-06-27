#ifndef FDWRITE_H
#define FDWRITE_H

#include <unistd.h>

ssize_t fdwrite(int fd, const char *str);

#endif // FDWRITE_H