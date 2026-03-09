#ifndef _TMAGAME_CPP_
#define _TMAGAME_CPP_

#include "ma_classes.h"
#include <ctype.h>
#include <string.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>

// **************************************
// Definici�n de m�todos de clase TMAGame
// **************************************
//
REGISTER_CLASS_ID(TMAGame);
// SDL_Surface *ScreenSurface;
PMAGame TMAGame::oc_MAGame=NULL;
bool TMAGame::KeyState[TMAGame::KEY_MAX];
bool TMAGame::ci_endgame=false;
bool TMAGame::ib_soundAvailable=false;
bool TMAGame::ib_joystickAvailable=false;
long TMAGame::il_time_millis=0;
int  TMAGame::ii_last_ascii_key=0;
int  TMAGame::ARGC=0;
char **TMAGame::ARGV=NULL;
SDL_Surface *TMAGame::ScreenSurf=NULL;
PMABitmap TMAGame::Screen=NULL;
TMAInputController TMAGame::inputController;

TMAGame::TMAGame(void)
{
    if (oc_MAGame!=NULL)
    {
        exit(0);
    }
    oc_MAGame = this;
    Screen = NULL;    
    ScreenSurf = NULL;    
    io_current_scene=NULL;
    io_next_scene=NULL;
    ib_remove_last_scene=false;
    releaseKeys();
    setGameFPS(0);
}

TMAGame::~TMAGame()
{
    cleanup();
}

void TMAGame::setupLanguage(const char *domain,const char *locale)
{}

void TMAGame::releaseKeys(void)
{
    memset(KeyState,0,sizeof(KeyState));
    releaseLastAsciiKeyPressed();
    inputController.clearStatus();
}

void TMAGame::setGameFPS(int fps)
{
    ii_game_speed = (fps<=0)?0:(1000/fps);
}

PMAGame TMAGame::getCurrentGame(void)
{
    return oc_MAGame;
}

long TMAGame::getCPUTime(void)
{
    return SDL_GetTicks();
}

void TMAGame::sleep(int ms)
{
   SDL_Delay(ms);
}

void TMAGame::showAlert(const char *txt)
{
    //
}

void TMAGame::showLastAlert()
{
    //
}

void TMAGame::showFatalError(const char *txt)
{
    //
    endGame();
}

void TMAGame::setWindowTitle(const char *txt)
{
    #if !defined(TARGET_GP2X) && !defined(TARGET_WII)
        SDL_WM_SetCaption(txt,NULL);
    #endif
}

void TMAGame::getScreenSize(int& width,int& height)
{
    SDL_Surface *lscr = (ScreenSurf!=NULL)?ScreenSurf:SDL_GetVideoSurface();

    width = lscr->w;
    height = lscr->h;
}

int TMAGame::getScreenWidth(void)
{
    SDL_Surface *lscr = (ScreenSurf!=NULL)?ScreenSurf:SDL_GetVideoSurface();
    return lscr->w;
}

int TMAGame::getScreenHeight(void)
{
    SDL_Surface *lscr = (ScreenSurf!=NULL)?ScreenSurf:SDL_GetVideoSurface();
    return lscr->h;
}

void TMAGame::FlipScreen(void)
{
    //SDL_Flip(ScreenSurf);
    /*if(SDL_MUSTLOCK(ScreenSurface)) SDL_LockSurface(ScreenSurface);
    int x, y;
    uint32_t *s = (uint32_t*)ScreenSurf->pixels;
    uint32_t *d = (uint32_t*)ScreenSurface->pixels;
    for(y=0; y<240; y++){
      for(x=0; x<160; x++){
        *d++ = *s++;
      }
      d+= 160;
    }
    if(SDL_MUSTLOCK(ScreenSurface)) SDL_UnlockSurface(ScreenSurface);
    */
    // SDL_SoftStretch(ScreenSurf, NULL, ScreenSurface, NULL);
    // SDL_Flip(ScreenSurface);
    int fd = open("/dev/fb0", O_RDWR);
    ioctl(fd, FBIO_WAITFORVSYNC, 0);
    SDL_Flip(ScreenSurf);
    close(fd);
}
    
