#include "bitmap.h"

#include <math.h>
#include <mm.h>
#include <string.h>


void bitmap_init(Bitmap * bitmap, MM_Info* mm_info) {
    bitmap->size = ceil((double) mm_info->items[1].mm_size / (0x1000 * 8.0));
    bitmap->mm_start = (u64) USR_ADDR_START;
    bitmap->page_total = mm_info->items[1].page_total;
    bitmap->page_used = 0;
    bitmap->map = (u8 *) BITMAP_ADDR;
    memset(bitmap->map,0,bitmap->size);
}

void bitmap_test(Bitmap * bitmap) {

}

void bitmap_set(Bitmap * bitmap, u64 index) {

}
void bitmap_scan(Bitmap * bitmap) {

}