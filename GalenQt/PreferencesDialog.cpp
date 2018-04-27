#include "PreferencesDialog.h"
#include "Settings.h"

#include <QColorDialog>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QMultiMap>
#include <QScrollArea>
#include <QFileDialog>
#include <QMenu>
#include <QAction>
#include <QDomDocument>
#include <QDomElement>
#include <QDebug>
#include <QMessageBox>
#include <QCloseEvent>
#include <QStatusBar>
#include <QFontDialog>

#include <cmath>

PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent)
{
    qDebug("Window flags\n%s\n", qUtf8Printable(windowFlagsToText(windowFlags())));
#ifdef Q_OS_MACOS
    // on MacOS High Sierra (and possibly others), the Qt::Dialog flag disables resizing
    // if I clear the Qt::Dialog flag and make sure the Qt::Window flag is set (it should be already)
    // then it seems to let me have resizing without any other side effects
    setWindowFlags(windowFlags() & (~Qt::Dialog) | Qt::Window);
#endif

    setWindowTitle(tr("Preferences Dialog"));
    setSizeGripEnabled(true);

    // store the current preferences
    const QStringList keys = Settings::allKeys();
    for (int i = 0; i < keys.size(); i++) m_oldPreferences[keys[i]] = Settings::value(keys[i], QVariant());

    InitialiseContents();
    restoreGeometry(Settings::value("_PreferencesDialogGeometry", QByteArray()).toByteArray());
}

PreferencesDialog::~PreferencesDialog()
{
}

void PreferencesDialog::StoreCurrentSettings()
{
    for (int i = 0; i < m_SettingsWidgetList.size(); i++)
    {
        QVariant item = m_SettingsWidgetList[i].item;
        QString key = m_SettingsWidgetList[i].key;
        QWidget *widget = m_SettingsWidgetList[i].widget;

        if (item.type() == QMetaType::Int) Settings::setValue(key, dynamic_cast<QLineEdit *>(widget)->text().toInt());
        if (item.type() == QMetaType::Double) Settings::setValue(key, dynamic_cast<QLineEdit *>(widget)->text().toDouble());
        if (item.type() == QMetaType::Float) Settings::setValue(key, dynamic_cast<QLineEdit *>(widget)->text().toFloat());
        if (item.type() == QMetaType::QString) Settings::setValue(key, dynamic_cast<QLineEdit *>(widget)->text());
        if (item.type() == QMetaType::Bool) Settings::setValue(key, dynamic_cast<QCheckBox *>(widget)->isChecked());
        if (item.type() == QMetaType::QColor) Settings::setValue(key, item);
        if (item.type() == QMetaType::QFont) Settings::setValue(key, item);
    }
    Settings::setValue("_PreferencesDialogGeometry", saveGeometry());
    Settings::sync();
}

