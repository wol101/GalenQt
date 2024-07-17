#include <algorithm>

#include <QDebug>
#include <QMouseEvent>
#include <QMessageBox>
#include <QApplication>
#include <QMenu>
#include <QAction>

#include "GraphicsView.h"
#include "GeometryEngine.h"
#include "StrokeFont.h"
#include "LabelledPoints.h"
#include "SingleChannelImage.h"
#include "Settings.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define CLAMP(n,lower,upper) (std::max(lower, std::min(n, upper)))

GraphicsView::GraphicsView(QWidget *parent) :
    QOpenGLWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); // this widget can shrink and expand and still be useful

    m_geometries = 0;
    m_strokeFont = 0;
    m_textureRed = 0;
    m_textureGreen = 0;
    m_textureBlue = 0;
    m_textureBlack = 0;
    m_textureMarker = 0;
    m_blackImage = 0;
    m_markerImage = 0;;
    m_updateRepeatCount = 0;
    m_textureUpdateRepeats = 1;
    m_drawRed = false;
    m_drawGreen = false;
    m_drawBlue = false;
    m_imageWidth = 1;
    m_imageHeight = 1;
    m_zoom = 1;
    m_nearPlane = 0;
    m_farPlane = 2;
    m_rotationAngle = 0;
    m_sizeHintScreenFraction = 0.8f;
    m_selectionColour = Settings::value("Selection Colour", QColor(0, 255, 0)).value<QColor>();
    m_markerSize = Settings::value("Marker Size", float(100)).toFloat();
    m_activeSelect = false;
    m_activePan = false;
    m_textureDisplayRed.minimum = 0; m_textureDisplayRed.maximum = 1; m_textureDisplayRed.gamma = 1; m_textureDisplayRed.zebra = 1;
    m_textureDisplayGreen.minimum = 0; m_textureDisplayGreen.maximum = 1; m_textureDisplayGreen.gamma = 1; m_textureDisplayGreen.zebra = 1;
    m_textureDisplayBlue.minimum = 0; m_textureDisplayBlue.maximum = 1; m_textureDisplayBlue.gamma = 1; m_textureDisplayBlue.zebra = 1;

    // these produce the context menu for the widget
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(menuRequest(const QPoint &)));

}

GraphicsView::~GraphicsView()
{
    makeCurrent();
    if (m_geometries) delete m_geometries;
    if (m_strokeFont) delete m_strokeFont;
    if (m_textureRed) delete m_textureRed;
    if (m_textureGreen) delete m_textureGreen;
    if (m_textureBlue) delete m_textureBlue;
    if (m_textureBlack) delete m_textureBlack;
    if (m_textureMarker) delete m_textureMarker;
    if (m_blackImage) delete m_blackImage;
    if (m_markerImage) delete m_markerImage;
    doneCurrent();
}

void GraphicsView::initializeGL()
{
    initializeOpenGLFunctions();

    QString versionString(QLatin1String(reinterpret_cast<const char*>(glGetString(GL_VERSION))));
    qDebug() << "Driver Version String:" << versionString;
    qDebug() << "Current Context:" << format();

    int openGLVersion = format().majorVersion() * 100 + format().minorVersion() * 10;
    if (openGLVersion < 330) // the current code really does not work with <3.3
    {
        QString errorMessage = QString("This application requires OpenGL 3.3 or greater.\nCurrent version is %1.\nApplication will abort.").arg(versionString);
        QMessageBox::critical(this, tr("CloudDigitiser"), errorMessage);
        exit(EXIT_FAILURE);
    }

    // Create a vertex array object. In OpenGL ES 2.0 and OpenGL 2.x
    // implementations this is optional and support may not be present
    // at all. Nonetheless the below code works in all cases and makes
    // sure there is a VAO when one is needed.
    m_vao.create();
    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao);

    QColor backgroundColour = palette().window().color();
    glClearColor(backgroundColour.redF(), backgroundColour.greenF(), backgroundColour.blueF(), backgroundColour.alphaF());

    initShaders();
    initTextures();

    bool invertV = true; // this allows the image to be displayed without mirroring
    m_geometries = new GeometryEngine(invertV);
    m_strokeFont = new StrokeFont;
}

