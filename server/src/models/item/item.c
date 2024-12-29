#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "item.h"

// Hàm để lấy ID Vật phẩm tiếp theo (tăng dần)
int getNextItemId()
{
    int next_item_id = 1;
    FILE *file = fopen("data/items.txt", "r");
    if (file == NULL)
    {
        return next_item_id; // Nếu file chưa tồn tại, bắt đầu với ID 1
    }

    Item item;
    while (fscanf(file, "%d %s %d %d %d %s\n", &item.item_id, item.item_name, &item.startingPrice, &item.buyNowPrice, &item.room_id, item.status) == 6)
    {
        if (item.item_id >= next_item_id)
        {
            next_item_id = item.item_id + 1;
        }
    }
    fclose(file);
    return next_item_id;
}

// Hàm tạo và lưu một vật phẩm mới
int createItem(const char *item_name, int startingPrice, int buyNowPrice, int room_id)
{
    int item_id = getNextItemId();

    FILE *file = fopen("data/items.txt", "a");
    if (file == NULL)
    {
        return 0; // Lỗi khi mở file
    }

    fprintf(file, "%d %s %d %d %d %s\n", item_id, item_name, startingPrice, buyNowPrice, room_id, "Available");
    
    fclose(file);
    return item_id; // Thành công
}

// Xóa vật phẩm theo ID
int deleteItem(int item_id)
{
    FILE *file = fopen("data/items.txt", "r");
    if (file == NULL)
    {
        perror("Error opening file to delete item");
        return -1; // Trả về lỗi nếu không mở được file
    }

    Item items[MAX_ITEM];
    int count = 0;

    // Đọc tất cả các item vào mảng
    while (fscanf(file, "%d %s %d %d %d %s\n", &items[count].item_id, items[count].item_name, &items[count].startingPrice,
                  &items[count].buyNowPrice, &items[count].room_id, items[count].status) == 6)
    {
        count++;
    }

    fclose(file); // Đóng file sau khi đọc

    // Mở lại file để ghi lại dữ liệu
    file = fopen("data/items.txt", "w");
    if (file == NULL)
    {
        perror("Error reopening file to rewrite items");
        return -1;
    }

    int deleted = 0;
    int room_id = 0;
    for (int i = 0; i < count; i++)
    {
        if (items[i].item_id != item_id)
        {
            fprintf(file, "%d %s %d %d %d %s\n", items[i].item_id, items[i].item_name, items[i].startingPrice,
                    items[i].buyNowPrice, items[i].room_id, items[i].status);
        }
        else
        {
            deleted = 1; // Đánh dấu đã xóa thành công
            room_id = items[i].room_id;
        }
    }

    fclose(file); // Đảm bảo đóng file sau khi ghi
    return deleted ? room_id : 0;
}

// Hàm khởi tạo file nếu chưa tồn tại
void initItemFile()
{
    FILE *file = fopen("data/items.txt", "r");
    if (file == NULL)
    {
        file = fopen("data/items.txt", "w");
        if (file)
        {
            fclose(file);
            printf("Initialized item file at %s\n", "data/items.txt");
        }
        else
        {
            perror("Error initializing item file");
        }
    }
    else
    {
        fclose(file);
    }
}

// Lấy các vật phẩm trong một phòng đấu giá
int loadItems(int room_id, Item *items)
{
    FILE *file = fopen("data/items.txt", "r");
    if (file == NULL)
    {
        perror("Error opening room file");
        return -1;
    }

    int item_count = 0;

    if (room_id != 0)
    {
        Item existedItem;
        while (fscanf(file, "%d %s %d %d %d %s\n", &existedItem.item_id, existedItem.item_name, &existedItem.startingPrice, &existedItem.buyNowPrice, &existedItem.room_id, existedItem.status) == 6)
        {
            if (existedItem.room_id == room_id)
            {
                items[item_count++] = existedItem;
            }
        }
    }
    else
    {
        Item existedItem;
        while (fscanf(file, "%d %s %d %d %d %s\n", &existedItem.item_id, existedItem.item_name, &existedItem.startingPrice, &existedItem.buyNowPrice, &existedItem.room_id, existedItem.status) == 6)
        {
            items[item_count++] = existedItem;
        }
    }

    fclose(file);    
    return item_count;
}

// Lấy thông tin một vật phẩm theo ID
int getItemById(int item_id, Item *item)
{
    FILE *file = fopen("data/items.txt", "r");
    if (file == NULL)
    {
        return 0;
    }

    Item existedItem;
    while (fscanf(file, "%d %s %d %d %d %s\n", &existedItem.item_id, existedItem.item_name, &existedItem.startingPrice, &existedItem.buyNowPrice, &existedItem.room_id, existedItem.status) == 6)
    {
        if (existedItem.item_id == item_id)
        {
            *item = existedItem;
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return 0; // Phòng không tồn tại
}

// Hàm lấy vật phẩm đầu tiên có trạng thái Available trong một room
int getFirstAvailableItem(int room_id, Item *item)
{
    FILE *file = fopen("data/items.txt", "r");
    if (file == NULL)
    {
        perror("Error opening file to find Available item");
        return 0; // Không mở được file
    }

    Item current_item;
    while (fscanf(file, "%d %s %d %d %d %s\n", 
                  &current_item.item_id, 
                  current_item.item_name, 
                  &current_item.startingPrice, 
                  &current_item.buyNowPrice, 
                  &current_item.room_id, 
                  current_item.status) == 6)
    {
        // Kiểm tra room_id khớp và trạng thái là Available
        if (current_item.room_id == room_id && strcmp(current_item.status, "Available") == 0)
        {
            *item = current_item; // Sao chép thông tin vật phẩm
            fclose(file);
            return 1; // Tìm thấy vật phẩm
        }
    }

    fclose(file);
    return 0; // Không tìm thấy vật phẩm nào chưa bán
}

// Hàm cập nhật thông tin vật phẩm
int updateItemById(int item_id, int room_id, int number, const char *option)
{
    Item items[MAX_ITEM_IN_ROOM];
    int count = loadItems(room_id, items);

    // Tìm vật phẩm cần cập nhật
    int updated = 0;
    for (int i = 0; i < count; i++)
    {
        if (items[i].item_id == item_id)
        {
            if (strcmp(option, "Sold") == 0)
            {
                strncpy(items[i].status, "Sold", sizeof(items[i].status)); // Cập nhật trạng thái vật phẩm
            }
            else if (strcmp(option, "price") == 0) // Cập nhật giá bán và chủ sở hữu mới
            {
                
            }
            updated = 1;
            break;
        }
    }

    if (!updated)
    {
        // Không tìm thấy vp cần cập nhật
        return 0;
    }

    // Ghi lại dữ liệu đã cập nhật vào file
    FILE *file = fopen("data/items.txt", "w");
    if (file == NULL)
    {
        perror("Error reopening file to save updated item");
        return 0;
    }

    for (int i = 0; i < count; i++)
    {
        fprintf(file, "%d %s %d %d %d %s\n", items[i].item_id, items[i].item_name, items[i].startingPrice, items[i].buyNowPrice, items[i].room_id, items[i].status);
    }
    fclose(file); // Đóng file sau khi ghi

    return item_id; // Thành công
}

