#include "text_renderer.hpp"

#include <iostream>
#include <vector>
#include <OpenGL/gl3ext.h>

//----------------------------------------------------------------------------

namespace {
    const char *VSHADER =
        "#version 410 core\n"
        ""
        "in vec4 position;\n"
        "out vec2 texCoords;\n"
        ""
        "void main(void) {\n"
        "    gl_Position = vec4(position.xy, 0, 1);\n"
        "    texCoords = position.zw;\n"
        "}\n";

    const char *FSHADER =
        "#version 410 core\n"
        "precision highp float;\n"
        ""
        "uniform sampler2D sampler;\n"
        "uniform vec4 color;\n"
        "in vec2 texCoords;\n"
        "out vec4 fragColor;\n"
        ""
        "void main(void) {\n"
        "    fragColor = vec4(1, 1, 1, texture(sampler, texCoords).r) * color;\n"
        "}\n";

    bool getShaderLog(GLuint shader, std::string &error) {
        char log[1024];
        GLsizei len = 0;
        glGetShaderInfoLog(shader, 1024, &len, log);
        error = std::string(log, log + len);
        return (len > 0);
    }
}

namespace vogl {

//----------------------------------------------------------------------------

TextRenderer::TextRenderer(const std::string &fontPath, unsigned int height)
    : height_(height)
{
    if(FT_Init_FreeType(&ft_) != 0) {
        std::cerr << "Unable to initialize FreeType library\n";
        return; // TODO throw exception
    }

    if(FT_New_Face(ft_, fontPath.c_str(), 0, &face_) != 0) {
        std::cerr << "Unlable to load font: " << fontPath << '\n';
        return; // TODO throw exception
    }

    // Initialize OpenGL objects
    std::string shaderLog;

    vertexShader_ = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader_, 1, &VSHADER, 0);
    glCompileShader(vertexShader_);
    if(getShaderLog(vertexShader_, shaderLog)) {
        std::cerr << "Unable to compile vertex shader: "  << shaderLog << '\n';
        return; // TODO throw exception
    }