void GraphicsView::initShaders()
{
    // Compile vertex shader
    if (!m_shaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/opengl/vertex_shader.glsl"))
        close();

    // Compile fragment shader
    if (!m_shaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/opengl/fragment_shader.glsl"))
        close();

    // Link shader pipeline
    if (!m_shaderProgram.link())
        close();

    // Bind shader pipeline for use
    if (!m_shaderProgram.bind())
        close();
}

void GraphicsView::initTextures()
{
    m_blackImage = new QImage(8, 8, QImage::Format_Grayscale8);
    m_blackImage->fill(0);
    if (m_textureBlack) delete m_textureBlack;
    m_textureBlack = new QOpenGLTexture(*m_blackImage, QOpenGLTexture::DontGenerateMipMaps);
    m_textureBlack->setMinificationFilter(QOpenGLTexture::Linear);
    m_textureBlack->setMagnificationFilter(QOpenGLTexture::Nearest);
    m_textureBlack->setWrapMode(QOpenGLTexture::ClampToEdge);
    if (m_textureRed) delete m_textureRed;
    m_textureRed = new QOpenGLTexture(*m_blackImage, QOpenGLTexture::DontGenerateMipMaps);
    m_textureRed->setMinificationFilter(QOpenGLTexture::Linear);
    m_textureRed->setMagnificationFilter(QOpenGLTexture::Nearest);
    m_textureRed->setWrapMode(QOpenGLTexture::ClampToEdge);
    if (m_textureGreen) delete m_textureGreen;
    m_textureGreen = new QOpenGLTexture(*m_blackImage, QOpenGLTexture::DontGenerateMipMaps);
    m_textureGreen->setMinificationFilter(QOpenGLTexture::Linear);
    m_textureGreen->setMagnificationFilter(QOpenGLTexture::Nearest);
    m_textureGreen->setWrapMode(QOpenGLTexture::ClampToEdge);
    if (m_textureBlue) delete m_textureBlue;
    m_textureBlue = new QOpenGLTexture(*m_blackImage, QOpenGLTexture::DontGenerateMipMaps);
    m_textureBlue->setMinificationFilter(QOpenGLTexture::Linear);
    m_textureBlue->setMagnificationFilter(QOpenGLTexture::Nearest);
    m_textureBlue->setWrapMode(QOpenGLTexture::ClampToEdge);

    m_markerImage = new QImage(":/images/cursor-cross.png");
    if (m_textureMarker) delete m_textureMarker;
    m_textureMarker = new QOpenGLTexture(*m_markerImage, QOpenGLTexture::DontGenerateMipMaps);
    m_textureMarker->setMinificationFilter(QOpenGLTexture::Linear);
    m_textureMarker->setMagnificationFilter(QOpenGLTexture::Nearest);
    m_textureMarker->setWrapMode(QOpenGLTexture::ClampToEdge);
}

float GraphicsView::zoom() const
{
    return m_zoom;
}

void GraphicsView::setZoom(float zoom)
{
    m_zoom = zoom;
}

int GraphicsView::imageHeight() const
{
    return m_imageHeight;
}

void GraphicsView::setImageHeight(int imageHeight)
{
    m_imageHeight = imageHeight;
}

int GraphicsView::imageWidth() const
{
    return m_imageWidth;
}

void GraphicsView::setImageWidth(int imageWidth)
{
    m_imageWidth = imageWidth;
}

bool GraphicsView::drawBlue() const
{
    return m_drawBlue;
}

void GraphicsView::setDrawBlue(bool drawBlue)
{
    m_drawBlue = drawBlue;
}

bool GraphicsView::drawGreen() const
{
    return m_drawGreen;
}

void GraphicsView::setDrawGreen(bool drawGreen)
{
    m_drawGreen = drawGreen;
}

bool GraphicsView::drawRed() const
{
    return m_drawRed;
}

void GraphicsView::setDrawRed(bool drawRed)
{
    m_drawRed = drawRed;
}

void GraphicsView::setTexture(float *image, int width, int height, ColourChannel colourChannel)
{
    int depth = 1;
    switch (colourChannel)
    {
    case Red:
        if (m_textureRed) delete m_textureRed;
        m_textureRed = new QOpenGLTexture(QOpenGLTexture::Target2D);
        m_textureRed->setSize(width, height, depth);
        m_textureRed->setFormat(QOpenGLTexture::R32F);
        m_textureRed->setAutoMipMapGenerationEnabled(false);
        m_textureRed->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::Float32);
        m_textureRed->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, image);
        m_textureRed->setMinificationFilter(QOpenGLTexture::Linear);
        m_textureRed->setMagnificationFilter(QOpenGLTexture::Nearest);
        m_textureRed->setWrapMode(QOpenGLTexture::ClampToEdge);
        setImageWidth();
        break;
    case Green:
        if (m_textureGreen) delete m_textureGreen;
        m_textureGreen = new QOpenGLTexture(QOpenGLTexture::Target2D);
        m_textureGreen->setSize(width, height, depth);
        m_textureGreen->setFormat(QOpenGLTexture::R32F);
        m_textureGreen->setAutoMipMapGenerationEnabled(false);
        m_textureGreen->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::Float32);
        m_textureGreen->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, image);
        m_textureGreen->setMinificationFilter(QOpenGLTexture::Linear);
        m_textureGreen->setMagnificationFilter(QOpenGLTexture::Nearest);
        m_textureGreen->setWrapMode(QOpenGLTexture::ClampToEdge);
        setImageWidth();
        break;
    case Blue:
        if (m_textureBlue) delete m_textureBlue;
        m_textureBlue = new QOpenGLTexture(QOpenGLTexture::Target2D);
        m_textureBlue->setSize(width, height, depth);
        m_textureBlue->setFormat(QOpenGLTexture::R32F);
        m_textureBlue->setAutoMipMapGenerationEnabled(false);
        m_textureBlue->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::Float32);
        m_textureBlue->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, image);
        m_textureBlue->setMinificationFilter(QOpenGLTexture::Linear);
        m_textureBlue->setMagnificationFilter(QOpenGLTexture::Nearest);
        m_textureBlue->setWrapMode(QOpenGLTexture::ClampToEdge);
        setImageWidth();
        break;
    case Black:
        qDebug("Error: ColourChannel = Black  not supported in GraphicsView::setTexture");
        break;
    }
    update();
}

