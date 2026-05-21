#!/bin/bash

set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$PROJECT_DIR"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

cleanup() {
    trap - SIGINT SIGTERM
    echo -e "\n${YELLOW}正在停止所有服务...${NC}"
    kill $BACKEND_PID $PROXY_PID $FRONTEND_PID 2>/dev/null
    wait
    exit 0
}

trap cleanup SIGINT SIGTERM

echo -e "${GREEN}================================${NC}"
echo -e "${GREEN}  AI智能聊天互动系统启动脚本${NC}"
echo -e "${GREEN}================================${NC}"

echo -e "\n${YELLOW}[1/3] 编译C++后端...${NC}"
if [ -f "ai_chat_server" ]; then
    echo -e "${GREEN}后端可执行文件已存在，跳过编译${NC}"
else
    make clean && make
    if [ $? -ne 0 ]; then
        echo -e "${RED}编译失败，请检查错误信息${NC}"
        exit 1
    fi
fi

echo -e "\n${YELLOW}[2/3] 启动C++后端服务器...${NC}"
./ai_chat_server &
BACKEND_PID=$!
sleep 2

if ! kill -0 $BACKEND_PID 2>/dev/null; then
    echo -e "${RED}后端服务器启动失败${NC}"
    exit 1
fi
echo -e "${GREEN}后端服务器已启动 (PID: $BACKEND_PID)${NC}"

echo -e "\n${YELLOW}[3/3] 启动前端和代理服务器...${NC}"
npm run dev:proxy &
PROXY_PID=$!
npm run dev:frontend &
FRONTEND_PID=$!

echo -e "\n${GREEN}================================${NC}"
echo -e "${GREEN}  所有服务已启动!${NC}"
echo -e "${GREEN}================================${NC}"
echo -e "后端服务器: PID $BACKEND_PID"
echo -e "前端服务: PID $FRONTEND_PID"
echo -e "\n${YELLOW}按 Ctrl+C 停止所有服务${NC}\n"

wait
