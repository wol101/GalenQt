#ifndef STROKEFONT_H
#define STROKEFONT_H

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>

class StrokeFont : protected QOpenGLFunctions
{
public:
    StrokeFont();
    ~StrokeFont();

    enum MarkerCode { XShape };

    void StrokeString(
                      const char *string,    /* character string */
                      int length,            /* number of characters to draw */
                      float x,               /* x coordinate of bottom left of character */
                      float y,               /* y coordinate ... */
                      float cwidth,          /* cwidth of character cell */
                      float cheight,         /* cheight of character cell */
                      int xJustification,    /* 0 - left, 1 - centre, 2 - right */
                      int yJustification,    /* 0 - bottom, 1 - centre, 2 - top */
                      int orient             /* if non-zero rotate everything 90 degrees AC */);

    void StrokeCharacter(
                         int ichar,             /* character code */
                         float x,               /* x coordinate of bottom left of character */
                         float y,               /* y coordinate ... */
                         float cwidth,          /* cwidth of character cell */
                         float cheight,         /* cheight of character cell */
                         int orient             /* if non-zero rotate everything 90 degrees AC */);

    void StrokeMarker(
                             MarkerCode code,       /* marker code */
                             float x,               /* x coordinate of centre of marker */
                             float y,               /* y coordinate ... */
                             float cwidth,          /* cwidth of character cell */
                             float cheight          /* cheight of character cell */);

    void DrawLine(float ix1, float iy1, float iz1, float ix2, float iy2, float iz2);

    void SetZ(float z) { m_z = z; }
    void SetRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) { m_r = r; m_g = g; m_b = b; m_a = a; }

    void ZeroLineBuffer() { n_lines = 0; }
    int GetNumLines() { return n_lines; }
    float *GetLineBuffer() { return line_buffer_vertices; }
    unsigned char *GetLineBufferVertexColours() { return line_buffer_vertex_colours; }

    void drawLineGeometry(QOpenGLShaderProgram *program);

protected:
    float m_z;
    unsigned char m_r;
    unsigned char m_g;
    unsigned char m_b;
    unsigned char m_a;

    int n_lines;
    int max_lines;
    float *line_buffer_vertices;
    unsigned char *line_buffer_vertex_colours;

#ifdef USE_RAW_OPENGL_STROKEFONT
    GLuint m_LineBuffer;
    GLuint m_LineColourBuffer;
#else
    QOpenGLBuffer vertexBuf;
    QOpenGLBuffer colourBuf;
#endif
};

#endif // STROKEFONT_H