void GraphicsView::setTextureDisplay(float minimum, float maximum, float gamma, float zebra, bool logFlag, ColourChannel colourChannel)
{
    switch (colourChannel)
    {
    case Red:
        m_textureDisplayRed.minimum = minimum;
        m_textureDisplayRed.maximum = maximum;
        m_textureDisplayRed.gamma = gamma;
        m_textureDisplayRed.zebra = zebra;
        m_textureDisplayRed.log = logFlag;
        break;
    case Green:
        m_textureDisplayGreen.minimum = minimum;
        m_textureDisplayGreen.maximum = maximum;
        m_textureDisplayGreen.gamma = gamma;
        m_textureDisplayGreen.zebra = zebra;
        m_textureDisplayGreen.log = logFlag;
        break;
    case Blue:
        m_textureDisplayBlue.minimum = minimum;
        m_textureDisplayBlue.maximum = maximum;
        m_textureDisplayBlue.gamma = gamma;
        m_textureDisplayBlue.zebra = zebra;
        m_textureDisplayBlue.log = logFlag;
        break;
    case Black:
        qDebug("Error: ColourChannel = Black  not supported in GraphicsView::setTextureDisplay");
        break;
    }
    update();
}

void GraphicsView::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
    // qDebug("resizeGL(%d, %d)", w, h);
}

void GraphicsView::paintGL()
{
    // this line is needed on the mac
    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao);

    // Clear color and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//    qDebug("m_textureDisplayRed.minimum=%g", m_textureDisplayRed.minimum);
