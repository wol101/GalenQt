/* routine to draw a stroke font character */
/* uses routine DrawLine(ix1,iy1,ix2,iy2) to draw character */

/* cwidth includes a reasonable space between characters, so don't add extra
 space between characters */

/* cheight is maximum cheight above baseline, but characters may descend some
 distance below this (max 3/7 or 0.43), so add c. 50% of cheight between
 lines to prevent overlap */

/* coordinate system assumes origin at bottom left (I think) */

#include "StrokeFont.h"

#include <QVector4D>
#include <QVector3D>

StrokeFont::StrokeFont()
{
    m_z = 0;
    m_r = 255;
    m_g = 255;
    m_b = 255;
    m_a = 255;
    n_lines = 0;
    max_lines = 10000;
    line_buffer_vertices = new float[max_lines * 6];
    line_buffer_vertex_colours = new unsigned char[max_lines * 8];

    initializeOpenGLFunctions();

#ifdef USE_RAW_OPENGL_STROKEFONT
    glGenBuffers(1, &m_LineBuffer);
    glGenBuffers(1, &m_LineColourBuffer);
#else
    // Generate VBOs and allocate buffers
    vertexBuf.create();
    colourBuf.create();

    // set the usage pattern expected
    vertexBuf.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    colourBuf.setUsagePattern(QOpenGLBuffer::DynamicDraw);

    // allocate vertex data to VBO 0
    vertexBuf.bind();
    vertexBuf.allocate(line_buffer_vertices, max_lines * 6 * sizeof(float));

    // allocate colour data to VBO 1
    colourBuf.bind();
    colourBuf.allocate(line_buffer_vertex_colours, max_lines * 8 * sizeof(unsigned char));
#endif
}

StrokeFont::~StrokeFont()
{
#ifdef USE_RAW_OPENGL_STROKEFONT
    glDeleteBuffers(1, &m_LineBuffer);
    glDeleteBuffers(1, &m_LineColourBuffer);
#else
    vertexBuf.destroy();
    colourBuf.destroy();
#endif
    delete line_buffer_vertices;
    delete line_buffer_vertex_colours;
}

void StrokeFont::drawLineGeometry(QOpenGLShaderProgram *program)
{
#ifdef USE_RAW_OPENGL_STROKEFONT
    int positionLocation = program->attributeLocation("vertexPosition");
    int sourceColourLocation = program->attributeLocation("vertexColour");

    glEnableVertexAttribArray(positionLocation);
    glEnableVertexAttribArray(sourceColourLocation);

    glBindBuffer(GL_ARRAY_BUFFER, m_LineBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * GetNumLines() * 6, GetLineBuffer(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, m_LineColourBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLubyte) * GetNumLines() * 8, GetLineBufferVertexColours(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_LineBuffer);
    glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, m_LineColourBuffer);
    glVertexAttribPointer(sourceColourLocation, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);

    glDrawArrays(GL_LINES, 0, GetNumLines() * 2); // it is the number of vertices to draw
    glDisableVertexAttribArray(positionLocation);
    glDisableVertexAttribArray(sourceColourLocation);
#else
    // Tell OpenGL which VBOs to use
    vertexBuf.bind();

    // copy over the vertex data
    vertexBuf.write(0, line_buffer_vertices, n_lines * 6 * sizeof(float));

    // Tell OpenGL programmable pipeline how to locate vertex position data
    int vertexLocation = program->attributeLocation("vertexPosition");
    program->enableAttributeArray(vertexLocation);
    program->setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, 0);

    // Tell OpenGL which VBOs to use
    colourBuf.bind();

    // copy over the vertex data
    colourBuf.write(0, line_buffer_vertex_colours, n_lines * 8 * sizeof(float));

    // Tell OpenGL programmable pipeline how to locate color data
    int vertexColour = program->attributeLocation("vertexColour");
    program->enableAttributeArray(vertexColour);
    program->setAttributeBuffer(vertexColour, GL_UNSIGNED_BYTE, 0, 4, 0);

    // draw the lines
    glDrawArrays(GL_LINES, 0, GetNumLines() * 2);

    // and release the resources
    program->disableAttributeArray(vertexLocation);
    program->disableAttributeArray(vertexColour);
    vertexBuf.release();
    colourBuf.release();
#endif
}

