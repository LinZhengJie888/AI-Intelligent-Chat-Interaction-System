#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import socket
import json
import time

def test_once(user_id, question):
    print(f"\n{'='*50}")
    print(f"测试 {user_id} 的问题: {question}")
    print(f"{'='*50}")
    
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        client.connect(('127.0.0.1', 8080))
        print("✓ 已连接")
        
        message = {
            "type": 50, 
            "from_user_id": user_id, 
            "to_user_id": "", 
            "content": question, 
            "extra": json.dumps({"is_group": False}), 
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S")
        }
        msg_str = json.dumps(message)
        
        print(f"发送: {msg_str}")
        client.send(msg_str.encode('utf-8'))
        
        client.settimeout(2.0)
        try:
            response = client.recv(4096).decode('utf-8')
            print(f"接收响应: {response}")
        except socket.timeout:
            print("✗ 等待响应超时！")
            
        client.close()
        print("✓ 已断开")
    except Exception as e:
        print(f"✗ 错误: {e}")
    finally:
        try:
            client.close()
        except:
            pass

if __name__ == "__main__":
    test_once("user001", "你好")
    print("\n\n等待 2 秒...\n")
    time.sleep(2)
    test_once("user001", "今天天气怎么样")