void PreferencesDialog::InitialiseContents()
{
    const QString COLOUR_STYLE("QPushButton { background-color : %1; color : %2; border: 4px solid %3; }");

    QVBoxLayout *verticalLayout;
    QScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QGridLayout *gridLayout;
    QDialogButtonBox *buttonBox;

    verticalLayout = new QVBoxLayout(this);
    verticalLayout->setSpacing(6);
    verticalLayout->setContentsMargins(11, 11, 11, 11);
    verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
    scrollArea = new QScrollArea(this);
    scrollArea->setObjectName(QStringLiteral("scrollArea"));
    scrollArea->setWidgetResizable(true);
    scrollAreaWidgetContents = new QWidget();
    scrollAreaWidgetContents->setObjectName(QStringLiteral("scrollAreaWidgetContents"));
    gridLayout = new QGridLayout(scrollAreaWidgetContents);
    gridLayout->setSpacing(6);
    gridLayout->setContentsMargins(11, 11, 11, 11);
    gridLayout->setObjectName(QStringLiteral("gridLayout"));

    QStringList keys = Settings::allKeys();
    keys.sort(Qt::CaseInsensitive);
    int row = 0;
    SettingsWidget settingsWidget;
    for (int i = 0; i < keys.size(); i++)
    {
        if (keys.at(i).startsWith("_")) continue;
        settingsWidget.key = keys[i];
        settingsWidget.item = Settings::value(settingsWidget.key, QVariant());

        QLabel *label = new QLabel();
        label->setText(settingsWidget.key);
        gridLayout->addWidget(label, row, 0);

//        // There is a bug in QSettings that means that it loses the type when saving bool values and ends up saving them as strings
//        if (settingsWidget.item.type() == QMetaType::QString)
//        {
//            if (settingsWidget.item.toString().compare("false", Qt::CaseInsensitive) || settingsWidget.item.toString().compare("true", Qt::CaseInsensitive))
//                settingsWidget.item.convert(QMetaType::Bool);
//        }

        QLineEdit *lineEdit;
        switch (settingsWidget.item.type())
        {
        case QMetaType::Int:
        {
            lineEdit = new QLineEdit();
            lineEdit->setMinimumWidth(200);
            lineEdit->setText(QString("%1").arg(settingsWidget.item.toInt()));
            gridLayout->addWidget(lineEdit, row, 1);
            settingsWidget.widget = lineEdit;
            m_SettingsWidgetList.append(settingsWidget);
            break;
        }

        case QMetaType::Double:
        {
            lineEdit = new QLineEdit();
            lineEdit->setMinimumWidth(200);
            lineEdit->setText(QString("%1").arg(settingsWidget.item.toDouble()));
            gridLayout->addWidget(lineEdit, row, 1);
            settingsWidget.widget = lineEdit;
            m_SettingsWidgetList.append(settingsWidget);
            break;
        }

        case QMetaType::Float:
        {
            lineEdit = new QLineEdit();
            lineEdit->setMinimumWidth(200);
            lineEdit->setText(QString("%1").arg(settingsWidget.item.toFloat()));
            gridLayout->addWidget(lineEdit, row, 1);
            settingsWidget.widget = lineEdit;
            m_SettingsWidgetList.append(settingsWidget);
            break;
        }

        case QMetaType::QString:
        {
            lineEdit = new QLineEdit();
            lineEdit->setMinimumWidth(200);
            lineEdit->setText(QString("%1").arg(settingsWidget.item.toString()));
            gridLayout->addWidget(lineEdit, row, 1);
            settingsWidget.widget = lineEdit;
            m_SettingsWidgetList.append(settingsWidget);
            lineEdit->setContextMenuPolicy(Qt::CustomContextMenu);
            QObject::connect(lineEdit, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(menuRequestPath(QPoint)));
            break;
        }

        case QMetaType::Bool:
        {
            QCheckBox *checkBox = new QCheckBox();
            checkBox->setChecked(settingsWidget.item.toBool());
            gridLayout->addWidget(checkBox, row, 1);
            settingsWidget.widget = checkBox;
            m_SettingsWidgetList.append(settingsWidget);
            break;
        }

        case QMetaType::QColor:
        {
            QPushButton *pushButton = new QPushButton();
            pushButton->setText("Colour");
            QColor color = qvariant_cast<QColor>(settingsWidget.item);
            pushButton->setStyleSheet(COLOUR_STYLE.arg(color.name()).arg(getIdealTextColour(color).name()).arg(getAlphaColourHint(color).name()));
            connect(pushButton, SIGNAL(clicked()), this, SLOT(colourButtonClicked()));
            gridLayout->addWidget(pushButton, row, 1);
            settingsWidget.widget = pushButton;
            m_SettingsWidgetList.append(settingsWidget);
            break;
        }

        case QMetaType::QFont:
        {
            QPushButton *pushButton = new QPushButton();
            pushButton->setText("Font");
            QFont font = qvariant_cast<QFont>(settingsWidget.item);
            pushButton->setFont(font);
            connect(pushButton, SIGNAL(clicked()), this, SLOT(fontButtonClicked()));
            gridLayout->addWidget(pushButton, row, 1);
            settingsWidget.widget = pushButton;
            m_SettingsWidgetList.append(settingsWidget);
            break;
        }

        default: // treat as string
        {
            qDebug("PreferencesDialog::InitialiseContents unexpected type = %s", settingsWidget.item.typeName());
            lineEdit = new QLineEdit();
            lineEdit->setMinimumWidth(200);
            lineEdit->setText(QString("%1").arg(settingsWidget.item.toString()));
            gridLayout->addWidget(lineEdit, row, 1);
            settingsWidget.widget = lineEdit;
            m_SettingsWidgetList.append(settingsWidget);
            lineEdit->setContextMenuPolicy(Qt::CustomContextMenu);
            QObject::connect(lineEdit, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(menuRequestPath(QPoint)));
            break;
        }
        }

        row++;

    }

    scrollArea->setWidget(scrollAreaWidgetContents);
    verticalLayout->addWidget(scrollArea);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->setObjectName(QStringLiteral("buttonBox"));
    QPushButton *importButton = buttonBox->addButton(tr("&Import..."), QDialogButtonBox::ActionRole);
    QPushButton *exportButton = buttonBox->addButton(tr("&Export..."), QDialogButtonBox::ActionRole);
    QPushButton *defaultsButton = buttonBox->addButton(tr("&Defaults"), QDialogButtonBox::ActionRole);
    verticalLayout->addWidget(buttonBox);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(acceptButtonClicked()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(rejectButtonClicked()));
    connect(importButton, SIGNAL(clicked()), this, SLOT(importButtonClicked()));
    connect(exportButton, SIGNAL(clicked()), this, SLOT(exportButtonClicked()));
    connect(defaultsButton, SIGNAL(clicked()), this, SLOT(defaultsButtonClicked()));
}

