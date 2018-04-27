#include "CustomScroller.h"
#include "GraphicsView.h"

#include <QScrollBar>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QGridLayout>

#include <cmath>
#include <cfloat>

CustomScroller::CustomScroller(QWidget *parent) : QWidget(parent)
{
    m_graphicsView = 0;

    m_gridLayout = new QGridLayout();
    m_horizontalScrollBar = new QScrollBar(Qt::Horizontal);
    m_verticalScrollBar = new QScrollBar(Qt::Vertical);
    m_gridLayout->addWidget(m_verticalScrollBar, 0, 1);
    m_gridLayout->addWidget(m_horizontalScrollBar, 1, 0);
    m_gridLayout->setMargin(0);
    m_gridLayout->setHorizontalSpacing(0);
    m_gridLayout->setVerticalSpacing(0);
    setLayout(m_gridLayout);

    connect(m_horizontalScrollBar, SIGNAL(valueChanged(int)), this, SLOT(scrollContents(int)));
    connect(m_verticalScrollBar, SIGNAL(valueChanged(int)), this, SLOT(scrollContents(int)));

    m_horizontalScrollBar->setRange(0, 1000);
    m_horizontalScrollBar->setValue(500);
    m_verticalScrollBar->setRange(0, 1000);
    m_verticalScrollBar->setValue(500);

}

void CustomScroller::scrollContents(int value)
{
    Q_UNUSED(value);
    if (m_graphicsView)
    {
        float horizontal = float(m_horizontalScrollBar->value()); // / float(m_horizontalScrollBar->maximum());
        float vertical = float(m_verticalScrollBar->maximum() - m_verticalScrollBar->value()); // / float(m_verticalScrollBar->maximum());
        QMatrix4x4 rotationMatrix;
        rotationMatrix.setToIdentity();
        rotationMatrix.rotate(-m_graphicsView->rotationAngle(), 0, 0, 1.0f);
        QVector3D rotatedX = rotationMatrix * QVector3D(1, 0, 0);
        QVector3D rotatedY = rotationMatrix * QVector3D(0, 1, 0);
        float newX = 0, newY = 0;
        if (rotatedX.x() >= 0) newX += horizontal * rotatedX.x();
        else newX -= (float(m_horizontalScrollBar->maximum()) - horizontal) * rotatedX.x();
        if (rotatedX.y() >= 0) newY += horizontal * rotatedX.y();
        else newY -= (float(m_horizontalScrollBar->maximum()) - horizontal) * rotatedX.y();
        if (rotatedY.x() >= 0) newX += vertical * rotatedY.x();
        else newX -= (float(m_verticalScrollBar->maximum()) - vertical) * rotatedY.x();
        if (rotatedY.y() >= 0) newY += vertical * rotatedY.y();
        else newY -= (float(m_verticalScrollBar->maximum()) - vertical) * rotatedY.y();
        m_graphicsView->setCentreX(newX);
        m_graphicsView->setCentreY(newY);
        m_graphicsView->update();
        //    qDebug("scrollContents newX = %g newY = %g", newX, newY);
    }
}

void CustomScroller::scrollToCentre(float x, float y)
{
    if (m_graphicsView)
    {
        QMatrix4x4 rotationMatrix;
        rotationMatrix.setToIdentity();
        rotationMatrix.rotate(m_graphicsView->rotationAngle(), 0, 0, 1.0f);
        QVector3D c(x, y, 0);
        QVector3D cr = rotationMatrix * c;

        m_horizontalScrollBar->setValue(int(cr.x() - m_bottomLeftBound.x()));
        m_verticalScrollBar->setValue(m_verticalScrollBar->maximum() - (int(cr.y() - m_bottomLeftBound.y())));
    }
}

void CustomScroller::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    // this where I have to adjust the various elements of the scroll bar
//    qDebug("resizeEvent QResizeEvent oldSize() = %d,%d size() = %d,%d", event->oldSize().width(), event->oldSize().height(), event->size().width(), event->size().height());
    if (m_graphicsView)
    {
        // set the ranges to the biggest necessary value
        float viewPortWidth = float(m_graphicsView->width()) / m_graphicsView->zoom();
        float viewPortHeight = float(m_graphicsView->height()) / m_graphicsView->zoom();
        QList<QVector3D> corners;
        corners.append(QVector3D(0, 0, 0));
        corners.append(QVector3D(m_graphicsView->imageWidth(), 0, 0));
        corners.append(QVector3D(0, m_graphicsView->imageHeight(), 0));
        corners.append(QVector3D(m_graphicsView->imageWidth(), m_graphicsView->imageHeight(), 0));
        QMatrix4x4 rotationMatrix;
        rotationMatrix.setToIdentity();
        rotationMatrix.rotate(m_graphicsView->rotationAngle(), 0, 0, 1.0f);
        m_bottomLeftBound = QVector3D(FLT_MAX, FLT_MAX, 0);
        m_topRightBound = QVector3D(-FLT_MAX, -FLT_MAX, 0);
        for (int i = 0; i < corners.size(); i++)
        {
            corners[i] = rotationMatrix * corners[i];
            m_bottomLeftBound.setX(std::min(m_bottomLeftBound.x(), corners[i].x()));
            m_bottomLeftBound.setY(std::min(m_bottomLeftBound.y(), corners[i].y()));
            m_topRightBound.setX(std::max(m_topRightBound.x(), corners[i].x()));
            m_topRightBound.setY(std::max(m_topRightBound.y(), corners[i].y()));
        }
        float hRange = m_topRightBound.x() - m_bottomLeftBound.x();
        float vRange = m_topRightBound.y() - m_bottomLeftBound.y();
        float hScrollFraction = float(m_horizontalScrollBar->value()) / float(m_horizontalScrollBar->maximum());
        float vScrollFraction = float(m_verticalScrollBar->value()) / float(m_verticalScrollBar->maximum());
        m_horizontalScrollBar->setRange(0, hRange);
        m_horizontalScrollBar->setPageStep(int(viewPortWidth));
        m_horizontalScrollBar->setValue(hScrollFraction * hRange);
        m_verticalScrollBar->setRange(0, vRange);
        m_verticalScrollBar->setPageStep(int(viewPortHeight));
        m_verticalScrollBar->setValue(vScrollFraction * vRange);
//        qDebug("resizeEvent hRange = %g vRange = %g viewPortWidth = %g viewPortHeight = %g", hRange, vRange, viewPortWidth, viewPortHeight);
        scrollContents(0);
    }
}

