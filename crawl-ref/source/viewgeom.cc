#include "AppHdr.h"

#include "viewgeom.h"

#include "options.h"
#include "player.h"
#include "state.h"
#include "stuff.h"

// ----------------------------------------------------------------------
// Layout helper classes
// ----------------------------------------------------------------------

// Moved from directn.h, where they didn't need to be.
// define VIEW_MIN_HEIGHT defined elsewhere
// define VIEW_MAX_HEIGHT use Options.view_max_height
// define VIEW_MIN_WIDTH defined elsewhere
// define VIEW_MAX_WIDTH use Options.view_max_width
#define HUD_WIDTH  42
#define HUD_HEIGHT 12
// #define MSG_MIN_HEIGHT defined elsewhere
#define MSG_MAX_HEIGHT Options.msg_max_height
#define MLIST_MIN_HEIGHT Options.mlist_min_height
#define MLIST_MIN_WIDTH 25  // non-inline layout only
#define MLIST_MAX_WIDTH 42
#define MLIST_GUTTER 1
#define HUD_MIN_GUTTER 2
#define HUD_MAX_GUTTER 4

// Helper for layouts.  Tries to increment lvalue without overflowing it.
static void _increment(int& lvalue, int delta, int max_value)
{
    lvalue = std::min(lvalue+delta, max_value);
}

class _layout
{
 public:
    _layout(coord_def termsz_, coord_def hudsz_) :
        termp(1,1),    termsz(termsz_),
        viewp(-1,-1),  viewsz(VIEW_MIN_WIDTH, VIEW_MIN_HEIGHT),
        hudp(-1,-1),   hudsz(hudsz_),
        msgp(-1,-1),   msgsz(0, MSG_MIN_HEIGHT),
        mlistp(-1,-1), mlistsz(MLIST_MIN_WIDTH, 0),
        hud_gutter(HUD_MIN_GUTTER),
        valid(false) {}

 protected:
    void _assert_validity() const
    {
#ifndef USE_TILE
        // Check that all the panes fit in the view.
        ASSERT( (viewp+viewsz - termp).x <= termsz.x );
        ASSERT( (viewp+viewsz - termp).y <= termsz.y );

        ASSERT( (hudp+hudsz - termp).x <= termsz.x );
        ASSERT( (hudp+hudsz - termp).y <= termsz.y );

        ASSERT( (msgp+msgsz - termp).x <= termsz.x );
        ASSERT( (msgp+msgsz - termp).y <= termsz.y );
        // Don't stretch message all the way to the bottom-right
        // character; it causes scrolling and badness.
        ASSERT( (msgp+msgsz - termp) != termsz );

        ASSERT( (mlistp+mlistsz-termp).x <= termsz.x );
        ASSERT( (mlistp+mlistsz-termp).y <= termsz.y );
#endif
    }
 public:
    const coord_def termp, termsz;
    coord_def viewp, viewsz;
    coord_def hudp;
    const coord_def hudsz;
    coord_def msgp, msgsz;
    coord_def mlistp, mlistsz;
    int hud_gutter;
    bool valid;
};

// vvvvvvghhh  v=view, g=hud gutter, h=hud, l=list, m=msg
// vvvvvvghhh
// vvvvvv lll
//        lll
// mmmmmmmmmm
class _inline_layout : public _layout
{
 public:
    _inline_layout(coord_def termsz_, coord_def hudsz_) :
        _layout(termsz_, hudsz_)
    {
        valid = _init();
    }

    bool _init()
    {
        // x: View gets leftover; then mlist; then hud gutter
        if (leftover_x() < 0)
            return (false);

        _increment(viewsz.x,   leftover_x(), Options.view_max_width);

        if ((viewsz.x % 2) != 1)
            --viewsz.x;

        mlistsz.x = hudsz.x;
        _increment(mlistsz.x,  leftover_x(), MLIST_MAX_WIDTH);
        _increment(hud_gutter, leftover_x(), HUD_MAX_GUTTER);
        _increment(mlistsz.x,  leftover_x(), INT_MAX);
        msgsz.x = termsz.x-1; // Can't use last character.

        // y: View gets as much as it wants.
        // mlist tries to get at least its minimum.
        // msg expands as much as it wants.
        // mlist gets any leftovers.
        if (leftover_y() < 0)
            return (false);

        _increment(viewsz.y, leftover_leftcol_y(), Options.view_max_height);
        if ((viewsz.y % 2) != 1)
            --viewsz.y;

        if (Options.classic_hud)
        {
            mlistsz.y = 0;
            _increment(msgsz.y,  leftover_y(), MSG_MAX_HEIGHT);
        }
        else
        {
            if (mlistsz.y < MLIST_MIN_HEIGHT)
                _increment(mlistsz.y, leftover_rightcol_y(), MLIST_MIN_HEIGHT);
            _increment(msgsz.y,  leftover_y(), MSG_MAX_HEIGHT);
            _increment(mlistsz.y, leftover_rightcol_y(), INT_MAX);
        }

        // Finish off by doing the positions.
        viewp  = termp;
        msgp   = termp + coord_def(0, std::max(viewsz.y, hudsz.y+mlistsz.y));
        hudp   = viewp + coord_def(viewsz.x+hud_gutter, 0);
        mlistp = hudp  + coord_def(0, hudsz.y);

        _assert_validity();
        return (true);
    }

