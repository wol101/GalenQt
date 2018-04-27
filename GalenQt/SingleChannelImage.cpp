/*
 *  SingleChannelImage.cpp
 *  GalenQt
 *
 *  Created by Bill Sellers on 18/10/2015.
 *  Copyright 2015 Bill Sellers. All rights reserved.
 *
 */

#include "SingleChannelImage.h"

#include "CImg.h"

#include <QImage>
#include <QDomDocument>
#include <QDomElement>
#include <QFileInfo>
#include <QDir>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <qglobal.h>

#define CLAMP(n,lower,upper) (std::max(lower, std::min(n, upper)))
#define ALMOST_ZERO 0.00001f

SingleChannelImage::SingleChannelImage()
{
    m_channel = 0;
    m_width = 0;
    m_height = 0;
    m_pixels = 0;
    m_data = 0;
    m_dataMin = 0;
    m_dataMax = 1;
    m_validMinMax = false;
    m_numBins = 0;
    m_histogram = 0;
    m_binEnds = 0;
    m_histogramMin = 0;
    m_histogramMax = 0;
    m_validHistogram = false;
    m_displayMin = 0;
    m_displayMax = 1;
    m_dataLogMin = 0;
    m_displayGamma = 1;
    m_validDisplayMinMax = false;
    m_displayZebra = 1;
    m_displayRed = false;
    m_displayGreen = false;
    m_displayBlue = false;
    m_selected = false;
    m_displayLogged = false;
}

SingleChannelImage::~SingleChannelImage()
{
    if (m_histogram) delete [] m_histogram;
    if (m_binEnds) delete [] m_binEnds;
    if (m_data) delete [] m_data;
}

void SingleChannelImage::AllocateMemory(int width, int height, bool fill, float fillValue)
{
    if (m_data) { delete [] m_data; m_data = 0; }
    m_width = width;
    m_height = height;
    m_pixels = m_width * m_height;
    m_data = new float[m_pixels];
    if (fill) std::fill(m_data, m_data + m_pixels, fillValue);
    m_dataMin = 0;
    m_dataMax = 0;
    m_validMinMax = false;
    m_validHistogram = false;
}

void SingleChannelImage::UpdateMinMax()
{
    m_dataMin = FLT_MAX;
    m_dataLogMin = FLT_MAX;
    m_dataMax = -FLT_MAX;
    float *dataPtr = m_data;
    for (int i = 0; i < m_pixels; i++)
    {
        if (*dataPtr > m_dataMax) m_dataMax = *dataPtr;
        if (*dataPtr < m_dataMin) m_dataMin = *dataPtr;
        if (*dataPtr < m_dataLogMin && *dataPtr > 0) m_dataLogMin = *dataPtr;
        dataPtr++;
    }
    m_validMinMax = true;
}

void SingleChannelImage::UpdateHistogram()
{
    int i, index;
    if (m_validMinMax == false) UpdateMinMax();
    if ((m_dataMax - m_dataMin) <= 0 || m_numBins < 2)
    {
        m_validHistogram = false;
        return;
    }
    // get the bin ranges
    float binWidth = (m_dataMax - m_dataMin) / m_numBins;
    // this is to get around any rounding error problems with the ends of ranges
    m_binEnds[0] = nextafterf(m_dataMin, -FLT_MAX);
    m_binEnds[m_numBins] = nextafterf(m_dataMax, FLT_MAX);
    for (i = 1; i < m_numBins; i++) m_binEnds[i] = m_dataMin + (i * binWidth);
    // now get the frequencies in the bins
    std::fill(m_histogram, m_histogram + m_numBins, 0);
    float *dataPtr = m_data;
    for (i = 0; i < m_pixels; i++)
    {
        index = BinarySearchRange(m_binEnds, m_numBins + 1, *dataPtr);
        m_histogram[index]++;
        dataPtr++;
    }
    m_histogramMin = INT_MAX;
    m_histogramMax = -INT_MAX;
    for (i = 0; i < m_numBins; i++)
    {
        if (m_histogram[i] < m_histogramMin) m_histogramMin = m_histogram[i];
        if (m_histogram[i] > m_histogramMax) m_histogramMax = m_histogram[i];
    }
    m_validHistogram = true;
}

