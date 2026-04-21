CC ?= cc
AR ?= ar
ARFLAGS ?= rcs
CFLAGS ?= -Wall -Wextra -Werror -std=c11
CURSES_LIBS ?= -lcurses

LIB_SRCS = buffer_api.c buffer_array.c buffer_list.c thread_buffer.c
LIB_OBJS = $(LIB_SRCS:.c=.o)

STATIC_LIB = libcb.a
SHARED_LIB = libcb.so
DYNAMIC_LIB = libcb.dylib
TEST_BIN = test.out
TEST_DYNAMIC_BIN = test_dynamic.out

.PHONY: all static shared dynamic test test-dynamic clean

all: static shared dynamic test test-dynamic

static: $(STATIC_LIB)

shared: $(SHARED_LIB)

dynamic: $(DYNAMIC_LIB)

test: $(TEST_BIN)

test-dynamic: $(TEST_DYNAMIC_BIN)

$(STATIC_LIB): $(LIB_OBJS)
	$(AR) $(ARFLAGS) $@ $^

$(SHARED_LIB): $(LIB_OBJS)
	$(CC) -shared -o $@ $^

$(DYNAMIC_LIB): $(LIB_OBJS)
	$(CC) -dynamiclib -o $@ $^

$(TEST_BIN): test.c $(STATIC_LIB)
	$(CC) $(CFLAGS) -o $@ test.c ./$(STATIC_LIB) $(CURSES_LIBS)

$(TEST_DYNAMIC_BIN): test.c $(DYNAMIC_LIB)
	$(CC) $(CFLAGS) -o $@ test.c -L. -lcb -Wl,-rpath,@executable_path $(CURSES_LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(LIB_OBJS) $(STATIC_LIB) $(SHARED_LIB) $(DYNAMIC_LIB) $(TEST_BIN) $(TEST_DYNAMIC_BIN)
