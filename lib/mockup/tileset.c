#include "r65816/cpu.h"
#include "tileset.h"

uint8_t tileset_init_gfx_store(tileset_t* tileset, gfx_store_t* gfx_store, uint8_t fg1, uint8_t fg2, uint8_t bg1, uint8_t fg3) {
    printf("debug\n");
    if(fg1 >= gfx_store->num_pages) return -1;
    if(fg2 >= gfx_store->num_pages) return -2;
    if(bg1 >= gfx_store->num_pages) return -3;
    printf("debug\n");
    if(fg3 >= gfx_store->num_pages) return -4;

    printf("debug\n");
    tileset->fg1 = &gfx_store->pages[fg1];
    printf("debug\n");
    tileset->fg2 = &gfx_store->pages[fg2];
    printf("debug\n");
    tileset->bg1 = &gfx_store->pages[bg1];
    printf("debug\n");
    tileset->fg3 = &gfx_store->pages[fg3];
    printf("debug\n");
    return 0;
}

uint8_t tileset_init_level(tileset_t* tileset, r65816_rom_t* rom, int num_level, gfx_store_t* gfx_store) {
    r65816_cpu_t cpu;
    r65816_cpu_init(&cpu, rom);

    cpu.ram[0x65] = cpu.rom->banks[5][0xE000 + 3 * num_level]; 
    cpu.ram[0x66] = cpu.rom->banks[5][0xE001 + 3 * num_level];
    cpu.ram[0x67] = cpu.rom->banks[5][0xE002 + 3 * num_level];

    r65816_cpu_add_exec_bp(&cpu, 0x0583B8);
    r65816_cpu_run_from(&cpu, 0x0583AC);

    int num_tileset = cpu.ram[0x1931];
    uint32_t addr = 0x292B + num_tileset * 4;
    uint8_t fg1 = rom->data[addr];
    uint8_t fg2 = rom->data[addr + 1];
    uint8_t bg1 = rom->data[addr + 2];
    uint8_t fg3 = rom->data[addr + 3];
    tileset_init_gfx_store(tileset, gfx_store, fg1, fg2, bg1, fg3);
}    
