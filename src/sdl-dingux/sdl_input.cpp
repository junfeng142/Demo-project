/*
 * FinalBurn Alpha for Dingux/OpenDingux
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <SDL/SDL.h>

#include "burner.h"
#include "snd.h"
#include "sdl_menu.h"
#include "sdl_input.h"
#include "sdl_run.h"

#define KEYPAD_UP       0x0001
#define KEYPAD_DOWN     0x0002
#define KEYPAD_LEFT     0x0004
#define KEYPAD_RIGHT    0x0008
#define KEYPAD_COIN     0x0010
#define KEYPAD_START    0x0020
#define KEYPAD_FIRE1    0x0040
#define KEYPAD_FIRE2    0x0080
#define KEYPAD_FIRE3    0x0100
#define KEYPAD_FIRE4    0x0200
#define KEYPAD_FIRE5    0x0400
#define KEYPAD_FIRE6    0x0800

#define BUTTON_UP       0x0001
#define BUTTON_DOWN     0x0002
#define BUTTON_LEFT     0x0004
#define BUTTON_RIGHT    0x0008
#define BUTTON_SELECT   0x0010
#define BUTTON_START    0x0020
#define BUTTON_A        0x0040
#define BUTTON_B        0x0080
#define BUTTON_X        0x0100
#define BUTTON_Y        0x0200
#define BUTTON_SL       0x0400
#define BUTTON_SR       0x0800
#define BUTTON_QT       0x1000
#define BUTTON_PAUSE    0x2000
#define BUTTON_QSAVE    0x4000
#define BUTTON_QLOAD    0x8000
#define BUTTON_MENU     0x10000

char joyCount = 0;
SDL_Joystick *joys[4];

unsigned int FBA_KEYPAD[4];
unsigned char ServiceRequest = 0;
unsigned char P1P2Start = 0;

// external variables
extern bool bPauseOn; // run.cpp

// external functions
void ChangeFrameskip(); // run.cpp

static int keypad = 0; // redefinable keys
static int keypc = 0;  // non-redefinabe keys

// Autofire state:
// 0 key up
// 1 first action or last action is auto up
// 2 last action is auto down
typedef struct
{
	int keymap;
	int keypc;
	int state;
	int interval;
} AUTOFIRE_STATE;

AUTOFIRE_STATE autofire_state[6];
int autofire_count;

void sdl_input_read(bool process_autofire) // called from do_keypad()
{
	SDL_Event event;

	int auto_it = process_autofire ? autofire_count : 0;
	while(auto_it > 0 || SDL_PollEvent(&event)) {
		// process autofire
		if (auto_it > 0) {
			bool handle = false;
			for (int it = auto_it - 1; it >= 0; it--) {
				auto_it--;
				if (autofire_state[it].state != 0) {
					if (nCurrentFrame % autofire_state[it].interval != 0) {
						continue;
					}
					event.type = (autofire_state[it].state == 1) ? SDL_KEYDOWN : SDL_KEYUP;
					autofire_state[it].state = autofire_state[it].state % 2 + 1;
					event.key.keysym.sym = (SDLKey)autofire_state[it].keymap;
					handle = true;
					break;
				}
			}
			if (!handle) {
				break;
			}
		}
		if (event.type == SDL_KEYUP) {
			// FBA keypresses
			if (event.key.keysym.sym == keymap.up) keypad &= ~KEYPAD_UP;
			else if (event.key.keysym.sym == keymap.down) keypad &= ~KEYPAD_DOWN;
			else if (event.key.keysym.sym == keymap.left) keypad &= ~KEYPAD_LEFT;
			else if (event.key.keysym.sym == keymap.right) keypad &= ~KEYPAD_RIGHT;
			else if (event.key.keysym.sym == keymap.fire1) keypad &= ~KEYPAD_FIRE1;
			else if (event.key.keysym.sym == keymap.fire2) keypad &= ~KEYPAD_FIRE2;
			else if (event.key.keysym.sym == keymap.fire3) keypad &= ~KEYPAD_FIRE3;
			else if (event.key.keysym.sym == keymap.fire4) keypad &= ~KEYPAD_FIRE4;
			else if (event.key.keysym.sym == keymap.fire5) keypad &= ~KEYPAD_FIRE5;
			else if (event.key.keysym.sym == keymap.fire6) keypad &= ~KEYPAD_FIRE6;
			else if (event.key.keysym.sym == keymap.coin1) keypad &= ~KEYPAD_COIN;
			else if (event.key.keysym.sym == keymap.start1) keypad &= ~KEYPAD_START;

			// handheld keypresses
			if (!process_autofire) {
				if (event.key.keysym.sym == SDLK_a) keypc &= ~BUTTON_A;
				else if (event.key.keysym.sym == SDLK_b) keypc &= ~BUTTON_B;
				else if (event.key.keysym.sym == SDLK_x) keypc &= ~BUTTON_X;
				else if (event.key.keysym.sym == SDLK_y) keypc &= ~BUTTON_Y;
				else if (event.key.keysym.sym == SDLK_m) keypc &= ~BUTTON_SL;
				else if (event.key.keysym.sym == SDLK_n) keypc &= ~BUTTON_SR;
				else if (event.key.keysym.sym == SDLK_k) keypc &= ~BUTTON_SELECT;
				else if (event.key.keysym.sym == SDLK_s) keypc &= ~BUTTON_START;
				else if (event.key.keysym.sym == SDLK_e) keypc &= ~BUTTON_QT;
				else if (event.key.keysym.sym == SDLK_p) keypc &= ~BUTTON_PAUSE;
				else if (event.key.keysym.sym == SDLK_i) keypc &= ~BUTTON_QSAVE;
				else if (event.key.keysym.sym == SDLK_j) keypc &= ~BUTTON_QLOAD;
				else if (event.key.keysym.sym == SDLK_q) keypc &= ~BUTTON_MENU;
			}
		} else if (event.type == SDL_KEYDOWN) {
			// FBA keypresses
			if (event.key.keysym.sym == keymap.up) keypad |= KEYPAD_UP;
			else if (event.key.keysym.sym == keymap.down) keypad |= KEYPAD_DOWN;
			else if (event.key.keysym.sym == keymap.left) keypad |= KEYPAD_LEFT;
			else if (event.key.keysym.sym == keymap.right) keypad |= KEYPAD_RIGHT;
			else if (event.key.keysym.sym == keymap.fire1) keypad |= KEYPAD_FIRE1;
			else if (event.key.keysym.sym == keymap.fire2) keypad |= KEYPAD_FIRE2;
			else if (event.key.keysym.sym == keymap.fire3) keypad |= KEYPAD_FIRE3;
			else if (event.key.keysym.sym == keymap.fire4) keypad |= KEYPAD_FIRE4;
			else if (event.key.keysym.sym == keymap.fire5) keypad |= KEYPAD_FIRE5;
			else if (event.key.keysym.sym == keymap.fire6) keypad |= KEYPAD_FIRE6;
			else if (event.key.keysym.sym == keymap.coin1) keypad |= KEYPAD_COIN;
			else if (event.key.keysym.sym == keymap.start1) keypad |= KEYPAD_START;

			// handheld keypresses
			if (!process_autofire) {
				if (event.key.keysym.sym == SDLK_a) keypc |= BUTTON_A;
				else if (event.key.keysym.sym == SDLK_b) keypc |= BUTTON_B;
				else if (event.key.keysym.sym == SDLK_x) keypc |= BUTTON_X;
				else if (event.key.keysym.sym == SDLK_y) keypc |= BUTTON_Y;
				else if (event.key.keysym.sym == SDLK_m) keypc |= BUTTON_SL;
				else if (event.key.keysym.sym == SDLK_n) keypc |= BUTTON_SR;
				else if (event.key.keysym.sym == SDLK_k) keypc |= BUTTON_SELECT;
				else if (event.key.keysym.sym == SDLK_s) keypc |= BUTTON_START;
				else if (event.key.keysym.sym == SDLK_e) keypc |= BUTTON_QT;
				else if (event.key.keysym.sym == SDLK_p) keypc |= BUTTON_PAUSE;
				else if (event.key.keysym.sym == SDLK_i) keypc |= BUTTON_QSAVE;
				else if (event.key.keysym.sym == SDLK_j) keypc |= BUTTON_QLOAD;
				else if (event.key.keysym.sym == SDLK_q) keypc |= BUTTON_MENU;
			}
		}
		if (process_autofire && auto_it <= 0) {
			break;
		}
	}
}

void do_keypad()
{
	int bVert = !options.rotate && (BurnDrvGetFlags() & BDF_ORIENTATION_VERTICAL);

	FBA_KEYPAD[0] = 0;
	FBA_KEYPAD[1] = 0;
	FBA_KEYPAD[2] = 0;
	FBA_KEYPAD[3] = 0;
	ServiceRequest = 0;
	P1P2Start = 0;

	sdl_input_read(false);
	for (int it = 0; it < autofire_count; it++) {
		if (autofire_state[it].state == 0 && (keypc & autofire_state[it].keypc)) {
			autofire_state[it].state = 1;
		}
		if (autofire_state[it].state != 0 && !(keypc & autofire_state[it].keypc)) {
			autofire_state[it].state = 0;
		}
	}
	sdl_input_read(true);

	// process redefinable keypresses
	if (keypad & KEYPAD_UP) FBA_KEYPAD[0] |= bVert ? KEYPAD_LEFT : KEYPAD_UP;
	if (keypad & KEYPAD_DOWN) FBA_KEYPAD[0] |= bVert ? KEYPAD_RIGHT : KEYPAD_DOWN;
	if (keypad & KEYPAD_LEFT) FBA_KEYPAD[0] |= bVert ? KEYPAD_DOWN : KEYPAD_LEFT;
	if (keypad & KEYPAD_RIGHT) FBA_KEYPAD[0] |= bVert ? KEYPAD_UP : KEYPAD_RIGHT;

	if (keypad & KEYPAD_COIN) FBA_KEYPAD[0] |= KEYPAD_COIN;
	if (keypad & KEYPAD_START) FBA_KEYPAD[0] |= KEYPAD_START;

	if (keypad & KEYPAD_FIRE1) FBA_KEYPAD[0] |= KEYPAD_FIRE1;		// A
	if (keypad & KEYPAD_FIRE2) FBA_KEYPAD[0] |= KEYPAD_FIRE2;		// B
	if (keypad & KEYPAD_FIRE3) FBA_KEYPAD[0] |= KEYPAD_FIRE3;		// X
	if (keypad & KEYPAD_FIRE4) FBA_KEYPAD[0] |= KEYPAD_FIRE4;		// Y
	if (keypad & KEYPAD_FIRE5) FBA_KEYPAD[0] |= KEYPAD_FIRE5;		// L
	if (keypad & KEYPAD_FIRE6) FBA_KEYPAD[0] |= KEYPAD_FIRE6;		// R

	// process non-redefinable keypresses
	if (keypc & BUTTON_QT) {
		GameLooping = false;
		keypc = keypad = 0;
	}

	if (keypc & BUTTON_MENU) {
		keypc = keypad = 0;
		SndPause(1);
		gui_Run();
		SndPause(0);
	}

	if (keypc & BUTTON_PAUSE) {
		bPauseOn = !bPauseOn;
		SndPause(bPauseOn);
		keypc &= ~BUTTON_PAUSE;
	}

	if(!bPauseOn) { // savestate fails inside pause
		if (keypc & BUTTON_QSAVE) {
			StatedSave(nSavestateSlot);
			keypc &= ~BUTTON_QSAVE;
		}

		if (keypc & BUTTON_QLOAD) {
			StatedLoad(nSavestateSlot);
			keypc &= ~BUTTON_QLOAD;
			bPauseOn = 0;
		}
	}

	if ((keypc & BUTTON_SL) && (keypc & BUTTON_SR)) {
		if (keypc & BUTTON_Y) { 
			ChangeFrameskip();
			keypc &= ~BUTTON_Y;
		} else if (keypc & BUTTON_B && !bPauseOn) {
			StatedSave(nSavestateSlot);
			keypc &= ~BUTTON_B;
		} else if (keypc & BUTTON_A && !bPauseOn) {
			StatedLoad(nSavestateSlot);
			keypc &= ~BUTTON_A;
			bPauseOn = 0;
		} else if (keypc & BUTTON_START) {
			keypc = keypad = 0;
			SndPause(1);
			gui_Run();
			SndPause(0);
		} else if (keypc & BUTTON_SELECT) ServiceRequest = 1;
	}
	else if ((keypc & BUTTON_START) && (keypc & BUTTON_SELECT)) P1P2Start = 1;

}

void sdl_input_init()
{
	joyCount = SDL_NumJoysticks();
	if (joyCount > 5) joyCount = 5;
	printf("%d Joystick(s) Found\n", joyCount);

	if (joyCount > 0) {
		for (int i = 0; i < joyCount; i++) {
			printf("%s\t",SDL_JoystickName(i));
			joys[i] = SDL_JoystickOpen(i);
			printf("Hats %d\t",SDL_JoystickNumHats(joys[i]));
			printf("Buttons %d\t",SDL_JoystickNumButtons(joys[i]));
			printf("Axis %d\n",SDL_JoystickNumAxes(joys[i]));
		}
	}
	sdl_autofire_init();
}

void sdl_autofire_init() {
	autofire_count = 0;
	CFG_AUTOFIRE_KEY *af = &autofire.fire1;
	int *key = &keymap.fire1;
	for(int i = 0; i < 6; af++, key++, i++) {
		if (af->fps != 0) {
			autofire_state[autofire_count].keymap = *key;
			autofire_state[autofire_count].state = 0;
			autofire_state[autofire_count].interval = af->fps / 2;
			
			int keydef = 0;
			if (af->key == SDLK_a) keydef = BUTTON_A;
			else if (af->key == SDLK_b) keydef = BUTTON_B;
			else if (af->key == SDLK_x) keydef = BUTTON_X;
			else if (af->key == SDLK_y) keydef = BUTTON_Y;
			else if (af->key == SDLK_m) keydef = BUTTON_SL;
			else if (af->key == SDLK_n) keydef = BUTTON_SR;

			if (key != 0) {
				autofire_state[autofire_count++].keypc = keydef;
			}
		}
	}
}