void SingleChannelImage::UpdateDisplay()
{
    if (m_validMinMax == false) UpdateMinMax();
    if (m_validDisplayMinMax == false) setDisplayRange(m_dataMin, m_dataMax);
}

void SingleChannelImage::setNumBins(int numBins)
{
    if (numBins == m_numBins) return;
    if (m_histogram) delete [] m_histogram;
    if (m_binEnds) delete [] m_binEnds;
    m_numBins = numBins;
    m_histogram = new int[m_numBins];
    m_binEnds = new float[m_numBins + 1];
    m_validHistogram = false;
}

bool SingleChannelImage::SaveImageToTiffFile(const QString &fileName)
{
    cimg_library::cimg::exception_mode(0); // enable quiet exception mode
    bool isShared = true;
    cimg_library::CImg<float> imageToWrite(m_data, m_width, m_height, 1, 1, isShared);
    unsigned int compressionType = 1; // LZW
    float *voxel_size = 0;
    char *description = 0;
    bool use_bigtiff = true;
    try
    {
        imageToWrite.save_tiff(fileName.toUtf8().constData(), compressionType, voxel_size, description, use_bigtiff); // note CImg copes with UTF8 to wchar_t using MultiByteToWideChar
    }
    catch (cimg_library::CImgException& e)
    {
        qDebug("SingleChannelImage::SaveImageToTiffFile CImg Library Error: %s\n", e.what());
        return false; //error
    }
    return true;
}

float *SingleChannelImage::getDisplayMappedDataCopy()
{
    float *newData = new float[m_pixels];
    if (m_displayLogged == false)
    {
        for (int i = 0; i < m_pixels; i++)
        {
            newData[i] = std::pow(std::fmod(CLAMP((m_data[i] - m_displayMin) / (m_displayMax - m_displayMin), 0.0f, 0.99999f) * m_displayZebra, 1.0f), m_displayGamma);
        }
    }
    else
    {
        float logDisplayMin = std::log(m_displayMin);
        float logDisplayMax = std::log(m_displayMax);
        for (int i = 0; i < m_pixels; i++)
        {
            newData[i] = std::pow(std::fmod(CLAMP((std::log(std::max(m_data[i], m_displayMin)) - logDisplayMin) / (logDisplayMax - logDisplayMin), 0.0f, 0.99999f) * m_displayZebra, 1.0f), m_displayGamma);
        }
    }
    return newData;
}

// try to create a single channel image from the filename
// copes with:
//RAW : consists in a very simple header (in ascii), then the image data.
//ASC (Ascii)
//HDR (Analyze 7.5)
//INR (Inrimage)
//PPM/PGM (Portable Pixmap)
//BMP (uncompressed)
//PAN (Pandore-5)
//DLM (Matlab ASCII)
// and also TIFF via libtiff

