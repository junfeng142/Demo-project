#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include <SDL/SDL.h>

#define NUM_ROWS 6
#define NUM_KEYS 18

#if defined(MIYOOMINI)

#define KEY_UP SDLK_UP
#define KEY_DOWN SDLK_DOWN
#define KEY_LEFT SDLK_LEFT
#define KEY_RIGHT SDLK_RIGHT
#define KEY_ENTER SDLK_SPACE // A
#define KEY_TOGGLE SDLK_LCTRL // B
#define KEY_BACKSPACE SDLK_t // R1
#define KEY_SHIFT SDLK_e // L1
#define KEY_LOCATION SDLK_LALT // Y
#define KEY_ACTIVATE SDLK_LSHIFT // X
#define KEY_QUIT SDLK_ESCAPE // MENU
//#define KEY_HELP SDLK_RETURN // 
#define KEY_TAB SDLK_RCTRL // SELECT
#define KEY_RETURN SDLK_RETURN // START
#define KEY_ARROW_LEFT SDLK_TAB // L2
#define KEY_ARROW_RIGHT SDLK_BACKSPACE // R2
//#define KEY_ARROW_UP SDLK_KP_DIVIDE // 
//#define KEY_ARROW_DOWN SDLK_KP_PERIOD // 

#elif defined(RS97)

#define KEY_UP SDLK_UP
#define KEY_DOWN SDLK_DOWN
#define KEY_LEFT SDLK_LEFT
#define KEY_RIGHT SDLK_RIGHT
#define KEY_ENTER SDLK_LCTRL // A
#define KEY_TOGGLE SDLK_LALT // B
#define KEY_BACKSPACE SDLK_BACKSPACE // R
#define KEY_SHIFT SDLK_TAB // L
#define KEY_LOCATION SDLK_LSHIFT // Y
#define KEY_ACTIVATE SDLK_SPACE // X
#define KEY_QUIT SDLK_HOME // SELECT
#define KEY_HELP SDLK_RETURN // START
#define KEY_TAB SDLK_ESCAPE // START
#define KEY_RETURN SDLK_RETURN // START
#define KEY_ARROW_LEFT SDLK_PAGEUP //LEFT
#define KEY_ARROW_RIGHT SDLK_PAGEDOWN //RIGHT
//#define KEY_ARROW_UP SDLK_KP_DIVIDE //LEFT
//#define KEY_ARROW_DOWN SDLK_KP_PERIOD //RIGHT

#else

#define KEY_UP SDLK_u
#define KEY_DOWN SDLK_d
#define KEY_LEFT SDLK_l
#define KEY_RIGHT SDLK_r
#define KEY_ENTER SDLK_a
#define KEY_TOGGLE SDLK_x
#define KEY_BACKSPACE SDLK_b
#define KEY_SHIFT SDLK_m
#define KEY_LOCATION SDLK_y
#define KEY_ACTIVATE SDLK_n
#define KEY_QUIT SDLK_q
#define KEY_HELP SDLK_k
#define KEY_TAB SDLK_ESCAPE
#define KEY_RETURN SDLK_s
#define KEY_ARROW_LEFT SDLK_l
#define KEY_ARROW_RIGHT SDLK_r
//#define KEY_ARROW_UP SDLK_HOME
//#define KEY_ARROW_DOWN SDLK_END

#endif

#define KMOD_SYNTHETIC (1 << 13)

void init_keyboard();
void draw_keyboard(SDL_Surface* surface);
int handle_keyboard_event(SDL_Event* event);
extern int active;
extern int show_help;

#endif
