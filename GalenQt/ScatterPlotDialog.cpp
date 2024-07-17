#include "ScatterPlotDialog.h"
#include "ui_ScatterPlotDialog.h"

#include "Settings.h"

#include <QChartView>
#include <QScatterSeries>

ScatterPlotDialog::ScatterPlotDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ScatterPlotDialog)
{
    ui->setupUi(this);

#ifdef Q_OS_MACOS
    // on MacOS High Sierra (and possibly others), the Qt::Dialog flag disables resizing
    // if I clear the Qt::Dialog flag and make sure the Qt::Window flag is set (it should be already)
    // then it seems to let me have resizing without any other side effects
    setWindowFlags(windowFlags() & (~Qt::Dialog) | Qt::Window);
#endif

    populateThemeBox();
    populateAnimationBox();
    populateLegendBox();
    populateAntialiasBox();
    updateUI();

    connect(ui->themeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateUI()));
    connect(ui->legendComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateUI()));
    connect(ui->animatedComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateUI()));
    connect(ui->antialiasCheckBox, SIGNAL(stateChanged(int)), this, SLOT(updateUI()));

    connect(this, SIGNAL(finished(int)), this, SLOT(cleanUp(int)));

    restoreGeometry(Settings::value("_ScatterPlotDialogGeometry", QByteArray()).toByteArray());
}

ScatterPlotDialog::~ScatterPlotDialog()
{
    delete ui;
}

//void ScatterPlotDialog::closeEvent(QCloseEvent *event)
//{
//    Settings::setValue("_ScatterPlotDialogGeometry", saveGeometry());
//    Settings::setValue("_ScatterPlotTheme", ui->themeComboBox->currentIndex());
//    Settings::setValue("_ScatterPlotAnimation", ui->animatedComboBox->currentIndex());
//    Settings::setValue("_ScatterPlotLegend", ui->legendComboBox->currentIndex());
//    Settings::setValue("_ScatterPlotAntialias", ui->antialiasCheckBox->isChecked());
//    Settings::sync();
//    event->accept();
//}

void ScatterPlotDialog::cleanUp(int result)
{
    Q_UNUSED(result);
    Settings::setValue("_ScatterPlotDialogGeometry", saveGeometry());
    Settings::setValue("_ScatterPlotTheme", ui->themeComboBox->currentIndex());
    Settings::setValue("_ScatterPlotAnimation", ui->animatedComboBox->currentIndex());
    Settings::setValue("_ScatterPlotLegend", ui->legendComboBox->currentIndex());
    Settings::setValue("_ScatterPlotAntialias", ui->antialiasCheckBox->isChecked());
    Settings::sync();
}

void ScatterPlotDialog::addPoints(const float *x, const float *y, int n, bool lastSet)
{
    QScatterSeries *series = new QScatterSeries();
    for (int i = 0; i < n; i++) series->append(x[i], y[i]);
    series->setMarkerShape(QScatterSeries::MarkerShapeCircle);
    ui->chartView->chart()->addSeries(series);
    if (lastSet)
    {
        ui->chartView->chart()->createDefaultAxes(); // default axes are tight to the data - I might want to fix this
        updateUI();
    }
}

void ScatterPlotDialog::populateThemeBox()
{
    // add items to theme combobox
    ui->themeComboBox->addItem("Light", QChart::ChartThemeLight);
    ui->themeComboBox->addItem("Blue Cerulean", QChart::ChartThemeBlueCerulean);
    ui->themeComboBox->addItem("Dark", QChart::ChartThemeDark);
    ui->themeComboBox->addItem("Brown Sand", QChart::ChartThemeBrownSand);
    ui->themeComboBox->addItem("Blue NCS", QChart::ChartThemeBlueNcs);
    ui->themeComboBox->addItem("High Contrast", QChart::ChartThemeHighContrast);
    ui->themeComboBox->addItem("Blue Icy", QChart::ChartThemeBlueIcy);
    ui->themeComboBox->addItem("Qt", QChart::ChartThemeQt);
    ui->themeComboBox->setCurrentIndex(Settings::value("_ScatterPlotTheme", 0).toInt());
}

void ScatterPlotDialog::populateAnimationBox()
{
    // add items to animation combobox
    ui->animatedComboBox->addItem("No Animations", QChart::NoAnimation);
    ui->animatedComboBox->addItem("GridAxis Animations", QChart::GridAxisAnimations);
    ui->animatedComboBox->addItem("Series Animations", QChart::SeriesAnimations);
    ui->animatedComboBox->addItem("All Animations", QChart::AllAnimations);
    ui->animatedComboBox->setCurrentIndex(Settings::value("_ScatterPlotAnimation", 0).toInt());
}

