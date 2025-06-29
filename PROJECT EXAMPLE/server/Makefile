##
## EPITECH PROJECT, 2025
## zappy_server
## File description:
## Makefile
##

CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -g
LDFLAGS = -lm

SRCDIR = src
OBJDIR = obj
BINDIR = bin

SRC = $(wildcard $(SRCDIR)/*.c)
OBJ = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRC))
DEPS = $(wildcard include/*.h)

SERVER = zappy_server

all: $(SERVER)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(DEPS) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

$(SERVER): $(OBJ) | $(BINDIR)
	$(CC) $(CFLAGS) $^ -o $(BINDIR)/$@ $(LDFLAGS)

clean:
	rm -rf $(OBJDIR) $(BINDIR)

re: clean all

.PHONY: all clean re
