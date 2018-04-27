/*
 *  HistogramDisplayWidget.h
 *  GalenQt
 *
 *  Created by Bill Sellers on 25/10/2015.
 *  Copyright 2015 Bill Sellers. All rights reserved.
 *
 */

#ifndef HISTOGRAMDISPLAYWIDGET_H
#define HISTOGRAMDISPLAYWIDGET_H

#include <QWidget>
#include <QColor>

class SingleChannelImage;

class HistogramDisplayWidget : public QWidget
{
    Q_OBJECT
public:
    explicit HistogramDisplayWidget(QWidget *parent = 0);
    ~HistogramDisplayWidget();

    SingleChannelImage *image() const;
    void setImage(SingleChannelImage *image);

    QColor histogramColour() const;
    void setHistogramColour(const QColor &histogramColour);

    QColor outputHistogramColour() const;
    void setOutputHistogramColour(const QColor &outputHistogramColour);

    QColor lineColour() const;
    void setLineColour(const QColor &lineColour);

signals:

public slots:

protected:
    void mousePressEvent(QMouseEvent *event);
    void paintEvent (QPaintEvent *);

private slots:

private:
    SingleChannelImage *m_image;
    QColor m_histogramColour;
    QColor m_outputHistogramColour;
    QColor m_lineColour;
};

#endif // HISTOGRAMDISPLAYWIDGET_H
