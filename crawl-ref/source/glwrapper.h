#ifndef GL_WRAPPER_H
#define GL_WRAPPER_H

#include <stdlib.h> // For size_t
#include <vector> // for std::vector

#ifdef USE_TILE

struct coord_def;

struct GLW_3VF
{
    GLW_3VF() {};
    GLW_3VF(float l, float m, float n) : x(l), y(m), z(n) {}

    inline void set(float l, float m, float n)
    {
        x = l;
        y = m;
        z = n;
    }

    union {float x; float r;};
    union {float y; float g;};
    union {float z; float b;};
};

struct GLW_4VF
{
    GLW_4VF() {};
    GLW_4VF(float l, float m, float n, float p) : x(l), y(m), z(n), t(p) {}

    inline void set(float l, float m, float n, float p)
    {
        x = l;
        y = m;
        z = n;
        t = p;
    }

    union {float x; float r;};
    union {float y; float g;};
    union {float z; float b;};
    union {float t; float a;};
};

enum MipMapOptions
{
    MIPMAP_CREATE,
    MIPMAP_NONE,
    MIPMAP_MAX
};

// TODO: Ixtli - Remove QUADS entirely.
typedef enum
{
    GLW_POINTS,
    GLW_LINES,
    GLW_TRIANGLES,
    GLW_TRIANGLE_STRIP,
    GLW_QUADS
} drawing_modes;

struct GLPrimitive
{
    GLPrimitive(long unsigned int sz, size_t ct, unsigned int vs,
        const void* v_pt, const void *c_pt, const void *t_pt);

    // Primitive Metadata
    drawing_modes mode;
    unsigned int vert_size;  // Coords per vertex
    long unsigned int size;
    size_t count;

    // Primitive Data
    const void *vert_pointer;
    const void *colour_pointer;
    const void *texture_pointer;

    // Primitive render manipulations
    GLW_3VF *pretranslate;
    GLW_3VF *prescale;

    // State manipulations
    GLW_3VF *color;
};

struct GLState
{
    GLState();

    // vertex arrays
    bool array_vertex;
    bool array_texcoord;
    bool array_colour;

    // render state
    bool blend;
    bool texture;
    bool depthtest;
    bool alphatest;
    unsigned char alpharef;
};

class GLStateManager
{
public:
    // To silence pre 4.3 g++ compiler warnings
    virtual ~GLStateManager() {};

    static void init();
    static void shutdown();

    // State Manipulation
    virtual void set(const GLState& state) = 0;
    virtual void pixelstore_unpack_alignment(unsigned int bpp) = 0;
    virtual void reset_view_for_redraw(float x, float y) = 0;
    virtual void reset_view_for_resize(coord_def &m_windowsz) = 0;
    virtual void set_transform(  const GLW_3VF *trans = NULL,
                                const GLW_3VF *scale = NULL) = 0;
    virtual void reset_transform() = 0;
    virtual void set_current_color(GLW_3VF &color) = 0;
    virtual void set_current_color(GLW_4VF &color) = 0;

    // Drawing GLPrimitives
    virtual void draw_primitive(const GLPrimitive &prim) = 0;

    // Texture-specific functinos
    virtual void delete_textures(size_t count, unsigned int *textures) = 0;
    virtual void generate_textures(size_t count, unsigned int *textures) = 0;
    virtual void bind_texture(unsigned int texture) = 0;
    virtual void load_texture(unsigned char *pixels, unsigned int width,
        unsigned int height, MipMapOptions mip_opt) = 0;

    // Debug
#ifdef DEBUG
    static bool _valid(int num_verts, drawing_modes mode);
#endif
};

// Main interface for GL functions
extern GLStateManager *glmanager;

#endif // use_tile
#endif // include guard