void StrokeFont::DrawLine(float ix1, float iy1, float iz1, float ix2, float iy2, float iz2)
{
    if (n_lines >= max_lines) return; // at some point I might want to dynamically increase max_lines

    float *fp = line_buffer_vertices + n_lines * 6;
    *fp = ix1; fp++;
    *fp = iy1; fp++;
    *fp = iz1; fp++;
    *fp = ix2; fp++;
    *fp = iy2; fp++;
    *fp = iz2;

    unsigned char *cp = line_buffer_vertex_colours + n_lines * 8;
    *cp = m_r; cp++;
    *cp = m_g; cp++;
    *cp = m_b; cp++;
    *cp = m_a; cp++;
    *cp = m_r; cp++;
    *cp = m_g; cp++;
    *cp = m_b; cp++;
    *cp = m_a;

    n_lines++;
}

void StrokeFont::StrokeString(
                  const char *string,    /* character string */
                  int length,            /* number of characters to draw */
                  float x,               /* x coordinate of bottom left of character */
                  float y,               /* y coordinate ... */
                  float cwidth,          /* cwidth of character cell */
                  float cheight,         /* cheight of character cell */
                  int xJustification,    /* 0 - left, 1 - centre, 2 - right */
                  int yJustification,    /* 0 - bottom, 1 - centre, 2 - top */
                  int orient             /* if non-zero rotate everything 90 degrees AC */)
{
    float width = (float)length * cwidth;
    float xOrigin = x;
    float yOrigin = y;
    int i;

    if (orient == 0)
    {
        /* set the origin depending on justifictaion */
        switch (xJustification)
        {
            case 0: /* left */
                xOrigin = x;
                break;

            case 1: /* centre */
                xOrigin = x - (width / 2);
                break;

            case 2: /* right */
                xOrigin = x - width;
                break;

            default:
                break;
        }

        switch (yJustification)
        {
            case 0: /* bottom */
                yOrigin = y;
                break;

            case 1: /* centre */
                yOrigin = y - (cheight / 2);
                break;

            case 2: /* right */
                yOrigin = y - cheight;
                break;

            default:
                break;
        }

        /* loop over the characters */

        for (i = 0; i < length; i++)
        {
            StrokeCharacter((int)string[i], xOrigin, yOrigin, cwidth, cheight, orient);
            xOrigin += cwidth;
        }
    }
    else
    {
        /* set the origin depending on justifictaion */
        switch (xJustification)
        {
            case 0: /* left */
                xOrigin = x;
                break;

            case 1: /* centre */
                xOrigin = x - (cheight / 2);
                break;

            case 2: /* right */
                xOrigin = x - cheight;
                break;

            default:
                break;
        }

        switch (yJustification)
        {
            case 0: /* bottom */
                yOrigin = y;
                break;

            case 1: /* centre */
                yOrigin = y - (width / 2);
                break;

            case 2: /* right */
                yOrigin = y - width;
                break;

            default:
                break;
        }

        /* loop over the characters */

        for (i = 0; i < length; i++)
        {
            StrokeCharacter((int)string[i], xOrigin, yOrigin, cwidth, cheight, orient);
            yOrigin += cwidth;
        }
    }
}

