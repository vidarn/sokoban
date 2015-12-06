#include "main.h"
#include <stdlib.h>
#include <stdio.h>

#include "editor.h"
#include "menu.h"
#include "easing.h"
#include "stb_tilemap_editor.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static int GAME_QUIT = 0;
void quit_game()
{
    GAME_QUIT = 1;
}
SDL_Renderer *renderer;

GameState editor_state;
GameState menu_state;

static GameState *game_state;

static u32 window_w = 800;
static u32 window_h = 512;

float tmp1 = 1.0f;
float tmp2 = 1.0f;

struct Tweener
{
    float  orig;
    float  value;
    u32    duration;
    u32    time;
    u32    type;
    float *subject;
    Tweener *next;
};

void add_tweener(float *subject, float value, u32 duration, u32 type)
{

    u8 found_existing = 0;
    Tweener *current = game_state->tweeners_head;
    Tweener *previous = 0;
    //NOTE(Vidar): replace existing tweener if it points at the same variable
    while(current != 0) {
        if(current->subject == subject){
            found_existing = 1;
            current->orig     = *subject;
            current->value    = value;
            current->duration = duration;
            current->time     = 0;
            current->type     = type;
            break;
        }
        current = current->next;
    }

    if(!found_existing){
        Tweener *e  = malloc(sizeof(Tweener));
        e->orig     = *subject;
        e->value    = value;
        e->duration = duration;
        e->time     = 0;
        e->type     = type;
        e->subject  = subject;
        e->next     = 0;
        if(game_state->tweeners_tail != 0){
            game_state->tweeners_tail->next = e;
        }
        game_state->tweeners_tail = e;
        if(game_state->tweeners_head == 0){
            game_state->tweeners_head = e;
        }
    }
}


float apply_tweening(float t, u32 type){
    switch(type){
        case TWEEN_LINEARINTERPOLATION: return LinearInterpolation(t);
        case TWEEN_QUADRATICEASEIN:     return QuadraticEaseIn(t);
        case TWEEN_QUADRATICEASEOUT:    return QuadraticEaseOut(t);
        case TWEEN_QUADRATICEASEINOUT:  return QuadraticEaseInOut(t);
        case TWEEN_CUBICEASEIN:         return CubicEaseIn(t);
        case TWEEN_CUBICEASEOUT:        return CubicEaseOut(t);
        case TWEEN_CUBICEASEINOUT:      return CubicEaseInOut(t);
        case TWEEN_QUARTICEASEIN:       return QuarticEaseIn(t);
        case TWEEN_QUARTICEASEOUT:      return QuarticEaseOut(t);
        case TWEEN_QUARTICEASEINOUT:    return QuarticEaseInOut(t);
        case TWEEN_QUINTICEASEIN:       return QuinticEaseIn(t);
        case TWEEN_QUINTICEASEOUT:      return QuinticEaseOut(t);
        case TWEEN_QUINTICEASEINOUT:    return QuinticEaseInOut(t);
        case TWEEN_SINEEASEIN:          return SineEaseIn(t);
        case TWEEN_SINEEASEOUT:         return SineEaseOut(t);
        case TWEEN_SINEEASEINOUT:       return SineEaseInOut(t);
        case TWEEN_CIRCULAREASEIN:      return CircularEaseIn(t);
        case TWEEN_CIRCULAREASEOUT:     return CircularEaseOut(t);
        case TWEEN_CIRCULAREASEINOUT:   return CircularEaseInOut(t);
        case TWEEN_EXPONENTIALEASEIN:   return ExponentialEaseIn(t);
        case TWEEN_EXPONENTIALEASEOUT:  return ExponentialEaseOut(t);
        case TWEEN_EXPONENTIALEASEINOUT:return ExponentialEaseInOut(t);
        case TWEEN_ELASTICEASEIN:       return ElasticEaseIn(t);
        case TWEEN_ELASTICEASEOUT:      return ElasticEaseOut(t);
        case TWEEN_ELASTICEASEINOUT:    return ElasticEaseInOut(t);
        case TWEEN_BACKEASEIN:          return BackEaseIn(t);
        case TWEEN_BACKEASEOUT:         return BackEaseOut(t);
        case TWEEN_BACKEASEINOUT:       return BackEaseInOut(t);
        case TWEEN_BOUNCEEASEIN:        return BounceEaseIn(t);
        case TWEEN_BOUNCEEASEOUT:       return BounceEaseOut(t);
        case TWEEN_BOUNCEEASEINOUT:     return BounceEaseInOut(t);
        default:                        assert(0); return 0.f;
    }
}

