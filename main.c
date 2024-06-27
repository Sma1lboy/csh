#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "commands.h"
#include "redirection.h"
#include "alias.h"

#define MAX_ARGS_BYTES 512 // Maximum number of bytes for command line arguments

/**
 * main - entry point of the shell program
 * @argc: argument count
 * @argv: argument vector
 *
 * Return: 0 on success, 1 on error
 */
int main(int argc, char *argv[])
{
  const char *prompt_msg = "csh> ";
  const char *read_error_msg = "Error: reading command\n";
  const char *file_open_error_msg = "Error: opening file\n";
  const char *fdopen_error_msg = "Error: fdopen\n";
  const char *strdup_error_msg = "Error: strdup failed\n";
  const char *file_read_error_msg = "Error: reading file\n";

  if (argc == 1)
  {
    char cmd_buffer[MAX_ARGS_BYTES + 1];
    while (1)
    {
      write(STDOUT_FILENO, prompt_msg, strlen(prompt_msg));
      if (fgets(cmd_buffer, MAX_ARGS_BYTES, stdin) == NULL)
      {
        write(STDERR_FILENO, read_error_msg, strlen(read_error_msg));
        exit(1);
      }
      cmd_buffer[strcspn(cmd_buffer, "\n")] = 0;
      cmd_buffer[MAX_ARGS_BYTES] = '\0';

      exec_command(cmd_buffer);
    }
  }
  else if (argc == 2)
  {
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0)
    {
      write(STDERR_FILENO, file_open_error_msg, strlen(file_open_error_msg));
      exit(1);
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    FILE *file = fdopen(fd, "r");
    if (!file)
    {
      write(STDERR_FILENO, fdopen_error_msg, strlen(fdopen_error_msg));
      close(fd);
      exit(1);
    }

    while ((nread = getline(&line, &len, file)) != -1)
    {
      if (nread > 0 && line[nread - 1] == '\n')
      {
        line[nread - 1] = '\0';
      }

      char *line_copy = strdup(line);
      if (line_copy == NULL)
      {
        write(STDERR_FILENO, strdup_error_msg, strlen(strdup_error_msg));
        free(line);
        fclose(file);
        close(fd);
        exit(1);
      }

      if (strlen(line_copy) == 0)
      {
        write(STDOUT_FILENO, "\n", 1);
      }
      else
      {
        write(STDOUT_FILENO, line_copy, strlen(line_copy));
        write(STDOUT_FILENO, "\n", 1);
        exec_command(line_copy);
      }

      free(line_copy);
    }

    if (ferror(file))
    {
      write(STDERR_FILENO, file_read_error_msg, strlen(file_read_error_msg));
    }

    free(line);
    fclose(file);
  }
  return 0;
}
