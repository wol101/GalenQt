#ifndef HDF5READERDIALOG_H
#define HDF5READERDIALOG_H

#include "hdf5.h"

#include <QDialog>
#include <QStringList>

class SingleChannelImage;

namespace Ui {
class HDF5ReaderDialog;
}

class HDF5ReaderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HDF5ReaderDialog(QWidget *parent = 0);
    ~HDF5ReaderDialog();

    void mapData(float *inputData, float **outputData);


    float *data() const;

    int rank() const;

    hsize_t *dims() const;

    hsize_t size() const;

    QList<SingleChannelImage *> ouputImages() const;

private slots:
    void okClicked();
    void cancelClicked();
    void inputFileClicked();
    void outputFileChanged(const QString &text);
    void outputFolderClicked();
    void outputFolderChanged(const QString &text);
    void infoTableItemSelected();

private:
    void do_dtype(hid_t);
    void do_dset(hid_t);
    void do_link(hid_t, char *);
    void scan_group(hid_t);
    void do_attr(hid_t);
    void scan_attrs(hid_t);
    void do_plist(hid_t);
    int print_info(const char *filename);
    bool readData();
    void writeTiffFiles();

    QStringList m_informationLines;
    QStringList m_dataSetNames;
    QVector<hsize_t> m_dataSetSizes;
    QList<SingleChannelImage *> m_ouputImages;
    QString m_datasetPath;
    float *m_data;
    int m_rank;
    hsize_t *m_dims;
    hsize_t m_size;
    int m_nx, m_ny, m_nz; // dimensions of the data
    int m_xs, m_ys, m_zs; // stride of the data

    Ui::HDF5ReaderDialog *ui;
};

#endif // HDF5READERDIALOG_H
