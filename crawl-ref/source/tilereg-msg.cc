/*
 *  File:       tilereg-msg.cc
 *
 *  Created by: ennewalker on Sat Jan 5 01:33:53 2008 UTC
 */

#include "AppHdr.h"

#ifdef USE_TILE

#include "tilereg-msg.h"

#include "libutil.h"
#include "macro.h"
#include "tilefont.h"

MessageRegion::MessageRegion(FontWrapper *font) :
    TextRegion(font),
    m_overlay(false)
{
}

int MessageRegion::handle_mouse(MouseEvent &event)
{
    // TODO enne - mouse scrolling here should mouse scroll up through
    // the message history in the message pane, without going to the CRT.

    if (!inside(event.px, event.py))
        return 0;

    if (event.event != MouseEvent::PRESS || event.button != MouseEvent::LEFT)
        return 0;

    if (mouse_control::current_mode() != MOUSE_MODE_COMMAND)
        return 0;

    return command_to_key(CMD_REPLAY_MESSAGES);
}

bool MessageRegion::update_tip_text(std::string& tip)
{
    if (mouse_control::current_mode() != MOUSE_MODE_COMMAND)
        return (false);

    tip = "[L-Click] Browse message history";
    return (true);
}

void MessageRegion::set_overlay(bool is_overlay)
{
    m_overlay = is_overlay;
}

void MessageRegion::render()
{
#ifdef DEBUG_TILES_REDRAW
    cprintf("rendering MessageRegion\n");
#endif
    int idx = -1;
    unsigned char char_back = 0;
    unsigned char col_back = 0;

    if (!m_overlay && !m_alt_text.empty())
    {
        coord_def min_pos(sx, sy);
        coord_def max_pos(ex, ey);
        m_font->render_string(sx + ox, sy + oy, m_alt_text.c_str(),
                              min_pos, max_pos, WHITE, false);
        return;
    }

    if (this == TextRegion::cursor_region && cursor_x > 0 && cursor_y > 0)
    {
        idx = cursor_x + mx * cursor_y;
        char_back = cbuf[idx];
        col_back  = abuf[idx];

        cbuf[idx] = '_';
        abuf[idx] = WHITE;
    }

    if (m_overlay)
    {
        int height;
        bool found = false;
        for (height = my; height > 0; height--)
        {
            unsigned char *buf = &cbuf[mx * (height - 1)];
            for (int x = 0; x < mx; x++)
            {
                if (buf[x] != ' ')
                {
                    found = true;
                    break;
                }
            }

            if (found)
                break;
        }

        if (height > 0)
        {
            height *= m_font->char_height();

            glmanager->reset_transform();

            ShapeBuffer buff;
            VColour col(100, 100, 100, 100);
            buff.add(sx, sy, ex, sy + height, col);
            buff.draw();
        }
    }

    m_font->render_textblock(sx + ox, sy + oy, cbuf, abuf, mx, my, m_overlay);

    if (idx >= 0)
    {
        cbuf[idx] = char_back;
        abuf[idx] = col_back;
    }
}

#endif
