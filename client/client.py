import socket

# 定义服务器地址和端口
server_address = "127.0.0.1"
server_port = 25

# 定义电子邮件内容和收件人
email_content = "Hello, 这是来自Python的测试邮件。\r\n"
email_recipient = "test@example.com"

# 创建一个套接字并连接到服务器
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((server_address, server_port))

# 从服务器读取欢迎消息
welcome_message = sock.recv(1024)
print(welcome_message.decode())

# 发送HELO命令并读取响应
helo_command = "HELO " + socket.gethostname() + "\r\n"
sock.send(helo_command.encode())
helo_response = sock.recv(1024)
print(helo_response.decode())

# 发送MAIL FROM命令并读取响应
mail_from_command = "MAIL FROM: " + email_recipient + "\r\n"
sock.send(mail_from_command.encode())
mail_from_response = sock.recv(1024)
print(mail_from_response.decode())

# 发送RCPT TO命令并读取响应
rcpt_to_command = "RCPT TO: " + email_recipient + "\r\n"
sock.send(rcpt_to_command.encode())
rcpt_to_response = sock.recv(1024)
print(rcpt_to_response.decode())

# 发送DATA命令并读取响应
data_command = "DATA\r\n"
sock.send(data_command.encode())
data_response = sock.recv(1024)
print(data_response.decode())

# 构造电子邮件的头部和内容
email_header = "Subject: 来自AI助手的问候\r\n"
email_header += "From: sender@example.com\r\n"
email_header += "To: recipient@example.com\r\n"
email_header += "\r\n"

# 发送电子邮件的头部和内容
sock.send(email_header.encode())
sock.send(email_content.encode())

# 发送邮件输入结束符号
sock.send(".\r\n".encode())
email_response = sock.recv(1024)
print(email_response.decode())

# 发送QUIT命令并读取响应
quit_command = "QUIT\r\n"
sock.send(quit_command.encode())
quit_response = sock.recv(1024)
print(quit_response.decode())

# 关闭套接字
sock.close()
