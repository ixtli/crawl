#include "debug.h"

#include "glwrapper.h"

#ifdef USE_TILE

#ifdef USE_SDL
#include <SDL_opengl.h>
#endif

/////////////////////////////////////////////////////////////////////////////
// GLState

// Note: these defaults should match the OpenGL defaults
GLState::GLState() :
    array_vertex(false),
    array_texcoord(false),
    array_colour(false),
    blend(false),
    texture(false),
    depthtest(false),
    alphatest(false),
    alpharef(0)
{
}

/////////////////////////////////////////////////////////////////////////////
// GLStateManager

void GLStateManager::init()
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0, 0.0, 0.0, 1.0f);
    glDepthFunc(GL_LEQUAL);
}

void GLStateManager::set(const GLState& state)
{
    if (state.array_vertex)
        glEnableClientState(GL_VERTEX_ARRAY);
    else
        glDisableClientState(GL_VERTEX_ARRAY);

    if (state.array_texcoord)
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    else
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    if (state.array_colour)
    {
        glEnableClientState(GL_COLOR_ARRAY);
    }
    else
    {
        glDisableClientState(GL_COLOR_ARRAY);

        // [enne] This should *not* be necessary, but the Linux OpenGL
        // driver that I'm using sets this to the last colour of the
        // colour array.  So, we need to unset it here.
        glColor3f(1.0f, 1.0f, 1.0f);
    }

    if (state.texture)
        glEnable(GL_TEXTURE_2D);
    else
        glDisable(GL_TEXTURE_2D);

    if (state.blend)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);

    if (state.depthtest)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    if (state.alphatest)
    {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_NOTEQUAL, state.alpharef);
    }
    else
        glDisable(GL_ALPHA_TEST);
}

void GLStateManager::drawQuadPTVert( long unsigned int size, size_t count,
    const void *vert_pointer, const void *tex_pointer)
{
    ASSERT(_valid(count, GLW_QUADS));
    glVertexPointer(2, GL_FLOAT, size, vert_pointer);
    glTexCoordPointer(2, GL_FLOAT, size, tex_pointer);
    glDrawArrays(GL_QUADS, 0, count);
}

void GLStateManager::drawQuadPCVert( long unsigned int size, size_t count,
    const void *vert_pointer, const void *color_pointer)
{
    ASSERT(_valid(count, GLW_QUADS));
    glVertexPointer(2, GL_FLOAT, size, vert_pointer);
    glColorPointer(4, GL_UNSIGNED_BYTE, size, color_pointer);
    glDrawArrays(GL_QUADS, 0, count);
}

void GLStateManager::drawLinePCVert( long unsigned int size, size_t count,
    const void *vert_pointer, const void *color_pointer)
{
    ASSERT(_valid(count, GLW_LINES));
    glVertexPointer(2, GL_FLOAT, size, vert_pointer);
    glColorPointer(4, GL_UNSIGNED_BYTE, size, color_pointer);
    glDrawArrays(GL_LINES, 0, count);
}

void GLStateManager::drawQuadPTCVert( long unsigned int size, size_t count,
    const void *vert_pointer, const void *tex_pointer,
    const void *color_pointer)
{
    ASSERT(_valid(count, GLW_QUADS));
    glVertexPointer(2, GL_FLOAT, size, vert_pointer);
    glTexCoordPointer(2, GL_FLOAT, size, tex_pointer);
    glColorPointer(4, GL_UNSIGNED_BYTE, size, color_pointer);
    glDrawArrays(GL_QUADS, 0, count);
}

void GLStateManager::drawQuadP3TCVert( long unsigned int size, size_t count,
    const void *vert_pointer, const void *tex_pointer,
    const void *color_pointer)
{
    ASSERT(_valid(count, GLW_QUADS));
    glVertexPointer(3, GL_FLOAT, size, vert_pointer);
    glTexCoordPointer(2, GL_FLOAT, size, tex_pointer);
    glColorPointer(4, GL_UNSIGNED_BYTE, size, color_pointer);
    glDrawArrays(GL_QUADS, 0, count);
}

void GLStateManager::pushMatrix()
{
    glPushMatrix();
}

void GLStateManager::popMatrix()
{
    glPopMatrix();
}

void GLStateManager::translatef(float x, float y, float z)
{
    glTranslatef(x, y ,z);
}

#ifdef DEBUG
bool GLStateManager::_valid(int num_verts, drawing_modes mode)
{
    switch( mode )
    {
        case GLW_QUADS:
        case GLW_TRIANGLE_STRIP:
        return (num_verts % 4 == 0);
        case GLW_TRIANGLES:
        return (num_verts % 3 == 0);
        case GLW_LINES:
        return (num_verts % 2 == 0);
        case GLW_POINTS:
        return (true);
        default:
        return (false);
    }
    
}
#endif

/////////////////////////////////////////////////////////////////////////////
// Static Methods

#endif
