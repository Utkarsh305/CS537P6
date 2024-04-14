.DEFAULT: all

CC = gcc
override CFLAGS += -c -g
override LDFLAGS += -lpthread
SERVER_OBJS = kv_store.o ring_buffer.o
CLIENT_OBJS = client.o ring_buffer.o
TEST_OBJS = test_ring_buffer.o ring_buffer.o
HEADERS = common.h

.PHONY: all, clean
all: client server test

client: $(CLIENT_OBJS)
	$(CC) $(CLIENT_OBJS) $(LDFLAGS) -o $@

server: $(SERVER_OBJS)
	$(CC) $(SERVER_OBJS) $(LDFLAGS) -o $@

test: $(TEST_OBJS)
	$(CC) $(TEST_OBJS) $(LDFLAGS) -g -o $@

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $<

clean: 
	rm -rf $(SERVER_OBJS) $(CLIENT_OBJS) $(TEST_OBJS) server client