void ScatterPlotDialog::populateLegendBox()
{
    // add items to legend combobox
    ui->legendComboBox->addItem("No Legend ", 0);
    ui->legendComboBox->addItem("Legend Top", Qt::AlignTop);
    ui->legendComboBox->addItem("Legend Bottom", Qt::AlignBottom);
    ui->legendComboBox->addItem("Legend Left", Qt::AlignLeft);
    ui->legendComboBox->addItem("Legend Right", Qt::AlignRight);
    ui->legendComboBox->setCurrentIndex(Settings::value("_ScatterPlotLegend", 0).toInt());
}

void ScatterPlotDialog::populateAntialiasBox()
{
    // set the antialias checkbox
    ui->antialiasCheckBox->setChecked(Settings::value("_ScatterPlotAntialias", true).toBool());
}


void ScatterPlotDialog::updateUI()
{
    QChart::ChartTheme theme = static_cast<QChart::ChartTheme>(ui->themeComboBox->itemData(ui->themeComboBox->currentIndex()).toInt());
    ui->chartView->chart()->setTheme(theme);

    // Set palette colors based on selected theme
    QPalette pal = window()->palette();
    if (theme == QChart::ChartThemeLight) {
        pal.setColor(QPalette::Window, QRgb(0xf0f0f0));
        pal.setColor(QPalette::WindowText, QRgb(0x404044));
    } else if (theme == QChart::ChartThemeDark) {
        pal.setColor(QPalette::Window, QRgb(0x121218));
        pal.setColor(QPalette::WindowText, QRgb(0xd6d6d6));
    } else if (theme == QChart::ChartThemeBlueCerulean) {
        pal.setColor(QPalette::Window, QRgb(0x40434a));
        pal.setColor(QPalette::WindowText, QRgb(0xd6d6d6));
    } else if (theme == QChart::ChartThemeBrownSand) {
        pal.setColor(QPalette::Window, QRgb(0x9e8965));
        pal.setColor(QPalette::WindowText, QRgb(0x404044));
    } else if (theme == QChart::ChartThemeBlueNcs) {
        pal.setColor(QPalette::Window, QRgb(0x018bba));
        pal.setColor(QPalette::WindowText, QRgb(0x404044));
    } else if (theme == QChart::ChartThemeHighContrast) {
        pal.setColor(QPalette::Window, QRgb(0xffab03));
        pal.setColor(QPalette::WindowText, QRgb(0x181818));
    } else if (theme == QChart::ChartThemeBlueIcy) {
        pal.setColor(QPalette::Window, QRgb(0xcee7f0));
        pal.setColor(QPalette::WindowText, QRgb(0x404044));
    } else {
        pal.setColor(QPalette::Window, QRgb(0xf0f0f0));
        pal.setColor(QPalette::WindowText, QRgb(0x404044));
    }
    window()->setPalette(pal);

    // Update antialiasing
    bool checked = ui->antialiasCheckBox->isChecked();
    ui->chartView->setRenderHint(QPainter::Antialiasing, checked);

    // Update animation options
    QChart::AnimationOptions options(ui->animatedComboBox->itemData(ui->animatedComboBox->currentIndex()).toInt());
    ui->chartView->chart()->setAnimationOptions(options);

    // Update legend alignment
    Qt::Alignment alignment(ui->legendComboBox->itemData(ui->legendComboBox->currentIndex()).toInt());

    if (!alignment) {
        ui->chartView->chart()->legend()->hide();
    } else {
        ui->chartView->chart()->legend()->setAlignment(alignment);
        ui->chartView->chart()->legend()->show();
    }
}

QColor ScatterPlotDialog::chooseColour(int i)
{
    static uint8_t rgb[] = {
        230, 25, 75, // Red
        60, 180, 75, // Green
        255, 225, 25, // Yellow
        0, 130, 200, // Blue
        245, 130, 48, // Orange
        145, 30, 180, // Purple
        70, 240, 240, // Cyan
        240, 50, 230, // Magenta
        210, 245, 60, // Lime
        250, 190, 190, // Pink
        0, 128, 128, // Teal
        230, 190, 255, // Lavender
        170, 110, 40, // Brown
        255, 250, 200, // Beige
        128, 0, 0, // Maroon
        170, 255, 195, // Mint
        128, 128, 0, // Olive
        255, 215, 180, // Coral
        0, 0, 128, // Navy
        128, 128, 128, // Grey
        255, 255, 255, // White
        0, 0, 0 // Black
    };
    int index = (i % (sizeof(rgb) / 3)) * 3;
    return QColor(rgb[index], rgb[index + 1], rgb[index + 2]);
}
