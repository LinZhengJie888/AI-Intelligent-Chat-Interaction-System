#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
AI智能聊天互动系统 - 测试客户端
用于测试各个业务模块功能
"""

import socket
import json
import time
import sys
import struct

class ChatClient:
    def __init__(self, host='127.0.0.1', port=8080):
        self.host = host
        self.port = port
        self.sock = None
        self.user_id = None
        self._recv_buffer = b""  # 接收缓冲区，用于处理TCP粘包（bytes类型）
        
    def connect(self, max_retries=3):
        """连接服务器（带重试）"""
        for attempt in range(max_retries):
            try:
                self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self.sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)  # 禁用Nagle算法
                self.sock.connect((self.host, self.port))
                self._recv_buffer = b""  # 清空接收缓冲区
                time.sleep(0.2)  # 等待服务端事件循环就绪
                print(f"[✓] 已连接到服务器 {self.host}:{self.port}")
                return True
            except Exception as e:
                if attempt < max_retries - 1:
                    print(f"[!] 连接失败，重试 {attempt + 2}/{max_retries}...")
                    time.sleep(0.5)
                else:
                    print(f"[✗] 连接失败: {e}")
                    return False
    
    def disconnect(self):
        """断开连接"""
        if self.sock:
            self.sock.close()
            self.sock = None
            self._recv_buffer = b""  # 使用bytes类型
            print("[✓] 已断开连接")
    
    def _recv_exact(self, nbytes, timeout=10.0):
        """
        精确接收指定字节数的数据
        处理TCP粘包/拆包问题
        """
        self.sock.settimeout(timeout)
        while len(self._recv_buffer) < nbytes:
            try:
                data = self.sock.recv(4096)
                if not data:
                    return None  # 连接已关闭
                self._recv_buffer += data
            except socket.timeout:
                return None  # 超时
            except Exception as e:
                print(f"[错误] 接收数据失败: {e}")
                return None
        
        # 从缓冲区取出所需数据
        result = self._recv_buffer[:nbytes]
        self._recv_buffer = self._recv_buffer[nbytes:]
        return result
    
    def recv_message(self, timeout=10.0):
        """
        接收一条完整消息（4字节长度头 + JSON消息体）
        返回解析后的dict，超时或连接关闭返回None
        """
        # 读取4字节长度头
        header = self._recv_exact(4, timeout)
        if header is None:
            return None
        
        # 解析长度（网络字节序，大端）
        msg_len = struct.unpack('!I', header)[0]
        
        # 安全检查
        if msg_len > 1024 * 1024:  # 限制消息最大1MB
            print(f"[错误] 消息长度过大: {msg_len}")
            return None
        
        # 读取消息体
        body = self._recv_exact(msg_len, timeout)
        if body is None:
            return None
        
        # 解析JSON
        try:
            return json.loads(body.decode('utf-8'))
        except json.JSONDecodeError as e:
            print(f"[警告] JSON解析失败: {body[:100]}...")
            return None
    
    def send_message(self, msg_type, from_user_id="", to_user_id="", content="", extra=None):
        """发送消息（4字节长度头 + JSON消息体）"""
        message = {
            "type": msg_type,
            "from_user_id": from_user_id,
            "to_user_id": to_user_id,
            "content": content,
            "extra": json.dumps(extra) if extra else "",
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S")
        }
        
        msg_bytes = json.dumps(message).encode('utf-8')
        header = struct.pack('!I', len(msg_bytes))  # 4字节长度头（网络字节序）
        
        print(f"\n[发送] {json.dumps(message, ensure_ascii=False)}")
        
        try:
            self.sock.sendall(header + msg_bytes)  # 发送长度头+消息体
            response = self.recv_message(timeout=10.0)  # 接收响应
            if response:
                print(f"[接收] {json.dumps(response, ensure_ascii=False)}")
            return response
        except Exception as e:
            print(f"[✗] 通信错误: {e}")
            return None
    
    def test_register(self, user_id, username, password):
        """测试注册功能"""
        print("\n" + "="*50)
        print("测试: 用户注册")
        print("="*50)
        
        extra = {
            "username": username,
            "password": password
        }
        
        return self.send_message(2, from_user_id=user_id, extra=extra)
    
    def test_login(self, user_id, password, captcha="", captcha_token=""):
        """测试登录功能"""
        print("\n" + "="*50)
        print("测试: 用户登录")
        print("="*50)
        
        extra = {
            "password": password,
            "captcha": captcha,
            "captcha_token": captcha_token
        }
        
        result = self.send_message(1, from_user_id=user_id, extra=extra)
        if result and result.get('code') == 0:
            self.user_id = user_id
        return result
    
    def test_get_captcha(self, phone="13800138000"):
        """测试获取验证码"""
        print("\n" + "="*50)
        print("测试: 获取验证码")
        print("="*50)
        
        extra = {"phone": phone}
        return self.send_message(10, extra=extra)
    
    def test_friend_add(self, from_user_id, to_user_id, msg="请求添加好友"):
        """测试添加好友"""
        print("\n" + "="*50)
        print("测试: 添加好友")
        print("="*50)
        
        return self.send_message(20, from_user_id=from_user_id, 
                                to_user_id=to_user_id, content=msg)
    
    def test_friend_agree(self, from_user_id, to_user_id):
        """测试同意好友请求"""
        print("\n" + "="*50)
        print("测试: 同意好友请求")
        print("="*50)
        
        return self.send_message(21, from_user_id=from_user_id, 
                                to_user_id=to_user_id)
    
    def test_friend_reject(self, from_user_id, to_user_id):
        """测试拒绝好友请求"""
        print("\n" + "="*50)
        print("测试: 拒绝好友请求")
        print("="*50)
        
        return self.send_message(22, from_user_id=from_user_id, 
                                to_user_id=to_user_id)
    
    def test_friend_list(self, user_id):
        """测试获取好友列表"""
        print("\n" + "="*50)
        print("测试: 获取好友列表")
        print("="*50)
        
        return self.send_message(23, from_user_id=user_id)
    
    def test_group_create(self, user_id, group_name):
        """测试创建群聊"""
        print("\n" + "="*50)
        print("测试: 创建群聊")
        print("="*50)
        
        extra = {"group_name": group_name}
        return self.send_message(30, from_user_id=user_id, extra=extra)
    
    def test_group_join(self, user_id, group_id, msg="请求加入群聊"):
        """测试加入群聊"""
        print("\n" + "="*50)
        print("测试: 加入群聊")
        print("="*50)
        
        return self.send_message(31, from_user_id=user_id, 
                                to_user_id=group_id, content=msg)
    
    def test_group_agree(self, operator_id, user_id, group_id):
        """测试同意加群申请"""
        print("\n" + "="*50)
        print("测试: 同意加群申请")
        print("="*50)
        
        extra = {"group_id": group_id}
        return self.send_message(32, from_user_id=operator_id, 
                                to_user_id=user_id, extra=extra)
    
    def test_group_members(self, group_id):
        """测试获取群成员列表"""
        print("\n" + "="*50)
        print("测试: 获取群成员列表")
        print("="*50)
        
        return self.send_message(35, to_user_id=group_id)
    
    def test_group_list(self, user_id):
        """测试获取群列表"""
        print("\n" + "="*50)
        print("测试: 获取群列表")
        print("="*50)
        
        return self.send_message(36, from_user_id=user_id)
    
    def test_private_chat(self, from_user_id, to_user_id, content):
        """测试私聊消息"""
        print("\n" + "="*50)
        print("测试: 私聊消息")
        print("="*50)
        
        return self.send_message(40, from_user_id=from_user_id, 
                                to_user_id=to_user_id, content=content)
    
    def test_ai_request(self, user_id, target_id, question, is_group=False):
        """测试AI请求"""
        print("\n" + "="*50)
        print("测试: AI请求")
        print("="*50)
        
        extra = {"is_group": is_group}
        return self.send_message(50, from_user_id=user_id, 
                                to_user_id=target_id, content=question, extra=extra)
    
    def test_ai_at(self, user_id, target_id, question, is_group=False):
        """测试AI @召唤"""
        print("\n" + "="*50)
        print("测试: AI @召唤")
        print("="*50)
        
        extra = {"is_group": is_group}
        return self.send_message(51, from_user_id=user_id, 
                                to_user_id=target_id, content=question, extra=extra)
    
    def test_ai_setting(self, user_id, nickname="AI助手", tone=0, priority=0):
        """测试AI设置"""
        print("\n" + "="*50)
        print("测试: AI设置")
        print("="*50)
        
        extra = {
            "ai_nickname": nickname,
            "ai_tone": tone,
            "ai_priority": priority
        }
        return self.send_message(52, from_user_id=user_id, extra=extra)


def run_full_test():
    """运行完整测试流程"""
    print("\n" + "="*60)
    print("AI智能聊天互动系统 - 功能测试")
    print("="*60)
    
    # 创建两个客户端
    client1 = ChatClient()
    client2 = ChatClient()
    
    # 连接服务器
    if not client1.connect() or not client2.connect():
        print("连接服务器失败，请确保服务器已启动")
        return
    
    try:
        # 1. 测试注册
        print("\n\n>>> 1. 测试用户注册")
        client1.test_register("user001", "张三", "123456")
        time.sleep(0.5)
        
        client2.test_register("user002", "李四", "123456")
        time.sleep(0.5)
        
        # 2. 测试获取验证码
        print("\n\n>>> 2. 测试获取验证码")
        captcha_result = client1.test_get_captcha("13800138000")
        time.sleep(0.5)
        
        # 3. 测试登录（不带验证码，应该失败）
        print("\n\n>>> 3. 测试登录（无验证码）")
        client1.test_login("user001", "123456")
        time.sleep(0.5)
        
        # 4. 测试好友添加
        print("\n\n>>> 4. 测试添加好友")
        client1.test_friend_add("user001", "user002", "你好，我是张三")
        time.sleep(0.5)
        
        # 5. 测试获取好友列表（应该为空）
        print("\n\n>>> 5. 测试获取好友列表")
        client1.test_friend_list("user001")
        time.sleep(0.5)
        
        # 6. 测试同意好友请求
        print("\n\n>>> 6. 测试同意好友请求")
        client2.test_friend_agree("user002", "user001")
        time.sleep(0.5)
        
        # 7. 再次获取好友列表（应该有user002）
        print("\n\n>>> 7. 再次获取好友列表")
        client1.test_friend_list("user001")
        time.sleep(0.5)
        
        # 8. 测试私聊消息
        print("\n\n>>> 8. 测试私聊消息")
        client1.test_private_chat("user001", "user002", "你好，李四！")
        time.sleep(0.5)
        
        # 9. 测试创建群聊
        print("\n\n>>> 9. 测试创建群聊")
        group_result = client1.test_group_create("user001", "测试群聊")
        time.sleep(0.5)
        
        # 10. 测试获取群列表
        print("\n\n>>> 10. 测试获取群列表")
        client1.test_group_list("user001")
        time.sleep(0.5)
        
        # 11. 测试AI设置
        print("\n\n>>> 11. 测试AI设置")
        client1.test_ai_setting("user001", "小智", 0, 0)
        time.sleep(0.5)
        
        # 12. 测试AI请求
        print("\n\n>>> 12. 测试AI请求")
        client1.test_ai_request("user001", "user002", "你好，请介绍一下自己")
        time.sleep(1)
        
        print("\n\n" + "="*60)
        print("测试完成！")
        print("="*60)
        
    except KeyboardInterrupt:
        print("\n测试被中断")
    finally:
        client1.disconnect()
        client2.disconnect()


def run_single_test(test_name):
    """运行单个测试"""
    client = ChatClient()
    if not client.connect():
        return
    
    try:
        if test_name == "register":
            user_id = input("请输入用户ID: ")
            username = input("请输入用户名: ")
            password = input("请输入密码: ")
            client.test_register(user_id, username, password)
            
        elif test_name == "captcha":
            phone = input("请输入手机号 (默认13800138000): ") or "13800138000"
            client.test_get_captcha(phone)
            
        elif test_name == "login":
            user_id = input("请输入用户ID: ")
            password = input("请输入密码: ")
            client.test_login(user_id, password)
            
        elif test_name == "friend_add":
            from_id = input("请输入你的用户ID: ")
            to_id = input("请输入对方用户ID: ")
            msg = input("请输入验证消息 (默认: 请求添加好友): ") or "请求添加好友"
            client.test_friend_add(from_id, to_id, msg)
            
        elif test_name == "friend_list":
            user_id = input("请输入用户ID: ")
            client.test_friend_list(user_id)
            
        elif test_name == "group_create":
            user_id = input("请输入你的用户ID: ")
            group_name = input("请输入群名称: ")
            client.test_group_create(user_id, group_name)
            
        elif test_name == "ai":
            user_id = input("请输入你的用户ID: ")
            question = input("请输入问题: ")
            client.test_ai_request(user_id, "", question)
            # 等待服务端异步推送的 AI 回复，最多等待 30 秒
            print("\n[等待AI回复...]")
            start_time = time.time()
            ai_messages = []
            while time.time() - start_time < 30.0:
                remaining = 30.0 - (time.time() - start_time)
                if remaining <= 0:
                    break
                result = client.recv_message(timeout=min(remaining, 5.0))
                if result is None:
                    continue  # 超时或无数据，继续等待
                # 打印收到的推送消息
                print(f"[接收推送] {json.dumps(result, ensure_ascii=False)}")
                ai_messages.append(result)
                # 检查是否收到AI回复（type=40且from_user_id包含"小"或"AI"）
                if result.get('type') == 40:
                    content = result.get('data', {}).get('content', '') if 'data' in result else result.get('content', '')
                    if content:
                        print(f"  -> AI回复: {content}")
            if not ai_messages:
                print("[提示] 未收到任何AI回复")
            else:
                print(f"\n[完成] 共收到 {len(ai_messages)} 条消息")
            
        else:
            print(f"未知测试: {test_name}")
            print("可用测试: register, captcha, login, friend_add, friend_list, group_create, ai")
            
    finally:
        client.disconnect()


def show_menu():
    """显示菜单"""
    print("\n" + "="*50)
    print("AI智能聊天互动系统 - 测试客户端")
    print("="*50)
    print("1. 运行完整测试流程")
    print("2. 测试注册")
    print("3. 测试验证码")
    print("4. 测试登录")
    print("5. 测试添加好友")
    print("6. 测试好友列表")
    print("7. 测试创建群聊")
    print("8. 测试AI对话")
    print("0. 退出")
    print("="*50)


if __name__ == "__main__":
    if len(sys.argv) > 1:
        if sys.argv[1] == "--full" or sys.argv[1] == "-f":
            run_full_test()
        else:
            run_single_test(sys.argv[1])
    else:
        while True:
            show_menu()
            choice = input("\n请选择操作: ").strip()
            
            if choice == "0":
                print("退出测试客户端")
                break
            elif choice == "1":
                run_full_test()
            elif choice == "2":
                run_single_test("register")
            elif choice == "3":
                run_single_test("captcha")
            elif choice == "4":
                run_single_test("login")
            elif choice == "5":
                run_single_test("friend_add")
            elif choice == "6":
                run_single_test("friend_list")
            elif choice == "7":
                run_single_test("group_create")
            elif choice == "8":
                run_single_test("ai")
            else:
                print("无效选择，请重新输入")
