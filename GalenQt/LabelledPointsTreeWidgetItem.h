#ifndef LabelledPointsTreeWidgetItem_H
#define LabelledPointsTreeWidgetItem_H

#include <QTreeWidgetItem>
#include <QList>

class LabelledPoints;

class LabelledPointsTreeWidgetItem : public QTreeWidgetItem
{
public:
    static const int LabelledPointsTreeWidgetItemUserType = UserType + 2;
    LabelledPointsTreeWidgetItem(int type = LabelledPointsTreeWidgetItemUserType);
    LabelledPointsTreeWidgetItem(QTreeWidgetItem *parent, const QStringList &strings, int type = LabelledPointsTreeWidgetItemUserType);
    LabelledPointsTreeWidgetItem(QTreeWidgetItem *parent, const QStringList &strings, LabelledPoints *labelledPoints, int type = LabelledPointsTreeWidgetItemUserType);
    ~LabelledPointsTreeWidgetItem();

    LabelledPoints *labelledPoints() const;
    void setLabelledPoints(LabelledPoints *labelledPoints);

    bool deleteLater() const;
    void setDeleteLater(bool deleteLater);

private:
    LabelledPoints *m_labelledPoints;
    bool m_deleteLater;
};

#endif // LabelledPointsTreeWidgetItem_H