bool SingleChannelImage::CreateSingleChannelImagesFromFile(const QString &fileName, SingleChannelImage **imageRead, int numHistogramBins)
{
    int ix, iy, ic;
    float *imageDataPtr;
    float v;
    *imageRead = 0;

    QFileInfo fileInfo(fileName);
    cimg_library::cimg::exception_mode(0); // enable quiet exception mode
    cimg_library::CImg<float> imageFromFile;
    try
    {
        imageFromFile.load(fileName.toUtf8().constData()); // note CImg copes with UTF8 to wchar_t using MultiByteToWideChar
    }
    catch (cimg_library::CImgException& e)
    {
        qDebug("SingleChannelImage::CreateSingleChannelImagesFromFile CImg Library Error: %s\n", e.what());
        return false; //error
    }
     qDebug("Read %s width=%d height=%d depth=%d spectrum=%d\n", fileName.toUtf8().constData(), imageFromFile.width(), imageFromFile.height(), imageFromFile.depth(), imageFromFile.spectrum());
    // what sort of image is it?
    if (imageFromFile.spectrum() == 1)
    {
        // already a single channel grey scale image
        SingleChannelImage *image = new SingleChannelImage();
        image->AllocateMemory(imageFromFile.width(), imageFromFile.height(), false);
        imageDataPtr = image->data();
        image->m_dataMin = FLT_MAX;
        image->m_dataLogMin = FLT_MAX;
        image->m_dataMax = -FLT_MAX;
        float *imageFromFilePtr = imageFromFile.data();
        int size = imageFromFile.width() * imageFromFile.height();
        for (int i = 0; i < size; i++)
        {
            v = *imageFromFilePtr;
            imageFromFilePtr++;
            *imageDataPtr = v;
            imageDataPtr++;
            if (v > image->m_dataMax) image->m_dataMax = v;
            if (v < image->m_dataMin) image->m_dataMin = v;
            if (v < image->m_dataLogMin && v > 0) image->m_dataLogMin = v;
        }
        image->m_validMinMax = true;
        image->setNumBins(numHistogramBins);
        image->setLocalPath(QDir::cleanPath(fileInfo.absoluteFilePath()));
        image->UpdateHistogram();
        image->UpdateDisplay();
        *imageRead = image;
    }
    else
    {
        // this is a multiple channel image so simply sum the pixel data
        SingleChannelImage *image = new SingleChannelImage();
        image->AllocateMemory(imageFromFile.width(), imageFromFile.height(), false);
        imageDataPtr = image->data();
        image->m_dataMin = FLT_MAX;
        image->m_dataLogMin = FLT_MAX;
        image->m_dataMax = -FLT_MAX;
        for (iy = 0; iy < imageFromFile.height(); iy++)
        {
            for (ix = 0; ix < imageFromFile.width(); ix++)
            {
                v = 0;
                for (ic = 0; ic < imageFromFile.spectrum(); ic++) v += imageFromFile.atXYZC(ix, iy, 0, ic);
                *imageDataPtr = v;
                imageDataPtr++;
                if (v > image->m_dataMax) image->m_dataMax = v;
                if (v < image->m_dataMin) image->m_dataMin = v;
                if (v < image->m_dataLogMin && v > 0) image->m_dataLogMin = v;
            }
        }
        image->m_validMinMax = true;
        image->setNumBins(numHistogramBins);
        image->setLocalPath(QDir::cleanPath(fileInfo.absoluteFilePath()));
        image->UpdateHistogram();
        image->UpdateDisplay();
        *imageRead = image;
    }
    return true; // success
}

