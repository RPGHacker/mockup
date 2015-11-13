#include <stdio.h>
#include <math.h>
#include <limits.h>

#include <SDL.h>
#include <SDL_opengl.h>

#include <imgui.h>
#include "imgui_impl_sdl.h"


#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

extern "C" {
#include "smw.h"
}

bool g_done = false;
SDL_Window* g_window;
bool show_palette_window = true,
    show_level_window = true,
    show_map16fg_window = true,
    show_map16bg_window = true,
    show_map8_window = true,
    show_app_metrics = true;

bool next_level;
bool prev_level;
ImVec4 g_clear_color = ImColor(114, 144, 154);

smw_t          smw;
r65816_rom_t   rom;
palette_pc_t   palette;
map16_pc_t     map16_fg, map16_bg;

uint32_t       map8_pixel_data[128*256];
uint32_t       map16_fg_pixel_data[256*512];
uint32_t       map16_bg_pixel_data[256*512];
uint16_t*      level_object_data = NULL;

int hot_index = INT_MAX;

uint8_t frame_num = 0;
int current_level = 0x100;
GLuint palette_texture = 0;
ImTextureID palette_tex_id;
GLuint map8_texture = 0;
ImTextureID map8_tex_id;

GLuint map16_fg_texture = 0;
ImTextureID map16_fg_tex_id;
GLuint map16_bg_texture = 0;
ImTextureID map16_bg_tex_id;


