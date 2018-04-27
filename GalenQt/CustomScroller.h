#ifndef CUSTOMSCROLLER_H
#define CUSTOMSCROLLER_H

#include <QWidget>
#include <QVector3D>

class GraphicsView;
class QScrollBar;
class QResizeEvent;
class QGridLayout;

class CustomScroller : public QWidget
{
    Q_OBJECT
public:
    explicit CustomScroller(QWidget *parent = 0);

    GraphicsView *graphicsView() const;
    void setGraphicsView(GraphicsView *graphicsView);

    void setScrollFractions(float horizontal, float vertical);
    void initialiseScrollBars(int hPos, int hRange, int vPos, int vRange);

signals:

public slots:
    void scrollContents(int value);
    void contentsResized();
    void scrollToCentre(float x, float y);

protected:
//    void scrollContentsBy(int dx, int dy);
    void resizeEvent(QResizeEvent *event);

    GraphicsView *m_graphicsView;
    QGridLayout *m_gridLayout;
    QScrollBar *m_horizontalScrollBar;
    QScrollBar *m_verticalScrollBar;
    QVector3D m_bottomLeftBound;
    QVector3D m_topRightBound;

};

#endif