bool TMAGame::setGraphicsMode(int width,int height,int colordepth,bool fullscreen)
{
    Uint32 flags =  SDL_HWSURFACE|SDL_DOUBLEBUF;
    
    #if defined(TARGET_WII) || defined(TARGET_GP2X)
        flags|=SDL_FULLSCREEN;
    #else
        if (fullscreen) flags|=SDL_FULLSCREEN;
    #endif
    
    if (Screen!=NULL)
    {
        delete Screen;
        Screen=NULL;
        SDL_FreeSurface(ScreenSurf);
    }

    #ifdef MIYOO_MODE
    ScreenSurf = SDL_SetVideoMode(width, height, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
    #else
    ScreenSurf = SDL_SetVideoMode(width, height, 16, SDL_HWSURFACE | SDL_TRIPLEBUF);
    #endif
    // ScreenSurface = SDL_SetVideoMode(320, 480, 16, flags);
    // ScreenSurf = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, colordepth, 0, 0, 0, 0);
    if (ScreenSurf!=NULL)
    {
        if (Screen!=NULL) delete Screen;
        Screen = new TMABitmap(ScreenSurf,true);
    }
    return (ScreenSurf!=NULL);
}

int TMAGame::getScreenDepth(void)  { return ScreenSurf->format->BitsPerPixel; }
        
bool TMAGame::isFullScreen()
{
    if (Screen==NULL) return false;
    return (ScreenSurf->flags & SDL_FULLSCREEN);
}

bool TMAGame::setFullScreen(bool fs)
{
    return setGraphicsMode(getScreenWidth(),getScreenHeight(),getScreenDepth(),fs);
}
        
void TMAGame::cleanup(void)
{
	if (Screen != NULL)
    {
        delete Screen;
        Screen = NULL;
    }
}

void TMAGame::endGame(void)
{
    ci_endgame = true;
}



int TMAGame::getLastAsciiKeyPressed(void)
{
    return ii_last_ascii_key;
}

void TMAGame::releaseLastAsciiKeyPressed(void)
{
    ii_last_ascii_key=0;
}

void TMAGame::setVolume(int v)
{
    Mix_Volume(-1,v);
    Mix_VolumeMusic(v);    
}

void TMAGame::run(void)
{    
    bool lbRun=true;
    long startTime=0;
    SDL_Event event;
    SDL_ShowCursor(SDL_DISABLE);
    
    // ---- CICLO PRINCIPAL DEL JUEGO
    //
    while (!ci_endgame)
    {
        // Invocamos el RUN
        //
        if (ii_game_speed>0) 
        {
            if (getCPUTime()>=startTime)
            {
                startTime = getCPUTime() + ii_game_speed;
                lbRun = true;
            }     
        }
        else lbRun=true;
        
        if (lbRun)
        {        
            updateInputController();
            			
			while (SDL_PollEvent (&event))
			{
				switch (event.type)
				{
					#if !defined (TARGET_GP2X) && !defined (TARGET_WII)
						case SDL_KEYDOWN:
							KeyState[event.key.keysym.sym]=true;
							ii_last_ascii_key = event.key.keysym.unicode;
							break;                        
						case SDL_KEYUP:
							KeyState[event.key.keysym.sym]=false;
							releaseLastAsciiKeyPressed();
							break;
					#endif
					
					case SDL_JOYBUTTONDOWN:
						updateInputController(event);
						break;
						
					case SDL_JOYBUTTONUP:
						updateInputController(event);
						break;
					
					case SDL_QUIT:
						endGame();
						break;
				}
			}			

            if ( (io_current_scene==NULL) || (!io_current_scene->doRun()) ) doRun();
            
            lbRun=false;
 
            if (io_next_scene==NULL)
            {
                // Invocamos el Paint
                //
                if (!io_current_scene) doPaint(*Screen);
                else io_current_scene->doPaint(*Screen);
                FlipScreen();
            }
            else
            {
                // Establecemos la nueva escena, si se cambio.
                //
                if (ib_remove_last_scene && io_current_scene!=NULL) delete io_current_scene;
                io_current_scene = io_next_scene;
                io_next_scene=NULL;
                continue;
            }   
        }        
    }
    //
    // ----------- FIN DE CICLO PRINCIPAL 
}

bool TMAGame::setScene(PMAScene esc,const PMAObject params,bool remove_last)
{
    if (io_next_scene!=NULL)
    {
        delete io_next_scene;
        io_next_scene=NULL;
    }
    
    if (!esc->isInitialized())
    {
        if (!esc->doInit(params)) return false;
        io_next_scene = esc;
        ib_remove_last_scene=remove_last;
    }
    
    return true;
}

// ***************************************
// Definici�n de m�todos de clase TMAScene
// ***************************************
//
REGISTER_CLASS_ID(TMAScene);

TMAScene::TMAScene()
{
    ib_initialized=false;
    ii_Estado=0;
}

bool TMAScene::doInit(const PMAObject params)
{
    ib_initialized=true;
    return true;
}

#endif
