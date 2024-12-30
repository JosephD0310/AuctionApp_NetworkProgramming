#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <stdbool.h>

#define MAX_CLIENTS 50


typedef struct {
    int sockfd;        
    int user_id;       
    int money;       
    char username[64];
} ClientSession;

extern ClientSession sessions[MAX_CLIENTS];
// Khởi tạo mảng quản lý phiên
void init_sessions();

bool add_session(int sockfd);

void remove_session(int sockfd);

ClientSession *find_session_by_socket(int sockfd);
ClientSession *find_session_by_user_id(int user_id);

bool update_session_user(int sockfd, int money);

void print_sessions();

#endif // SESSION_MANAGER_H