    fragmentShader_ = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader_, 1, &FSHADER, 0);
    glCompileShader(fragmentShader_);
    if(getShaderLog(fragmentShader_, shaderLog)) {
        std::cerr << "Unable to compile fragment shader: "  << shaderLog << '\n';
        return; // TODO throw exception
    }

    program_ = glCreateProgram();
    glAttachShader(program_, vertexShader_);
    glAttachShader(program_, fragmentShader_);
    glBindAttribLocation(program_, 0, "position");
    glLinkProgram(program_);
    glUseProgram(program_);
    {
        colorUniform_ = glGetUniformLocation(program_, "color");
        textureUniform_ = glGetUniformLocation(program_, "sampler");
    }
    glUseProgram(0);

    glGenTextures(1, &texture_);
    glGenSamplers(1, &sampler_);
    glSamplerParameteri(sampler_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(sampler_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenBuffers(1, &vbo_);
    glGenVertexArrays(1, &vao_);
    createAtlas();
}

TextRenderer::~TextRenderer() {
    FT_Done_Face(face_);
    FT_Done_FreeType(ft_);
    glDeleteTextures(1, &texture_);
    glDeleteSamplers(1, &sampler_);
    glDeleteBuffers(1, &vbo_);
    glDeleteVertexArrays(1, &vao_);
    glDeleteProgram(program_);
    glDeleteShader(vertexShader_);
    glDeleteShader(fragmentShader_);
}

//----------------------------------------------------------------------------

void TextRenderer::createAtlas() {
    glyphs_.clear();

    if(face_) {
        FT_Set_Pixel_Sizes(face_, 0, height_);
        const FT_GlyphSlot g = face_->glyph;

        // First get the size of the atlas so we can initialize the texture
        int atlasWidth = 0, atlasHeight = 0;
        for(unsigned char charIndex = 32; charIndex < 128; charIndex++) {
            if(FT_Load_Char(face_, charIndex, FT_LOAD_RENDER) == 0) {
                atlasWidth += g->bitmap.width;
                atlasHeight = std::max(atlasHeight, static_cast<int>(g->bitmap.rows));
            } else {
                std::cerr << "Could not load character: " << charIndex << '\n';
                // TODO throw exception
            }
        }

        // Prepare the texture
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlasWidth, atlasHeight, 0,
                     GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer);

        // Initialize texture and glyph data
        const float invWidth = 1.0f / atlasWidth;
        const float invHeight = 1.0f / atlasHeight;
        int x = 0;
        for(unsigned int charIndex = 32; charIndex < 128; ++charIndex) {
            if(FT_Load_Char(face_, charIndex, FT_LOAD_RENDER) == 0) {
                // Set texture data
                glTexSubImage2D(GL_TEXTURE_2D, 0, x, 0, g->bitmap.width,
                                g->bitmap.rows, GL_RED, GL_UNSIGNED_BYTE,
                                g->bitmap.buffer);

                // Set glyph data
                glyphs_[charIndex] = {
                    g->bitmap_left, g->bitmap_top,
                    g->bitmap.width, g->bitmap.rows,
                    g->advance.x >> 6, g->advance.y >> 6,
                    x * invWidth, (x + g->bitmap.width)*invWidth,
                    g->bitmap.rows*invHeight, 0.0f,
                };

                x += g->bitmap.width;
            }
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    }
}

//----------------------------------------------------------------------------

void TextRenderer::setScale(float sx, float sy) {
    this->sx_ = sx;
    this->sy_ = sy;
}

//----------------------------------------------------------------------------

void TextRenderer::bind() {
    if(vao_) {
        glEnable(GL_BLEND);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_);
        glBindSampler(0, sampler_);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBindVertexArray(vao_);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

        glUseProgram(program_);
        glUniform4f(colorUniform_, 0, 0, 0, 1);
        glUniform1i(textureUniform_, 0);
    }
}

void TextRenderer::unbind() {
    if(vao_) {
        glDisable(GL_BLEND);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindSampler(0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glUseProgram(0);
    }
}

//----------------------------------------------------------------------------

void TextRenderer::drawText(const std::string &text, float x, float y,
                            const Color &color) const
{
    if(!vao_)
        return;

    glUniform4fv(colorUniform_, 1, color.rgba);

    struct GlyphVBOData {
        float x;
        float y;
        float s;
        float t;
    };
    std::vector<GlyphVBOData> vboData(text.length() * 6);

    size_t numVerts = 0;
    const float initialX = x;
    for(const auto p : text) {
        if(p == '\n') {
            x = initialX;
            y -= height_*sy_;
        } else {
            const auto glyphIter = glyphs_.find(static_cast<unsigned int>(p));
            if(glyphIter != glyphs_.end()) {
                const auto &glyph = glyphIter->second;
                const float x2 =  x + glyph.x * sx_;
                const float y2 =  y + glyph.y * sy_;
                const float w = glyph.width * sx_;
                const float h = glyph.height * sy_;
                const float tl = glyph.textureLeft;
                const float tr = glyph.textureRight;
                const float tt = glyph.textureTop;
                const float tb = glyph.textureBottom;

                vboData[numVerts++] = {x2    , y2    , tl, tb};
                vboData[numVerts++] = {x2    , y2 - h, tl, tt};
                vboData[numVerts++] = {x2 + w, y2    , tr, tb};

                vboData[numVerts++] = {x2 + w, y2    , tr, tb};
                vboData[numVerts++] = {x2    , y2 - h, tl, tt};
                vboData[numVerts++] = {x2 + w, y2 - h, tr, tt};

                x += glyph.advanceX * sx_;
                y += glyph.advanceY * sy_;
            }
        }
    }

    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLint>(vboData.size()*sizeof(GlyphVBOData)),
                 vboData.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLint>(numVerts));
}

//----------------------------------------------------------------------------

}