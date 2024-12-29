#ifndef ITEM_H
#define ITEM_H

#define MAX_LENGTH 20
#define MAX_ITEM 100
#define MAX_ITEM_IN_ROOM 10

typedef struct {
    int item_id;             // ID của vật phẩm
    char item_name[MAX_LENGTH];  // Tên vật phẩm
    int startingPrice;      // Giá khởi điểm
    int buyNowPrice;        // Giá bán ngay
    int room_id;             // ID phòng chứa vật phẩm
    char status[MAX_LENGTH];       // Trạng thái
} Item;

// Hàm khai báo
int getNextItemId(); 
int createItem(const char *item_name, int startingPrice, int buyNowPrice, int room_id);
int deleteItem(int item_id);
void initItemFile();
int loadItems(int room_id, Item *items);
int getItemById(int item_id, Item *item);
int getFirstAvailableItem(int room_id, Item *item);
int updateItemById(int item_id, int room_id, int number, const char *option);

#endif
