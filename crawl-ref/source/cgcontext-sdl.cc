#ifdef USE_TILE
#include "cgcontext.h"

#ifdef USE_SDL
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

/*
 * This file should only call functions that operate specifically
 * on Graphics Contexts.  If it needs to do something else, like
 * get an SDL-specific environment variable, call a UIWrapper
 * member function for increased modularity.
*/

GraphicsContext::GraphicsContext():
    palette(NULL),
    surf(NULL)
{
}

int GraphicsContext::load_image( const char *file )
{
    if (surf) SDL_FreeSurface(surf);

    FILE *imgfile = fopen(file, "rb");
    if (imgfile)
    {
        SDL_RWops *rw = SDL_RWFromFP(imgfile, 0);
        if (rw)
        {
            surf = IMG_Load_RW(rw, 0);
            SDL_RWclose(rw);
        }
        fclose(imgfile);
    }

    if (!surf)
        return (-1);

    create_palette();

    return (0);
}

int GraphicsContext::lock()
{
    return (SDL_LockSurface(surf));
}

void GraphicsContext::unlock()
{
    SDL_UnlockSurface(surf);
}

int GraphicsContext::height()
{
    if ( !surf )
        return -1;
    return (surf->h);
}

int GraphicsContext::width()
{
    if ( !surf )
        return -1;
    return (surf->w);
}

short int GraphicsContext::pitch()
{
    if ( !surf )
        return (-1);
    return (surf->pitch);
}

unsigned char GraphicsContext::bytes_per_pixel()
{
    if ( !surf )
        return (-1);
    return (surf->format->BytesPerPixel);
}

void *GraphicsContext::pixels()
{
    return (surf->pixels);
}

void *GraphicsContext::native_surface()
{
    return ((void *)surf);
}

unsigned int GraphicsContext::color_key()
{
    if ( !surf )
        return (-1);
    return (surf->format->colorkey);
}

void GraphicsContext::get_rgba(unsigned int pixel, unsigned char *r,
        unsigned char *g, unsigned char *b, unsigned char *a)
{
    if ( !surf )
        return;
    SDL_GetRGBA(pixel, surf->format, r, g, b, a);
}

void GraphicsContext::get_rgb(unsigned int pixel,     unsigned char *r,
                                    unsigned char *g,
                                    unsigned char *b)
{
    if ( !surf )
        return;
    SDL_GetRGB(pixel, surf->format, r, g, b);
}

void GraphicsContext::create_palette()
{
    // TODO: Figure out if this is really safe
    // ui_palette and ui_color are defined in the same way as
    // their SDL counterparts, so we just cast pointers here
    if ( !surf )
        return;

    // Per SDL spec, if bitsPerPixel>8 palette is NULL
    if ( surf->format->BitsPerPixel > 8 )
        return;

    // Otherwise do some magic to get access
    // TODO: Figure some way to do this elegantly that still allows
    // quick access to color pixels and doesn't use pointer casting
    palette = (ui_palette*)surf->format->palette;
    palette->colors = (ui_color*)surf->format->palette->colors;
}

void GraphicsContext::destroy_palette()
{
    if (palette)
        palette->colors = NULL;
    palette = NULL;
}

GraphicsContext::~GraphicsContext()
{
    destroy_palette();
    if ( surf )
        SDL_FreeSurface(surf);
}

#endif // USE_SDL
#endif // USE_TILE
