#include "main.h"
#include "game.h"
#include "sprite.h"
#include "editor.h"
#include "assets.h"
#include "stretchy_buffer.h"
#include "khash.h"

enum{
    OBJ_ALIVE   = (1 << 0),
    OBJ_VISIBLE = (1 << 1),
    OBJ_SOLID   = (1 << 2),
    OBJ_DEFAULT = OBJ_ALIVE|OBJ_VISIBLE|OBJ_SOLID,
};
typedef struct
{
    void (*update)(Object *obj, GameData *gd, float dt);
    void (*turn)(Object *obj, GameData *gd);
    void (*free)(Object *obj);
}ObjectFuncs;
enum{
    OBJ_player,
    OBJ_crate,
    OBJ_high_tile,
};
ObjectFuncs obj_funcs[] = {
    {0,0,0},
    {0,0,0},
    {0,0,0},
};
struct Object
{
    void *data;
    u32 type;
    uvec2 pos;
    u32 sprite;
    u8 flags;
    uvec2 prev_pos;
    vec2 offset;
    u32 index;
};

//TODO(Vidar): move this to GameData
static u32 map_size_x = 8;
static u32 map_size_y = 6;

struct GameData
{
    float turn_time;
    u8 turn;
    float time;
    u32 player_id;
    Camera camera;
    Object *objs;
    u32 *map;
    s32 *obj_map; // store the obj index at each tile
};

u32 add_obj(Object obj, GameData *gd)
{
    obj.prev_pos = obj.pos;
    size_t count = sb_count(gd->objs);
    u8 found = 0;
    for(int i=0;i<count;i++){
        if(!(gd->objs[i].flags & OBJ_ALIVE)){
            obj.index = i;
            gd->objs[i] = obj;
            found = 1;
            break;
        }
    }
    if(!found){
        obj.index = count;
        sb_push(gd->objs,obj);
    }
    u32 a = obj.pos.x + obj.pos.y*map_size_x;
    assert(gd->obj_map[a] == -1);
    gd->obj_map[a] = obj.index;
    return obj.index;
}


void move_obj(Object *obj, float x, float y, GameData *gd);
u8 is_valid_location(u32 x, u32 y, GameData *gd)
{
    if(x>=map_size_x || y >= map_size_y){
        return 0;
    }
    u32 i = gd->obj_map[x+y*map_size_x];
    if(i != -1){
        if(gd->objs[i].flags & OBJ_SOLID){
            return 0;
        }
    }
    return gd->map[x+y*map_size_x] == 0;
}
Object *get_obj_at_location(u32 x, u32 y, GameData *gd)
{
    Object *obj = 0;
    u32 index = gd->obj_map[x+y*map_size_x];
    if(index != -1){
        obj = gd->objs + index;
    }
    return obj;
}


void spawn_hero(u32 x, u32 y, GameData *gd)
{
    Object obj = {0,OBJ_player,{(float)x,(float)y},SPRITE_hero,
        OBJ_VISIBLE|OBJ_ALIVE};
    gd->player_id = add_obj(obj,gd);
}
void spawn_crate(u32 x, u32 y, GameData *gd)
{
    Object obj = {0,OBJ_crate,{(float)x,(float)y},SPRITE_crate,
        OBJ_VISIBLE|OBJ_ALIVE};
    add_obj(obj,gd);
}
void spawn_high_tile(u32 x, u32 y, GameData *gd)
{
    Object obj = {0,OBJ_high_tile,{(float)x,(float)y},SPRITE_high_tile,
        OBJ_DEFAULT};
    add_obj(obj,gd);
}
u8 is_crate_movable(Object *obj, float x, float y, GameData *gd){
    return is_valid_location(obj->pos.x+x,obj->pos.y+y,gd)
        && get_obj_at_location(obj->pos.x+x,obj->pos.y+y,gd)==0;
}


void load_level(const char *filename, GameData *gd);

void move_obj(Object *obj, float x, float y, GameData *gd)
{
    u32 a = obj->pos.x + obj->pos.y*map_size_x;
    gd->obj_map[a] = -1;

    obj->prev_pos = obj->pos;
    obj->pos.x += x;
    obj->pos.y += y;

    a = obj->pos.x + obj->pos.y*map_size_x;
    assert(gd->obj_map[a] == -1);
    gd->obj_map[a] = obj->index;
}

static void move_player(GameData *gd,float x, float y)
{
    if(!gd->turn){
        Object *player = gd->objs + gd->player_id;
        u32 target_x = player->pos.x+x;
        u32 target_y = player->pos.y+y;
        if(is_valid_location(target_x, target_y,gd)){
            u8 valid_move = 1;
            Object *other = get_obj_at_location(target_x,target_y,gd);
            if(other){
                switch(other->type){
                    case OBJ_crate:
                        valid_move = valid_move &&
                            is_crate_movable(other,x,y,gd);
                        if(valid_move){
                            move_obj(other,x,y,gd);
                        }
                        break;
                }
            }
            if(valid_move){
                gd->turn = 1;
                gd->turn_time = 0.f;
                add_tweener(&(gd->turn_time),1,100,TWEEN_CUBICEASEOUT);
                move_obj(player, x,y, gd);
                for(int i=0;i<sb_count(gd->objs);i++){
                    Object *obj = gd->objs + i;
                    if(obj->flags & OBJ_ALIVE){
                        if(obj_funcs[obj->type].turn){
                            obj_funcs[obj->type].turn(obj,gd);
                        }
                    }
                }
            }
        }
    }
}

