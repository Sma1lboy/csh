#ifndef COMMANDS_H
#define COMMANDS_H

int env_cmd_check(char *args[], int size);
int exit_cmd(char *args[], int size);
int export_cmd(char *args[], int size);
int unset_cmd(char *args[], int size);
void exec_command(char *cmd);
int builtin_command_check(char *args[], int size);
int pipe_command_check(char *cmd);

#endif
