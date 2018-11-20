/*
 *  SingleChannelImage.h
 *  GalenQt
 *
 *  Created by Bill Sellers on 18/10/2015.
 *  Copyright 2015 Bill Sellers. All rights reserved.
 *
 */

#ifndef SINGLECHANNELIMAGE_H
#define SINGLECHANNELIMAGE_H

#include <QString>
#include <QRect>

class QImage;
class QDomElement;
class QDomDocument;

class SingleChannelImage
{
public:
    SingleChannelImage();
    ~SingleChannelImage();

    void AllocateMemory(int width, int height, bool fill, float fillValue = 0.0f);
    void UpdateHistogram();
    void AddToDomDocument(QDomDocument *doc, QDomElement *parent, const QString &parentFolder);
    bool SaveImageToTiffFile(const QString &fileName);
    float optimalGamma();
    float *getDisplayMappedDataCopy();

    static bool CreateSingleChannelImagesFromFile(const QString &fileName, SingleChannelImage **imageRead, int numHistogramBins);
    static bool SaveAsColour8BitTiff(SingleChannelImage *redImage, SingleChannelImage *greenImage, SingleChannelImage *blueImage, const QString &fileName);
    static bool CreateFromDomElement(const QDomElement &element, SingleChannelImage **imageRead, const QString &parentFolder);
    static int BinarySearchRange(float array[], int listlen, float item); // returns the index of array[index] <= item < array[index+1] or -1 if not found
    static float quickSelect(int k, float *x, int len);

    float *binEnds() const;
    float *data() const;
    float data(int ix, int iy) const;
    int *histogram() const;
    float dataMax() const;
    float dataMin() const;
    float dataLogMin() const;
    float displayGamma() const;
    float displayMax() const;
    float displayMin() const;
    float displayZebra() const;
    bool displayRed() const;
    bool displayGreen() const;
    bool displayBlue() const;
    bool selected() const;
    QString doi() const;
    int height() const;
    int histogramMax() const;
    int histogramMin() const;
    QString name() const;
    QString notes() const;
    int numBins() const;
    int pixels() const;
    QString url() const;
    QString localPath() const;
    int width() const;
    QRect selectionRect() const;
    bool deleteLater() const;
    bool displayLogged() const;
    float permilleile(int n) const;

    void setDisplayGamma(float displayGamma);
    void setDisplayRange(float displayMin, float displayMax);
    void setDisplayZebra(float displayZebra);
    void setDisplayRed(bool displayRed);
    void setDisplayGreen(bool displayGreen);
    void setDisplayBlue(bool displayBlue);
    void setSelected(bool selected);
    void setDoi(const QString &doi);
    void setName(const QString &name);
    void setNotes(const QString &notes);
    void setNumBins(int numBins);
    void setUrl(const QString &url);
    void setLocalPath(const QString &localPath);
    void setSelectionRect(const QRect &selectionRect);
    void setDeleteLater(bool deleteLater);
    void setDisplayLogged(bool displayLogged);

private:
    // image metadata
    QString m_name;
    QString m_url;
    QString m_localPath;
    QString m_doi;
    QString m_notes;
    int m_channel;

    // display metadata
    float m_displayMin;
    float m_displayMax;
    float m_displayGamma;
    float m_displayZebra;
    bool m_displayRed;
    bool m_displayGreen;
    bool m_displayBlue;
    bool m_selected;
    bool m_displayLogged;

    // properties derived from the image
    int m_width;
    int m_height;
    int m_pixels;
    float *m_data;
    float m_dataMin;
    float m_dataMax;
    float m_dataLogMin;
    float m_permilleile[1001];
    int *m_histogram;
    float *m_binEnds;
    int m_numBins;
    int m_histogramMin;
    int m_histogramMax;
    QRect m_selectionRect;

    bool m_deleteLater;
};

#endif // SINGLECHANNELIMAGE_H