bool SingleChannelImage::SaveAsColour8BitTiff(SingleChannelImage *redImage, SingleChannelImage *greenImage, SingleChannelImage *blueImage, const QString &fileName)
{
    if (redImage == 0 && greenImage == 0 && blueImage == 0) { qDebug("SingleChannelImage::SaveAsColour8BitTiff: No image data so could not export."); return false; }
    bool status = true; // set to flase on error
    float *redImageDisplay = 0;
    float *greenImageDisplay = 0;
    float *blueImageDisplay = 0;
    int pixels = 0, width = 0, height = 0;
    if (redImage)
    {
        redImageDisplay = redImage->getDisplayMappedDataCopy();
        pixels = redImage->pixels();
        width = redImage->width();
        height = redImage->height();
        if (greenImage) greenImageDisplay = greenImage->getDisplayMappedDataCopy();
        if (blueImage) blueImageDisplay = blueImage->getDisplayMappedDataCopy();
    }
    else
    {
        if (greenImage)
        {
            greenImageDisplay = greenImage->getDisplayMappedDataCopy();
            pixels = greenImage->pixels();
            width = greenImage->width();
            height = greenImage->height();
            if (blueImage) blueImageDisplay = blueImage->getDisplayMappedDataCopy();
        }
        else
        {
            blueImageDisplay = blueImage->getDisplayMappedDataCopy();
            pixels = blueImage->pixels();
            width = blueImage->width();
            height = blueImage->height();
        }
    }

    // cimg pixel data is (width*height*depth*dim). So, a color image with dim=3 and depth=1, will be stored in memory as
    // R1R2R3R4R5R6......G1G2G3G4G5G6.......B1B2B3B4B5B6.... (i.e following a 'planar' structure) where R1 = img(0,0,0,0)
    // is the first upper-left pixel of the red component of the image
    uint8_t *imageBuffer = new uint8_t[pixels * 3];
    for (int i = 0; i < pixels; i++)
    {
        if (redImage) imageBuffer[i] = static_cast<uint8_t>(redImageDisplay[i] * 259.999f);
        else imageBuffer[i] = 0;
        if (greenImage) imageBuffer[i + pixels] = static_cast<uint8_t>(greenImageDisplay[i] * 259.999f);
        else imageBuffer[i + pixels] = 0;
        if (blueImage) imageBuffer[i + pixels + pixels] = static_cast<uint8_t>(blueImageDisplay[i] * 259.999f);
        else imageBuffer[i + 2 * pixels] = 0;
    }
    cimg_library::cimg::exception_mode(0); // enable quiet exception mode
    bool isShared = true;
    cimg_library::CImg<uint8_t> imageToWrite(imageBuffer, width, height, 1, 3, isShared);
    unsigned int compressionType = 1; // LZW
    float *voxel_size = 0;
    char *description = 0;
    bool use_bigtiff = true;
    try
    {
        imageToWrite.save_tiff(fileName.toUtf8().constData(), compressionType, voxel_size, description, use_bigtiff); // note CImg copes with UTF8 to wchar_t using MultiByteToWideChar
    }
    catch (cimg_library::CImgException& e)
    {
        qDebug("Error exporting TIFF file %s: %s", qUtf8Printable(fileName), e.what());
        status =  false; //error
    }
    if (redImageDisplay) delete [] redImageDisplay;
    if (greenImageDisplay) delete [] greenImageDisplay;
    if (blueImageDisplay) delete [] blueImageDisplay;
    delete [] imageBuffer;
    return status;
}

// return the index of a matching item in a sorted array
// special case when I'm searching for a range rather than an exact match
// returns the index of array[index] <= item < array[index+1]
int SingleChannelImage::BinarySearchRange(float array[], int listlen, float item)
{
    int first = 0;
    int last = listlen-1;
    int mid;
    while (first <= last)
    {
        mid = (first + last) / 2;
        if (array[mid + 1] <= item) first = mid + 1;
        else if (array[mid] > item) last = mid - 1;
        else return mid;
    }
    return -1;
}

void SingleChannelImage::AddToDomDocument(QDomDocument *doc, QDomElement *parent, const QString &parentFolder)
{
    QDir dir(parentFolder);
    QString relativePath = dir.relativeFilePath(m_localPath);

    QDomElement imageElement = doc->createElement("SingleChannelImage");
    parent->appendChild(imageElement);

    imageElement.setAttribute("name", m_name);
    imageElement.setAttribute("url", m_url);
    imageElement.setAttribute("localPath", relativePath);
    imageElement.setAttribute("doi", m_doi);
    imageElement.setAttribute("notes", m_notes);

    imageElement.setAttribute("displayMin", m_displayMin);
    imageElement.setAttribute("displayMax", m_displayMax);
    imageElement.setAttribute("displayGamma", m_displayGamma);
    imageElement.setAttribute("displayZebra", m_displayZebra);
    imageElement.setAttribute("displayRed", m_displayRed);
    imageElement.setAttribute("displayGreen", m_displayGreen);
    imageElement.setAttribute("displayBlue", m_displayBlue);
    imageElement.setAttribute("displayLogged", m_displayLogged);
    imageElement.setAttribute("selected", m_selected);
    imageElement.setAttribute("numBins", m_numBins);
}

