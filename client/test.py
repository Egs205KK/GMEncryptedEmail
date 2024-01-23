# Import the modules
import socket

# Define the server address and port
server_address = "127.0.0.1" # Change this to your server's IP address
server_port = 25 # Change this to your server's port number

# Define the email content and recipient
email_content = "Hello, this is a test email from Python.\r\n"
email_recipient = "test@example.com" # Change this to your email recipient

# Create a socket and connect to the server
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((server_address, server_port))

# Read the welcome message from the server
welcome_message = sock.recv(1024)
print(welcome_message.decode())

# Send the HELO command and read the response
helo_command = "HELO " + socket.gethostname() + "\r\n"
sock.send(helo_command.encode())
helo_response = sock.recv(1024)
print(helo_response.decode())

# Send the MAIL FROM command and read the response
mail_from_command = "MAIL FROM: " + email_recipient + "\r\n"
sock.send(mail_from_command.encode())
mail_from_response = sock.recv(1024)
print(mail_from_response.decode())

# Send the RCPT TO command and read the response
rcpt_to_command = "RCPT TO: " + email_recipient + "\r\n"
sock.send(rcpt_to_command.encode())
rcpt_to_response = sock.recv(1024)
print(rcpt_to_response.decode())

# Send the DATA command and read the response
data_command = "DATA\r\n"
sock.send(data_command.encode())
data_response = sock.recv(1024)
print(data_response.decode())

# Send the email content and the end of mail input
email_command = email_content + ".\r\n"
sock.send(email_command.encode())
email_response = sock.recv(1024)
print(email_response.decode())

# Send the QUIT command and read the response
quit_command = "QUIT\r\n"
sock.send(quit_command.encode())
quit_response = sock.recv(1024)
print(quit_response.decode())

# Close the socket
sock.close()