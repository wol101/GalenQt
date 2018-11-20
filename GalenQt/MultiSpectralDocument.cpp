/*
 *  MultiSpectralDocument.cpp
 *  GalenQt
 *
 *  Created by Bill Sellers on 18/10/2015.
 *  Copyright 2015 Bill Sellers. All rights reserved.
 *
 */

#include <QList>
#include <QString>
#include <QStringList>
#include <QDomDocument>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>

#include "MultiSpectralDocument.h"
#include "SingleChannelImage.h"
#include "LabelledPoints.h"
#include "ImageTreeWidgetItem.h"
#include "LabelledPointsTreeWidgetItem.h"

MultiSpectralDocument::MultiSpectralDocument()
{
    m_currentImage = 0;
    m_currentLabelledPoints = 0;
    m_labelledPointsRoot = 0;
    m_imagesRoot = 0;

}

MultiSpectralDocument::~MultiSpectralDocument()
{
}

bool MultiSpectralDocument::Write(const QString &fileName)
{
    m_fileName = fileName;
    QFileInfo info(fileName);
    m_parentFolder = QDir::cleanPath(info.absolutePath());

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(0, "Warning", QString("Unable to open MultiSpectralDocument file: %1").arg(fileName));
        return false;
    }

    QDomDocument doc("GalenQtDocument_v1.0");
    QDomProcessingInstruction  pi = doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(pi);
    QDomElement root = doc.createElement("MultiSpectralDocument");
    doc.appendChild(root);

    if (m_imagesRoot)
    {
        for (int i = 0; i < m_imagesRoot->childCount(); i++)
        {
            WriteImageChild(&doc, &root, dynamic_cast<ImageTreeWidgetItem *>(m_imagesRoot->child(i)));
        }
    }

    if (m_labelledPointsRoot)
    {
        for (int i = 0; i < m_labelledPointsRoot->childCount(); i++)
        {
            WritePointsChild(&doc, &root, dynamic_cast<LabelledPointsTreeWidgetItem *>(m_labelledPointsRoot->child(i)));
        }
    }

    // and now the actual xml doc
    QByteArray xmlData = doc.toByteArray();
    qint64 bytesWritten = file.write(xmlData);
    if (bytesWritten != xmlData.size())
    {
        QMessageBox::warning(0, "Warning", QString("Unable to write MultiSpectralDocument file: %1").arg(fileName));
        file.close();
        return false;
    }
    file.close();
    return true;
}

void MultiSpectralDocument::WriteImageChild(QDomDocument *doc, QDomElement *parent, QTreeWidgetItem *child)
{
    if (child == 0) return;
    ImageTreeWidgetItem *item = dynamic_cast<ImageTreeWidgetItem *>(child);
    if (child->childCount() == 0 && item && item->image()) // this is an image
    {
        item->image()->AddToDomDocument(doc, parent, m_parentFolder);
    }
    else // this is a folder
    {
        QDomElement folderElement = doc->createElement("ImagesFolder");
        folderElement.setAttribute("name", child->data(0, Qt::DisplayRole).toString());
        parent->appendChild(folderElement);
        for (int i = 0; i < child->childCount(); i++)
        {
            WriteImageChild(doc, &folderElement, child->child(i));
        }
    }
}

void MultiSpectralDocument::WritePointsChild(QDomDocument *doc, QDomElement *parent, QTreeWidgetItem *child)
{
    if (child == 0) return;
    LabelledPointsTreeWidgetItem *item = dynamic_cast<LabelledPointsTreeWidgetItem *>(child);
    if (child->childCount() == 0 && item && item->labelledPoints()) // this is a poinmt list
    {
        item->labelledPoints()->AddToDomDocument(doc, parent);
    }
    else // this is a folder
    {
        QDomElement folderElement = doc->createElement("LabelledPointsFolder");
        folderElement.setAttribute("name", child->data(0, Qt::DisplayRole).toString());
        parent->appendChild(folderElement);
        for (int i = 0; i < child->childCount(); i++)
        {
            WritePointsChild(doc, &folderElement, child->child(i));
        }
    }
}

bool MultiSpectralDocument::Read(const QString &fileName)
{
    Q_ASSERT(m_labelledPointsRoot);
    Q_ASSERT(m_imagesRoot);

    m_fileName = fileName;
    QFileInfo info(fileName);
    m_parentFolder = QDir::cleanPath(info.absolutePath());

    QDomDocument doc;
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(0, "Warning", QString("Unable to read MultiSpectralDocument file: %1").arg(fileName));
        return false;
    }
    if (!doc.setContent(&file))
    {
        QMessageBox::warning(0, "Warning", QString("Unable to parse MultiSpectralDocument file: %1").arg(fileName));
        file.close();
        return false;
    }
    file.close();

    QDomElement docElem = doc.documentElement();
    if (docElem.tagName() != "MultiSpectralDocument")
    {
        QMessageBox::warning(0, "Warning", QString("docElem.tagName() != \"MultiSpectralDocument\" : : %1").arg(fileName));
        return false;
    }

    QDomNode n = docElem.firstChild();
    while(!n.isNull())
    {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        if (!e.isNull())
        {
            ReadElement(&e, m_imagesRoot, m_labelledPointsRoot);
        }
        n = n.nextSibling();
    }
    return true;
}

