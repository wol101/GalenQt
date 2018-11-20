#ifndef PCADIALOG_H
#define PCADIALOG_H

#include "PCA.h"

#include <Eigen/Dense>

#include <QDialog>
#include <QVector>
#include <QFutureWatcher>

class SingleChannelImage;

namespace Ui {
class PCADialog;
}

class PCADialog : public QDialog
{
    Q_OBJECT

public:
    explicit PCADialog(QWidget *parent = 0);
    ~PCADialog();

    void setInputImages(const QList<SingleChannelImage *> &inputImages);

    QList<SingleChannelImage *> ouputImages() const;
    QString outputFolder() const;

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void doPCAClicked();
    void cancelClicked();
    void outputFolderClicked();
    void outputFolderChanged(const QString &text);
    void pcaFinished();

private:
    void StoreSettings();
    void LoadSettings();

    Ui::PCADialog *ui;
    QList<SingleChannelImage *> m_inputImages;
    QList<SingleChannelImage *> m_ouputImages;
    QVector<float *> m_buffersToDelete;
    PCA m_pca;
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> m_data;
    QFutureWatcher<void> m_watcher;
};

#endif // PCADIALOG_H
