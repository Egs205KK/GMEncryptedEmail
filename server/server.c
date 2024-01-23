// server.c
// Linux系统的SMTP服务器例子，给网络组的同志参考
// 用 gcc server.c -o server 编译
// 用 ./server 运行

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 25 // SMTP的端口
#define BUFFER_SIZE 1024 // socket通信的缓存区
#define FILE_NAME "mail.txt" // 保存邮件的文件名

// 发送消息到客户端并检查是否出错的函数
void send_message(int sockfd, char* message) {
    int n = write(sockfd, message, strlen(message));
    if (n < 0) {
        perror("Error writing to socket");
        exit(1);
    }
}

// 从客户端读取信息并检查是否出错的函数
void read_message(int sockfd, char* buffer) {
    bzero(buffer, BUFFER_SIZE); // 清空缓存区
    int n = read(sockfd, buffer, BUFFER_SIZE - 1);
    if (n < 0) {
        perror("Error reading from socket");
        exit(1);
    }
}

// 解析缓存区里邮件内容的函数
void parse_mail(char* buffer, char* mail) {
    char* start = strstr(buffer, "\r\n\r\n"); // 查找邮件开头
    if (start == NULL) {
        strcpy(mail, ""); // 没找到内容
    } else {
        start += 4; // 跳过 \r\n\r\n
        char* end = strstr(start, "\r\n.\r\n"); // 找到邮件的结尾
        if (end == NULL) {
            strcpy(mail, start); // 复制整个缓存区作为邮件内容
        } else {
            strncpy(mail, start, end - start); // 复制邮件内容到结尾
            mail[end - start] = '\0'; // 加上字符串结尾的标志
        }
    }
}

// 保存邮件内容到一个文件的函数
void save_mail(char* mail) {
    FILE* fp = fopen(FILE_NAME, "a"); // 用追加模式打开文件
    if (fp == NULL) {
        perror("Error opening file");
        exit(1);
    }
    fprintf(fp, "%s\n", mail); // 把邮件内容写到文件里
    fclose(fp); // 关闭文件
}

// 主函数
int main() {
    int sockfd, newsockfd; // socket文件描述符
    struct sockaddr_in serv_addr, cli_addr; // 服务器地址和客户端地址
    socklen_t clilen; // 客户端地址长度
    char buffer[BUFFER_SIZE]; // socket通信地址长度
    char mail[BUFFER_SIZE]; // 邮件内容的缓存区

    // 创建监听用的socket，检查是否成功
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        exit(1);
    }

    // 初始化服务器地址的结构体
    bzero((char *) &serv_addr, sizeof(serv_addr)); // Clear the structure
    serv_addr.sin_family = AF_INET; // Set the address family
    serv_addr.sin_addr.s_addr = INADDR_ANY; // Set the IP address
    serv_addr.sin_port = htons(PORT); // Set the port number

    // 把socket绑定到服务器的端口上然后检查是否出错
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error binding socket");
        exit(1);
    }

    // 监听端口并检查是否出错
    if (listen(sockfd, 5) < 0) {
        perror("Error listening on socket");
        exit(1);
    }

    // 接收客户端的连接
    clilen = sizeof(cli_addr); // 设置客户端地址结构体的长度
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); // 接收客户端的连接并得到通信的socket文件描述符
    if (newsockfd < 0) {
        perror("Error accepting connection");
        exit(1);
    }

    // print客户端的地址和端口
    printf("Connected to client %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

    // 发送welcome信息给客户端
    send_message(newsockfd, "220 Welcome to my SMTP server\r\n");

    // 进入跟客户端通信的循环
    int quit = 0; // 记录客户端是否发送了退出指令的标志
    int mail_from = 0; // 记录客户端是否发送过MAIL FROM指令的标志
    int rcpt_to = 0; // 记录客户端是否发送过RCPT TO指令的标志
    int data = 0; // 记录客户端是否发送过DATA指令的标志
     
    while (!quit) {
        // 从客户端读取消息
        read_message(newsockfd, buffer);

        // print消息
        printf("Client: %s", buffer);

        // 检查消息合法性并相应地回复
        if (strncmp(buffer, "HELO", 4) == 0) { // HELO指令
            send_message(newsockfd, "250 Hello, nice to meet you\r\n");
        } else if (strncmp(buffer, "MAIL FROM", 9) == 0) { // MAIL FROM指令
            send_message(newsockfd, "250 OK\r\n");
            mail_from = 1; // 设置标志
        } else if (strncmp(buffer, "RCPT TO", 7) == 0) { // RCPT TO指令
            if (mail_from) { // 检查是否MAIL FROM指令被发送过
                send_message(newsockfd, "250 OK\r\n");
                rcpt_to = 1; // 设置标志
            } else {
                send_message(newsockfd, "503 Bad sequence of commands\r\n");
            }
        } else if (strncmp(buffer, "DATA", 4) == 0) { // DATA指令
            if (rcpt_to) { // 检查是否RCPT TO指令被发送过
                send_message(newsockfd, "354 Start mail input; end with <CRLF>.<CRLF>\r\n");
                data = 1; // 设置标志
            } else {
                send_message(newsockfd, "503 Bad sequence of commands\r\n");
            }
        } else if (strncmp(buffer, ".", 1) == 0) { // 邮件传输结束
            if (data) { // 检查是否DATA指令被发送过
                printf("Now buffer is:%s\n",buffer);
                parse_mail(buffer, mail); // 解析缓存区里的邮件内容
                save_mail(mail); // 保存邮件内容到文件里
                send_message(newsockfd, "250 OK, mail saved\r\n");
                mail_from = 0; // 重置标志
                rcpt_to = 0;
                data = 0;
            } else {
                send_message(newsockfd, "500 Syntax error, command unrecognized 1!!!!\r\n");
            }
        } else if (strncmp(buffer, "QUIT", 4) == 0) { // QUIT指令
            send_message(newsockfd, "221 Bye\r\n");
            quit = 1; // 设置标志
        } else { // 位置指令
            send_message(newsockfd, "500 Syntax error, command unrecognized 2!!!!\r\n");
        }
    }

    // 关闭socket
    close(newsockfd);
    close(sockfd);

    return 0;
}
