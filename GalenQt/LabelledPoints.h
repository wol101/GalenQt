#ifndef LABELLEDPOINTS_H
#define LABELLEDPOINTS_H

#include <QString>
#include <QList>
#include <QPointF>
#include <QColor>

class QDomDocument;
class QDomElement;

class LabelledPoints
{
public:
    LabelledPoints();

    void AddPoint(float x, float y);

    void AddToDomDocument(QDomDocument *doc, QDomElement *parent);
    bool ReadFromDomElement(const QDomElement &element);

    static QColor GetNextColour();

    QString name() const;
    void setName(const QString &name);

    QColor colour() const;
    void setColour(const QColor &colour);

    QList<QPointF> *points();
    void setPoints(const QList<QPointF> &points);

    bool display() const;
    void setDisplay(bool display);

    bool current() const;
    void setCurrent(bool current);

    bool selected() const;
    void setSelected(bool selected);

    bool deleteLater() const;
    void setDeleteLater(bool deleteLater);

private:
    QString m_name;
    QList<QPointF> m_points;
    QColor m_colour;
    bool m_display;
    bool m_current;
    bool m_selected;
    bool m_deleteLater;
};

#endif // LABELLEDPOINTS_H