void PreferencesDialog::DeleteContents()
{
    delete layout();
}


void PreferencesDialog::colourButtonClicked()
{
    const QString COLOUR_STYLE("QPushButton { background-color : %1; color : %2; border: 4px solid %3; }");
    int i;
    QPushButton *pushButton = dynamic_cast<QPushButton *>(QObject::sender());
    for (i = 0; i < m_SettingsWidgetList.size(); i++)
        if (m_SettingsWidgetList[i].widget == pushButton) break;
    if (i >= m_SettingsWidgetList.size()) return;

    QColor colour = QColorDialog::getColor(qvariant_cast<QColor>(m_SettingsWidgetList[i].item), this, "Select Color",
                                           QColorDialog::ShowAlphaChannel | QColorDialog::ColorDialogOption(EXTRA_COLOUR_DIALOG_OPTIONS));
    if (colour.isValid())
    {
        pushButton->setStyleSheet(COLOUR_STYLE.arg(colour.name()).arg(getIdealTextColour(colour).name()).arg(getAlphaColourHint(colour).name()));
        m_SettingsWidgetList[i].item = colour;
    }
}


// return an ideal label colour, based on the given background colour.
// Based on http://www.codeproject.com/cs/media/IdealTextColor.asp
QColor PreferencesDialog::getIdealTextColour(const QColor& rBackgroundColour)
{
    const int THRESHOLD = 105;
    int BackgroundDelta = (rBackgroundColour.red() * 0.299) + (rBackgroundColour.green() * 0.587) + (rBackgroundColour.blue() * 0.114);
    return QColor((255- BackgroundDelta < THRESHOLD) ? Qt::black : Qt::white);
}

