/*
 *  MultiSpectralDocument.h
 *  GalenQt
 *
 *  Created by Bill Sellers on 18/10/2015.
 *  Copyright 2015 Bill Sellers. All rights reserved.
 *
 */

#ifndef MULTISPECTRALDOCUMENT_H
#define MULTISPECTRALDOCUMENT_H

#include <QList>
#include <QString>
#include <QStringList>

class SingleChannelImage;
class LabelledPoints;
class QTreeWidgetItem;
class QDomDocument;
class QDomElement;

class MultiSpectralDocument
{
public:
    MultiSpectralDocument();
    ~MultiSpectralDocument();

    bool Write(const QString &fileName);
    bool Read(const QString &fileName);


    QTreeWidgetItem *labelledPointsRoot() const;
    void setLabelledPointsRoot(QTreeWidgetItem *labelledPointsRoot);

    QTreeWidgetItem *imagesRoot() const;
    void setImagesRoot(QTreeWidgetItem *imagesRoot);

    LabelledPoints *currentLabelledPoints() const;
    void setCurrentLabelledPoints(LabelledPoints *currentLabelledPoints);

    SingleChannelImage *currentImage() const;
    void setCurrentImage(SingleChannelImage *currentImage);

private:
    void WriteImageChild(QDomDocument *doc, QDomElement *parent, QTreeWidgetItem *child);
    void WritePointsChild(QDomDocument *doc, QDomElement *parent, QTreeWidgetItem *child);
    void ReadElement(const QDomElement *element, QTreeWidgetItem *imagesTreeItem, QTreeWidgetItem *labelledPointsTreeItem);

    QTreeWidgetItem *m_labelledPointsRoot;
    LabelledPoints *m_currentLabelledPoints;
    QTreeWidgetItem *m_imagesRoot;
    SingleChannelImage *m_currentImage;

    QString m_fileName;
    QString m_parentFolder;
};

#endif // MULTISPECTRALDOCUMENT_H
