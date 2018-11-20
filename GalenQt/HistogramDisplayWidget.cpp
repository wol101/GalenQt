/*
 *  HistogramDisplayWidget.cpp
 *  GalenQt
 *
 *  Created by Bill Sellers on 25/10/2015.
 *  Copyright 2015 Bill Sellers. All rights reserved.
 *
 */

#include "HistogramDisplayWidget.h"
#include "SingleChannelImage.h"
#include "Settings.h"

#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPalette>
#include <QMenu>
#include <QAction>
#include <QtDebug>

#include <cmath>
#include <cfloat>

#define CLAMP(n,lower,upper) (std::max(lower, std::min(n, upper)))

HistogramDisplayWidget::HistogramDisplayWidget(QWidget *parent) : QWidget(parent)
{
    m_image = 0;

    m_histogramColour = Settings::value("Histogram Bar Colour", QColor(0, 100, 50)).value<QColor>();
    m_outputHistogramColour = Settings::value("Output Histogram Bar Colour", QColor(150, 100, 0)).value<QColor>();
    m_lineColour = Settings::value("Histogram Line Colour", QColor(0, 100, 150)).value<QColor>();
}

HistogramDisplayWidget::~HistogramDisplayWidget()
{
}

void HistogramDisplayWidget::mousePressEvent(QMouseEvent *event)
{

    //qDebug() << event->pos().x() << " " << event->pos().y() << "\n";

    if (event->buttons() & Qt::LeftButton)
    {
        update();
    }
}