//    qDebug("m_textureDisplayRed.maximum=%g", m_textureDisplayRed.maximum);
//    qDebug("m_textureDisplayGreen.minimum=%g", m_textureDisplayGreen.minimum);
//    qDebug("m_textureDisplayGreen.maximum=%g", m_textureDisplayGreen.maximum);
//    qDebug("m_textureDisplayBlue.minimum=%g", m_textureDisplayBlue.minimum);
//    qDebug("m_textureDisplayBlue.maximum=%g", m_textureDisplayBlue.maximum);

    m_shaderProgram.bind();

    m_shaderProgram.setUniformValue("hasTexture", true);

    if (m_drawRed)
    {
        m_shaderProgram.setUniformValue("redGamma", m_textureDisplayRed.gamma);
        m_shaderProgram.setUniformValue("redLog", m_textureDisplayRed.log);
        m_shaderProgram.setUniformValue("redMax", m_textureDisplayRed.maximum);
        m_shaderProgram.setUniformValue("redMin", m_textureDisplayRed.minimum);
        m_shaderProgram.setUniformValue("redZebra", m_textureDisplayRed.zebra);
    }
    else
    {
        m_shaderProgram.setUniformValue("redGamma", 1.0f);
        m_shaderProgram.setUniformValue("redLog", false);
        m_shaderProgram.setUniformValue("redMax", 1.0f);
        m_shaderProgram.setUniformValue("redMin", 0.0f);
        m_shaderProgram.setUniformValue("redZebra", 1.0f);
    }
    if (m_drawGreen)
    {
        m_shaderProgram.setUniformValue("greenGamma", m_textureDisplayGreen.gamma);
        m_shaderProgram.setUniformValue("greenLog", m_textureDisplayGreen.log);
        m_shaderProgram.setUniformValue("greenMax", m_textureDisplayGreen.maximum);
        m_shaderProgram.setUniformValue("greenMin", m_textureDisplayGreen.minimum);
        m_shaderProgram.setUniformValue("greenZebra", m_textureDisplayGreen.zebra);
    }
    else
    {
        m_shaderProgram.setUniformValue("greenGamma", 1.0f);
        m_shaderProgram.setUniformValue("greenLog", false);
        m_shaderProgram.setUniformValue("greenMax", 1.0f);
        m_shaderProgram.setUniformValue("greenMin", 0.0f);
        m_shaderProgram.setUniformValue("greenZebra", 1.0f);
    }
    if (m_drawBlue)
    {
        m_shaderProgram.setUniformValue("blueGamma", m_textureDisplayBlue.gamma);
        m_shaderProgram.setUniformValue("blueLog", m_textureDisplayBlue.log);
        m_shaderProgram.setUniformValue("blueMax", m_textureDisplayBlue.maximum);
        m_shaderProgram.setUniformValue("blueMin", m_textureDisplayBlue.minimum);
        m_shaderProgram.setUniformValue("blueZebra", m_textureDisplayBlue.zebra);
    }
    else
    {
        m_shaderProgram.setUniformValue("blueGamma", 1.0f);
        m_shaderProgram.setUniformValue("blueLog", false);
        m_shaderProgram.setUniformValue("blueMax", 1.0f);
        m_shaderProgram.setUniformValue("blueMin", 0.0f);
        m_shaderProgram.setUniformValue("blueZebra", 1.0f);
    }

    // Set the 3 textures
    glActiveTexture(GL_TEXTURE0);
    if (m_drawRed) m_textureRed->bind();
    else m_textureBlack->bind();
    m_shaderProgram.setUniformValue("textureSamplerRed", 0);
    glActiveTexture(GL_TEXTURE1);
    if (m_drawGreen) m_textureGreen->bind();
    else m_textureBlack->bind();
    m_shaderProgram.setUniformValue("textureSamplerGreen", 1);
    glActiveTexture(GL_TEXTURE2);
    if (m_drawBlue) m_textureBlue->bind();
    else m_textureBlack->bind();
    m_shaderProgram.setUniformValue("textureSamplerBlue", 2);

    // Calculate model view projection transformation
    QMatrix4x4 reverseRotationMatrix;
    reverseRotationMatrix.setToIdentity();
    reverseRotationMatrix.rotate(-m_rotationAngle, 0, 0, 1.0f);
    float viewPortWidth = float(width()) / m_zoom;
    float viewportHeight = float(height()) / m_zoom;
    m_nearPlane = 0;
    m_farPlane = 2;
    m_projection.setToIdentity();
    float halfViewportWidth = viewPortWidth / 2;
    float halfViewportHeight = viewportHeight / 2;
    m_projection.ortho(-halfViewportWidth, halfViewportWidth, -halfViewportHeight, halfViewportHeight, m_nearPlane, m_farPlane);

    m_eye = m_center + QVector3D(0, 0, 1);
    m_up = reverseRotationMatrix * QVector3D(0, 1, 0);
    m_view.setToIdentity();
    m_view.lookAt(m_eye, m_center, m_up);

    m_model.setToIdentity();
    m_model.scale(m_imageWidth, m_imageHeight, 1);

    m_mvpMatrix = m_projection * m_view * m_model;

