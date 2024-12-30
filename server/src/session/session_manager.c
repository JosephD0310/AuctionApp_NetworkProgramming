#include "session_manager.h"
#include <stdio.h>
#include <string.h>

ClientSession sessions[MAX_CLIENTS];

void init_sessions() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        sessions[i].sockfd = -1; // Phiên chưa được sử dụng
        sessions[i].user_id = -1; // Chưa đăng nhập
    }
}

bool add_session(int sockfd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (sessions[i].sockfd == -1) { // Tìm một slot trống
            sessions[i].sockfd = sockfd;
            sessions[i].user_id = -1;
            memset(sessions[i].username, 0, sizeof(sessions[i].username));
            printf("Session added: sockfd=%d\n", sockfd);
            return true;
        }
    }
    printf("Failed to add session: Max clients reached.\n");
    return false;
}

void remove_session(int sockfd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (sessions[i].sockfd == sockfd) {
            printf("Session removed: sockfd=%d, user_id=%d\n", sockfd, sessions[i].user_id);
            sessions[i].sockfd = -1;
            sessions[i].user_id = -1;
            memset(sessions[i].username, 0, sizeof(sessions[i].username));
            return;
        }
    }
    printf("Session not found: sockfd=%d\n", sockfd);
}

ClientSession *find_session_by_socket(int sockfd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (sessions[i].sockfd == sockfd) {
            return &sessions[i];
        }
    }
    return NULL;
}

ClientSession *find_session_by_user_id(int user_id) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (sessions[i].user_id == user_id) {
            return &sessions[i];
        }
    }
    return NULL;
}

bool update_session_user(int sockfd, int money) {
    ClientSession *session = find_session_by_socket(sockfd);
    if (session) {
        session->money = money;
        // strncpy(session->username, username, sizeof(session->username) - 1);
        // printf("Session updated: sockfd=%d, user_id=%d, username=%s\n", sockfd, user_id, username);
        return true;
    }
    return false;
}

void print_sessions() {
    printf("Current Sessions:\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (sessions[i].sockfd != -1) {
            printf("Socket: %d, UserID: %d, Username: %s\n", 
                   sessions[i].sockfd, sessions[i].user_id, sessions[i].username);
        }
    }
}
