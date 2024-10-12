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

#ifndef __EMU_H__
#define __EMU_H__

#include <string.h>
#include <stdint.h>
#include <SDL.h>
#include <SDL_types.h>

typedef enum SYSTEM {
  SYS_ARCADE = 0,
  SYS_HOME,
  SYS_UNIBIOS,
  SYS_MAX
} SYSTEM;

typedef enum COUNTRY {
  CTY_JAPAN = 0,
  CTY_EUROPE,
  CTY_USA,
  CTY_ASIA,
  CTY_MAX
} COUNTRY;

struct config {
  char *game;
  uint8_t sound;
  uint8_t do_message;
  uint8_t rom_type;
  uint8_t special_bios;
  uint8_t extra_xor;
  SYSTEM system;
  COUNTRY country;

  uint8_t show_fps;
  char message[128];

  uint16_t a_btn;
  uint16_t b_btn;
  uint16_t x_btn;
  uint16_t y_btn;
  uint16_t l_btn;
  uint16_t r_btn;
  Uint16 x_start;
  Uint16 y_start;
  Uint16 res_x;
  Uint16 res_y;
  Uint16 sample_rate;
  Uint16 test_switch;
  Uint8 vsync;
  //Uint8 snd_st_reg_create;
  Uint8 nb_joy;
  Uint8 raster;
  Uint8 debug;
  Uint8 pal;
  Uint8 accurate940;
  Uint8 autoframeskip;
  Uint8 sleep_idle;
  Uint8 screen320;
  char fps[4];
  int *p1_key;
  int *p2_key;
  SDL_Joystick **joy;
  int *p1_joy;
  int *p2_joy;
  int *p1_hotkey0, *p1_hotkey1, *p1_hotkey2, *p1_hotkey3;
  int *p2_hotkey0, *p2_hotkey1, *p2_hotkey2, *p2_hotkey3;

  int p1_hotkey[4];
  int p2_hotkey[4];
};

extern struct config conf;

enum {
  BUT_A = 0,
  BUT_B,
  BUT_C,
  BUT_D,
  BUT_START,
  BUT_COIN,
  KB_UP,
  KB_DOWN,
  KB_LEFT,
  KB_RIGHT,
};
enum {
  AXE_X = 6,
  AXE_Y,
  AXE_X_DIR,
  AXE_Y_DIR
};

extern uint8_t key[SDLK_LAST];
extern uint8_t *joy_button[2];
extern int32_t *joy_axe[2];
extern uint32_t joy_numaxes[2];

extern void debug_loop(void);
extern void main_loop(void);
extern void init_neo(void);
extern void cpu_68k_dpg_step(void);
extern void setup_misc_patch(char *name);
extern void neogeo_reset(void);

#endif

