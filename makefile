# default compiler 
CC = gcc

# option 
CFLAGS = -Wall -Wextra -std=c99

# output file 
TARGET = csh

# source 
SRCS = main.c commands.c redirection.c alias.c fdwrite.c

OBJS = $(SRCS:.c=.o)

# HHeader
DEPS = commands.h redirection.h alias.h fdwrite.h

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@


install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/$(TARGET)


clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
