# default compiler 
CC = gcc

# option 
CFLAGS = -Wall -Wextra -std=c99

# output file 
TARGET = csh

# source 
SRCS = main.c commands.c redirection.c alias.c 

OBJS = $(SRCS:.c=.o)

# HHeader
DEPS = commands.h redirection.h alias.h 

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
