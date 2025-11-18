TARGETS = mysh
CC     = gcc
CFLAGS = -g -std=c99 -Wall -Wvla -fsanitize=address,undefined

all: $(TARGETS)

%.o: %.c 
	$(CC) $(CFLAGS) -c -o $@ $<

mysh: mysh.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(TARGETS) *.o *.a *.dylib *.dSYM

.PHONY: all clean