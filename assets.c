#include "assets.h"
#include "sprite.h"

Tile tiles[] = {
    {SPRITE_tile},
    {SPRITE_high_tile},
};

#define SPAWNER_LIST \
    SPAWNER(SPRITE_hero,hero) \
    SPAWNER(SPRITE_crate,crate) \
    SPAWNER(SPRITE_high_tile,high_tile) \

#define SPAWNER(sprite,name) \
    void spawn_##name(u32 x, u32 y, GameData *gd); 
SPAWNER_LIST
#undef SPAWNER

#define SPAWNER(sprite,name) {sprite, spawn_##name},
Spawner spawners[] = {
SPAWNER_LIST
};
#undef SPAWNER

const u32 num_tiles = sizeof(tiles)/sizeof(Tile);
const u32 num_spawners = sizeof(spawners)/sizeof(Spawner);
