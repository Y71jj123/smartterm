# 服务器端编译（跑在 PC / Ubuntu 上）
# gcc chat_server.c cJSON.c -o chat_server -lpthread
CC ?= gcc
chat_server: chat_server.c cJSON.c cJSON.h
	$(CC) -O2 -Wall -o chat_server chat_server.c cJSON.c -lpthread
clean:
	rm -f chat_server
