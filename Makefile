CC = gcc
CFLAGS = -Wall -Wextra -std=c99
INCLUDES = -Iinclude

# Library sources
LIB_SRC = $(wildcard src/*.c)
LIB_OBJ = $(LIB_SRC:.c=.o)

# Examples
EXAMPLES = examples/basic examples/layout examples/widget examples/filer examples/finder examples/sysmon examples/todo examples/gitui

all: $(EXAMPLES)

examples/%: examples/%.c $(LIB_OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

src/%.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

clean:
	rm -f src/*.o $(EXAMPLES)

format:
	clang-format -i src/*.c include/*.h examples/*.c

.PHONY: all clean format
