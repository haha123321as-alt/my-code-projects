CC = gcc

CFLAGS = -Wall -Wextra -Iinclude -g
LDFLAGS = -lrt

SRC_DIR = src
BUILD_DIR = build

COMMON = $(SRC_DIR)/ipc.c

SOCKET_SRC = $(SRC_DIR)/socket_ipc.c
FIFO_SRC = $(SRC_DIR)/fifo_ipc.c
MQ_SRC = $(SRC_DIR)/mq_ipc.c

SENDER_SRC = $(SRC_DIR)/sender.c
RECEIVER_SRC = $(SRC_DIR)/receiver.c

HEADERS = include/ipc.h include/socket_ipc.h include/fifo_ipc.h include/mq_ipc.h

SENDER = $(BUILD_DIR)/sender
RECEIVER = $(BUILD_DIR)/receiver

.PHONY: all clean rebuild

all: $(SENDER) $(RECEIVER)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(SENDER): $(SENDER_SRC) $(COMMON) $(SOCKET_SRC) $(FIFO_SRC) $(MQ_SRC) $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(filter %.c,$^) -o $@ $(LDFLAGS)

$(RECEIVER): $(RECEIVER_SRC) $(COMMON) $(SOCKET_SRC) $(FIFO_SRC) $(MQ_SRC) $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(filter %.c,$^) -o $@ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)
	rm -f /tmp/ipc_task1_socket
	rm -f /tmp/ipc_task1_fifo
	rm -f /dev/mqueue/ipc_task1_mq

rebuild: clean all

