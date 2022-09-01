SOURCES = arg.c str.c
OBJS = $(patsubst %.c,%.o,$(SOURCES))
CFLAGS = -std=c11
LIB = libargparser.a

$(LIB): $(OBJS)
	ar rcs $@ $^

clean:
	rm -f $(OBJS) $(LIB)