    int leftover_x() const
    {
        int width = (viewsz.x + hud_gutter + std::max(hudsz.x, mlistsz.x));
        return (termsz.x - width);
    }
    int leftover_rightcol_y() const { return termsz.y-hudsz.y-mlistsz.y-msgsz.y; }
    int leftover_leftcol_y() const  { return termsz.y-viewsz.y-msgsz.y; }
    int leftover_y() const
    {
        return std::min(leftover_rightcol_y(), leftover_leftcol_y());
    }
};

// ll vvvvvvghhh  v=view, g=hud gutter, h=hud, l=list, m=msg
// ll vvvvvvghhh
// ll vvvvvv
// mmmmmmmmmmmmm
class _mlist_col_layout : public _layout
{
 public:
    _mlist_col_layout(coord_def termsz_, coord_def hudsz_)
        : _layout(termsz_, hudsz_)
    { valid = _init(); }
    bool _init()
    {
        // Don't let the mlist column steal all the width.  Up front,
        // take some for the view.  If it makes the layout fail, that's fine.
        _increment(viewsz.x, MLIST_MIN_WIDTH/2, Options.view_max_width);

        // x: View and mlist share leftover; then hud gutter.
        if (leftover_x() < 0)
            return (false);

        _increment(mlistsz.x,  leftover_x()/2, MLIST_MAX_WIDTH);
        _increment(viewsz.x,   leftover_x(),   Options.view_max_width);

        if ((viewsz.x % 2) != 1)
            --viewsz.x;

        _increment(mlistsz.x,  leftover_x(),   MLIST_MAX_WIDTH);
        _increment(hud_gutter, leftover_x(),   HUD_MAX_GUTTER);
        msgsz.x = termsz.x-1; // Can't use last character.

        // y: View gets leftover; then message.
        if (leftover_y() < 0)
            return (false);

        _increment(viewsz.y, leftover_y(), Options.view_max_height);

        if ((viewsz.y % 2) != 1)
            --viewsz.y;

        _increment(msgsz.y,  leftover_y(), INT_MAX);
        mlistsz.y = viewsz.y;

        // Finish off by doing the positions.
        mlistp = termp;
        viewp  = mlistp+ coord_def(mlistsz.x+MLIST_GUTTER, 0);
        msgp   = termp + coord_def(0, viewsz.y);
        hudp   = viewp + coord_def(viewsz.x+hud_gutter, 0);

        _assert_validity();
        return (true);
    }
 private:
    int leftover_x() const
    {
        int width = (mlistsz.x + MLIST_GUTTER + viewsz.x + hud_gutter + hudsz.x);
        return (termsz.x - width);
    }
    int leftover_y() const
    {
        const int top_y = std::max(std::max(viewsz.y, hudsz.y), mlistsz.y);
        const int height = top_y + msgsz.y;
        return (termsz.y - height);
    }
};

//////////////////////////////////////////////////////////////////////////////
// crawl_view_buffer

crawl_view_buffer::crawl_view_buffer()
    : buffer(NULL)
{
}

crawl_view_buffer::~crawl_view_buffer()
{
    delete [] buffer;
}

void crawl_view_buffer::size(const coord_def &sz)
{
    delete [] buffer;
    buffer = new screen_buffer_t [ sz.x * sz.y * 2 ];
}

// ----------------------------------------------------------------------
// crawl_view_geometry
// ----------------------------------------------------------------------

crawl_view_geometry::crawl_view_geometry()
    : termp(1, 1), termsz(80, 24),
      viewp(1, 1), viewsz(33, 17),
      hudp(40, 1), hudsz(-1, -1),
      msgp(1, viewp.y + viewsz.y), msgsz(80, 7),
      mlistp(hudp.x, hudp.y + hudsz.y),
      mlistsz(hudsz.x, msgp.y - mlistp.y),
      vbuf(), vgrdc(), viewhalfsz(), glos1(), glos2(),
      vlos1(), vlos2(), mousep(), last_player_pos()
{
}