#ifdef TEST_THE_MATRIX
    // test the matrix
    QVector3D input(0,0,0);
    QVector3D result = m_mvpMatrix * input;
    qDebug("(0,0,0) maps to (%g,%g,%g)", result.x(), result.y(), result.z());
    input = QVector3D(1,1,0);
    result = m_mvpMatrix * input;
    qDebug("(1,1,0) maps to (%g,%g,%g)", result.x(), result.y(), result.z());
#endif

    // Set modelview-m_projection matrix
    m_shaderProgram.setUniformValue("mvp_matrix", m_mvpMatrix);

    // Draw quad geometry
    m_geometries->drawQuadGeometry(&m_shaderProgram);

    // Draw any lines that are needed
    m_shaderProgram.setUniformValue("hasTexture", false);

    m_strokeFont->ZeroLineBuffer();
    m_strokeFont->SetZ(0.0);
    m_mvpMatrix = m_projection * m_view;
    m_shaderProgram.setUniformValue("mvp_matrix", m_mvpMatrix);

    if (m_startSelect != m_endSelect)
    {
        m_strokeFont->SetRGBA(m_selectionColour.red(), m_selectionColour.green(), m_selectionColour.blue(), m_selectionColour.alpha());
        m_strokeFont->DrawLine(m_startSelect.x(), m_startSelect.y(), 0.0f, m_endSelect.x(), m_startSelect.y(), 0.0f);
        m_strokeFont->DrawLine(m_endSelect.x(), m_startSelect.y(), 0.0f, m_endSelect.x(), m_endSelect.y(), 0.0f);
        m_strokeFont->DrawLine(m_endSelect.x(), m_endSelect.y(), 0.0f, m_startSelect.x(), m_endSelect.y(), 0.0f);
        m_strokeFont->DrawLine(m_startSelect.x(), m_endSelect.y(), 0.0f, m_startSelect.x(), m_startSelect.y(), 0.0f);
        m_strokeFont->drawLineGeometry(&m_shaderProgram);
    }

    for (int i = 0; i < m_labelledPoints.size(); i++)
    {
        m_strokeFont->ZeroLineBuffer();
        m_strokeFont->SetZ(0.0);
        QColor colour = m_labelledPoints[i]->colour();
        m_strokeFont->SetRGBA(colour.red(), colour.green(), colour.blue(), colour.alpha());
        QList<QPointF> *points = m_labelledPoints[i]->points();
        for (int j = 0; j < points->size(); j++)
        {
            m_strokeFont->StrokeMarker(StrokeFont::XShape, points->at(j).x(), points->at(j).y(), m_markerSize, m_markerSize);
        }
        m_strokeFont->drawLineGeometry(&m_shaderProgram);
    }


    // this allows me to have multiple updates when there are buffering issues with textures
    if (m_updateRepeatCount)
    {
        m_updateRepeatCount--;
        update();
    }

    m_shaderProgram.release();
}