QColor PreferencesDialog::getAlphaColourHint(const QColor& colour)
{
    // (source * Blend.SourceAlpha) + (background * Blend.InvSourceAlpha)
    QColor background;
    background.setRgbF(1.0, 1.0, 1.0);
    QColor hint;
    hint.setRedF((colour.redF() * colour.alphaF()) + (background.redF() * (1 - colour.alphaF())));
    hint.setGreenF((colour.greenF() * colour.alphaF()) + (background.greenF() * (1 - colour.alphaF())));
    hint.setBlueF((colour.blueF() * colour.alphaF()) + (background.blueF() * (1 - colour.alphaF())));
    return hint;
}

void PreferencesDialog::fontButtonClicked()
{
    int i;
    QPushButton *pushButton = dynamic_cast<QPushButton *>(QObject::sender());
    for (i = 0; i < m_SettingsWidgetList.size(); i++)
        if (m_SettingsWidgetList[i].widget == pushButton) break;
    if (i >= m_SettingsWidgetList.size()) return;

    bool ok;
    QFont font = QFontDialog::getFont(&ok, qvariant_cast<QFont>(m_SettingsWidgetList[i].item), this);
    if (ok)
    {
        pushButton->setFont(font);
        m_SettingsWidgetList[i].item = font;
    }
}


void PreferencesDialog::importButtonClicked()
{
    QString lastImportedFile = Settings::value("_LastImportedPreferencesFile", QString("")).toString();
    QString filename = QFileDialog::getOpenFileName(this, tr("Import Settings File"), lastImportedFile, tr("Exported Settings Files (*.xml)"), 0);
    if (filename.isNull() == false)
    {
        Import(filename);
        DeleteContents();
        InitialiseContents();
    }
}

void PreferencesDialog::Import(const QString &filename)
{
    Settings::clear();

    QDomDocument doc("GalenQt_0.1Preferences");
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(0, "Warning", QString("Unable to open settings export file: %1").arg(filename));
        return;
    }
    if (!doc.setContent(&file))
    {
        QMessageBox::warning(0, "Warning", QString("Unable to read settings export file: %1").arg(filename));
        return;
    }
    file.close();

    QDomElement docElem = doc.documentElement();
//    qDebug("PreferencesDialog::Import %s", qUtf8Printable(docElem.tagName()));
    if (docElem.tagName() != "PREFERENCES")
    {
        QMessageBox::warning(0, "Warning", QString("Unable to find tag PREFERENCES: %1").arg(docElem.tagName()));
        return;
    }

    QString key;
    QString value;
    QString type;
    QDomNode n = docElem.firstChild();
    while(!n.isNull())
    {
        QDomElement e = n.toElement(); // try to convert the node to an element.
//        qDebug("PreferencesDialog::Import %s", qUtf8Printable(e.tagName()));;
        if (!e.isNull())
        {
            if (e.tagName() == "SETTING")
            {
                key = e.attribute("key");
                value = e.attribute("value");
                type = e.attribute("type");
                QVariant::Type vType = QVariant::nameToType(type.toUtf8());
                QVariant variant;
                QFont font;
//                qDebug("PreferencesDialog::Import key=%s type=%d value=%s", qUtf8Printable(key), int(vType), qUtf8Printable(value));
                switch (vType)
                {
                case QMetaType::Int:
                    Settings::setValue(key, value.toInt());
                    break;
                case QMetaType::Double:
                    Settings::setValue(key, value.toDouble());
                    break;
                case QMetaType::Float:
                    Settings::setValue(key, value.toFloat());
                    break;
                case QMetaType::QString:
                    Settings::setValue(key, value);
                    break;
                case QMetaType::Bool:
                    variant = value;
                    variant.convert(QMetaType::Bool);
                    Settings::setValue(key, variant);
                    break;
                case QMetaType::QColor:
                    Settings::setValue(key, QColor(value));
                    break;
                case QMetaType::QByteArray:
                    Settings::setValue(key, QByteArray::fromBase64(QString(value).toUtf8(), QByteArray::Base64UrlEncoding));
                    break;
                case QMetaType::QFont:
                    font.fromString(value);
                    Settings::setValue(key, font);
                    break;
                default:
                    qDebug("PreferencesDialog::Import Unexpected type=%s. Treating as QString.", qUtf8Printable(type));
                    Settings::setValue(key, value);
                    break;
                }
            }
        }
        n = n.nextSibling();
    }
    Settings::setValue("_LastImportedPreferencesFile", filename);
}

