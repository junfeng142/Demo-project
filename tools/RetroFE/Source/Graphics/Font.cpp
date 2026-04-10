/* This file is part of RetroFE.
 *
 * RetroFE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RetroFE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RetroFE.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "Font.h"
#include "../SDL.h"
#include "../Utility/Log.h"
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
//#include <SDL/SDL_gfxBlitFunc.h>
#include <cstdio>
#include <cstring>

Font::Font(std::string fontPath, int fontSize, SDL_Color color)
    : texture(NULL)
    , fontPath_(fontPath)
    , fontSize_(fontSize)
    , color_(color)
{
}

Font::~Font()
{
    deInitialize();
}

SDL_Surface *Font::getTexture()
{
    return texture;
}

int Font::getHeight()
{
    return height;
}
int Font::getAscent()
{
    return ascent;
}

SDL_Surface* Font::renderText(const std::string& text)
{
    if (!font_ || text.empty()) {
        return NULL;
    }
    
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font_, text.c_str(), color_);
    if (!surface) {
        Logger::write(Logger::ZONE_WARNING, "Font", 
            std::string("Failed to render text: ") + TTF_GetError());
    }
    
    return surface;
}

bool Font::initialize()
{
    font_ = TTF_OpenFont(fontPath_.c_str(), fontSize_);

    if (!font_)
    {
        std::stringstream ss;
        ss << "Could not open font: " << TTF_GetError();
        Logger::write(Logger::ZONE_ERROR, "FontCache", ss.str());
        return false;
    }

    height = TTF_FontHeight(font_);
    ascent = TTF_FontAscent(font_);

    return true;
}

void Font::deInitialize()
{
    if (font_) {
        TTF_CloseFont(font_);
        font_ = NULL;
    }
}
