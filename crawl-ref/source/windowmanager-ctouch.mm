#include "AppHdr.h"

#include <string.h>

#include "cio.h"
#include "files.h"
#include "options.h"

#ifdef USE_TILE

#include "glwrapper.h"
#include "windowmanager.h"

#ifdef USE_CTOUCH

#include "windowmanager-ctouch.h"

WindowManager *wm = NULL;

void WindowManager::create()
{
    if (wm)
        return;

    wm = new CTWrapper();
}

void WindowManager::shutdown()
{
    delete wm;
    wm = NULL;
}

CTWrapper::CTWrapper()
{
    // Set the currently active DCSSController as ours.
    controller = [DCSSController currentlyActiveController];
    [controller retain];
}

CTWrapper::~CTWrapper()
{
    [controller crawlShutdown];
    [controller release];
}

int CTWrapper::init(coord_def *m_windowsz)
{
    return ([controller crawlStartup]);
}

int CTWrapper::screen_width() const
{
    return ([controller vinfo].current_w);
}

int CTWrapper::screen_height() const
{
    return ([controller vinfo].current_h);
}

void CTWrapper::set_window_title(const char *title)
{
    // SDL_WM_SetCaption(title, CRAWL);
}

bool CTWrapper::set_window_icon(const char* icon_name)
{
    return (true);
}

void CTWrapper::resize(coord_def &m_windowsz)
{
    glmanager->reset_view_for_resize(m_windowsz);
}

unsigned int CTWrapper::get_ticks() const
{
    return (0);
}

key_mod CTWrapper::get_mod_state() const
{
    return (MOD_NONE);
}

void CTWrapper::set_mod_state(key_mod mod)
{
}

int CTWrapper::wait_event(wm_event *event)
{
    return (1);
}

void CTWrapper::set_timer(unsigned int interval, wm_timer_callback callback)
{
}

int CTWrapper::raise_custom_event()
{
    return (0);
}

void CTWrapper::swap_buffers()
{
}

void CTWrapper::delay(unsigned int ms)
{
}

unsigned int CTWrapper::get_event_count(wm_event_type type)
{
    return (0);
}

bool CTWrapper::load_texture(GenericTexture *tex, const char *filename,
                              MipMapOptions mip_opt, unsigned int &orig_width,
                              unsigned int &orig_height, tex_proc_func proc,
                              bool force_power_of_two)
{	
	NSString *path = @"dat/tiles";
	NSString *name = [NSString stringWithCString:filename encoding:[NSString defaultCStringEncoding]];
	NSString *fullPath = [[NSBundle mainBundle] pathForResource:name ofType:nil inDirectory: path];
	
	NSLog(@"Loading texture: %s", filename);
	
	return (true);
}

int CTWrapper::byte_order()
{
    // Both the simulator and iPhone/iPad are LIL_ENDIAN
    return (WM_LIL_ENDIAN);
}

#endif // USE_CTOUCH
#endif // USE_TILE
