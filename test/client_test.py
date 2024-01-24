import socket
import time

# 定义服务器地址和端口
SERVER_ADDRESS = 'localhost'
SERVER_PORT = 25

# 创建socket对象
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# 连接到服务器
sock.connect((SERVER_ADDRESS, SERVER_PORT))

# 接收服务器的欢迎消息
response = sock.recv(1024)
print(response.decode())

# 发送HELO指令
sock.sendall(b'HELO example.com\r\n')
response = sock.recv(1024)
print(response.decode())

# 发送MAIL FROM指令
sock.sendall(b'MAIL FROM: <sender@example.com>\r\n')
response = sock.recv(1024)
print(response.decode())

# 发送RCPT TO指令
sock.sendall(b'RCPT TO: <recipient@example.com>\r\n')
response = sock.recv(1024)
print(response.decode())

# 发送DATA指令
sock.sendall(b'DATA\r\n')
response = sock.recv(1024)
print(response.decode())

# 发送邮件内容
current_time = time.strftime("%a, %d %b %Y %H:%M:%S +0000", time.gmtime())
subject = 'Subject: Test email\r\n'
from_addr = 'From: sender@example.com\r\n'
to_addr = 'To: recipient@example.com\r\n'
date = f'Date: {current_time}\r\n'
message_body = 'This is a test email.\r\n'


message = subject + from_addr + to_addr + date + '\r\n' + message_body

sock.sendall(message.encode())

# 发送结束符号
sock.sendall(b'.\r\n')
response = sock.recv(1024)
print(response.decode())

# 发送QUIT指令
sock.sendall(b'QUIT\r\n')
response = sock.recv(1024)
print(response.decode())

# 关闭socket连接
sock.close()
