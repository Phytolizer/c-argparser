SOURCES = arg.c str.c
OBJS = $(patsubst %.c,%.o,$(SOURCES))
CFLAGS = -std=c11 -MD -MP -Wall -Wextra -Wpedantic -Wmissing-prototypes
LIB = libargparser.a

all: $(LIB)

example: $(LIB) main.c
	$(CC) $(CFLAGS) main.c -L. -largparser -o $@

$(LIB): $(OBJS)
	ar rcs $@ $^

.PHONY: clean
clean:
	rm -f $(OBJS) $(LIB)

-include $(SOURCES:.c=.d)
