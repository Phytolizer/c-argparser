SOURCES = arg.c str.c
OBJS = $(patsubst %.c,%.o,$(SOURCES))
CFLAGS = -std=c11 -MD -MP
LIB = libargparser.a

all: $(LIB)

$(LIB): $(OBJS)
	ar rcs $@ $^

.PHONY: clean
clean:
	rm -f $(OBJS) $(LIB)

-include $(SOURCES:.c=.d)
