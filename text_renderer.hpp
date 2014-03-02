#ifndef VOGL_TEXT_RENDERER_HPP
#define VOGL_TEXT_RENDERER_HPP

#include <string>
#include <map>

#include <OpenGL/gl3.h>
#include <ft2build.h>
#include FT_FREETYPE_H


namespace vogl {

struct Color {
    Color(float r, float g, float b, float a = 1.f) {
        rgba[0] = r;
        rgba[1] = g;
        rgba[2] = b;
        rgba[3] = a;
    }

    float rgba[4];
};

/**
 * A class for drawing text.
 */
class TextRenderer {
public:
    /**
     * Constructs a tet renderer for a given font size.
     *
     * @param fontPath  path to the TTF font to use
     * @param height  height of the font, in pixels
     */
    TextRenderer(const std::string &fontPath, unsigned int height = 20);

    ~TextRenderer();

    /// Binds OpenGL objects for rendering text.
    void bind();

    /// Unbinds OpenGL objects bound by #bind()
    void unbind();

    /**
     * Set the scales that translate pixels to normalized device coordinates.
     *
     * @param sx  1 pixel width, in normalized device coordinates
     * @param sy  1 pixel height, in normalized device coordinates
     */
    void setScale(float sx, float sy);

    /**
     * Draws text at a given position.
     *
     * @param text  the text to draw
     * @param x  x-coordinate for text, in normalized device coordinates
     * @param y  y-coordinate for text, in normalized device coordinates
     * @param color  RGBA color for the text
     */
    void drawText(const std::string &text, float x, float y,
                  const Color &color) const;

private:
    // Non-copyable
   TextRenderer(const TextRenderer &) {}
   TextRenderer & operator=(const TextRenderer &) { return (*this); }

protected:
    struct GlyphData {
        int x;
        int y;
        int width;
        int height;

        long advanceX;
        long advanceY;

        float textureLeft;
        float textureRight;
        float textureTop;
        float textureBottom;
    };

    /// Creates a texture atlas for the current face.
    void createAtlas();

private:
    typedef std::map<unsigned int, GlyphData> GlyphMap;

    GLuint texture_;
    GLuint sampler_;
    GLuint vbo_;
    GLuint vao_;

    GLuint vertexShader_;
    GLuint fragmentShader_;
    GLuint program_;
    GLuint colorUniform_;
    GLuint textureUniform_;

    FT_Face face_;
    FT_Library ft_;
    GlyphMap glyphs_;

    unsigned int height_;
    float sx_;
    float sy_;
};

}

#endif