bool SingleChannelImage::CreateFromDomElement(const QDomElement &element, SingleChannelImage **imageRead, const QString &parentFolder)
{
    QString relativePath = element.attribute("localPath");
    QDir dir(parentFolder);
    QString localPath = QDir::cleanPath(dir.absoluteFilePath(relativePath));

    int numHistogramBins = element.attribute("numBins").toInt();
    bool err = SingleChannelImage::CreateSingleChannelImagesFromFile(localPath, imageRead, numHistogramBins);
    if (err == false) return err;

    (*imageRead)->setName(element.attribute("name"));
    (*imageRead)->setUrl(element.attribute("url"));
    (*imageRead)->setLocalPath(localPath);
    (*imageRead)->setDoi(element.attribute("doi"));
    (*imageRead)->setNotes(element.attribute("notes"));

    (*imageRead)->setDisplayRange(element.attribute("displayMin").toDouble(), element.attribute("displayMax").toDouble());
    (*imageRead)->setDisplayGamma(element.attribute("displayGamma").toDouble());
    (*imageRead)->setDisplayZebra(element.attribute("displayZebra").toDouble());
    (*imageRead)->setDisplayRed(element.attribute("displayRed").toInt());
    (*imageRead)->setDisplayGreen(element.attribute("displayGreen").toInt());
    (*imageRead)->setDisplayBlue(element.attribute("displayBlue").toInt());
    (*imageRead)->setDisplayLogged(element.attribute("displayLogged").toInt());
    (*imageRead)->setSelected(element.attribute("selected").toInt());

    return err; // success is true, error is false
}

float SingleChannelImage::optimalGamma()
{
    // that routine calculates the gamma value that sets the median of the transformed data to be at the middle of the range
    float *tempData = new float[m_pixels];
    std::memcpy(tempData, m_data, m_pixels * sizeof(float));
    float medianPixel = quickSelect(m_pixels / 2, tempData, m_pixels); // quickselect is moderately quick but destructive so we have to copy the data first
    delete [] tempData;
    if (medianPixel == 0) medianPixel = 1; // otherwise we end up with a gamma of zero which is not helpful
    float normMedianPixel = (float(medianPixel) - m_dataMin) / (m_dataMax - m_dataMin);
    float optimalGamma = std::log(0.5f) / std::log(normMedianPixel);
    return optimalGamma;
}

/* Return the k-th smallest item in array x of length len */
#define quickSelect_swapElement(a, b) { float t = x[a]; x[a] = x[b], x[b] = t; }
float SingleChannelImage::quickSelect(int k, float *x, int len)
{
   int left = 0, right = len - 1;
   int pos, i;
   float pivot;

   while (left < right)
   {
      pivot = x[k];
      quickSelect_swapElement(k, right);
      for (i = pos = left; i < right; i++)
      {
         if (x[i] < pivot)
         {
            quickSelect_swapElement(i, pos);
            pos++;
         }
      }
      quickSelect_swapElement(right, pos);
      if (pos == k) break;
      if (pos < k) left = pos + 1;
      else right = pos - 1;
   }
   return x[k];
}

void SingleChannelImage::setDisplayRange(float displayMin, float displayMax)
{
    m_displayMin = displayMin;
    m_displayMax = displayMax;
    m_validDisplayMinMax = true;
}


QString SingleChannelImage::name() const
{
    return m_name;
}

void SingleChannelImage::setName(const QString &name)
{
    m_name = name;
}

QString SingleChannelImage::url() const
{
    return m_url;
}

void SingleChannelImage::setUrl(const QString &url)
{
    m_url = url;
}

