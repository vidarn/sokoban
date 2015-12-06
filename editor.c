#include "main.h"
#define STB_TILEMAP_EDITOR_IMPLEMENTATION
void STBTE_DRAW_RECT(s32 x0, s32 y0, s32 x1, s32 y1, u32 color);
void STBTE_DRAW_TILE(s32 x0, s32 y0,
              u16 id, s32 highlight, float *data);
#include "stb_tilemap_editor.h"

typedef struct
{
    stbte_tilemap *tilemap;
}EditorData;

static const int tile_edit_size = 2;

static u32 hotkeys[] = {
   STBTE_act_undo,          SDLK_u,
   STBTE_tool_select,       SDLK_s,
   STBTE_tool_brush,        SDLK_d,
   STBTE_tool_erase,        SDLK_e,
   STBTE_tool_rectangle,    SDLK_r,
   STBTE_tool_eyedropper,   SDLK_i,
   STBTE_tool_link,         SDLK_l,
   STBTE_act_toggle_grid,   SDLK_g,
   STBTE_act_copy,          SDLK_c,
   STBTE_act_paste,         SDLK_p,
   STBTE_scroll_left,       SDLK_LEFT,
   STBTE_scroll_right,      SDLK_RIGHT,
   STBTE_scroll_up,         SDLK_UP,
   STBTE_scroll_down,       SDLK_DOWN,
};
static u32 num_hotkeys = sizeof(hotkeys)/2/sizeof(u32);

void STBTE_DRAW_RECT(s32 x0, s32 y0, s32 x1, s32 y1, u32 color)
{
    SDL_SetRenderDrawColor(renderer,color>>16,color>>8,color,255);
    SDL_Rect rect = {x0*tile_edit_size,y0*tile_edit_size,
        (x1-x0)*tile_edit_size,(y1-y0)*tile_edit_size};
    SDL_RenderFillRect(renderer,&rect);
}

// these are the drawmodes used in STBTE_DRAW_TILE
/*enum
{
   STBTE_drawmode_deemphasize = -1,
   STBTE_drawmode_normal      =  0,
   STBTE_drawmode_emphasize   =  1,
};*/

static u32 tile_colors[] = {
    (255<<16) + (255<<8) + (255),
    (  0<<16) + (255<<8) + (255),
    (  0<<16) + (  0<<8) + (255),
};

void STBTE_DRAW_TILE(s32 x0, s32 y0,
              u16 id, s32 highlight, float *data)
{
    u32 color = tile_colors[id];
    SDL_SetRenderDrawColor(renderer,color>>16,color>>8,color,255);
    SDL_Rect rect = {x0*tile_edit_size,y0*tile_edit_size,
        32*tile_edit_size,32*tile_edit_size};
    SDL_RenderFillRect(renderer,&rect);
}

static void input_editor(GameStateData *data,SDL_Event event)
{
    EditorData *ed = (EditorData*)data->data;
    switch(event.type){
        case SDL_KEYDOWN: {
            for(int i=0;i<num_hotkeys;i++){
                if(hotkeys[i*2+1] == event.key.keysym.sym){
                    stbte_action(ed->tilemap, hotkeys[i*2]);
                }
            }
        }
        case SDL_MOUSEMOTION:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEWHEEL:
        {
            stbte_mouse_sdl(ed->tilemap, &event, 1.f/(float)tile_edit_size,
                    1.f/(float)tile_edit_size, 0, 0);
        } break;
        case SDL_WINDOWEVENT: {
            if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED){
                u32 window_w = event.window.data1;
                u32 window_h = event.window.data2;
                stbte_set_display(0, 0, window_w/tile_edit_size,
                        window_h/tile_edit_size);
            }
        }break;
    }
}

static void draw_editor(GameStateData *data)
{
    EditorData *ed = (EditorData*)data->data;
    stbte_draw(ed->tilemap);
}

static void update_editor(GameStateData *data,float dt)
{
    EditorData *ed = (EditorData*)data->data;
    stbte_tick(ed->tilemap, dt);
}

GameState create_editor_state(u32 window_w, u32 window_h) {
    GameStateData *data = malloc(sizeof(GameStateData));
    memset(data,0,sizeof(GameStateData));
    EditorData *ed = malloc(sizeof(EditorData));
    memset(ed,0,sizeof(EditorData));
    stbte_tilemap *tilemap = stbte_create_map(32, 32, 3, 32, 32, 64);
    stbte_set_display(0, 0, window_w/tile_edit_size, window_h/tile_edit_size);
    stbte_define_tile(tilemap, 0, 3, "tiles");
    stbte_define_tile(tilemap, 1, 3, "tiles");
    stbte_define_tile(tilemap, 2, 3, "tiles");
    ed->tilemap = tilemap;
    data->data = ed;
    GameState editor_state = {input_editor, draw_editor, update_editor,data,0,0};
    return  editor_state;
}


