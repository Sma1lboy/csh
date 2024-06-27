#ifndef ALIAS_H
#define ALIAS_H

typedef struct alias_q
{
  char *key;
  char *value;
  struct alias_q *next;
} alias_q;

extern alias_q *header_alias_q;

alias_q *create_alias(char *key, char *value);
alias_q *get_alias(char *key);
int add_alias(alias_q *a);
int alias_cmd(char *args[], int size);

#endif
