CC = gcc
CFLAGS = -Wall -O2 -pthread
TARGET = udp_client
SRC = udp_client.c

# 默认目标：编译生成可执行文件
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ -lcjson

# 清理目标
clean:
	rm -f $(TARGET)
