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


#include "Text.h"
#include "../../Utility/Log.h"
#include "../../SDL.h"
#include "../Font.h"
#include <sstream>


Text::Text( std::string text, Page &p, Font *font, float scaleX, float scaleY )
    : Component(p)
    , textData_(text)
    , fontInst_(font)
    , scaleX_(scaleX)
    , scaleY_(scaleY)
    , renderedText_(NULL)
{
    allocateGraphicsMemory( );
}

Text::~Text( )
{
    freeGraphicsMemory( );
    textData_.clear();
}

void Text::freeGraphicsMemory( )
{
    Component::freeGraphicsMemory( );
    if (renderedText_) {
        SDL_FreeSurface(renderedText_);
        renderedText_ = NULL;
    }
    textData_.clear();
}

void Text::allocateGraphicsMemory( )
{
    Component::allocateGraphicsMemory( );
    renderTextToSurface();
}

void Text::deInitializeFonts( )
{
    fontInst_->deInitialize( );
}

void Text::initializeFonts( )
{
    fontInst_->initialize( );
    renderTextToSurface();
}

void Text::setText( std::string text, int id )
{
    if ( getId( ) == id )
        textData_ = text;
        renderTextToSurface();
}

void Text::renderTextToSurface()
{
    if (renderedText_) {
        SDL_FreeSurface(renderedText_);
        renderedText_ = NULL;
    }
    
    if (textData_.empty()) return;
    
    Font* font = baseViewInfo.font ? baseViewInfo.font : fontInst_;
    if (!font) return;
    
    renderedText_ = font->renderText(textData_);
}

void Text::draw( )
{
    Component::draw( );

    if (!renderedText_ || baseViewInfo.Alpha <= 0.0f) return;
    
    float scale = 1.0f;
    float xOrigin = baseViewInfo.XRelativeToOrigin();
    float yOrigin = baseViewInfo.YRelativeToOrigin();
    
    SDL_Rect destRect;
    destRect.w = static_cast<int>(renderedText_->w * scaleX_);
    destRect.h = static_cast<int>(renderedText_->h * scaleY_);
    destRect.x = (static_cast<int>(xOrigin) > destRect.w / 2) ? (static_cast<int>(xOrigin) - destRect.w / 2) : static_cast<int>(xOrigin);
    destRect.y = (static_cast<int>(yOrigin) > destRect.h / 2) ? (static_cast<int>(yOrigin) - destRect.h / 2) : static_cast<int>(yOrigin);
    
    SDL::renderCopy(renderedText_, baseViewInfo.Alpha, NULL, &destRect, baseViewInfo);
}
