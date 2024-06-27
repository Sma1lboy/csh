#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "redirection.h"

#define STDERR_FILENO 2 // Standard error output file descriptor

/**
 * redirection_check - checks for output redirection
 * @args: array of command arguments
 * @size: number of arguments
 *
 * Return: file descriptor if redirection is found, -1 if not, -2 on error
 */
int redirection_check(char *args[], int size)
{
  const char *redir_error_msg = "Redirection error\n";
  const char *file_error_msg = "Error: opening file\n";

  if (strlen(args[0]) == 1 && args[0][0] == '>')
  {
    write(STDERR_FILENO, redir_error_msg, strlen(redir_error_msg));
    return -2;
  }

  int redir_f = 0;
  for (int i = 0; i < size; i++)
  {
    if (strcmp(args[i], ">") == 0)
    {
      if (redir_f)
      {
        write(STDERR_FILENO, redir_error_msg, strlen(redir_error_msg));
        return -2;
      }
      redir_f = 1;

      if (i + 1 >= size || args[i + 1][0] == '>')
      {
        write(STDERR_FILENO, redir_error_msg, strlen(redir_error_msg));
        return -2;
      }

      if (i + 2 != size)
      {
        write(STDERR_FILENO, redir_error_msg, strlen(redir_error_msg));
        return -2;
      }

      for (int j = i + 1; j < size; j++)
      {
        if (args[j][0] == '>')
        {
          write(STDERR_FILENO, redir_error_msg, strlen(redir_error_msg));
          return -2;
        }
      }

      int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd < 0)
      {
        write(STDERR_FILENO, file_error_msg, strlen(file_error_msg));
        return -2;
      }

      args[i] = NULL;
      return fd;
    }
  }

  return -1;
}
