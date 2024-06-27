#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include "commands.h"
#include "redirection.h"
#include "alias.h"
#include "fdwrite.h"

#define MAX_ARGS_BYTES 512 // Maximum number of bytes for command line arguments

int env_initializer(char *exec_path)
{
  if (setenv("SHELL", exec_path, 1) == -1)
  {
    fdwrite(STDERR_FILENO, "Error: setting SHELL environment variable\n");
    return 0;
  }
  return 1;
}

void run(int argc, char *argv[])
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
      fdwrite(STDOUT_FILENO, prompt_msg);
      if (fgets(cmd_buffer, MAX_ARGS_BYTES, stdin) == NULL)
      {
        fdwrite(STDERR_FILENO, read_error_msg);
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
      fdwrite(STDERR_FILENO, file_open_error_msg);
      exit(1);
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    FILE *file = fdopen(fd, "r");
    if (!file)
    {
      fdwrite(STDERR_FILENO, fdopen_error_msg);
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
        fdwrite(STDERR_FILENO, strdup_error_msg);
        free(line);
        fclose(file);
        close(fd);
        exit(1);
      }

      if (strlen(line_copy) == 0)
      {
        fdwrite(STDOUT_FILENO, "\n");
      }
      else
      {
        fdwrite(STDOUT_FILENO, line_copy);
        fdwrite(STDOUT_FILENO, "\n");
        exec_command(line_copy);
      }

      free(line_copy);
    }

    if (ferror(file))
    {
      fdwrite(STDERR_FILENO, file_read_error_msg);
    }

    free(line);
    fclose(file);
  }
}

/**
 * get_executable_path - get the absolute path of the current executable
 * @abs_path: buffer to store the absolute path
 * @size: size of the buffer
 *
 * Return: 0 on success, -1 on failure
 */
int get_executable_path(char *abs_path, size_t size)
{
#ifdef __APPLE__
  uint32_t path_size = (uint32_t)size;
  if (_NSGetExecutablePath(abs_path, &path_size) != 0)
  {
    return -1;
  }
  return 0;
#elif __linux__
  ssize_t len = readlink("/proc/self/exe", abs_path, size - 1);
  if (len == -1)
  {
    return -1;
  }
  abs_path[len] = '\0';
  return 0;
#else
#error "Unsupported platform"
#endif
}

/**
 * init_config - initialize configuration from ~/.cshrc
 *
 * Return: 0 on success, -1 on failure
 */
int init_config()
{
  const char *home = getenv("HOME");
  if (!home)
  {
    fdwrite(STDERR_FILENO, "Error: HOME environment variable not set\n");
    return -1;
  }

  char config_path[PATH_MAX];
  snprintf(config_path, sizeof(config_path), "%s/.cshrc", home);

  FILE *file = fopen(config_path, "r");
  if (!file)
  {
    // If the file doesn't exist, create it and add a comment
    file = fopen(config_path, "w");
    if (!file)
    {
      fdwrite(STDERR_FILENO, "Error: creating ~/.cshrc\n");
      return -1;
    }
    fprintf(file, "# This is your csh configuration file\n");
    fclose(file);
    return 0;
  }

  char *line = NULL;
  size_t len = 0;
  ssize_t nread;

  while ((nread = getline(&line, &len, file)) != -1)
  {
    if (nread > 0 && line[nread - 1] == '\n')
    {
      line[nread - 1] = '\0';
    }

    // Skip comments and empty lines
    if (line[0] == '#' || strlen(line) == 0)
    {
      continue;
    }

    char *line_copy = strdup(line);
    if (line_copy == NULL)
    {
      fdwrite(STDERR_FILENO, "Error: strdup failed\n");
      free(line);
      fclose(file);
      return -1;
    }

    exec_command(line_copy);
    free(line_copy);
  }

  if (ferror(file))
  {
    fdwrite(STDERR_FILENO, "Error: reading ~/.cshrc\n");
    free(line);
    fclose(file);
    return -1;
  }

  free(line);
  fclose(file);
  return 0;
}

/**
 * main - entry point of the shell program
 * @argc: argument count
 * @argv: argument vector
 *
 * Return: 0 on success, 1 on error
 */
int main(int argc, char *argv[])
{
  char abs_path[PATH_MAX];
  if (get_executable_path(abs_path, sizeof(abs_path)) != 0)
  {
    fprintf(stderr, "Error resolving absolute path\n");
    return EXIT_FAILURE;
  }
  if (!env_initializer(abs_path))
  {
    return EXIT_FAILURE;
  }
  if (init_config() != 0)
  {
    return EXIT_FAILURE;
  }
  run(argc, argv);
  return 0;
}