void PreferencesDialog::exportButtonClicked()
{
    QString lastExportedFile = Settings::value("_LastExportedPreferencesFile", QString("")).toString();
    QString filename = QFileDialog::getSaveFileName(this, tr("Export Settings File"), lastExportedFile, tr("Exported Settings Files (*.xml)"), 0, QFileDialog::Options(EXTRA_FILE_DIALOG_OPTIONS));
    if (filename.isNull() == false)
    {
        StoreCurrentSettings();

        QDomDocument doc("GalenQt_0.1Preferences");
        QDomProcessingInstruction  pi = doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
        doc.appendChild(pi);
        QDomElement root = doc.createElement("PREFERENCES");
        doc.appendChild(root);

        // this bit of code gets the SettingsItems sorted by order
        QStringList keys = Settings::allKeys();
        keys.sort(Qt::CaseInsensitive);
        for (int i = 0; i < keys.size(); i++)
        {
            QString key = keys[i];
            QVariant item = Settings::value(key, QVariant());
//            qDebug("PreferencesDialog::exportButtonClicked %s %s %s", item.typeName(), qUtf8Printable(key), qUtf8Printable(item.toString()));
            QDomElement setting = doc.createElement("SETTING");
            root.appendChild(setting);
            setting.setAttribute("key", key);
            setting.setAttribute("type", item.typeName());
            switch (item.type())
            {
            case QMetaType::QByteArray:
                setting.setAttribute("value", QString::fromUtf8(item.toByteArray().toBase64(QByteArray::Base64UrlEncoding)));
                break;
            case QMetaType::QColor:
                setting.setAttribute("value", qvariant_cast<QColor>(item).name(QColor::HexArgb));
                break;
            case QMetaType::QFont:
                setting.setAttribute("value", qvariant_cast<QFont>(item).toString());
                break;
            default:
                setting.setAttribute("value", item.toString());
            }
        }

        QFile file(filename);
        if (!file.open(QIODevice::WriteOnly))
        {
            QMessageBox::warning(this, "Warning", QString("Unable to open settings export file: %1").arg(filename));
            return;
        }

        // and now the actual xml doc
        QByteArray xmlData = doc.toByteArray();
        qint64 bytesWritten = file.write(xmlData);
        if (bytesWritten != xmlData.size())
        {
            QMessageBox::warning(this, "Warning", QString("Unable to write settings export file: %1").arg(filename));
            file.close();
            return;
        }
        file.close();
        Settings::setValue("_LastExportedPreferencesFile", filename);
    }
}

void PreferencesDialog::defaultsButtonClicked()
{
    Import(":/data/DefaultSettings.xml");
    DeleteContents();
    InitialiseContents();
}

void PreferencesDialog::acceptButtonClicked()
{
    StoreCurrentSettings();
    accept();
}

void PreferencesDialog::rejectButtonClicked()
{
    Settings::clear();
    for(QHash<QString, QVariant>::iterator i = m_oldPreferences.begin(); i != m_oldPreferences.end(); i++) Settings::setValue(i.key(), i.value());
    Settings::sync();
    reject();
}

void PreferencesDialog::closeEvent(QCloseEvent *event)
{
    Settings::clear();
    for(QHash<QString, QVariant>::iterator i = m_oldPreferences.begin(); i != m_oldPreferences.end(); i++) Settings::setValue(i.key(), i.value());
    Settings::sync();
    event->accept();
}