void MultiSpectralDocument::ReadElement(const QDomElement *element, QTreeWidgetItem *imagesTreeItem, QTreeWidgetItem *labelledPointsTreeItem)
{
    if (element == 0) return;
//    qDebug("element->tagName() = %s\n", qUtf8Printable(element->tagName()));

    if (element->tagName() == "ImagesFolder")
    {
        QStringList itemStrings;
        itemStrings << element->attribute("name") << "" << "" << "" << "";
        ImageTreeWidgetItem *newParent = new ImageTreeWidgetItem(imagesTreeItem, itemStrings);
        newParent->setData(1, Qt::CheckStateRole, QVariant());
        newParent->setData(2, Qt::CheckStateRole, QVariant());
        newParent->setData(3, Qt::CheckStateRole, QVariant());
        newParent->setData(4, Qt::CheckStateRole, QVariant());
        QDomNode n = element->firstChild();
        while(!n.isNull())
        {
            QDomElement e = n.toElement(); // try to convert the node to an element.
            if (!e.isNull()) ReadElement(&e, newParent, labelledPointsTreeItem);
            n = n.nextSibling();
        }
        return;
    }
    if (element->tagName() == "LabelledPointsFolder")
    {
        QStringList itemStrings;
        itemStrings << element->attribute("name") << "" << "" << "";
        LabelledPointsTreeWidgetItem *newParent = new LabelledPointsTreeWidgetItem(labelledPointsTreeItem, itemStrings);
        newParent->setData(1, Qt::CheckStateRole, QVariant());
        newParent->setData(2, Qt::CheckStateRole, QVariant());
        newParent->setData(3, Qt::CheckStateRole, QVariant());
        QDomNode n = element->firstChild();
        while(!n.isNull())
        {
            QDomElement e = n.toElement(); // try to convert the node to an element.
            if (!e.isNull()) ReadElement(&e, imagesTreeItem, newParent);
            n = n.nextSibling();
        }
        return;
    }
    if (element->tagName() == "SingleChannelImage")
    {
        SingleChannelImage *image;
        if (SingleChannelImage::CreateFromDomElement(*element, &image, m_parentFolder))
        {
            QStringList itemStrings;
            itemStrings << image->name() << "" << "" << "" << "";
            ImageTreeWidgetItem *newImage = new ImageTreeWidgetItem(imagesTreeItem, itemStrings, image);
            Q_UNUSED(newImage);
        }
        return;
    }
    if (element->tagName() == "LabelledPoints")
    {
        LabelledPoints *labelledPoints = new LabelledPoints();
        if (labelledPoints->ReadFromDomElement(*element))
        {
            QStringList itemStrings;
            itemStrings << labelledPoints->name() << "" << "" << "";
            LabelledPointsTreeWidgetItem *newLabelledPoints = new LabelledPointsTreeWidgetItem(labelledPointsTreeItem, itemStrings, labelledPoints);
            Q_UNUSED(newLabelledPoints);
        }
        return;
    }
}

QTreeWidgetItem *MultiSpectralDocument::labelledPointsRoot() const
{
    return m_labelledPointsRoot;
}

void MultiSpectralDocument::setLabelledPointsRoot(QTreeWidgetItem *labelledPointsRoot)
{
    m_labelledPointsRoot = labelledPointsRoot;
}

QTreeWidgetItem *MultiSpectralDocument::imagesRoot() const
{
    return m_imagesRoot;
}

void MultiSpectralDocument::setImagesRoot(QTreeWidgetItem *imagesRoot)
{
    m_imagesRoot = imagesRoot;
}

LabelledPoints *MultiSpectralDocument::currentLabelledPoints() const
{
    return m_currentLabelledPoints;
}

void MultiSpectralDocument::setCurrentLabelledPoints(LabelledPoints *currentLabelledPoints)
{
    m_currentLabelledPoints = currentLabelledPoints;
}

SingleChannelImage *MultiSpectralDocument::currentImage() const
{
    return m_currentImage;
}

void MultiSpectralDocument::setCurrentImage(SingleChannelImage *currentImage)
{
    m_currentImage = currentImage;
}