void GraphicsView::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton && event->modifiers() == Qt::NoModifier)
    {
        // convert to view perspective coordinates (basically pixel coordinates on the image)
        float x = 2.0f * float(event->pos().x()) / float(width()) - 1;
        float y = 2.0f * float(height() - event->pos().y()) / float(height()) - 1;
        QMatrix4x4 invertedVPMatrix = (m_projection * m_view).inverted();
        m_panMatrix = invertedVPMatrix;
        m_startPan = invertedVPMatrix * QVector3D(x, y, 0);
        m_currentPan = m_startPan;
        m_activePan = true;
//        qDebug("x=%g y=%g m_startPan=%g,%g,%g", x, y, m_startPan.x(), m_startPan.y(), m_startPan.z());
        update();
    }
#ifdef FIX_ME_SELECTION_NOT_USEFUL
    if (event->buttons() & Qt::LeftButton && event->modifiers() == Qt::ControlModifier)
    {
        // convert to view perspective coordinates (basically pixel coordinates on the image)
        float x = 2.0f * float(event->pos().x()) / float(width()) - 1;
        float y = 2.0f * float(height() - event->pos().y()) / float(height()) - 1;
        QMatrix4x4 invertedVPMatrix = (m_projection * m_view).inverted();
        m_startSelect = invertedVPMatrix * QVector3D(x, y, 0);
        m_endSelect = m_startSelect;
        m_activeSelect = true;
        qDebug("x=%g y=%g m_startSelect=%g,%g,%g", x, y, m_startSelect.x(), m_startSelect.y(), m_startSelect.z());
        update();
    }
#endif
    if (event->buttons() & Qt::LeftButton && event->modifiers() == Qt::ShiftModifier)
    {
        // convert to view perspective coordinates (basically pixel coordinates on the image)
        float x = 2.0f * float(event->pos().x()) / float(width()) - 1;
        float y = 2.0f * float(height() - event->pos().y()) / float(height()) - 1;
        QMatrix4x4 invertedVPMatrix = (m_projection * m_view).inverted();
        QVector3D newCoordinate = invertedVPMatrix * QVector3D(x, y, 0);
        emit newLabelledPoint(newCoordinate.x(), newCoordinate.y());
        update();
    }

}

void GraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton && m_activePan)
    {
        // convert to view perspective coordinates (basically pixel coordinates on the image)
        float x = 2.0f * float(event->pos().x()) / float(width()) - 1;
        float y = 2.0f * float(height() - event->pos().y()) / float(height()) - 1;
        m_currentPan = m_panMatrix * QVector3D(x, y, 0);
//        qDebug("x=%g y=%g m_currentPan=%g,%g,%g", x, y, m_currentPan.x(), m_currentPan.y(), m_currentPan.z());
        m_center -= (m_currentPan - m_startPan);
        m_center.setX(CLAMP(m_center.x(), 0.0f, (float)m_imageWidth));
        m_center.setY(CLAMP(m_center.y(), 0.0f, (float)m_imageHeight));
        m_startPan = m_currentPan;
        emit newCentre(m_center.x(), m_center.y());
        update(); // maybe repaint() would be better here but it does not seem to track any better
    }
#ifdef FIX_ME_SELECTION_NOT_USEFUL
    if (event->buttons() & Qt::LeftButton)
    {
        // convert to view perspective coordinates (basically pixel coordinates on the image)
        float x = 2.0f * float(event->pos().x()) / float(width()) - 1;
        float y = 2.0f * float(height() - event->pos().y()) / float(height()) - 1;
        QMatrix4x4 invertedVPMatrix = (m_projection * m_view).inverted();
        m_endSelect = invertedVPMatrix * QVector3D(x, y, 0);
        update();
    }
#endif
}


void GraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_activePan)
    {
        // convert to view perspective coordinates (basically pixel coordinates on the image)
        float x = 2.0f * float(event->pos().x()) / float(width()) - 1;
        float y = 2.0f * float(height() - event->pos().y()) / float(height()) - 1;
        m_currentPan = m_panMatrix * QVector3D(x, y, 0);
//        qDebug("x=%g y=%g m_currentPan=%g,%g,%g", x, y, m_currentPan.x(), m_currentPan.y(), m_currentPan.z());
        m_center -= (m_currentPan - m_startPan);
        m_center.setX(CLAMP(m_center.x(), 0.0f, (float)m_imageWidth));
        m_center.setY(CLAMP(m_center.y(), 0.0f, (float)m_imageHeight));
        m_startPan = m_currentPan;
        emit newCentre(m_center.x(), m_center.y());
        m_activePan = false;
        update();
    }
