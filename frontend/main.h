/*  Pcsx - Pc Psx Emulator
 *  Copyright (C) 1999-2002  Pcsx Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA
 */

#ifndef __FRONTEND_MAIN_H__
#define __FRONTEND_MAIN_H__

#include "config.h"

#define MEMCARD_DIR "/mnt/FunKey/.pcsx/memcards/"
#define PLUGINS_DIR "/mnt/FunKey/.pcsx/plugins/"
#define PLUGINS_CFG_DIR "/mnt/FunKey/.pcsx/plugins/cfg"
#define PCSX_DOT_DIR "/mnt/FunKey/.pcsx/"
#define STATES_DIR "/mnt/FunKey/.pcsx/sstates"
#define CHEATS_DIR "/mnt/FunKey/.pcsx/cheats/"
#define PATCHES_DIR "/mnt/FunKey/.pcsx/patches/"
#define BIOS_DIR "/mnt/FunKey/.pcsx/bios/"
#define CFG_DIR "/mnt/FunKey/.pcsx/cfg"

extern char cfgfile_basename[MAXPATHLEN];

extern int state_slot;

/* emu_core_preinit - must be the very first call
 * emu_core_init - to be called after platform-specific setup */
int emu_core_preinit(void);
int emu_core_init(void);

void emu_core_ask_exit(void);

void emu_set_default_config(void);
void emu_on_new_cd(int show_hud_msg);

int get_state_filename(char *buf, int size, int i);
int emu_check_state(int slot);
int emu_save_state(int slot);
int emu_load_state(int slot);

void set_cd_image(const char *fname);

extern unsigned long gpuDisp;
extern int ready_to_go, g_emu_want_quit, g_emu_resetting;
extern int need_screen_cleared;

extern char hud_msg[64];
extern int hud_new_msg;

enum sched_action {
	SACTION_NONE,
	SACTION_ENTER_MENU,
	SACTION_LOAD_STATE,
	SACTION_PRE_SAVE_STATE,
	SACTION_SAVE_STATE,
	SACTION_NEXT_SSLOT,
	SACTION_PREV_SSLOT,
	SACTION_TOGGLE_FSKIP,
	SACTION_SWITCH_DISPMODE,
	SACTION_ASPECT_RATIO_FACTOR_DECREASE,
	SACTION_ASPECT_RATIO_FACTOR_INCREASE,
	SACTION_FAST_FORWARD,
	SACTION_SCREENSHOT,
	SACTION_VOLUME_UP,
	SACTION_VOLUME_DOWN,
	SACTION_MINIMIZE,
	SACTION_GUN_TRIGGER = 16,
	SACTION_TOGGLE_FPS,
	SACTION_TOGGLE_FULLSCREEN,
	SACTION_GUN_A,
	SACTION_GUN_B,
	SACTION_GUN_TRIGGER2,
	SACTION_BRIGHTNESS_DOWN,
	SACTION_BRIGHTNESS_UP,
	SACTION_QUICK_SAVE_AND_POWEROFF,
};

#define SACTION_GUN_MASK (0x0f << SACTION_GUN_TRIGGER)

extern enum sched_action emu_action, emu_action_old, emu_action_future;
static inline void emu_set_action(enum sched_action action_)
{
	extern int stop;

	if (action_ == SACTION_NONE)
		emu_action_old = 0;
	else if (action_ != emu_action_old)
		stop = 1;
	emu_action = action_;
}

#endif /* __FRONTEND_MAIN_H__ */