void crawl_view_geometry::init_view()
{
    viewhalfsz = viewsz / 2;
    vbuf.size(viewsz);
    set_player_at(you.pos(), true);
}

void crawl_view_geometry::shift_player_to(const coord_def &c)
{
    // Preserve vgrdc offset after moving.
    const coord_def offset = crawl_view.vgrdc - you.pos();
    crawl_view.vgrdc = offset + c;
    last_player_pos = c;

    set_player_at(c);

    ASSERT(crawl_view.vgrdc == offset + c);
    ASSERT(last_player_pos == c);
}

void crawl_view_geometry::set_player_at(const coord_def &c, bool centre)
{
    if (centre)
    {
        vgrdc = c;
    }
    else
    {
        const coord_def oldc = vgrdc;
        const int xmarg = Options.scroll_margin_x + LOS_RADIUS <= viewhalfsz.x
                            ? Options.scroll_margin_x
                            : viewhalfsz.x - LOS_RADIUS;
        const int ymarg = Options.scroll_margin_y + LOS_RADIUS <= viewhalfsz.y
                            ? Options.scroll_margin_y
                            : viewhalfsz.y - LOS_RADIUS;

        if (Options.view_lock_x)
            vgrdc.x = c.x;
        else if (c.x - LOS_RADIUS < vgrdc.x - viewhalfsz.x + xmarg)
            vgrdc.x = c.x - LOS_RADIUS + viewhalfsz.x - xmarg;
        else if (c.x + LOS_RADIUS > vgrdc.x + viewhalfsz.x - xmarg)
            vgrdc.x = c.x + LOS_RADIUS - viewhalfsz.x + xmarg;

        if (Options.view_lock_y)
            vgrdc.y = c.y;
        else if (c.y - LOS_RADIUS < vgrdc.y - viewhalfsz.y + ymarg)
            vgrdc.y = c.y - LOS_RADIUS + viewhalfsz.y - ymarg;
        else if (c.y + LOS_RADIUS > vgrdc.y + viewhalfsz.y - ymarg)
            vgrdc.y = c.y + LOS_RADIUS - viewhalfsz.y + ymarg;

        if (vgrdc != oldc && Options.center_on_scroll)
            vgrdc = c;

        if (!Options.center_on_scroll && Options.symmetric_scroll
            && !Options.view_lock_x
            && !Options.view_lock_y
            && (c - last_player_pos).abs() == 2
            && (vgrdc - oldc).abs() == 1)
        {
            const coord_def dp = c - last_player_pos;
            const coord_def dc = vgrdc - oldc;
            if ((dc.x == dp.x) != (dc.y == dp.y))
                vgrdc = oldc + dp;
        }
    }

    glos1 = c - coord_def(LOS_RADIUS, LOS_RADIUS);
    glos2 = c + coord_def(LOS_RADIUS, LOS_RADIUS);

    vlos1 = glos1 - vgrdc + view_centre();
    vlos2 = glos2 - vgrdc + view_centre();

    last_player_pos = c;
}

void crawl_view_geometry::init_geometry()
{
    termsz = coord_def( get_number_of_cols(), get_number_of_lines() );
    hudsz  = coord_def(HUD_WIDTH,
                       HUD_HEIGHT + (Options.show_gold_turns ? 1 : 0));

    const _inline_layout lay_inline(termsz, hudsz);
    const _mlist_col_layout lay_mlist(termsz, hudsz);

    if (! lay_inline.valid)
    {
#ifndef USE_TILE
        // Terminal too small; exit with an error.
        if (!crawl_state.need_save)
        {
            end(1, false, "Terminal too small (%d,%d); need at least (%d,%d)",
                termsz.x, termsz.y,
                termsz.x + std::max(0, -lay_inline.leftover_x()),
                termsz.y + std::max(0, -lay_inline.leftover_y()));
        }
#endif
    }

    const _layout* winner = &lay_inline;
    if (Options.mlist_allow_alternate_layout
        && !Options.classic_hud
        && lay_mlist.valid)
    {
        winner = &lay_mlist;
    }

    msgp    = winner->msgp;
    msgsz   = winner->msgsz;
    viewp   = winner->viewp;
    viewsz  = winner->viewsz;
    hudp    = winner->hudp;
    hudsz   = winner->hudsz;
    mlistp  = winner->mlistp;
    mlistsz = winner->mlistsz;

#ifdef USE_TILE
    // libgui may redefine these based on its own settings.
    gui_init_view_params(*this);
#endif

    init_view();
    return;
}