#ifdef FIX_ME_SELECTION_NOT_USEFUL
    if (m_activeSelect)
    {
        // convert to view perspective coordinates (basically pixel coordinates on the image)
        float x = 2.0f * float(event->pos().x()) / float(width()) - 1;
        float y = 2.0f * float(height() - event->pos().y()) / float(height()) - 1;
        QMatrix4x4 invertedVPMatrix = (m_projection * m_view).inverted();
        m_endSelect = invertedVPMatrix * QVector3D(x, y, 0);
        m_activeSelect = false;
        update();
    }
#endif
}

void GraphicsView::wheelEvent(QWheelEvent * event)
{
    // assume each ratchet of the wheel gives a score of 120 (8 * 15 degrees)
    float sensitivity = 2400;
    float delta = event->angleDelta().y() / sensitivity;
    float scale = 1.0 + CLAMP(delta, -0.5f, 0.5f);
    m_zoom *= scale;
    emit statusString(QString("Zoom %1").arg(m_zoom));
    emit emitZoom(m_zoom);

    // convert to view perspective coordinates (basically pixel coordinates on the image)
    float x = 2.0f * float(event->position().x()) / float(width()) - 1;
    float y = 2.0f * float(height() - event->position().y()) / float(height()) - 1;
    QMatrix4x4 invertedVPMatrix = (m_projection * m_view).inverted();
    QVector3D wheelPoint = invertedVPMatrix * QVector3D(x, y, 0);
    QVector3D centerToPoint = wheelPoint - m_center;
    m_center += centerToPoint * delta;
    m_center.setX(CLAMP(m_center.x(), 0.0f, (float)m_imageWidth));
    m_center.setY(CLAMP(m_center.y(), 0.0f, (float)m_imageHeight));
    emit newCentre(m_center.x(), m_center.y());
    update();
}

QSize GraphicsView::sizeHint() const
{
    // the use of this value is dependent on the size policy

//    QSizePolicy::Fixed  0   The QWidget::sizeHint() is
//    the only acceptable alternative, so the widget can never
//    grow or shrink (e.g. the vertical direction of a push
//    button).

//    QSizePolicy::Minimum   GrowFlag    The
//    sizeHint() is minimal, and sufficient. The widget can be
//    expanded, but there is no advantage to it being larger
//    (e.g. the horizontal direction of a push button). It
//    cannot be smaller than the size provided by sizeHint().

//    QSizePolicy::Maximum    ShrinkFlag  The sizeHint() is
//    a maximum. The widget can be shrunk any amount without
//    detriment if other widgets need the space (e.g. a
//    separator line). It cannot be larger than the size
//    provided by sizeHint().

//    QSizePolicy::Preferred  GrowFlag | ShrinkFlag   The
//    sizeHint() is best, but the widget can be shrunk and
//    still be useful. The widget can be expanded, but there
//    is no advantage to it being larger than sizeHint() (the
//    default QWidget policy).

//    QSizePolicy::Expanding  GrowFlag | ShrinkFlag |
//    ExpandFlag  The sizeHint() is a sensible size, but the
//    widget can be shrunk and still be useful. The widget can
//    make use of extra space, so it should get as much space
//    as possible (e.g. the horizontal direction of a
//    horizontal slider).

//    QSizePolicy::MinimumExpanding   GrowFlag |
//    ExpandFlag  The sizeHint() is minimal, and sufficient.
//    The widget can make use of extra space, so it should get
//    as much space as possible (e.g. the horizontal direction
//    of a horizontal slider).

//    QSizePolicy::Ignored    ShrinkFlag | GrowFlag |
//    IgnoreFlag  The sizeHint() is ignored. The widget will
//    get as much space as possible.

    // this could be something cleverer where I look at the screen size and decide from that
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect  rec = screen->geometry();
    int height = rec.height();
    int width = rec.width();
    int minDimension = MIN(height, width);
    QSize minimumSize(int(minDimension * m_sizeHintScreenFraction), int(minDimension * m_sizeHintScreenFraction));
    return minimumSize;
}

