#ifndef RECIPESDIALOG_H
#define RECIPESDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QList>
#include <QMap>
#include <string>

class QTableWidget;
class QLineEdit;

class Preferences;
class SingleChannelImage;
class LabelledPoints;


struct Recipe
{
    QString title;
    QString commandLine;
    QString programName;
    QString outputImageRegexp;
    QString programText;
    QString subSamples;
    QString outputDimensions;
};

class RecipesDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RecipesDialog(QWidget *parent = 0);
    ~RecipesDialog();

    int ParseRecipes();
    QString ExtractTag(int *lineIndex, const QStringList &lines, const QString &tag);
    int HandleRecipe(const Recipe &recipe);

    void setPreferences(Preferences *preferences);
    void setInputImages(QList<SingleChannelImage *> *inputImages);
    void setLabelledPoints(QList<LabelledPoints *> *labelledPoints);
    void setOutputDimensions(int outputDimension);

    QString workingFolder() const;
    QStringList outputImages() const;

    // utility routines
    static QStringList SplitCommandLine(const QString &cmdLine);
    static void ReplaceStringInPlace(std::string& subject, const std::string& search, const std::string& replace);


signals:

public slots:
    void accept();
    void browseButtonClicked();

private:
    QString m_recipeFile;
    QList<Recipe> m_recipeList;
    QMap<QString, QString> m_searchLocations;
    QString m_workingFolder;

    Preferences *m_preferences;
    QList<SingleChannelImage *> *m_inputImages;
    QList<LabelledPoints *> *m_labelledPoints;
    QStringList m_outputImages;

    QTableWidget *m_tableWidget;
    QLineEdit *m_lineEdit;
};

#endif // RECIPESDIALOG_H
