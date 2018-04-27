#include "ImageTreeWidgetItem.h"
#include "SingleChannelImage.h"

ImageTreeWidgetItem::ImageTreeWidgetItem(int type)
    : QTreeWidgetItem(type)
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    m_image = 0;
    m_deleteLater = false;
}

ImageTreeWidgetItem::ImageTreeWidgetItem(QTreeWidgetItem *parent, const QStringList &strings, int type)
    : QTreeWidgetItem(parent, strings, type)
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    m_image = 0;
    m_deleteLater = false;
}

ImageTreeWidgetItem::ImageTreeWidgetItem(QTreeWidgetItem *parent, const QStringList &strings, SingleChannelImage *image, int type)
    : QTreeWidgetItem(parent, strings, type)
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    setImage(image);
    m_deleteLater = false;
}

ImageTreeWidgetItem::~ImageTreeWidgetItem()
{
    if (m_image) delete m_image;
}

SingleChannelImage *ImageTreeWidgetItem::image() const
{
    return m_image;
}

void ImageTreeWidgetItem::setImage(SingleChannelImage *image)
{
    Q_ASSERT(image);
    m_image = image;
    this->setCheckState(1, m_image->displayRed() ? Qt::Checked : Qt::Unchecked);
    this->setCheckState(2, m_image->displayGreen() ? Qt::Checked : Qt::Unchecked);
    this->setCheckState(3, m_image->displayBlue() ? Qt::Checked : Qt::Unchecked);
    this->setCheckState(4, m_image->selected() ? Qt::Checked : Qt::Unchecked);
    setFlags(flags() & ~Qt::ItemIsDropEnabled);
}

bool ImageTreeWidgetItem::deleteLater() const
{
    return m_deleteLater;
}

void ImageTreeWidgetItem::setDeleteLater(bool deleteLater)
{
    m_deleteLater = deleteLater;
    if (m_image) m_image->setDeleteLater(deleteLater);
    for (int i = 0; i < childCount(); i++)
    {
        ImageTreeWidgetItem *item = dynamic_cast<ImageTreeWidgetItem *>(child(i));
        if (item) item->setDeleteLater(deleteLater);
    }
}