void updateTweeners(u32 delta_ticks)
{
    Tweener *current = game_state->tweeners_head;
    Tweener *previous = 0;
    while(current != 0) {
        u8 free_current = 0;
        current->time += delta_ticks;
        if(current->time > current->duration){
            if(previous != 0){
                previous->next = current->next;
            }else{
                game_state->tweeners_head = current->next;
            }
            if(game_state->tweeners_tail == current){
                game_state->tweeners_tail = previous;
            }
            current->time = current->duration;
            free_current = 1;
        }
        float t = (float)current->time/(float)current->duration;
        float x = apply_tweening(t,current->type);
        *current->subject = (1.f-x)*current->orig
            + x*(current->value);
        if(free_current){
            Tweener *tmp = current;
            current = current->next;
            free(tmp);
        }else{
            previous = current;
            current = current->next;
        }
    }
}

void screen2pixels(float x, float y, u32 *x_out, u32 *y_out)
{
    *x_out = x*window_w;
    *y_out = y*window_h;
}

void pixels2screen(u32 x, u32 y, float *x_out, float *y_out)
{
    *x_out = (float)x/(float)window_w;
    *y_out = (float)y/(float)window_h;
}

TTF_Font *hud_font = NULL;
TTF_Font *menu_font = NULL;

void draw_text(TTF_Font *font,SDL_Color font_color, const char *message,s32 x,
        s32 y, float center_x, float center_y, float scale){
    SDL_Surface *surf = TTF_RenderText_Blended(font, message, font_color);
	if (surf != 0){
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surf);
        if (texture != 0){
            int iw, ih;
            SDL_QueryTexture(texture, NULL, NULL, &iw, &ih);
            float w = (float)iw*scale;
            float h = (float)ih*scale;
            SDL_Rect dest_rect = {x-center_x*w,y-center_y*h,w,h};
            SDL_RenderCopy(renderer,texture,NULL,&dest_rect);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surf);
	}

}

static void draw_fps(float dt)
{
    char fps_buffer[32] = {};
    SDL_Color font_color = {255,255,255,255};
    sprintf(fps_buffer,"fps: %1.2f",1.f/dt);
    u32 x,y;
    screen2pixels(1.f,0.f,&x,&y);
    draw_text(hud_font,font_color,fps_buffer,x,y,1.f,0.f,1.f);
}

static u32 last_tick = 0;
static float last_dt = 0;
static void main_callback(void * vdata)
{
    u32 current_tick = SDL_GetTicks();
    u32 delta_ticks = current_tick - last_tick;
    if (delta_ticks > 16) {
        float dt = (float)delta_ticks/1000.f;
        last_tick = current_tick;
        last_dt = dt;

        // Input
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            switch(event.type){
                case SDL_QUIT: {
                    quit_game();
                }break;
                case SDL_WINDOWEVENT: {
                    switch (event.window.event){
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                            window_w = event.window.data1;
                            window_h = event.window.data2;
                        break;
                        default:break;
                    }
                }break;
                case SDL_KEYDOWN: {
                    switch(event.key.keysym.sym){
                        case SDLK_F1:
                            game_state = &menu_state;
                            break;
                        case SDLK_F2:
                            game_state = &editor_state;
                            break;
                        case SDLK_a:
                            add_tweener(&tmp1,1.f,1800,TWEEN_EXPONENTIALEASEOUT);
                            break;
                        case SDLK_s:
                            add_tweener(&tmp1,8.f,800,TWEEN_BOUNCEEASEOUT);
                            break;
                    }
                }break;
            }
            game_state->input(game_state->data,event);
        }
        // Update
        updateTweeners(delta_ticks);
        game_state->update(game_state->data,dt);
    }
    // Render
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    game_state->draw(game_state->data);
    draw_fps(last_dt);
    SDL_RenderPresent(renderer);
}

int main(int argc, char** argv) {

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Ludum Dare 34",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_w, window_h,
        SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    if(window == 0){
    }
    
    renderer = SDL_CreateRenderer(window, -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(renderer == 0){
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    if(TTF_Init() != 0){
        printf("Could not init font\n");
    }
    hud_font  = TTF_OpenFont("data/font.ttf", 24);
    menu_font = TTF_OpenFont("data/font.ttf", 64);

    editor_state = create_editor_state(window_w,window_h);
    menu_state   = create_menu_state();
    game_state   = &menu_state;

    add_tweener(&tmp1,5.f,1800,TWEEN_BACKEASEOUT);
    add_tweener(&tmp2,3.f,1600,TWEEN_CUBICEASEOUT);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(main_callback, 0, 0, 1);
#else
    while (!GAME_QUIT) {
        main_callback(0);
    }
#endif
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