QString SingleChannelImage::doi() const
{
    return m_doi;
}

void SingleChannelImage::setDoi(const QString &doi)
{
    m_doi = doi;
}

QString SingleChannelImage::notes() const
{
    return m_notes;
}

void SingleChannelImage::setNotes(const QString &notes)
{
    m_notes = notes;
}

float SingleChannelImage::displayMin() const
{
    return m_displayMin;
}

float SingleChannelImage::displayMax() const
{
    return m_displayMax;
}

float SingleChannelImage::displayGamma() const
{
    return m_displayGamma;
}

void SingleChannelImage::setDisplayGamma(float displayGamma)
{
    m_displayGamma = displayGamma;
}

float SingleChannelImage::displayZebra() const
{
    return m_displayZebra;
}

void SingleChannelImage::setDisplayZebra(float displayZebra)
{
    m_displayZebra = displayZebra;
}

QRect SingleChannelImage::selectionRect() const
{
    return m_selectionRect;
}

void SingleChannelImage::setSelectionRect(const QRect &selectionRect)
{
    m_selectionRect = selectionRect;
}

bool SingleChannelImage::deleteLater() const
{
    return m_deleteLater;
}

void SingleChannelImage::setDeleteLater(bool deleteLater)
{
    m_deleteLater = deleteLater;
}

bool SingleChannelImage::displayLogged() const
{
    return m_displayLogged;
}

void SingleChannelImage::setDisplayLogged(bool displayLogged)
{
    m_displayLogged = displayLogged;
    if (m_displayLogged) m_displayMin = std::max(m_displayMin, m_dataLogMin);
}

float SingleChannelImage::dataLogMin() const
{
    return m_dataLogMin;
}

bool SingleChannelImage::displayRed() const
{
    return m_displayRed;
}

void SingleChannelImage::setDisplayRed(bool displayRed)
{
    m_displayRed = displayRed;
}

bool SingleChannelImage::displayGreen() const
{
    return m_displayGreen;
}

void SingleChannelImage::setDisplayGreen(bool displayGreen)
{
    m_displayGreen = displayGreen;
}

bool SingleChannelImage::displayBlue() const
{
    return m_displayBlue;
}

void SingleChannelImage::setDisplayBlue(bool displayBlue)
{
    m_displayBlue = displayBlue;
}

bool SingleChannelImage::selected() const
{
    return m_selected;
}

void SingleChannelImage::setSelected(bool selected)
{
    m_selected = selected;
}

QString SingleChannelImage::localPath() const
{
    return m_localPath;
}

void SingleChannelImage::setLocalPath(const QString &localPath)
{
    m_localPath = localPath;
}

int SingleChannelImage::width() const
{
    return m_width;
}

int SingleChannelImage::height() const
{
    return m_height;
}

int SingleChannelImage::pixels() const
{
    return m_pixels;
}

float *SingleChannelImage::data() const
{
    return m_data;
}

float SingleChannelImage::data(int ix, int iy) const
{
    return m_data[ix + iy * m_width];
}


float SingleChannelImage::dataMin() const
{
    return m_dataMin;
}

float SingleChannelImage::dataMax() const
{
    return m_dataMax;
}

bool SingleChannelImage::validMinMax() const
{
    return m_validMinMax;
}

int *SingleChannelImage::histogram() const
{
    return m_histogram;
}

float *SingleChannelImage::binEnds() const
{
    return m_binEnds;
}

int SingleChannelImage::numBins() const
{
    return m_numBins;
}

int SingleChannelImage::histogramMin() const
{
    return m_histogramMin;
}

int SingleChannelImage::histogramMax() const
{
    return m_histogramMax;
}

bool SingleChannelImage::validHistogram() const
{
    return m_validHistogram;
}

bool SingleChannelImage::validDisplayMinMax() const
{
    return m_validDisplayMinMax;
}

