// 学习阶段的试验

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <json-c/json.h>
#include <regex.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>


#define PORT 25 // SMTP的端口
#define BUFFER_SIZE 10240 // socket通信的缓存区
#define FILE_NAME "mail.json" // 保存邮件的文件名

struct mail_content {
    char mail_from[256];
    char rept_to[256];
    struct mail_header *headers;
    char mail_body[BUFFER_SIZE];
};

struct mail_header {
    char key[256];
    char value[256];
    struct mail_header *next;
};

/*
int main() {
    const char* original_data = "Hello, World!";
    int input_length = strlen(original_data);
    
    // 编码
    char* encoded_data = base64_encode((const unsigned char *)original_data, input_length);
    printf("Base64 encoded data: %s\n", encoded_data);
    
    // 解码
    int decoded_length;
    unsigned char* decoded_data = base64_decode(encoded_data, strlen(encoded_data), &decoded_length);
    printf("Base64 decoded data: %s\n", decoded_data);
    
    free(encoded_data);
    free(decoded_data);
    
    return 0;
}
*/  


// Base64 编码
char* base64_encode(const unsigned char *input, int length) {
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;
    
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // 不添加换行符
    
    BIO_write(bio, input, length);
    BIO_flush(bio);
    
    BIO_get_mem_ptr(bio, &bufferPtr);
    BIO_set_close(bio, BIO_NOCLOSE);
    BIO_free_all(bio);
    
    return bufferPtr->data;
}

// Base64 解码
unsigned char* base64_decode(const char *input, int length, int *output_length) {
    BIO *bio, *b64;
    unsigned char *output = (unsigned char*)malloc(length); // 分配足够的空间来存储解码后的数据
    
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_mem_buf(input, length);
    bio = BIO_push(b64, bio);
    
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // 不添加换行符
    
    *output_length = BIO_read(bio, output, length);
    
    BIO_free_all(bio);
    
    return output;
}




// 发送消息到客户端并检查是否出错的函数
void send_message(int sockfd, char *message)
{
    int n = write(sockfd, message, strlen(message));
    if (n < 0) {
        perror("Error writing to socket");
        exit(1);
    }
}