void GraphicsView::setImageWidth()
{
    int lastImageWidth = m_imageWidth;
    int lastImageHeight = m_imageHeight;
    int redWidth = 1;
    int redHeight = 1;
    int greenWidth = 1;
    int greenHeight = 1;
    int blueWidth = 1;
    int blueHeight = 1;
    if (m_textureRed) { redWidth = m_textureRed->width(); redHeight = m_textureRed->height(); }
    if (m_textureGreen) { greenWidth = m_textureGreen->width(); greenHeight = m_textureGreen->height(); }
    if (m_textureBlue) { blueWidth = m_textureBlue->width(); blueHeight = m_textureBlue->height(); }
    m_imageWidth = MAX(redWidth, MAX(greenWidth, blueWidth));
    m_imageHeight = MAX(redHeight, MAX(greenHeight, blueHeight));
    if (m_imageWidth != lastImageWidth || m_imageHeight != lastImageHeight) emit imageDimensionsChanged();
}

float GraphicsView::markerSize() const
{
    return m_markerSize;
}

void GraphicsView::setMarkerSize(float markerSize)
{
    m_markerSize = markerSize;
}

QColor GraphicsView::selectionColour() const
{
    return m_selectionColour;
}

void GraphicsView::setSelectionColour(const QColor &selectionColour)
{
    m_selectionColour = selectionColour;
}

void GraphicsView::menuRequest(const QPoint &p)
{
    QMenu menu(this);
    QAction *deletePointAct = new QAction(QIcon(":/images/edit-delete-2.png"), tr("Delete Nearest Point"), this);
    deletePointAct->setStatusTip(tr("Deletes the nearest point to the cursor in the active list"));

    menu.addAction(deletePointAct);
    if (m_labelledPoints.size() == 0) deletePointAct->setEnabled(false);

    QPoint gp = this->mapToGlobal(p);
    QAction *action = menu.exec(gp);
    if (action == deletePointAct)
    {
        float x = 2.0f * float(p.x()) / float(width()) - 1;
        float y = 2.0f * float(height() - p.y()) / float(height()) - 1;
        QMatrix4x4 invertedVPMatrix = (m_projection * m_view).inverted();
        QVector3D newCoordinate = invertedVPMatrix * QVector3D(x, y, 0);
        emit deleteLabelledPoint(newCoordinate.x(), newCoordinate.y());
    }
}

float GraphicsView::centreY() const
{
    return m_center.y();
}

void GraphicsView::setCentreY(float centreY)
{
    m_center.setY(centreY);
}

float GraphicsView::centreX() const
{
    return m_center.x();
}

void GraphicsView::setCentreX(float centreX)
{
    m_center.setX(centreX);
}

float GraphicsView::rotationAngle() const
{
    return m_rotationAngle;
}

void GraphicsView::setRotationAngle(float rotationAngle)
{
    m_rotationAngle = rotationAngle;
}

GraphicsView::TextureDisplay GraphicsView::textureDisplayBlue() const
{
    return m_textureDisplayBlue;
}

void GraphicsView::setTextureDisplayBlue(const TextureDisplay &textureDisplayBlue)
{
    m_textureDisplayBlue = textureDisplayBlue;
}

GraphicsView::TextureDisplay GraphicsView::textureDisplayGreen() const
{
    return m_textureDisplayGreen;
}

void GraphicsView::setTextureDisplayGreen(const TextureDisplay &textureDisplayGreen)
{
    m_textureDisplayGreen = textureDisplayGreen;
}

GraphicsView::TextureDisplay GraphicsView::textureDisplayRed() const
{
    return m_textureDisplayRed;
}

void GraphicsView::setTextureDisplayRed(const TextureDisplay &textureDisplayRed)
{
    m_textureDisplayRed = textureDisplayRed;
}

QList<LabelledPoints *> *GraphicsView::labelledPoints()
{
    return &m_labelledPoints;
}