void gl_init_stuff() {
    glGenTextures(1, &palette_texture);
    glBindTexture(GL_TEXTURE_2D, palette_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    palette_tex_id = (void *)(intptr_t)palette_texture;

    glGenTextures(1, &map8_texture);
    glBindTexture(GL_TEXTURE_2D, map8_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 128, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    map8_tex_id = (void *)(intptr_t)map8_texture;

    
    glGenTextures(1, &map16_fg_texture);
    glBindTexture(GL_TEXTURE_2D, map16_fg_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    map16_fg_tex_id = (void *)(intptr_t)map16_fg_texture;

    glGenTextures(1, &map16_bg_texture);
    glBindTexture(GL_TEXTURE_2D, map16_bg_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    map16_bg_tex_id = (void *)(intptr_t)map16_bg_texture;
}


void gl_update_stuff() {
    glBindTexture(GL_TEXTURE_2D, palette_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 16, 16, GL_RGBA, GL_UNSIGNED_BYTE, &palette.data);
    glBindTexture(GL_TEXTURE_2D, map8_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 128, 256, GL_RGBA, GL_UNSIGNED_BYTE, &map8_pixel_data);
    glBindTexture(GL_TEXTURE_2D, map16_fg_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 512, GL_RGBA, GL_UNSIGNED_BYTE, &map16_fg_pixel_data);
    glBindTexture(GL_TEXTURE_2D, map16_bg_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 512, GL_RGBA, GL_UNSIGNED_BYTE, &map16_bg_pixel_data);
}

void load_level() {

    
}

void main_loop() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSdl_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
            g_done = true;
    }
    ImGui_ImplSdl_NewFrame(g_window);

    ImGui::ShowMetricsWindow(&show_app_metrics);
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Show Palette Window", NULL, &show_palette_window);
            ImGui::MenuItem("Show Map16 FG Window", NULL, &show_map16fg_window);
            ImGui::MenuItem("Show Map16 BG Window", NULL, &show_map16bg_window);
            ImGui::MenuItem("Show Level Window", NULL, &show_level_window);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Prev")) {
            if(current_level > 0) current_level--;
            smw_level_load(&smw, current_level);
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Next")) {
            if(current_level < 0x1FF) current_level++;
            smw_level_load(&smw, current_level);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    
    for(int i = 0; i < round(60.0/ImGui::GetIO().Framerate); i++) {
    level_animate(&smw.levels[current_level], frame_num, &smw.gfx_pages);
    frame_num++;
    }

    //Palette
    palette_to_pc(&smw.levels[current_level].palette, &palette);
    for(int i = 0; i < 512; i++) {
        for(int j = 0; j < 64; j++) {
            map8_pixel_data[(i % 16) * 8 + (j % 8) + ((i / 16) * 8 + (j / 8)) * 128]
                = palette.data[smw.levels[current_level].map8.tiles[i].pixels[j]];
        }
    }

    //Map16
    map16_pc_init(&map16_fg, &smw.levels[current_level].map16_fg);
    map16_pc_init(&map16_bg, &smw.levels[current_level].map16_bg);
    for(int i = 0; i < 2; i++) {
        for(int k = 0; k < 512; k++) {
            for(int j = 0; j < 256; j++) {
                map16_fg_pixel_data[(k % 16) * 16 + (j % 16) + ((k / 16) * 16 + (j / 16)) * 256]
                    = palette.data[map16_fg.tiles[k].data[j]];
                map16_bg_pixel_data[(k % 16) * 16 + (j % 16) + ((k / 16) * 16 + (j / 16)) * 256]
                    = palette.data[map16_bg.tiles[k].data[j]];
            }
        }
    }

    // Level objects
    level_object_data = (uint16_t*)realloc(level_object_data, smw.levels[current_level].width * smw.levels[current_level].height * sizeof(uint16_t));
    for(int i = 0; i < smw.levels[current_level].width * smw.levels[current_level].height; i++) {
        level_object_data[i] = 0x25;
    }


    
    for(int i = 0; i < smw.levels[current_level].layer1_objects.length; i++) {
        object_pc_t* obj = &smw.levels[current_level].layer1_objects.objects[i];
        if(!obj->tiles) continue;
        int obj_width = obj->bb_xmax - obj->bb_xmin + 1;
        int obj_height = obj->bb_ymax - obj->bb_ymin + 1;
        for(int j = 0; j < obj_height; j++) {
            for(int k = 0; k < obj_width; k++) {
                if(j + obj->bb_ymin >= 0 && k + obj->bb_xmin >= 0 &&
                   j + obj->bb_ymin < smw.levels[current_level].height &&
                   k + obj->bb_xmin < smw.levels[current_level].width &&
                   obj->tiles[j * obj_width + k] != 0xFFFF) {
                    level_object_data[(j + obj->bb_ymin) * smw.levels[current_level].width + k + obj->bb_xmin]
                        = obj->tiles[j * obj_width + k];
                    if(hot_index == i) {
                        level_object_data[(j + obj->bb_ymin) * smw.levels[current_level].width + k + obj->bb_xmin] |= 0x8000;
                    }
                }
            }
        }
    }
    
    gl_update_stuff();
    map16_pc_deinit(&map16_fg);
    map16_pc_deinit(&map16_bg);

    
    {
        ImGui::Begin("Palette Window", &show_palette_window,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Image(palette_tex_id, ImVec2(256, 256));
        ImGui::End();
    }

    {
        ImGui::Begin("Map8 Window", &show_map8_window,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Image(map8_tex_id, ImVec2(128, 256));
        ImGui::End();
    }
    
    {
        ImGui::Begin("Map16 FG Window", &show_map16fg_window,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Image(map16_fg_tex_id, ImVec2(256, 512));
        ImGui::End();
    }

    {
        ImGui::Begin("Map16 BG Window", &show_map16bg_window,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Image(map16_bg_tex_id, ImVec2(256, 512));
        ImGui::End();
    }

    {
        char window_title[64];
        ImGui::SetNextWindowSize(ImVec2(600, 556), ImGuiSetCond_FirstUseEver);

        if(smw.levels[current_level].is_boss_level) {
            sprintf(window_title, "Level %x - Boss Level###Level", current_level);
            ImGui::Begin(window_title, &show_level_window);
            ImGui::End();
        } else {
            sprintf(window_title, "Level %x###Level", current_level);
        
            ImGui::Begin(window_title, &show_level_window);
            ImGui::BeginChild("scrolling", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            int width = smw.levels[current_level].width;
            int height = smw.levels[current_level].height;
            ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
            ImVec2 canvas_size = ImVec2(width * 16, height * 16);
            ImVec2 actual_size = ImGui::GetContentRegionAvail();
            ImVec2 mouse_pos_in_canvas = ImVec2(ImGui::GetIO().MousePos.x - canvas_pos.x, ImGui::GetIO().MousePos.y - canvas_pos.y);
            ImGui::InvisibleButton("canvas", canvas_size);

            hot_index = INT_MAX;
            
            for(int i = 0; i < smw.levels[current_level].layer1_objects.length; i++) {
                object_pc_t* obj = &smw.levels[current_level].layer1_objects.objects[i];
                if(!obj->tiles) continue;
                if(obj->bb_ymin * 16.0 <= mouse_pos_in_canvas.y &&
                   obj->bb_xmin * 16.0 <= mouse_pos_in_canvas.x &&
                   (obj->bb_ymax + 1) * 16.0 >= mouse_pos_in_canvas.y &&
                   (obj->bb_xmax + 1) * 16.0 >= mouse_pos_in_canvas.x) {
                    int local_x = (mouse_pos_in_canvas.x - obj->bb_xmin * 16.0) / 16;
                    int local_y = (mouse_pos_in_canvas.y - obj->bb_ymin * 16.0) / 16;
                    if(obj->tiles[local_y * (obj->bb_xmax - obj->bb_xmin + 1) + local_x] != 0xFFFF) {
                        hot_index = i;
                    }
                }
            }
            
            { // Layer 3
                draw_list->AddRectFilled(ImVec2(canvas_pos.x, canvas_pos.y),
                                         ImVec2(canvas_pos.x + (float)width*16.0, canvas_pos.y + (float)height*16.0),
                                         smw.levels[current_level].background_color);
            }
            
            { // Layer 2
                if(!smw.levels[current_level].is_vertical_level) {
                    //Horizontal level
                    if(smw.levels[current_level].has_layer2_bg) {
                        // Layer 2 BG
                        draw_list->PushTextureID(map16_bg_tex_id);
                        draw_list->PrimReserve(height * width * 6, height * width * 4);
                        for(int j = 0; j < height; j++) {
                            for(int i = 0; i < width; i++) {
                                ImVec2 a = ImVec2(16.0f*i + canvas_pos.x, 16.0f*j + canvas_pos.y);
                                ImVec2 b = ImVec2(a.x + 16.0f, a.y + 16.0f);
                                uint16_t tile_num = smw.levels[current_level].layer2_background
                                    .data[(i & 0x10) * 27 + j * 16 + (i & 0x0F)];
                                ImVec2 uv0 = ImVec2((float)((tile_num & 0x00F)) * 0.0625f, (float)((tile_num & 0xFF0) >> 4) * 0.03125);
                                ImVec2 uv1 = ImVec2(uv0.x + 0.0625f, uv0.y + 0.03125f);
                                draw_list->PrimRectUV(a, b, uv0, uv1, 0xFFFFFFFF);
                            }
                        }
                        draw_list->PopTextureID();
                    } else {
                        // Layer 2 Objects
                        // TODO: Implement
                    }
                } else {
                    // Vertical Level
                    // TODO: Implement
                }
            }
        
            { // Layer 1
                draw_list->PushTextureID(map16_fg_tex_id);
                draw_list->PrimReserve(height * width * 6, height * width * 4);
                for(int j = 0; j < height; j++) {
                    for(int i = 0; i < width; i++) {
                        ImVec2 a = ImVec2(16.0f*i + canvas_pos.x, 16.0f*j + canvas_pos.y);
                        ImVec2 b = ImVec2(a.x + 16.0f, a.y + 16.0f);
                        uint16_t tile_data = level_object_data[j * width + i];
                        uint16_t tile_num = tile_data & 0x8FFF;
                        uint16_t do_tint = tile_data >> 15;
                        ImVec2 uv0 = ImVec2((float)((tile_num & 0x00F)) * 0.0625f, (float)((tile_num & 0xFF0) >> 4) * 0.03125);
                        ImVec2 uv1 = ImVec2(uv0.x + 0.0625f, uv0.y + 0.03125f);
                        if(do_tint) {
                            draw_list->PrimRectUV(a, b, uv0, uv1, 0xFFFF8080);
                        } else {
                            draw_list->PrimRectUV(a, b, uv0, uv1, 0xFFFFFFFF);
                        }
                    }
                }
                draw_list->PopTextureID();
            }

            if(ImGui::IsMouseHoveringWindow()) {
                if(ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_PageUp))) {
                    if(current_level < 0x1FF) current_level++;
                    smw_level_load(&smw, current_level);
                }
                if(ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_PageDown))) {
                    if(current_level > 0) current_level--;
                    smw_level_load(&smw, current_level);
                }
            }
        
            ImGui::EndChild();
            ImGui::End();
        }
    }
    
    // Rendering
    glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
    glClearColor(g_clear_color.x, g_clear_color.y, g_clear_color.z, g_clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui::Render();
    SDL_GL_SwapWindow(g_window);
}

int main(int, char**) {
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    
    g_window = SDL_CreateWindow("Mockup", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    SDL_GLContext glcontext = SDL_GL_CreateContext(g_window);
    
    // Setup ImGui binding
    ImGui_ImplSdl_Init(g_window);

#if 0
    r65816_rom_load(&rom, "smw.sfc");
#else
#include "smw_file.h"
    rom.banksize = 0x8000; //Currently only LOROM is supported
    unsigned int filesize = sizeof(smw_sfc);
    unsigned int headersize = filesize % rom.banksize;
    rom.num_banks = (filesize - headersize) / rom.banksize;
    rom.banks = (r65816_bank*)malloc(rom.num_banks * sizeof(r65816_bank));
    rom.data = smw_sfc + headersize;
    r65816_guess_header(&rom);
    for(int i = 0; i < rom.num_banks; i++) {
        rom.banks[i] = rom.data + i * rom.banksize - 0x8000;
    }
#endif
    smw_init(&smw, &rom);
    smw_level_load(&smw, current_level);

    gl_init_stuff();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.f;
    style.ChildWindowRounding = 0.f;




        
    // Main loop
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, 1);
#else
    while (!g_done)
    {
        main_loop();
    }
#endif
    
    // Cleanup
    smw_deinit(&smw);
    free(rom.banks);
    
    ImGui_ImplSdl_Shutdown();
    SDL_GL_DeleteContext(glcontext);
    SDL_DestroyWindow(g_window);
    SDL_Quit();

    return 0;
}