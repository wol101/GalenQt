#ifndef IMAGETREEWIDGETITEM_H
#define IMAGETREEWIDGETITEM_H

#include <QTreeWidgetItem>
#include <QList>

class SingleChannelImage;

class ImageTreeWidgetItem : public QTreeWidgetItem
{
public:
    static const int ImageTreeWidgetItemUserType = UserType + 1;
    ImageTreeWidgetItem(int type = ImageTreeWidgetItemUserType);
    ImageTreeWidgetItem(QTreeWidgetItem *parent, const QStringList &strings, int type = ImageTreeWidgetItemUserType);
    ImageTreeWidgetItem(QTreeWidgetItem *parent, const QStringList &strings, SingleChannelImage *image, int type = ImageTreeWidgetItemUserType);
    ~ImageTreeWidgetItem();

    SingleChannelImage *image() const;
    void setImage(SingleChannelImage *image);

    bool deleteLater() const;
    void setDeleteLater(bool deleteLater);

private:
    SingleChannelImage *m_image;
    bool m_deleteLater;
};

#endif // IMAGETREEWIDGETITEM_H
