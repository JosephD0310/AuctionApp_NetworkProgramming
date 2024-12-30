#include "auth_service.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "message_type.h"
#include "user.h"

int handle_login(int sockfd, const char *username, const char *password, User *user_data)
{
    logSystem("CLIENT", "LOGIN", "Yêu cầu đăng nhập vào hệ thống - username: %s", username);
    char buffer[BUFFER_SIZE];
    User user;
    strncpy(user.username, username, sizeof(user.username));
    strncpy(user.password, password, sizeof(user.password));

    buffer[0] = LOGIN;
    memcpy(&buffer[1], &user, sizeof(user));

    // Gửi dữ liệu qua socket
    if (send(sockfd, buffer, sizeof(user) + 1, 0) < 0)
    {
        perror("send failed");
        return 0;
    }

    // Đọc phản hồi từ server
    memset(buffer, 0, BUFFER_SIZE);
    if (recv(sockfd, buffer, BUFFER_SIZE, 0) < 0)
    {
        perror("Failed to receive response");
        return -1;
    }

    memcpy(user_data, &buffer[1], sizeof(User));
    return buffer[0];
}

int handle_register(int sockfd, const char *username, const char *password, User *user_data)
{
    logSystem("CLIENT", "REGISTER", "Yêu cầu đăng ký vào hệ thống - username: %s", username);
    char buffer[BUFFER_SIZE];
    User user;
    strncpy(user.username, username, sizeof(user.username));
    strncpy(user.password, password, sizeof(user.password));

    printf("Username: %s, Password: %s\n", user.username, user.password);

    buffer[0] = REGISTER;
    memcpy(&buffer[1], &user, sizeof(user));

    // Gửi dữ liệu qua socket
    if (send(sockfd, buffer, sizeof(user) + 1, 0) < 0)
    {
        perror("send failed");
        return 0;
    }

    // Đọc phản hồi từ server
    memset(buffer, 0, BUFFER_SIZE);
    if (recv(sockfd, buffer, BUFFER_SIZE, 0) < 0)
    {
        perror("Failed to receive response");
        return -1;
    }

    memcpy(user_data, &buffer[1], sizeof(User));
    return buffer[0];
}