CC = g++
CFLAGS = -std=c++11 -Wall -O2
INCLUDES = -I./backend/include -I./backend/include/reactor

REACTOR_SRCS = ./backend/src/reactor/InetAddress.cpp \
               ./backend/src/reactor/Socket.cpp \
               ./backend/src/reactor/Epoll.cpp \
               ./backend/src/reactor/Channel.cpp \
               ./backend/src/reactor/EventLoop.cpp \
               ./backend/src/reactor/TcpServer.cpp \
               ./backend/src/reactor/Acceptor.cpp \
               ./backend/src/reactor/Connection.cpp \
               ./backend/src/reactor/Buffer.cpp \
               ./backend/src/reactor/ThreadPool.cpp \
               ./backend/src/reactor/Timestamp.cpp

MAIN_SRC = ./backend/src/main.cpp

TARGET = ai_chat_server

all: $(TARGET)

$(TARGET): $(MAIN_SRC) $(REACTOR_SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TARGET) $(MAIN_SRC) $(REACTOR_SRCS) -lpthread

clean:
	rm -f $(TARGET)
