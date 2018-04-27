#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QColor>
#include <QVariant>
#include <QDomElement>

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:

    struct SettingsWidget
    {
        QWidget *widget;
        QString key;
        QVariant item;
    };

    explicit PreferencesDialog(QWidget *parent = 0);
    ~PreferencesDialog();

    static void Import(const QString &filename);
    static QColor getIdealTextColour(const QColor& rBackgroundColour);
    static QColor getAlphaColourHint(const QColor& colour);
    static QString windowFlagsToText(Qt::WindowFlags flags);

public slots:
    void colourButtonClicked();
    void fontButtonClicked();
    void importButtonClicked();
    void exportButtonClicked();
    void defaultsButtonClicked();
    void acceptButtonClicked();
    void rejectButtonClicked();
    void menuRequestPath(QPoint pos);

protected:
    void closeEvent(QCloseEvent *event);

private:
    void InitialiseContents();
    void DeleteContents();
    void ParseQDomElement(const QDomElement &docElem);
    void StoreCurrentSettings();

    QList<SettingsWidget> m_SettingsWidgetList;
    QHash<QString, QVariant> m_oldPreferences;
};


#endif // PREFERENCESDIALOG_H
