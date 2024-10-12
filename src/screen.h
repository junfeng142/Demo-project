/*
 *  Copyright (C) 2021 Steward Fu
 *  Copyright (C) 2001 Peponas Mathieu
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __SCREEN_H__
#define __SCREEN_H__

#include "SDL.h"
#include "list.h"

void screen_flip(void);
void sdl_init(void);

typedef struct RGB2YUV
{
  Uint16 y;
  Uint8  u;
  Uint8  v;
  Uint32 yuy2;
}RGB2YUV;

extern RGB2YUV rgb2yuv[65536];

extern void init_rgb2yuv_table(void);
extern SDL_Surface *screen;
extern SDL_Surface *buffer, *sprbuf, *fps_buf, *scan, *fontbuf;
//SDL_Surface *triplebuf[2];
extern SDL_Rect visible_area;
extern int yscreenpadding;
extern uint8_t interpolation;
extern uint8_t nblitter;
extern uint8_t neffect;
extern uint8_t scale;
extern uint8_t fullscreen;
extern uint8_t get_effect_by_name(char *name);
extern uint8_t get_blitter_by_name(char *name);
extern void print_blitter_list(void);
extern void print_effect_list(void);
//void screen_change_blitter_and_effect(char *bname,char *ename);
extern LIST* create_effect_list(void);
extern LIST* create_blitter_list(void);
extern int screen_init();
extern int screen_reinit(void);
extern int screen_resize(int w, int h);
extern void screen_update();
extern void screen_close();
extern void screen_fullscreen();
extern void sdl_set_title(char *name);
extern void init_sdl(void);

#endif