static void input_game(GameStateData *data,SDL_Event event)
{
    GameData *gd = (GameData*)data->data;
    switch(event.type){
            case SDL_KEYDOWN: {
                if(!event.key.repeat && event.key.type == SDL_KEYDOWN){
                    switch(event.key.keysym.sym){
                        case SDLK_RETURN:
                            load_level("data/current.map",gd);
                            break;
                        case SDLK_LEFT:
                            move_player(gd,-1.f,0.f);
                            break;
                        case SDLK_RIGHT:
                            move_player(gd, 1.f,0.f);
                            break;
                        case SDLK_DOWN:
                            move_player(gd,0.f, 1.f);
                            break;
                        case SDLK_UP:
                            move_player(gd,0.f,-1.f);
                            break;
                    }
                }
            }break;
        default: break;
    }
}

static void draw_game(GameStateData *data)
{
    GameData *gd = (GameData*)data->data;
    u32 tile;
    for(int y=0;y<map_size_y;y++){
        for(int x=0;x<map_size_x;x++){
            switch(gd->map[x+y*map_size_x]){
                case 0:
                    tile = SPRITE_tile;
                    break;
                case 1:
                    tile = SPRITE_high_tile;
                    break;
            }
            draw_sprite_at_tile(tile, x, y, gd->camera);
        }
    }
    for(int y=0;y<map_size_y;y++){
        for(int x=0;x<map_size_x;x++){
            u32 i = gd->obj_map[x+y*map_size_x];
            if(i!=-1){
                Object *obj = &gd->objs[i];
                float x = gd->turn_time*obj->pos.x
                    + (1.f-gd->turn_time)*obj->prev_pos.x+obj->offset.x;
                float y = gd->turn_time*obj->pos.y
                    + (1.f-gd->turn_time)*obj->prev_pos.y+obj->offset.y;
                draw_sprite_at_tile(obj->sprite, x, y, gd->camera);
            }
        }
    }
}

static void update_game(GameStateData *data,float dt)
{
    GameData *gd = (GameData*)data->data;
    gd->time += dt; //NOTE(Vidar): I know this is stupid, should be fixed point...
    if(gd->turn){
        if(gd->turn_time > 1.f - 0.0001f){
            gd->turn = 0;
            for(int i=0;i<sb_count(gd->objs);i++){
                Object *obj = gd->objs + i;
                obj->prev_pos = obj->pos;
            }
        }
    }
    gd->camera.scale = (float)window_w/(float)tile_w/(float)map_size_x;
    gd->camera.scale = min((float)window_h/(float)tile_h/(float)map_size_y,
            gd->camera.scale);
    float a = max(window_w,window_h);
    gd->camera.offset_x = 0.5f*((float)window_w
            -gd->camera.scale*(float)tile_w*(float)map_size_x);
    gd->camera.offset_y = 0.5f*((float)window_h
            -gd->camera.scale*(float)tile_h*(float)map_size_y);
    for(int i=0;i<sb_count(gd->objs);i++){
        Object *obj = gd->objs + i;
        if(obj->flags & OBJ_ALIVE){
            if(obj_funcs[obj->type].update){
                obj_funcs[obj->type].update(obj,gd,dt);
            }
        }
    }
}

void load_level(const char *filename, GameData *gd)
{
    gd->turn = 0;
    gd->turn_time = 0;
    gd->time = 0;
    gd->player_id = 0;
    for(int i=0;i<sb_count(gd->objs);i++){
        Object *obj = gd->objs + i;
        if(obj->flags & OBJ_ALIVE){
            if(obj_funcs[obj->type].free){
                obj_funcs[obj->type].free(obj);
            }
            obj->flags = 0;
        }
    }
    Map m = load_map(filename);
    map_size_x = m.w;
    map_size_y = m.h;
    if(gd->map){
        free(gd->map);
    }
    if(gd->obj_map){
        free(gd->obj_map);
    }
    gd->map = malloc(sizeof(u32)*m.w*m.h);
    gd->obj_map = malloc(sizeof(s32)*m.w*m.h);
    //layer 0 - tiles
    for(int y=0;y<m.h;y++){
        for(int x=0;x<m.w;x++){
            gd->map[x+y*m.w] = m.tiles[x+y*m.w];
            gd->obj_map[x+y*m.w] = -1;
        } 
    }
    //layer 1 - spawners
    s16 *t = m.tiles + m.w*m.h;
    for(int y=0;y<m.h;y++){
        for(int x=0;x<m.w;x++){
            s16 s = t[x+y*m.w];
            if(s != -1){
                Spawner spawner = spawners[s-num_tiles];
                spawner.spawn_func(x,y,gd);
            }
        }
    }
    delete_map(m);
}

GameState create_game_state() {
    GameStateData *data = malloc(sizeof(GameStateData));
    memset(data,0,sizeof(GameStateData));
    GameData *gd = malloc(sizeof(GameData));
    memset(gd,0,sizeof(GameData));
    FILE *f = fopen("data/hero.png","rb");
    s32 w,h,c;
    load_sprites();
    load_level("data/current.map",gd);
    data->data = gd;
    GameState game_state = {input_game, draw_game,update_game,data,0,0};
    return  game_state;
}