void HistogramDisplayWidget::paintEvent (QPaintEvent *)
{
    int i;
    int xMargin = 4;
    int yMargin = 4;
    QPainter qpainter(this);

    // this puts a standard frame around the widget
    qpainter.setPen(palette().dark().color()); // windowText().color() might be better
    qpainter.setBrush(palette().base().color()); // this is the background colour for text entry so very suitable (pale)
    qpainter.drawRect(0, 0, width() - 1, height() - 1);
    if (m_image == 0) return;

    // draw the image histogram (scaled to fit)
    qpainter.setPen(m_histogramColour);
    int horizontalRange = width() - xMargin * 2;
    int verticalRange = height() - yMargin * 2;
    float *binEnds = m_image->binEnds();
    int *histogram = m_image->histogram();
    int ix, iy, ix0, iy0;
    float x, y, tx;
    iy0 = height() - yMargin;
    for (i = 0; i < m_image->numBins(); i++)
    {
        x = (binEnds[i] + (binEnds[i + 1] - binEnds[i]) * 0.5f);
        ix0 = xMargin + int(horizontalRange * (x - m_image->dataMin()) / (m_image->dataMax() - m_image->dataMin()));
        ix = ix0;
        iy = height() - (yMargin + int((float(verticalRange) / float(m_image->histogramMax())) * float(histogram[i])));
        qpainter.drawLine(ix0, iy0, ix, iy);
    }

    if (m_image->displayLogged() == false)
    {
        // draw the histogram after processing with the tranfer function on the same axes (scaled to fit)
        qpainter.setPen(m_outputHistogramColour);
        float txMin = FLT_MAX;
        float txMax = -FLT_MAX;
        for (i = 0; i < m_image->numBins(); i++)
        {
            x = (binEnds[i] + (binEnds[i + 1] - binEnds[i]) * 0.5f);
            tx = std::pow(std::fmod(CLAMP((x - m_image->displayMin()) / (m_image->displayMax() - m_image->displayMin()), 0.0f, 0.99999f) * m_image->displayZebra(), 1.0f), m_image->displayGamma());
            if (tx < txMin) txMin = tx;
            if (tx > txMax) txMax = tx;
        }
        for (i = 0; i < m_image->numBins(); i++)
        {
            x = (binEnds[i] + (binEnds[i + 1] - binEnds[i]) * 0.5f);
            tx = std::pow(std::fmod(CLAMP((x - m_image->displayMin()) / (m_image->displayMax() - m_image->displayMin()), 0.0f, 0.99999f) * m_image->displayZebra(), 1.0f), m_image->displayGamma());
            ix0 = xMargin + int(horizontalRange * (tx - txMin) / (txMax - txMin));
            ix = ix0;
            iy = height() - (yMargin + int((float(verticalRange) / float(m_image->histogramMax())) * float(histogram[i])));
            qpainter.drawLine(ix0, iy0, ix, iy);
        }

        // transfer function line on the same axes (scaled to fit)
        qpainter.setPen(m_lineColour);
        for (ix = xMargin; ix < width() - xMargin; ix++)
        {
            x = (float((ix - xMargin)) / horizontalRange) * (m_image->dataMax() - m_image->dataMin()) + m_image->dataMin();
            y = std::pow(std::fmod(CLAMP((x - m_image->displayMin()) / (m_image->displayMax() - m_image->displayMin()), 0.0f, 0.99999f) * m_image->displayZebra(), 1.0f), m_image->displayGamma()); // apply the operators, y is now 0-1
            iy = height() - (int(y * verticalRange) + yMargin); // remember the origin is the top left
            // qDebug("x=%f y1=%d y=%f iy=%d", x, y1, y, iy);
            if (ix != xMargin) qpainter.drawLine(ix0, iy0, ix, iy);
            ix0 = ix;
            iy0 = iy;
        }
    }
    else
    {
        // draw the histogram after processing with the tranfer function on the same axes (scaled to fit)
        qpainter.setPen(m_outputHistogramColour);
        float txMin = FLT_MAX;
        float txMax = -FLT_MAX;
        float logDisplayMin = std::log(m_image->displayMin());
        float logDisplayMax = std::log(m_image->displayMax());
        for (i = 0; i < m_image->numBins(); i++)
        {
            x = (binEnds[i] + (binEnds[i + 1] - binEnds[i]) * 0.5f);
            tx = std::pow(std::fmod(CLAMP((std::log(std::max(x, m_image->displayMin())) - logDisplayMin) / (logDisplayMax - logDisplayMin), 0.0f, 0.99999f) * m_image->displayZebra(), 1.0f), m_image->displayGamma());
            if (tx < txMin) txMin = tx;
            if (tx > txMax) txMax = tx;
        }
        for (i = 0; i < m_image->numBins(); i++)
        {
            x = (binEnds[i] + (binEnds[i + 1] - binEnds[i]) * 0.5f);
            tx = std::pow(std::fmod(CLAMP((std::log(std::max(x, m_image->displayMin())) - logDisplayMin) / (logDisplayMax - logDisplayMin), 0.0f, 0.99999f) * m_image->displayZebra(), 1.0f), m_image->displayGamma());
            ix0 = xMargin + int(horizontalRange * (tx - txMin) / (txMax - txMin));
            ix = ix0;
            iy = height() - (yMargin + int((float(verticalRange) / float(m_image->histogramMax())) * float(histogram[i])));
            qpainter.drawLine(ix0, iy0, ix, iy);
        }

        // transfer function line on the same axes (scaled to fit)
        qpainter.setPen(m_lineColour);
        for (ix = xMargin; ix < width() - xMargin; ix++)
        {
            x = (float((ix - xMargin)) / horizontalRange) * (m_image->dataMax() - m_image->dataMin()) + m_image->dataMin();
            y = std::pow(std::fmod(CLAMP((std::log(std::max(x, m_image->displayMin())) - logDisplayMin) / (logDisplayMax - logDisplayMin), 0.0f, 0.99999f) * m_image->displayZebra(), 1.0f), m_image->displayGamma()); // apply the operators, y is now 0-1
            iy = height() - (int(y * verticalRange) + yMargin); // remember the origin is the top left
            // qDebug("x=%f y1=%d y=%f iy=%d", x, y1, y, iy);
            if (ix != xMargin) qpainter.drawLine(ix0, iy0, ix, iy);
            ix0 = ix;
            iy0 = iy;
        }
    }
}


QColor HistogramDisplayWidget::lineColour() const
{
    return m_lineColour;
}

void HistogramDisplayWidget::setLineColour(const QColor &lineColour)
{
    m_lineColour = lineColour;
}

QColor HistogramDisplayWidget::outputHistogramColour() const
{
    return m_outputHistogramColour;
}

void HistogramDisplayWidget::setOutputHistogramColour(const QColor &outputHistogramColour)
{
    m_outputHistogramColour = outputHistogramColour;
}

QColor HistogramDisplayWidget::histogramColour() const
{
    return m_histogramColour;
}

void HistogramDisplayWidget::setHistogramColour(const QColor &histogramColour)
{
    m_histogramColour = histogramColour;
}


SingleChannelImage *HistogramDisplayWidget::image() const
{
    return m_image;
}

void HistogramDisplayWidget::setImage(SingleChannelImage *image)
{
    m_image = image;
}



