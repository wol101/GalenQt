#include "LabelledPoints.h"

#include "Settings.h"

#include <QDomDocument>
#include <QDomElement>
#include <QVariant>
#include <QColor>
#include <QRandomGenerator>

LabelledPoints::LabelledPoints()
{
    m_colour = GetNextColour();
    m_display = false;
    m_current = false;
    m_selected = false;
}

QString LabelledPoints::name() const
{
    return m_name;
}

void LabelledPoints::setName(const QString &name)
{
    m_name = name;
}

QColor LabelledPoints::colour() const
{
    return m_colour;
}

void LabelledPoints::setColour(const QColor &colour)
{
    m_colour = colour;
}

QList<QPointF> *LabelledPoints::points()
{
    return &m_points;
}

void LabelledPoints::setPoints(const QList<QPointF> &points)
{
    m_points = points;
}

bool LabelledPoints::display() const
{
    return m_display;
}

void LabelledPoints::setDisplay(bool display)
{
    m_display = display;
}

bool LabelledPoints::current() const
{
    return m_current;
}

void LabelledPoints::setCurrent(bool current)
{
    m_current = current;
}

bool LabelledPoints::selected() const
{
    return m_selected;
}

void LabelledPoints::setSelected(bool selected)
{
    m_selected = selected;
}

bool LabelledPoints::deleteLater() const
{
    return m_deleteLater;
}

void LabelledPoints::setDeleteLater(bool deleteLater)
{
    m_deleteLater = deleteLater;
}

void LabelledPoints::AddPoint(float x, float y)
{
    m_points.push_back(QPointF(x, y));
}

void LabelledPoints::AddToDomDocument(QDomDocument *doc, QDomElement *parent)
{
    QDomElement labelledPoints = doc->createElement("LabelledPoints");
    parent->appendChild(labelledPoints);
    labelledPoints.setAttribute("name", m_name);
    labelledPoints.setAttribute("colour", m_colour.name(QColor::HexArgb));
    labelledPoints.setAttribute("display", m_display);
    labelledPoints.setAttribute("current", m_current);
    labelledPoints.setAttribute("selected", m_selected);

    for (int i = 0; i < m_points.size(); i++)
    {
        QDomElement point = doc->createElement("Point");
        labelledPoints.appendChild(point);
        point.setAttribute("x", m_points.at(i).x());
        point.setAttribute("y", m_points.at(i).y());
    }
}

bool LabelledPoints::ReadFromDomElement(const QDomElement &element)
{
    m_name = element.attribute("name");
    if (m_name.isEmpty()) return false;
    m_colour.setNamedColor(element.attribute("colour"));
    setDisplay(element.attribute("display").toInt());
    setCurrent(element.attribute("current").toInt());
    setSelected(element.attribute("selected").toInt());

    m_points.clear();
    QDomNode n = element.firstChild();
    while(!n.isNull())
    {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        if (!e.isNull())
        {
            if (e.tagName() == "Point")
            {
                QPointF point;
                point.setX(e.attribute("x").toFloat());
                point.setY(e.attribute("y").toFloat());
                m_points.push_back(point);
            }
        }
        n = n.nextSibling();
    }
    return true;
}

QColor LabelledPoints::GetNextColour()
{
    static int nextColour = -1;
    static double rgb_sequence[] =
    {
        0.3333,   0.3333,        0,
        0.3333,   0.6667,        0,
        0.3333,   1.0000,        0,
        0.6667,   0.3333,        0,
        0.6667,   0.6667,        0,
        0.6667,   1.0000,        0,
        1.0000,   0.3333,        0,
        1.0000,   0.6667,        0,
        1.0000,   1.0000,        0,
             0,   0.3333,   0.5000,
             0,   0.6667,   0.5000,
             0,   1.0000,   0.5000,
        0.3333,        0,   0.5000,
        0.3333,   0.3333,   0.5000,
        0.3333,   0.6667,   0.5000,
        0.3333,   1.0000,   0.5000,
        0.6667,        0,   0.5000,
        0.6667,   0.3333,   0.5000,
        0.6667,   0.6667,   0.5000,
        0.6667,   1.0000,   0.5000,
        1.0000,        0,   0.5000,
        1.0000,   0.3333,   0.5000,
        1.0000,   0.6667,   0.5000,
        1.0000,   1.0000,   0.5000,
             0,   0.3333,   1.0000,
             0,   0.6667,   1.0000,
             0,   1.0000,   1.0000,
        0.3333,        0,   1.0000,
        0.3333,   0.3333,   1.0000,
        0.3333,   0.6667,   1.0000,
        0.3333,   1.0000,   1.0000,
        0.6667,        0,   1.0000,
        0.6667,   0.3333,   1.0000,
        0.6667,   0.6667,   1.0000,
        0.6667,   1.0000,   1.0000,
        1.0000,        0,   1.0000,
        1.0000,   0.3333,   1.0000,
        1.0000,   0.6667,   1.0000,
        0.1667,        0,        0,
        0.3333,        0,        0,
        0.5000,        0,        0,
        0.6667,        0,        0,
        0.8333,        0,        0,
        1.0000,        0,        0,
             0,   0.1667,        0,
             0,   0.3333,        0,
             0,   0.5000,        0,
             0,   0.6667,        0,
             0,   0.8333,        0,
             0,   1.0000,        0,
             0,        0,   0.1667,
             0,        0,   0.3333,
             0,        0,   0.5000,
             0,        0,   0.6667,
             0,        0,   0.8333,
             0,        0,   1.0000/*,
             0,        0,        0,
        0.1429,   0.1429,   0.1429,
        0.2857,   0.2857,   0.2857,
        0.4286,   0.4286,   0.4286,
        0.5714,   0.5714,   0.5714,
        0.7143,   0.7143,   0.7143,
        0.8571,   0.8571,   0.8571,
        1.0000,   1.0000,   1.0000*/ // we don't want the greys
    };
    static std::vector<int> numbersAvailable;
    if (Settings::value("Random Label Colours", true).toBool() == false)
    {
        nextColour++;
        if (nextColour >= sizeof(rgb_sequence) / (3 * sizeof(rgb_sequence[0]))) nextColour = 0;
    }
    else
    {
        if (numbersAvailable.size() == 0) for (int i = 0; i < sizeof(rgb_sequence) / (3 * sizeof(rgb_sequence[0])); i++) numbersAvailable.push_back(i);
        int index = QRandomGenerator::global()->bounded(0, int(numbersAvailable.size()));
        nextColour = numbersAvailable[index];
        numbersAvailable.erase(numbersAvailable.begin() + index);
    }
    int r = int(rgb_sequence[nextColour * 3] * 255.999999);
    int g = int(rgb_sequence[nextColour * 3 + 1] * 255.999999);
    int b = int(rgb_sequence[nextColour * 3 + 2] * 255.999999);
    QColor colour(r, g, b);
    return colour;
}

