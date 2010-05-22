#include "AppHdr.h"

#ifdef USE_TILE
#ifdef USE_CTOUCH

#include "defines.h"
#include "files.h"
#include "format.h"
#include "fontwrapper-ctouch.h"
#include "glwrapper.h"
#include "tilebuf.h"
#include "tilefont.h"

FontWrapper* FontWrapper::create()
{
    return (new CTFontWrapper());
}

CTFontWrapper::CTFontWrapper()
{
    m_buf = GLShapeBuffer::create(true, true);
}

CTFontWrapper::~CTFontWrapper()
{
    delete m_buf;
}

bool CTFontWrapper::load_font(const char *font_name, unsigned int font_size,
                              bool outl)
{
    return (true);
}

void CTFontWrapper::render_textblock(unsigned int x_pos, unsigned int y_pos,
                                     unsigned char *chars,
                                     unsigned char *colours,
                                     unsigned int width, unsigned int height,
                                     bool drop_shadow)
{
    if (!chars || !colours || !width || !height)
        return;

    GLState state;
    state.array_vertex = true;
    state.array_texcoord = true;
    state.array_colour = true;
    state.blend = true;
    state.texture = true;

    // Bind the texture we're going to draw from
    m_tex.bind();

    GLW_3VF trans(x_pos, y_pos, 0.0f);
    GLW_3VF scale(1, 1, 1);

    if (drop_shadow)
    {
        GLState state_shadow;
        state_shadow.array_colour = false;
        state_shadow.colour = VColour::black;

        GLW_3VF trans_shadow(trans.x + 1, trans.y + 1, 0.0f);
        glmanager->set_transform(trans_shadow, scale);

        m_buf->draw(state_shadow);
    }

    glmanager->set_transform(trans, scale);
    m_buf->draw(state);
}

static void _draw_box(int x_pos, int y_pos, float width, float height,
                      float box_width, unsigned char box_colour,
                      unsigned char box_alpha)
{
    std::auto_ptr<GLShapeBuffer> buf(GLShapeBuffer::create(false, true));
    GLWPrim rect(x_pos - box_width, y_pos - box_width,
                 x_pos + width + box_width, y_pos + height + box_width);

    VColour colour(term_colours[box_colour].r,
                   term_colours[box_colour].g,
                   term_colours[box_colour].b,
                   box_alpha);
    rect.set_col(colour);

    buf->add(rect);

    // Load identity matrix
    glmanager->reset_transform();

    GLState state;
    state.array_vertex = true;
    state.array_colour = true;
    state.blend = true;
    buf->draw(state);
}

unsigned int CTFontWrapper::string_height(const formatted_string &str) const
{
    std::string temp = str.tostring();
    return string_height(temp.c_str());
}

unsigned int CTFontWrapper::string_height(const char *text) const
{
    int height = 1;
    for (const char *itr = text; (*itr); itr++)
        if (*itr == '\n')
            height++;

    return char_height() * height;
}

unsigned int CTFontWrapper::string_width(const formatted_string &str) const
{
    std::string temp = str.tostring();
    return string_width(temp.c_str());
}

unsigned int CTFontWrapper::string_width(const char *text) const
{
    unsigned int max_width = 0;

    return (max_width);
}

int CTFontWrapper::find_index_before_width(const char *text, int max_width)
{
    return (-1);
}

formatted_string CTFontWrapper::split(const formatted_string &str,
                                      unsigned int max_width,
                                      unsigned int max_height)
{
    formatted_string ret;
    ret += str;

    return (ret);
}

void CTFontWrapper::render_string(unsigned int px, unsigned int py,
                                  const char *text,
                                  const coord_def &min_pos,
                                  const coord_def &max_pos,
                                  unsigned char font_colour, bool drop_shadow,
                                  unsigned char box_alpha,
                                  unsigned char box_colour,
                                  unsigned int outline,
                                  bool tooltip)
{
    ASSERT(text);

    // Determine extent of this text
    unsigned int max_rows = 1;
    unsigned int cols = 0;
    unsigned int max_cols = 0;
    for (const char *itr = text; *itr; itr++)
    {
        cols++;
        max_cols = std::max(cols, max_cols);

        // NOTE: only newlines should be used for tool tips.  Don't use EOL.
        ASSERT(*itr != '\r');

        if (*itr == '\n')
        {
            cols = 0;
            max_rows++;
        }
    }

    // Create the text block
    unsigned char *chars = (unsigned char *)malloc(max_rows * max_cols);
    unsigned char *colours = (unsigned char *)malloc(max_rows * max_cols);
    memset(chars, ' ', max_rows * max_cols);
    memset(colours, font_colour, max_rows * max_cols);

    // Fill the text block

    // Find a suitable location on screen
    const int buffer = 5;  // additional buffer size from edges

    int wx = string_width(text);
    int wy = max_rows * char_height();

    int sx, sy; // box starting location, uses extra buffer
    int tx, ty; // text starting location

    tx = px - wx / 2;
    sx = tx - buffer;
    if (tooltip)
    {
        sy = py + outline;
        ty = sy + buffer;
    }
    else
    {
        ty = py - wy - outline;
        sy = ty - buffer;
    }
    // box ending position
    int ex = tx + wx + buffer;
    int ey = ty + wy + buffer;

    if (ex > max_pos.x)
        tx += max_pos.x - ex;
    else if (sx < min_pos.x)
        tx -= sx - min_pos.x;

    if (ey > max_pos.y)
        ty += max_pos.y - ey;
    else if (sy < min_pos.y)
        ty -= sy - min_pos.y;

    if (box_alpha != 0)
        _draw_box(tx, ty, wx, wy, outline, box_colour, box_alpha);

    render_textblock(tx, ty, chars, colours, max_cols, max_rows, drop_shadow);
}

void CTFontWrapper::store(FontBuffer &buf, float &x, float &y,
                          const std::string &str, const VColour &col)
{
    store(buf, x, y, str, col, x);
}

void CTFontWrapper::store(FontBuffer &buf, float &x, float &y,
                          const std::string &str, const VColour &col,
                          float orig_x)
{
    // TODO: Redo without knowing max_advance
}

void CTFontWrapper::store(FontBuffer &buf, float &x, float &y,
                          const formatted_string &fs)
{
    store(buf, x, y, fs, x);
}

void CTFontWrapper::store(FontBuffer &buf, float &x, float &y,
                          const formatted_string &fs, float orig_x)
{
    int colour = LIGHTGREY;
    for (unsigned int i = 0; i < fs.ops.size(); i++)
    {
        switch (fs.ops[i].type)
        {
            case FSOP_COLOUR:
                // Only foreground colors for now...
                colour = fs.ops[i].x & 0xF;
                break;
            case FSOP_TEXT:
                store(buf, x, y, fs.ops[i].text, term_colours[colour], orig_x);
                break;
            default:
                break;
        }
    }
}

void CTFontWrapper::store(FontBuffer &buf, float &x, float &y,
                          unsigned char c, const VColour &col)
{
    // TODO: Redo without knowing about glyphs
}

unsigned int CTFontWrapper::char_width() const
{
    return (12);
}

unsigned int CTFontWrapper::char_height() const
{
    return (12);
}

const GenericTexture *CTFontWrapper::font_tex() const
{
    return (&m_tex);
}

#endif // USE_FT
#endif // USE_TILE
