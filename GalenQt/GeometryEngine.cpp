#include "GeometryEngine.h"

#include <QVector2D>
#include <QVector3D>

struct VertexData
{
    QVector3D position;
    QVector2D texCoord;
};

GeometryEngine::GeometryEngine(bool invertV)
    : indexBuf(QOpenGLBuffer::IndexBuffer)
{
    initializeOpenGLFunctions();

    // Generate 2 VBOs
    arrayBuf.create();
    indexBuf.create();

    // Initializes quad geometry and transfers it to VBOs
    initQuadGeometry(invertV);
}

GeometryEngine::~GeometryEngine()
{
    arrayBuf.destroy();
    indexBuf.destroy();
}

void GeometryEngine::initQuadGeometry(bool invertV)
{
    // For quad we need only 4 vertices
    VertexData vertices[] = {
        // Vertex data for face 0
        {QVector3D(0.0f, 0.0f,  0.0f), QVector2D(0.0f, 0.0f)},  // v0
        {QVector3D(1.0f, 0.0f,  0.0f), QVector2D(1.0f, 0.0f)}, // v1
        {QVector3D(0.0f, 1.0f,  0.0f), QVector2D(0.0f, 1.0f)},  // v2
        {QVector3D(1.0f, 1.0f,  0.0f), QVector2D(1.0f, 1.0f)}, // v3
    };
    if (invertV)
    {
        vertices[0].texCoord.setY(1.0f);
        vertices[1].texCoord.setY(1.0f);
        vertices[2].texCoord.setY(0.0f);
        vertices[3].texCoord.setY(0.0f);
    }

    // Indices for drawing quad faces using triangle strips.
    // Triangle strips can be connected by duplicating indices
    // between the strips. If connecting strips have opposite
    // vertex order then last index of the first strip and first
    // index of the second strip needs to be duplicated. If
    // connecting strips have same vertex order then only last
    // index of the first strip needs to be duplicated.
    GLushort indices[] = {
         0,  1,  2,  3,  3     // Face 0 - triangle strip ( v0,  v1,  v2,  v3)
    };

    // Transfer vertex data to VBO 0
    arrayBuf.bind();
    arrayBuf.allocate(vertices, 24 * sizeof(VertexData));

    // Transfer index data to VBO 1
    indexBuf.bind();
    indexBuf.allocate(indices, 34 * sizeof(GLushort));
}

void GeometryEngine::drawQuadGeometry(QOpenGLShaderProgram *program)
{
    // Tell OpenGL which VBOs to use
    arrayBuf.bind();
    indexBuf.bind();

    // Offset for position
    quintptr offset = 0;

    // Tell OpenGL programmable pipeline how to locate vertex position data
    int vertexLocation = program->attributeLocation("vertexPosition");
    program->enableAttributeArray(vertexLocation);
    program->setAttributeBuffer(vertexLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    // Offset for texture coordinate
    offset += sizeof(QVector3D);

    // Tell OpenGL programmable pipeline how to locate vertex texture coordinate data
    int texcoordLocation = program->attributeLocation("vertexUV");
    program->enableAttributeArray(texcoordLocation);
    program->setAttributeBuffer(texcoordLocation, GL_FLOAT, offset, 2, sizeof(VertexData));

    // Draw quad geometry using indices from VBO 1
    glDrawElements(GL_TRIANGLE_STRIP, 34, GL_UNSIGNED_SHORT, 0);

    program->disableAttributeArray(vertexLocation);
    program->disableAttributeArray(texcoordLocation);
}