// 从客户端读取信息并检查是否出错的函数
char *read_message(int sockfd, char *buffer)
{
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
void parse_mail(char *buffer, struct mail_content *m)
{
    regex_t header_regex, body_regex;
    regmatch_t header_pmatch[3], body_pmatch[2];
    int ret;

    // 初始化 headers
    m->headers = NULL;

    // 创建 buffer 的副本
    char *buffer_copy = strdup(buffer); // 使用 strdup 函数复制 buffer
    if (buffer_copy == NULL) {
        perror("Error duplicating buffer");
        exit(1);
    }

    // 匹配邮件头
    char *line = strtok(buffer_copy, "\r\n"); // 分割副本的每一行
    while (line != NULL && strlen(line) > 0) { // 处理非空行
        ret = regcomp(&header_regex, "^(.*?): ([^\r\n]*)", REG_EXTENDED | REG_NEWLINE);
        if (ret != 0) {
            perror("Error compiling regex");
            exit(1);
        }
        ret = regexec(&header_regex, line, 3, header_pmatch, 0);
        if (ret == 0) {
            char key[256];
            char value[256];
            strncpy(key, &line[header_pmatch[1].rm_so],
                    header_pmatch[1].rm_eo - header_pmatch[1].rm_so);
            key[header_pmatch[1].rm_eo - header_pmatch[1].rm_so] = '\0';
            strncpy(value, &line[header_pmatch[2].rm_so],
                    header_pmatch[2].rm_eo - header_pmatch[2].rm_so);
            value[header_pmatch[2].rm_eo - header_pmatch[2].rm_so] = '\0';

            struct mail_header *header = malloc(sizeof(struct mail_header));
            strncpy(header->key, key, 256);
            strncpy(header->value, value, 256);
            header->next = NULL;

            if (m->headers == NULL) {
                m->headers = header;
            } else {
                struct mail_header *last_header = m->headers;
                while (last_header->next != NULL) {
                    last_header = last_header->next;
                }
                last_header->next = header;
            }
        }
        regfree(&header_regex);
        line = strtok(NULL, "\r\n");
    }

    // 释放 buffer 的副本
    free(buffer_copy); // 使用 free 函数释放 buffer_copy

    // 匹配邮件体
    ret = regcomp(&body_regex, "\r\n\r\n(.*)", REG_EXTENDED | REG_NEWLINE);
    if (ret != 0) {
        perror("Error compiling regex");
        exit(1);
    }
    ret = regexec(&body_regex, buffer, 2, body_pmatch, 0);

    char mail_body[BUFFER_SIZE];
    int len = body_pmatch[1].rm_eo - body_pmatch[1].rm_so;
    strncpy(mail_body, buffer + body_pmatch[1].rm_so, len);
    mail_body[len] = '\0';
    strcpy(m->mail_body, mail_body);


    regfree(&body_regex);

}



// 保存邮件内容到一个文件的函数
void save_mail(struct mail_content *m)
{
    json_object *jobj = json_object_new_object();
    json_object *jbody = json_object_new_string(m->mail_body);

    // 添加自定义邮件头
    struct mail_header *header = m->headers;
    while (header != NULL) {
        json_object_object_add(jobj, header->key,
        json_object_new_string(header->value));
        header = header->next;
    }
    json_object_object_add(jobj, "Body", jbody);

    FILE *fp = fopen(FILE_NAME, "w");
    if (fp == NULL) {
        perror("Error opening file");
        exit(1);
    }
    fprintf(fp, "%s\n", json_object_to_json_string(jobj)); // 把JSON写到文件里
    fclose(fp); // 关闭文件
}



// 主函数
int main()
{
    SSL_CTX *ctx;
    SSL *ssl;
    int sockfd, newsockfd; // socket文件描述符
    struct sockaddr_in serv_addr, cli_addr; // 服务器地址和客户端地址
    socklen_t clilen; // 客户端地址长度
    char buffer[BUFFER_SIZE]; // socket通信地址长度
    struct mail_content mail; // 邮件结构体


    // 初始化OpenSSL库
    SSL_library_init();

    // 创建SSL上下文
    ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        exit(1);
    }

    // 加载服务器证书和私钥
    if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(1);
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(1);
    }

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
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
                       &clilen); // 接收客户端的连接并得到通信的socket文件描述符
    if (newsockfd < 0) {
        perror("Error accepting connection");
        exit(1);
    }

    // print客户端的地址和端口
    printf("Connected to client %s:%d\n", inet_ntoa(cli_addr.sin_addr),
           ntohs(cli_addr.sin_port));

    // 发送welcome信息给客户端
    send_message(newsockfd, "220 Welcome to my SMTP server\r\n");

    // 进入跟客户端通信的循环
    int quit = 0; // 记录客户端是否发送了退出指令的标志
    int have_mail_from = 0; // 记录客户端是否发送过MAIL FROM指令的标志
    int have_rcpt_to = 0; // 记录客户端是否发送过RCPT TO指令的标志
    int data = 0; // 记录客户端是否发送过DATA指令的标志
    int in_data = 0; // 记录是否处于接收邮件数据的状态
    int in_tls = 0;
    int flag = 0;
    int auth_passed = 0;

    while (!quit) {
        if(in_tls){
            while (in_tls)
            {
                SSL_read(ssl, buffer, BUFFER_SIZE);
                if (strncmp(buffer, "HELO", 4) == 0) {
                    SSL_write(ssl, "250 Hello!\r\n", strlen("250 Hello!\r\n"));
                } else if (strncmp(buffer, "EHLO", 4) == 0) {
                SSL_write(ssl, "250-moekikan.com\r\n250-SIZE 10240\r\n250-STARTTLS\r\n250-AUTH LOGIN PLAIN XOAUTH XOAUTH2\r\n250-AUTH=LOGIN\r\n250-8BITMIME\r\n250 OK\r\n", strlen("250-moekikan.com\r\n250-SIZE 10240\r\n250-STARTTLS\r\n250-AUTH LOGIN PLAIN XOAUTH XOAUTH2\r\n250-AUTH=LOGIN\r\n250-8BITMIME\r\n250 OK\r\n"));
                }else if (strncmp(buffer, "AUTH PLAIN", 10) == 0) {
                    // 逻辑以后再写
                    auth_passed = 1;
                    if(auth_passed){
                        SSL_write(ssl, "235 Authentication successful\r\n", strlen("235 Authentication successful\r\n"));
                    }
                }else if (strncmp(buffer, "MAIL FROM", 9) == 0) {
                    SSL_write(ssl, "250 OK\r\n", strlen("250 OK\r\n"));
                    have_mail_from = 1;
                    

                } else if (strncmp(buffer, "RCPT TO", 7) == 0) {
                    if (have_mail_from) {
                        SSL_write(ssl, "250 OK\r\n", strlen("250 OK\r\n"));
                        have_rcpt_to = 1;
                    } else {
                        SSL_write(ssl, "503 Bad sequence of commands\r\n", strlen("503 Bad sequence of commands\r\n"));
                    }












                } else if (strncmp(buffer, "DATA", 4) == 0) { // DATA指令
                    if (have_rcpt_to) { //
                        SSL_write(ssl, "354 Start mail input; end with <CRLF>.<CRLF>\r\n", strlen("354 Start mail input; end with <CRLF>.<CRLF>\r\n"));
                        printf("354 Start mail input; end with <CRLF>.<CRLF>Now buffer is: %s", buffer);
                        in_data = 1; // 进入接收邮件数据状态
                        SSL_read(ssl, buffer, BUFFER_SIZE);
                        printf("Now buffer is: %s", buffer);
                        parse_mail(buffer, &mail);//用parse_mail把邮件头解析成mail_content结构体
                        bzero(buffer, BUFFER_SIZE);
                        // 正文附加到结构体里
















                        SSL_read(ssl, buffer, BUFFER_SIZE);
                        save_mail(&mail);//将结构体传入save_mail保存
                        printf("111Now buffer is: %s", buffer);



                        if (strncmp(buffer, "\r\n.\r\n", 1) == 0 &&
                                in_data) { // 接收到单独一个 . 的消息表示结束邮件数据的输入
                            SSL_write(ssl, "250 OK\r\n", strlen("250 OK\r\n"));
                            printf("Now buffer is: %s", buffer);
                            in_data = 0; // 退出接收邮件数据状态
                        }
                    } else {
                        SSL_write(ssl, "503 Bad sequence of commands\r\n", strlen("503 Bad sequence of commands\r\n"));
                    }










                } else  if (strncmp(buffer, "QUIT", 4) == 0) { // QUIT指令
                    quit = 1; // 设置标志
                    SSL_write(ssl, "221 Bye\r\n", strlen("221 Bye\r\n"));
                    printf("isaybyeNow buffer is: %s", buffer);









                } else if (strncmp(buffer, "NOOP", 4) == 0) { // QUIT指令
                    quit = 1; // 设置标志
                    SSL_write(ssl, "OK IDLE\r\n", strlen("OK IDLE\r\n"));
                    printf("isayIDLENow buffer is: %s", buffer);








                }
                // }else { // 其他指令都无效
                //     SSL_write(ssl, "500 Authentication successful\r\n", strlen("235 Authentication successful\r\n"));
                //     if(flag < 10 )printf("I say 500 Authentication successful\r\n Now buffer is: %s", buffer);
                //     flag += 1;
                // }

                // 清空缓存区
                bzero(buffer, BUFFER_SIZE);

                // 如果客户端发送了QUIT指令，跳出循环
                if (quit) {
                    break;
                }
            }
            
        }
        // 从客户端读取消息
        read_message(newsockfd, buffer);


        // print消息
        if(buffer != "\n"){
            printf("Client: %s", buffer);
        }

        // 检查消息合法性并相应地回复
        if (strncmp(buffer, "HELO", 4) == 0) { // HELO指令
            send_message(newsockfd, "250 Hello, nice to meet you\r\n");
        } else if (strncmp(buffer, "EHLO", 4) == 0) {
            send_message(newsockfd, "250-moekikan.com\r\n250-SIZE 10240\r\n250-STARTTLS\r\n250-AUTH LOGIN PLAIN XOAUTH XOAUTH2\r\n250-AUTH=LOGIN\r\n250-8BITMIME\r\n250 OK\r\n");
        }else if (strncmp(buffer, "STARTTLS", 8) == 0) {// 开启SSL连接
            // 发送"220 Ready to start TLS\r\n"响应
            send_message(newsockfd, "220 Ready to start TLS\r\n");

            // 设置服务器证书验证模式
            SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

            // 创建SSL对象并与客户端socket关联
            ssl = SSL_new(ctx);
            SSL_set_fd(ssl, newsockfd);

            // 执行SSL握手
            if (SSL_accept(ssl) <= 0) {
                ERR_print_errors_fp(stderr);
                exit(1);
            }

            // 检查证书验证结果
            if (SSL_get_verify_result(ssl) != X509_V_OK) {
                fprintf(stderr, "Certificate verification failed\n");
                exit(1);
            }

            // 进入加密数据传输阶段
            in_tls = 1;
        }
        else { // 其他指令都无效
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
    SSL_shutdown(ssl);
    SSL_free(ssl);

    close(newsockfd);
    close(sockfd);
    
    SSL_CTX_free(ctx);
    return 0;
}