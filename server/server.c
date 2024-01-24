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
#include <json-c/json.h>
#include <regex.h>

#define PORT 25 // SMTP的端口
#define BUFFER_SIZE 1024 // socket通信的缓存区
#define FILE_NAME "mail.json" // 保存邮件的文件名

struct mail_content {
    char from[256];
    char to[256];
    char subject[256];
    char content[BUFFER_SIZE];
};

// 发送消息到客户端并检查是否出错的函数
void send_message(int sockfd, char* message) {
    int n = write(sockfd, message, strlen(message));
    if (n < 0) {
        perror("Error writing to socket");
        exit(1);
    }
}

// 从客户端读取信息并检查是否出错的函数
char* read_message(int sockfd, char* buffer) {
    bzero(buffer, BUFFER_SIZE); // 清空缓存区
    int n = read(sockfd, buffer, BUFFER_SIZE - 1);
    if (n < 0) {
        perror("Error reading from socket");
        exit(1);
    }
    buffer[n] = '\0'; // 添加字符串结束符
    return buffer;
}


// 解析邮件内容的函数
void parse_mail(char* buffer, struct mail_content* m) {
    regex_t regex;
    regmatch_t pmatch[2];
    int ret;

    // 匹配 From 字段
    ret = regcomp(&regex, "From: ([^\r\n]*)", REG_EXTENDED);
    if (ret != 0) {
        perror("Error compiling regex");
        exit(1);
    }
    ret = regexec(&regex, buffer, 2, pmatch, 0);
    if (ret == 0) {
        strncpy(m->from, &buffer[pmatch[1].rm_so], pmatch[1].rm_eo - pmatch[1].rm_so);
        m->from[pmatch[1].rm_eo - pmatch[1].rm_so] = '\0';
    } else {
        m->from[0] = '\0';
    }
    regfree(&regex);

    // 匹配 To 字段
    ret = regcomp(&regex, "To: ([^\r\n]*)", REG_EXTENDED);
    if (ret != 0) {
        perror("Error compiling regex");
        exit(1);
    }
    ret = regexec(&regex, buffer, 2, pmatch, 0);
    if (ret == 0) {
        strncpy(m->to, &buffer[pmatch[1].rm_so], pmatch[1].rm_eo - pmatch[1].rm_so);
        m->to[pmatch[1].rm_eo - pmatch[1].rm_so] = '\0';
    } else {
        m->to[0] = '\0';
    }
    regfree(&regex);

    // 匹配 Subject 字段
    ret = regcomp(&regex, "Subject: ([^\r\n]*)", REG_EXTENDED);
    if (ret != 0) {
        perror("Error compiling regex");
        exit(1);
    }
    ret = regexec(&regex, buffer, 2, pmatch, 0);
    if (ret == 0) {
        strncpy(m->subject, &buffer[pmatch[1].rm_so], pmatch[1].rm_eo - pmatch[1].rm_so);
        m->subject[pmatch[1].rm_eo - pmatch[1].rm_so] = '\0';
    } else {
        m->subject[0] = '\0';
    }
    regfree(&regex);

    // 设置邮件正文
    char* start = strstr(buffer, "\r\n\r\n");
    if (start != NULL) {
        start += 4;
        strncpy(m->content, start, BUFFER_SIZE - 1);
        m->content[BUFFER_SIZE - 1] = '\0';
    } else {
        m->content[0] = '\0';
    }
}

// 保存邮件内容到一个文件的函数
void save_mail(struct mail_content* m) {
    json_object *jobj = json_object_new_object();
    json_object *jfrom = json_object_new_string(m->from);
    json_object *jto = json_object_new_string(m->to);
    json_object *jsubject = json_object_new_string(m->subject);
    json_object *jcontent = json_object_new_string(m->content);

    json_object_object_add(jobj, "from", jfrom);
    json_object_object_add(jobj, "to", jto);
    json_object_object_add(jobj, "subject", jsubject);
    json_object_object_add(jobj, "content", jcontent);

    FILE* fp = fopen(FILE_NAME, "w");
    if (fp == NULL) {
        perror("Error opening file");
        exit(1);
    }
    fprintf(fp, "%s\n", json_object_to_json_string(jobj)); // 把JSON写到文件里
    fclose(fp); // 关闭文件
}

// 主函数
int main() {
    int sockfd, newsockfd; // socket文件描述符
    struct sockaddr_in serv_addr, cli_addr; // 服务器地址和客户端地址
    socklen_t clilen; // 客户端地址长度
    char buffer[BUFFER_SIZE]; // socket通信地址长度
    struct mail_content mail; // 邮件结构体

    // 创建监听用的socket，检查是否成功
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        exit(1);
    }

    // 初始化服务器地址的结构体
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

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
    int in_data = 0; // 记录是否处于接收邮件数据的状态

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
            if (rcpt_to) { //
                send_message(newsockfd, "354 Start mail input; end with <CRLF>.<CRLF>\r\n");
                in_data = 1; // 进入接收邮件数据状态
                read_message(newsockfd, buffer);
                //用parse_mail把mail_temp解析成mail_content结构体
                parse_mail(buffer, &mail);
                //如果结构体的前三部分有空值则回复一个554状态码拒收
                if(mail.from == 0 || mail.to == 0 || mail.subject == 0){
                    send_message(newsockfd, "554 Transaction failed. Missing required fields: From/To/Subject.\r\n");
                }
                //将结构体传入save_mail保存
                save_mail(&mail);
                if (strncmp(buffer, ".", 1) == 0 && in_data) { // 接收到单独一个 . 的消息表示结束邮件数据的输入
                    send_message(newsockfd, "250 OK\r\n");
                    in_data = 0; // 退出接收邮件数据状态
                }
            } else {
                send_message(newsockfd, "503 Bad sequence of commands\r\n");
            }
        } else  if (strncmp(buffer, "QUIT", 4) == 0) { // QUIT指令
            quit = 1; // 设置标志
            send_message(newsockfd, "221 Bye\r\n");
        } else { // 其他指令都无效
            send_message(newsockfd, "500 Command unrecognized\r\n");
        }

        // 清空缓存区
        bzero(buffer, BUFFER_SIZE);

        // 如果客户端发送了QUIT指令，跳出循环
        if (quit) {
            break;
        }
    }

    // 关闭socket文件描述符并退出
    close(newsockfd);
    close(sockfd);
    return 0;
}
