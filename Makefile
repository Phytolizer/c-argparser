SOURCES = arg.c str.c
OBJS = $(patsubst %.c,%.o,$(SOURCES))
LIB = libargparser.a

$(LIB): $(OBJS)
	ar rcs $@ $^

clean:
	rm -f $(OBJS) $(LIB)