GraphicsView *CustomScroller::graphicsView() const
{
    return m_graphicsView;
}

void CustomScroller::setGraphicsView(GraphicsView *graphicsView)
{
    m_graphicsView = graphicsView;
    m_gridLayout->addWidget(m_graphicsView, 0, 0);
}

void CustomScroller::contentsResized()
{
    if (m_graphicsView)
    {
        // set the ranges to the biggest necessary value
        float viewPortWidth = float(m_graphicsView->width()) / m_graphicsView->zoom();
        float viewPortHeight = float(m_graphicsView->height()) / m_graphicsView->zoom();
        QVector<QVector3D> corners(4);
        corners[0] = QVector3D(0, 0, 0);
        corners[1] = QVector3D(m_graphicsView->imageWidth(), 0, 0);
        corners[2] = QVector3D(0, m_graphicsView->imageHeight(), 0);
        corners[3] = QVector3D(m_graphicsView->imageWidth(), m_graphicsView->imageHeight(), 0);
        QMatrix4x4 rotationMatrix;
        rotationMatrix.setToIdentity();
        rotationMatrix.rotate(m_graphicsView->rotationAngle(), 0, 0, 1.0f);
        m_bottomLeftBound = QVector3D(FLT_MAX, FLT_MAX, 0);
        m_topRightBound = QVector3D(-FLT_MAX, -FLT_MAX, 0);
        for (int i = 0; i < corners.size(); i++)
        {
            corners[i] = rotationMatrix * corners[i];
            m_bottomLeftBound.setX(std::min(m_bottomLeftBound.x(), corners[i].x()));
            m_bottomLeftBound.setY(std::min(m_bottomLeftBound.y(), corners[i].y()));
            m_topRightBound.setX(std::max(m_topRightBound.x(), corners[i].x()));
            m_topRightBound.setY(std::max(m_topRightBound.y(), corners[i].y()));
        }
        float hRange = m_topRightBound.x() - m_bottomLeftBound.x();
        float vRange = m_topRightBound.y() - m_bottomLeftBound.y();
        float hScrollFraction = float(m_horizontalScrollBar->value()) / float(m_horizontalScrollBar->maximum());
        float vScrollFraction = float(m_verticalScrollBar->value()) / float(m_verticalScrollBar->maximum());
        m_horizontalScrollBar->setRange(0, hRange);
        m_horizontalScrollBar->setPageStep(int(viewPortWidth));
        m_horizontalScrollBar->setValue(hScrollFraction * hRange);
        m_verticalScrollBar->setRange(0, vRange);
        m_verticalScrollBar->setPageStep(int(viewPortHeight));
        m_verticalScrollBar->setValue(vScrollFraction * vRange);
//        float hRange = m_topRightBound.x() - m_bottomLeftBound.x();
//        float vRange = m_topRightBound.y() - m_bottomLeftBound.y();
//        m_horizontalScrollBar->setRange(0, hRange);
//        m_horizontalScrollBar->setPageStep(int(viewPortWidth));
//        m_verticalScrollBar->setRange(0, vRange);
//        m_verticalScrollBar->setPageStep(int(viewPortHeight));
//        qDebug("resizeEvent hRange = %g vRange = %g viewPortWidth = %g viewPortHeight = %g", hRange, vRange, viewPortWidth, viewPortHeight);
        scrollContents(0);
    }
}

void CustomScroller::setScrollFractions(float horizontal, float vertical)
{
    int newX = m_horizontalScrollBar->maximum() * horizontal;
    int newY = m_verticalScrollBar->maximum() * vertical;
//    qDebug("setScrollFractions newX = %d newY = %d", newX, newY);
    m_horizontalScrollBar->setValue(newX);
    m_verticalScrollBar->setValue(newY);
}

void CustomScroller::initialiseScrollBars(int hPos, int hRange, int vPos, int vRange)
{
    m_horizontalScrollBar->setRange(0, hRange);
    m_horizontalScrollBar->setValue(hPos);
    m_verticalScrollBar->setRange(0, vRange);
    m_verticalScrollBar->setValue(vPos);
}


