#include <stdio.h>
#include <string.h>
#include "user.h"
#include "session_manager.h"

// Hàm để lấy ID người dùng tiếp theo (tăng dần)
int getNextuser_id()
{
    int nextuser_id = 1;
    FILE *file = fopen("data/users.txt", "r");
    if (file == NULL)
    {
        return nextuser_id; // Nếu file chưa tồn tại, bắt đầu với ID 1
    }

    User user;
    while (fscanf(file, "%d %s %s %d\n", &user.user_id, user.username, user.password, &user.money) == 4)
    {
        if (user.user_id >= nextuser_id)
        {
            nextuser_id = user.user_id + 1;
        }
    }
    fclose(file);
    return nextuser_id;
}

// Hàm lưu thông tin người dùng vào file .txt
int saveUser(User user)
{
    if (checkUserExists(user.username))
    {
        return 0; // Username đã tồn tại
    }

    FILE *file = fopen("data/users.txt", "a");
    if (file == NULL)
    {
        return -1;
    }

    user.user_id = getNextuser_id();
    user.money = 10000;
    fprintf(file, "%d %s %s %d\n", user.user_id, user.username, user.password, user.money);
    fclose(file);
    return user.user_id;
}

// Hàm kiểm tra xem username đã tồn tại chưa
int checkUserExists(const char *username)
{
    FILE *file = fopen("data/users.txt", "r");
    if (file == NULL)
    {
        return 0;
    }

    User user;
    while (fscanf(file, "%d %s %s %d\n", &user.user_id, user.username, user.password, &user.money) == 4)
    {
        if (strcmp(user.username, username) == 0)
        {
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0;
}

// Hàm xác thực thông tin người dùng (username và password)
int authenticateUser(User user)
{
    FILE *file = fopen("data/users.txt", "r");
    if (file == NULL)
    {
        return -1;
    }
    User registeredUser;
    while (fscanf(file, "%d %s %s %d\n", &registeredUser.user_id, registeredUser.username, registeredUser.password, &registeredUser.money) == 4)
    {
        if (strcmp(registeredUser.username, user.username) == 0 && strcmp(registeredUser.password, user.password) == 0)
        {
            fclose(file);
            return registeredUser.user_id;
        }
    }
    fclose(file);
    return 0;
}

// Hàm lấy thông tin người dùng theo id
int getUserById(int user_id, User *user)
{
    FILE *file = fopen("data/users.txt", "r");
    if (file == NULL)
    {
        return 0;
    }

    User existedUser;
    while (fscanf(file, "%d %s %s %d\n", &existedUser.user_id, existedUser.username, existedUser.password, &existedUser.money) == 4)
    {
        if (existedUser.user_id == user_id)
        {
            *user = existedUser;
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return 0;
}

int loadUsers(User *users)
{
    FILE *file = fopen("data/users.txt", "r");
    if (file == NULL)
    {
        perror("Error opening user file");
        return -1;
    }

    int user_count = 0;

    User existedUser;
    while (fscanf(file, "%d %s %s %d\n", &existedUser.user_id, existedUser.username, existedUser.password, &existedUser.money) == 4)
    {
        users[user_count++] = existedUser;
    }

    fclose(file);
    return user_count;
}

// Hàm cập nhật tiền của người dùng
int updateMoney(int user_id, int amount, const char *option)
{
    User users[MAX_CLIENTS];
    int count = loadUsers(users);

    int updated = 0;
    for (int i = 0; i < count; i++)
    {
        if (users[i].user_id == user_id)
        {
            ClientSession *session = find_session_by_user_id(user_id);
            if (strcmp(option, "collect") == 0)
            {
                users[i].money = users[i].money + amount; // Thu tiền
                session->money = users[i].money;
            }
            else if (strcmp(option, "spend") == 0)
            {
                users[i].money = users[i].money - amount; // Chi tiền
                session->money = users[i].money;
            }
            updated = 1;
            break;
        }
    }

    if (!updated)
    {
        // Không tìm thấy user cần cập nhật
        return 0;
    }

    // Ghi lại dữ liệu đã cập nhật vào file
    FILE *file = fopen("data/users.txt", "w");
    if (file == NULL)
    {
        perror("Error reopening file to save updated user");
        return 0;
    }

    for (int i = 0; i < count; i++)
    {
        fprintf(file, "%d %s %s %d\n", users[i].user_id, users[i].username, users[i].password, users[i].money);
    }
    fclose(file); // Đóng file sau khi ghi

    return 1; // Thành công
}