void StrokeFont::StrokeCharacter(
                     int ichar,             /* character code */
                     float x,              /* x coordinate of bottom left of character */
                     float y,              /* y coordinate ... */
                     float cwidth,         /* cwidth of character cell */
                     float cheight,        /* cheight of character cell */
                     int orient             /* if non-zero rotate everything 90 degrees AC */)
{
    int draw;               /* draw flag */
    int istart,iend;        /* character start and end index */
    int istr;               /* index pointer */
    int istrok;             /* stroke value */
    int ix2,iy2;            /* integer stroke components */
    float x2,y2;           /* scaled line ends */
    float x1,y1;           /* scaled line starts */

    /* data for stroke font */

    static unsigned char stroke[706]=
    {

        0x80,
        0x20,0x21,0x80,0x23,0x26,
        0x24,0x26,0x80,0x54,0x56,
        0x20,0x26,0x80,0x40,0x46,0x80,0x04,0x64,0x80,0x02,0x62,
        0x2f,0x27,0x80,0x01,0x10,0x30,0x41,0x42,0x33,0x13,0x04,0x05,0x16,0x36,0x45,
        0x11,0x55,0x80,0x14,0x15,0x25,0x24,0x14,0x80,0x41,0x51,0x52,0x42,0x41,
        0x50,0x14,0x15,0x26,0x36,0x45,0x44,0x11,0x10,0x30,0x52,
        0x34,0x36,
        0x4e,0x11,0x14,0x47,
        0x0e,0x31,0x34,0x07,
        0x30,0x36,0x80,0x11,0x55,0x80,0x15,0x51,0x80,0x03,0x63,
        0x30,0x36,0x80,0x03,0x63,
        0x11,0x20,0x2f,0x0d,
        0x03,0x63,
        0x00,0x01,0x11,0x10,0x00,
        0x00,0x01,0x45,0x46,
        0x01,0x05,0x16,0x36,0x45,0x41,0x30,0x10,0x01,
        0x04,0x26,0x20,0x80,0x00,0x40,
        0x05,0x16,0x36,0x45,0x44,0x00,0x40,0x41,
        0x05,0x16,0x36,0x45,0x44,0x33,0x42,0x41,0x30,0x10,
        0x01,0x80,0x13,0x33,
        0x06,0x03,0x43,0x80,0x20,0x26,
        0x01,0x10,0x30,0x41,0x42,0x33,0x03,0x06,0x46,
        0x02,0x13,0x33,0x42,0x41,0x30,0x10,0x01,0x05,0x16,0x36,0x45,
        0x06,0x46,0x44,0x00,
        0x01,0x02,0x13,0x04,0x05,0x16,0x36,0x45,0x44,0x33,0x42,0x41,0x30,0x10,0x01,0x80,0x13,0x33,
        0x01,0x10,0x30,0x41,0x45,0x36,0x16,0x05,0x04,0x13,0x33,0x44,
        0x15,0x25,0x24,0x14,0x15,0x80,0x12,0x22,0x21,0x11,0x12,
        0x15,0x25,0x24,0x14,0x15,0x80,0x21,0x11,0x12,0x22,0x20,0x1f,
        0x30,0x03,0x36,
        0x02,0x42,0x80,0x04,0x44,
        0x10,0x43,0x16,
        0x06,0x17,0x37,0x46,0x45,0x34,0x24,0x22,0x80,0x21,0x20,
        0x50,0x10,0x01,0x06,0x17,0x57,0x66,0x63,0x52,0x32,0x23,0x24,0x35,0x55,0x64,
        0x00,0x04,0x26,0x44,0x40,0x80,0x03,0x43,
        0x00,0x06,0x36,0x45,0x44,0x33,0x42,0x41,0x30,0x00,0x80,0x03,0x33,
        0x45,0x36,0x16,0x05,0x01,0x10,0x30,0x41,
        0x00,0x06,0x36,0x45,0x41,0x30,0x00,
        0x40,0x00,0x06,0x46,0x80,0x03,0x23,
        0x00,0x06,0x46,0x80,0x03,0x23,
        0x45,0x36,0x16,0x05,0x01,0x10,0x30,0x41,0x43,0x23,
        0x00,0x06,0x80,0x03,0x43,0x80,0x40,0x46,
        0x10,0x30,0x80,0x20,0x26,0x80,0x16,0x36,
        0x01,0x10,0x30,0x41,0x46,
        0x00,0x06,0x80,0x02,0x46,0x80,0x13,0x40,
        0x40,0x00,0x06,
        0x00,0x06,0x24,0x46,0x40,
        0x00,0x06,0x80,0x05,0x41,0x80,0x40,0x46,
        0x01,0x05,0x16,0x36,0x45,0x41,0x30,0x10,0x01,
        0x00,0x06,0x36,0x45,0x44,0x33,0x03,
        0x12,0x30,0x10,0x01,0x05,0x16,0x36,0x45,0x41,0x30,
        0x00,0x06,0x36,0x45,0x44,0x33,0x03,0x80,0x13,0x40,
        0x01,0x10,0x30,0x41,0x42,0x33,0x13,0x04,0x05,0x16,0x36,0x45,
        0x06,0x46,0x80,0x20,0x26,
        0x06,0x01,0x10,0x30,0x41,0x46,
        0x06,0x02,0x20,0x42,0x46,
        0x06,0x00,0x22,0x40,0x46,
        0x00,0x01,0x45,0x46,0x80,0x40,0x41,0x05,0x06,
        0x06,0x24,0x20,0x80,0x24,0x46,
        0x06,0x46,0x45,0x01,0x00,0x40,
        0x37,0x17,0x1f,0x3f,
        0x06,0x05,0x41,0x40,
        0x17,0x37,0x3f,0x1f,
        0x04,0x26,0x44,
        0x0f,0x7f,
        0x54,0x36,
        0x40,0x43,0x34,0x14,0x03,0x01,0x10,0x30,0x41,
        0x06,0x01,0x10,0x30,0x41,0x43,0x34,0x14,0x03,
        0x41,0x30,0x10,0x01,0x03,0x14,0x34,0x43,
        0x46,0x41,0x30,0x10,0x01,0x03,0x14,0x34,0x43,
        0x41,0x30,0x10,0x01,0x03,0x14,0x34,0x43,0x42,0x02,
        0x20,0x25,0x36,0x46,0x55,0x80,0x03,0x43,
        0x41,0x30,0x10,0x01,0x03,0x14,0x34,0x43,0x4f,0x3e,0x1e,0x0f,
        0x00,0x06,0x80,0x03,0x14,0x34,0x43,0x40,
        0x20,0x23,0x80,0x25,0x26,
        0x46,0x45,0x80,0x43,0x4f,0x3e,0x1e,0x0f,
        0x00,0x06,0x80,0x01,0x34,0x80,0x12,0x30,
        0x20,0x26,
        0x00,0x04,0x80,0x03,0x14,0x23,0x34,0x43,0x40,
        0x00,0x04,0x80,0x03,0x14,0x34,0x43,0x40,
        0x01,0x03,0x14,0x34,0x43,0x41,0x30,0x10,0x01,
        0x04,0x0e,0x80,0x01,0x10,0x30,0x41,0x43,0x34,0x14,0x03,
        0x41,0x30,0x10,0x01,0x03,0x14,0x34,0x43,0x80,0x44,0x4e,
        0x00,0x04,0x80,0x03,0x14,0x44,
        0x01,0x10,0x30,0x41,0x32,0x12,0x03,0x14,0x34,0x43,
        0x04,0x44,0x80,0x26,0x21,0x30,0x40,0x51,
        0x04,0x01,0x10,0x30,0x41,0x80,0x44,0x40,
        0x04,0x02,0x20,0x42,0x44,
        0x04,0x00,0x22,0x40,0x44,
        0x00,0x44,0x80,0x04,0x40,
        0x04,0x01,0x10,0x30,0x41,0x80,0x44,0x4f,0x3e,0x1e,0x0f,
        0x04,0x44,0x00,0x40,
        0x40,0x11,0x32,0x03,0x34,0x15,0x46,
        0x20,0x23,0x80,0x25,0x27,
        0x00,0x31,0x12,0x43,0x14,0x35,0x06,
        0x06,0x27,0x46,0x67,
        0x07,0x77
    };

    /* index to stroke font */

    static int index[97]=
    {

        1,2,7,12,23,38,52,63,65,69,73,84,89,93,95,100,104,
        113,119,127,141,147,156,168,172,190,202,213,225,228,233,236,
        247,262,270,283,291,298,305,311,321,329,337,342,350,353,358,
        366,375,382,392,402,414,419,425,430,435,444,450,456,460,464,
        468,471,473,475,484,493,501,510,520,528,540,548,553,561,569,
        571,580,588,597,608,619,625,635,643,651,656,661,666,677,681,
        688,693,700,704,706
    };


    /* test range */

    if (ichar<32  || ichar >127) ichar = 128; /* set to dummy character */

    /* set initial values */

    draw=0;
    istart=index[ichar-32]-1;
    iend=index[ichar-31]-2;

    /* loop round strokes */

    for (istr=istart;istr<=iend;istr++)
    {
        istrok=(int)stroke[istr];

        /* test for move */

        if (istrok==0x80)
        {
            draw=0;
            continue;
        }

        /* normal vector */

        if (orient == 0)
        {
            ix2=istrok/16;
            x2=(float)ix2*cwidth/7.0f+x;
            iy2=istrok%16;
            if (iy2>7) iy2=(iy2%8)-8;
            y2=(float)iy2*cheight/7.0f+y;
        }
        else
        {
            ix2=istrok%16;
            if (ix2>7) ix2=(ix2%8)-8;
            ix2 = -ix2;
            x2=(float)ix2*cheight/7.0f+x;
            iy2=istrok/16;
            y2=(float)iy2*cwidth/7.0f+y;
        }

        /* draw vector if needed */

        if (draw!=0) DrawLine(x1, y1, m_z, x2, y2, m_z);

        /* set ix1,iy1 */

        x1=x2;
        y1=y2;
        draw=1;
    }
}

void StrokeFont::StrokeMarker(
                         MarkerCode code,       /* marker code */
                         float x,               /* x coordinate of centre of marker */
                         float y,               /* y coordinate ... */
                         float cwidth,          /* cwidth of character cell */
                         float cheight          /* cheight of character cell */)
{
    float x1, y1, x2, y2;
    switch (code)
    {
    case XShape:
        x1 = x - cwidth / 2;
        x2 = x + cwidth / 2;
        y1 = y - cheight / 2;
        y2 = y + cheight / 2;
        DrawLine(x1, y1, m_z, x2, y2, m_z);
        DrawLine(x1, y2, m_z, x2, y1, m_z);
        break;
    }
}

