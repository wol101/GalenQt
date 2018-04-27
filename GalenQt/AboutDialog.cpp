#include "AboutDialog.h"
#include "ui_AboutDialog.h"

#include "Settings.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
#ifdef Q_OS_MACOS
    // on MacOS High Sierra (and possibly others), the Qt::Dialog flag disables resizing
    // if I clear the Qt::Dialog flag and make sure the Qt::Window flag is set (it should be already)
    // then it seems to let me have resizing without any other side effects
    setWindowFlags(windowFlags() & (~Qt::Dialog) | Qt::Window);
#endif
    ui->setupUi(this);
    restoreGeometry(Settings::value("_AboutDialogGeometry", QByteArray()).toByteArray());
}

AboutDialog::~AboutDialog()
{
    Settings::setValue("_PreferencesDialogGeometry", saveGeometry());
    delete ui;
}
