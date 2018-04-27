#ifndef GRAPHICSVIEW_H
#define GRAPHICSVIEW_H

//#include <QGraphicsView>
#include <QWidget>
#include <QPointF>

#include <QOpenGLWidget>
#include <QElapsedTimer>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QMatrix4x4>
#include <QVector2D>
#include <QVector3D>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QSize>

class GeometryEngine;
class StrokeFont;
class LabelledPoints;

class GraphicsView : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    GraphicsView(QWidget *parent = 0);
    ~GraphicsView();

    enum ColourChannel {Red, Green, Blue, Black};
    struct TextureDisplay
    {
        float minimum;
        float maximum;
        float gamma;
        float zebra;
        bool log;
    };


    void setTexture(float *image, int width, int height, ColourChannel colourChannel);
    void setTextureDisplay(float minimum, float maximum, float gamma, float zebra, bool logFlag, ColourChannel colourChannel);

    QSize sizeHint() const;

    bool drawRed() const;
    void setDrawRed(bool drawRed);

    bool drawGreen() const;
    void setDrawGreen(bool drawGreen);

    bool drawBlue() const;
    void setDrawBlue(bool drawBlue);

    int imageWidth() const;
    void setImageWidth(int imageWidth);

    int imageHeight() const;
    void setImageHeight(int imageHeight);

    float zoom() const;
    void setZoom(float zoom);

    QList<LabelledPoints *> *labelledPoints();

    TextureDisplay textureDisplayRed() const;
    void setTextureDisplayRed(const TextureDisplay &textureDisplayRed);

    TextureDisplay textureDisplayGreen() const;
    void setTextureDisplayGreen(const TextureDisplay &textureDisplayGreen);

    TextureDisplay textureDisplayBlue() const;
    void setTextureDisplayBlue(const TextureDisplay &textureDisplayBlue);

    float rotationAngle() const;
    void setRotationAngle(float rotationAngle);

    float centreX() const;
    void setCentreX(float centreX);

    float centreY() const;
    void setCentreY(float centreY);

    QColor selectionColour() const;
    void setSelectionColour(const QColor &selectionColour);

    float markerSize() const;
    void setMarkerSize(float markerSize);

signals:
    void newLabelledPoint(float x, float y);
    void deleteLabelledPoint(float x, float y);
    void statusString(QString s);
    void emitZoom(float zoom);
    void emitDrawLog(bool drawLog);
    void newCentre(float x, float y);
    void imageDimensionsChanged();

public slots:

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent * event);

    void initShaders();
    void initTextures();

private slots:
    void menuRequest(const QPoint &p);

private:
    void setImageWidth();

    QOpenGLVertexArrayObject m_vao;
    QOpenGLShaderProgram m_shaderProgram;
    GeometryEngine *m_geometries;
    StrokeFont *m_strokeFont;

    QMatrix4x4 m_projection;
    QMatrix4x4 m_view;
    QMatrix4x4 m_model;
    QMatrix4x4 m_mvpMatrix;
    float m_nearPlane;
    float m_farPlane;
    QVector3D m_eye;
    QVector3D m_center;
    QVector3D m_up;

    float m_rotationAngle;

    QOpenGLTexture *m_textureRed;
    QOpenGLTexture *m_textureGreen;
    QOpenGLTexture *m_textureBlue;
    QOpenGLTexture *m_textureBlack;
    QOpenGLTexture *m_textureMarker;

    unsigned char m_updateRepeatCount;
    unsigned char m_textureUpdateRepeats;
    bool m_drawRed;
    bool m_drawGreen;
    bool m_drawBlue;
    TextureDisplay m_textureDisplayRed;
    TextureDisplay m_textureDisplayGreen;
    TextureDisplay m_textureDisplayBlue;

    float m_zoom;
    int m_imageWidth;
    int m_imageHeight;

    QMatrix4x4 m_panMatrix;
    QVector3D m_startPan;
    QVector3D m_currentPan;
    bool m_activePan;

    QVector3D m_startSelect;
    QVector3D m_endSelect;
    QColor m_selectionColour;
    bool m_activeSelect;

    float m_sizeHintScreenFraction;
    float m_markerSize;

    QList<LabelledPoints *> m_labelledPoints;
};

#endif // GRAPHICSVIEW_H
