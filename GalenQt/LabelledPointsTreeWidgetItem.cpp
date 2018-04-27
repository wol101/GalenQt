#include "LabelledPointsTreeWidgetItem.h"

#include "LabelledPoints.h"

LabelledPointsTreeWidgetItem::LabelledPointsTreeWidgetItem(int type)
    : QTreeWidgetItem(type)
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    m_labelledPoints = 0;
    m_deleteLater = false;
}

LabelledPointsTreeWidgetItem::LabelledPointsTreeWidgetItem(QTreeWidgetItem *parent, const QStringList &strings, int type)
    : QTreeWidgetItem(parent, strings, type)
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    m_labelledPoints = 0;
    m_deleteLater = false;
}

LabelledPointsTreeWidgetItem::LabelledPointsTreeWidgetItem(QTreeWidgetItem *parent, const QStringList &strings, LabelledPoints *labelledPoints, int type)
    : QTreeWidgetItem(parent, strings, type)
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    setLabelledPoints(labelledPoints);
    m_deleteLater = false;
}

LabelledPointsTreeWidgetItem::~LabelledPointsTreeWidgetItem()
{
    if (m_labelledPoints) delete m_labelledPoints;
}

LabelledPoints *LabelledPointsTreeWidgetItem::labelledPoints() const
{
    return m_labelledPoints;
}

void LabelledPointsTreeWidgetItem::setLabelledPoints(LabelledPoints *labelledPoints)
{
    Q_ASSERT(labelledPoints);
    m_labelledPoints = labelledPoints;
    this->setCheckState(1, m_labelledPoints->display() ? Qt::Checked : Qt::Unchecked);
    this->setCheckState(2, m_labelledPoints->current() ? Qt::Checked : Qt::Unchecked);
    this->setCheckState(3, m_labelledPoints->selected() ? Qt::Checked : Qt::Unchecked);
    setFlags(flags() & ~Qt::ItemIsDropEnabled);
}

bool LabelledPointsTreeWidgetItem::deleteLater() const
{
    return m_deleteLater;
}

void LabelledPointsTreeWidgetItem::setDeleteLater(bool deleteLater)
{
    m_deleteLater = deleteLater;
    if (m_labelledPoints) m_labelledPoints->setDeleteLater(deleteLater);
    for (int i = 0; i < childCount(); i++)
    {
        LabelledPointsTreeWidgetItem *item = dynamic_cast<LabelledPointsTreeWidgetItem *>(child(i));
        if (item) item->setDeleteLater(deleteLater);
    }
}


