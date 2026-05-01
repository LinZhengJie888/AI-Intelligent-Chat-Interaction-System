CC = g++
CFLAGS = -std=c++11 -Wall -O2
INCLUDES = -I./backend/include -I./backend/include/reactor \
           -I./backend/include/model -I./backend/include/module \
           -I./backend/include/common

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

MODEL_SRCS = ./backend/src/model/UserDAO.cpp \
             ./backend/src/model/ChatRecordDAO.cpp

MODULE_SRCS = ./backend/src/module/Database.cpp \
              ./backend/src/module/Config.cpp

COMMON_SRCS = ./backend/src/common/Util.cpp

MAIN_SRC = ./backend/src/main.cpp

TARGET = ai_chat_server

all: $(TARGET)

$(TARGET): $(MAIN_SRC) $(REACTOR_SRCS) $(MODEL_SRCS) $(MODULE_SRCS) $(COMMON_SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TARGET) $(MAIN_SRC) $(REACTOR_SRCS) $(MODEL_SRCS) $(MODULE_SRCS) $(COMMON_SRCS) -lpthread -lmysqlclient -lcrypto

clean:
	rm -f $(TARGET)
