#ifndef CTOUCH_WINDOWMANAGER_H
#define CTOUCH_WINDOWMANAGER_H

#ifdef USE_TILE
#ifdef USE_CTOUCH

#include "windowmanager.h"

// Wrapper for Cocoa Touch
class CTWrapper : public WindowManager
{
public:
    CTWrapper();
    ~CTWrapper();

    // Class functions
    virtual int init(coord_def *m_windowsz);

    // Environment state functions
    virtual void set_window_title(const char *title);
    virtual bool set_window_icon(const char* icon_name);
    virtual key_mod get_mod_state() const;
    virtual void set_mod_state(key_mod mod);
    virtual int byte_order();

    // System time functions
    virtual void set_timer(unsigned int interval,
                           wm_timer_callback callback);
    virtual unsigned int get_ticks() const;
    virtual void delay(unsigned int ms);

    // Event functions
    virtual int raise_custom_event();
    virtual int wait_event(wm_event *event);
    virtual unsigned int get_event_count(wm_event_type type);

    // Display functions
    virtual void resize(coord_def &m_windowsz);
    virtual void swap_buffers();
    virtual int screen_width() const;
    virtual int screen_height() const;

    // Texture loading
    virtual bool load_texture(GenericTexture *tex, const char *filename,
                              MipMapOptions mip_opt, unsigned int &orig_width,
                              unsigned int &orig_height,
                              tex_proc_func proc = NULL,
                              bool force_power_of_two = true);

protected:
    struct {
        size_t current_w, current_h;
    } video_info;
};

#endif // USE_CTOUCH
#endif // USE_TILE

#endif // CTOUCH_WINDOWMANAGER_H
