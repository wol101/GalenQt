#ifndef LDADIALOG_H
#define LDADIALOG_H

#include "LDA.h"

#include <Eigen/Dense>

#include <QDialog>
#include <QVector>
#include <QFutureWatcher>

class SingleChannelImage;
class LabelledPoints;

namespace Ui {
class LDADialog;
}

class LDADialog : public QDialog
{
    Q_OBJECT

public:
    explicit LDADialog(QWidget *parent = 0);
    ~LDADialog();

    void setInputImages(const QList<SingleChannelImage *> &inputImages);
    void setInputPoints(const QList<LabelledPoints *> &inputPoints);

    QList<SingleChannelImage *> ouputImages() const;
    QString outputFolder() const;

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void doLDAClicked();
    void cancelClicked();
    void outputFolderClicked();
    void outputFolderChanged(const QString &text);
    void ldaFinished();

private:
    void StoreSettings();
    void LoadSettings();
    void scatterPlot();

    Ui::LDADialog *ui;
    QList<SingleChannelImage *> m_inputImages;
    QList<SingleChannelImage *> m_ouputImages;
    QList<LabelledPoints *> m_inputPoints;
    int m_totalInputPoints;
    QVector<float *> m_buffersToDelete;
    LDA m_lda;
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> m_data;
    QFutureWatcher<void> m_watcher;
};

#endif // LDADIALOG_H
