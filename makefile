CC = gcc
CFLAGS = -g -Wall -Wshadow -Wunreachable-code -Wredundant-decls -Wmissing-declarations -Wold-style-definition -Wmissing-prototypes -Wdeclaration-after-statement -Wno-return-local-addr -Wuninitialized -Wextra -Wunused
OBJS = psush.o
INCLUDES = psush.h

all: psush

psush: $(OBJS)
	$(CC) $(OBJS) -o psush

%.o: %.c $(INCLUDES)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o psush

DIR = $(shell basename $(PWD))
canvas:
	cd .. ; tar --exclude-backups --exclude-vcs -c -a -f ./$(LOGNAME)-$(DIR)-Lab.tar.gz $(DIR)

