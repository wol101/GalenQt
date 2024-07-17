#ifndef SCATTERPLOTDIALOG_H
#define SCATTERPLOTDIALOG_H

#include <QDialog>

class QScatterSeries;

namespace Ui {
class ScatterPlotDialog;
}

class ScatterPlotDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ScatterPlotDialog(QWidget *parent = 0);
    ~ScatterPlotDialog();

    void addPoints(const float *x, const float *y, int n, bool lastSet);
    QColor chooseColour(int i);

//protected:
//    void closeEvent(QCloseEvent *event);

private slots:
    void updateUI();
    void cleanUp(int result);

private:
    void populateThemeBox();
    void populateAnimationBox();
    void populateLegendBox();
    void populateAntialiasBox();

    Ui::ScatterPlotDialog *ui;
    int m_currentColourIndex;
};

#endif // SCATTERPLOTDIALOG_H