void PreferencesDialog::menuRequestPath(QPoint pos)
{
    // this should always come from a QLineEdit
    QLineEdit *lineEdit = qobject_cast<QLineEdit*>(sender());
    if (lineEdit == 0) return;

    QMenu *menu = lineEdit->createStandardContextMenu();
    menu->addSeparator();
    menu->addAction(tr("Select File..."));
    menu->addAction(tr("Select Folder..."));


    QPoint gp = lineEdit->mapToGlobal(pos);

    QAction *action = menu->exec(gp);
    if (action)
    {
        if (action->text() == tr("Select Folder..."))
        {
            QString dir = QFileDialog::getExistingDirectory(this, "Select required folder", lineEdit->text(), QFileDialog::ShowDirsOnly | QFileDialog::Options(EXTRA_FILE_DIALOG_OPTIONS));
            if (!dir.isEmpty())
            {
                lineEdit->setText(dir);
            }
        }
        if (action->text() == tr("Select Filer..."))
        {
            QString file = QFileDialog::getOpenFileName(this, "Select required file", lineEdit->text(), tr("All Files (*.*)"), 0, QFileDialog::Options(EXTRA_FILE_DIALOG_OPTIONS));
            if (!file.isEmpty())
            {
                lineEdit->setText(file);
            }
        }
    }
    delete menu;
}

QString PreferencesDialog::windowFlagsToText(Qt::WindowFlags flags)
{
    QString text;

    QString binaryString(QString::number((unsigned int)flags, 2 ));
    for (int i = 0; i < binaryString.size(); i++)
    {
        if (binaryString.at(binaryString.size() - 1 - i) == '1')
        {
            text += QString("0x%1\n").arg((unsigned int)std::pow(2, i), 8, 16, QChar('0'));
        }
    }

    Qt::WindowFlags type = (flags & Qt::WindowType_Mask);
    if (type == Qt::Window) {
        text += "Qt::Window";
    } else if (type == Qt::Dialog) {
        text += "Qt::Dialog";
    } else if (type == Qt::Sheet) {
        text += "Qt::Sheet";
    } else if (type == Qt::Drawer) {
        text += "Qt::Drawer";
    } else if (type == Qt::Popup) {
        text += "Qt::Popup";
    } else if (type == Qt::Tool) {
        text += "Qt::Tool";
    } else if (type == Qt::ToolTip) {
        text += "Qt::ToolTip";
    } else if (type == Qt::SplashScreen) {
        text += "Qt::SplashScreen";
    }

    if (flags & Qt::MSWindowsFixedSizeDialogHint)
        text += "\n| Qt::MSWindowsFixedSizeDialogHint";
    if (flags & Qt::X11BypassWindowManagerHint)
        text += "\n| Qt::X11BypassWindowManagerHint";
    if (flags & Qt::FramelessWindowHint)
        text += "\n| Qt::FramelessWindowHint";
    if (flags & Qt::NoDropShadowWindowHint)
        text += "\n| Qt::NoDropShadowWindowHint";
    if (flags & Qt::WindowTitleHint)
        text += "\n| Qt::WindowTitleHint";
    if (flags & Qt::WindowSystemMenuHint)
        text += "\n| Qt::WindowSystemMenuHint";
    if (flags & Qt::WindowMinimizeButtonHint)
        text += "\n| Qt::WindowMinimizeButtonHint";
    if (flags & Qt::WindowMaximizeButtonHint)
        text += "\n| Qt::WindowMaximizeButtonHint";
    if (flags & Qt::WindowCloseButtonHint)
        text += "\n| Qt::WindowCloseButtonHint";
    if (flags & Qt::WindowContextHelpButtonHint)
        text += "\n| Qt::WindowContextHelpButtonHint";
    if (flags & Qt::WindowShadeButtonHint)
        text += "\n| Qt::WindowShadeButtonHint";
    if (flags & Qt::WindowStaysOnTopHint)
        text += "\n| Qt::WindowStaysOnTopHint";
    if (flags & Qt::CustomizeWindowHint)
        text += "\n| Qt::CustomizeWindowHint";

    return text;
}



