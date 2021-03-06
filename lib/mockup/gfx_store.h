#ifndef MOCKUP_GFX_STORE_H
#define MOCKUP_GFX_STORE_H

#include "r65816/r65816.h"

typedef struct {
    int length;
    u8* data;
} gfx_page_t;

typedef struct {
    int num_pages;
    gfx_page_t* pages;
} gfx_store_t;

void gfx_store_init(gfx_store_t* gfx_store, r65816_rom_t* rom);
void gfx_store_deinit(gfx_store_t* gfx_store);


#endif //MOCKUP_GFX_STORE_H
