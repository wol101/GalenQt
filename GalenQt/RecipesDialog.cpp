#include "RecipesDialog.h"

#include "SingleChannelImage.h"
#include "LabelledPoints.h"
#include "Settings.h"

#include <QFile>
#include <QRegularExpression>
#include <QTableWidget>
#include <QHeaderView>
#include <QStringList>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfoList>
#include <QMessageBox>
#include <QProcess>
#include <QDebug>
#include <QFileDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

#include <string>

RecipesDialog::RecipesDialog(QWidget *parent) : QDialog(parent)
{
    m_recipeFile = ":/data/DefaultRecipes.txt";
    m_workingFolder = "/tmp/GalenQt";

    m_preferences = 0;
    m_inputImages = 0;
    m_labelledPoints = 0;

    m_tableWidget = 0;
    m_lineEdit = 0;

#ifdef Q_OS_MAC
    // on a mac the matlab binary typically installs in "/Applications/MATLAB_R2015aSP1.app/bin/matlab"
    QDir applicationsDir("/Applications");
    QStringList applicationsStringList = applicationsDir.entryList(QStringList("MATLAB*"), QDir::Dirs, QDir::Name); // this actually defaults to case insensitive anyway
    if (applicationsStringList.size())
    {
        QDir matlabDir(applicationsDir.filePath(applicationsStringList[0]));
        QStringList matlabStringList = matlabDir.entryList(QStringList("bin"), QDir::Dirs, QDir::Name);
        if (matlabStringList.size())
        {
            QDir matlabBin(matlabDir.filePath(matlabStringList[0]));
            QStringList matlabExecutable = matlabBin.entryList(QStringList("matlab"), QDir::Files, QDir::Name);
            if (matlabExecutable.size() > 0) m_searchLocations["matlab"] = matlabBin.filePath(matlabExecutable[0]);
        }
    }
#endif

    setWindowTitle(tr("Recipes Dialog"));

    // this is the standard default vertical layout
    QVBoxLayout *topLayout = new QVBoxLayout;
    setLayout(topLayout);

    // start with a list view
    m_tableWidget = new QTableWidget();
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    topLayout->addWidget(m_tableWidget);

    // populate it
    ParseRecipes();

    m_tableWidget->setColumnCount(3);
    m_tableWidget->setHorizontalHeaderLabels(QStringList() << "Name" << "Samples" << "Dimensions");
    m_tableWidget->setRowCount(m_recipeList.size());
    m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    for (int r = 0; r < m_recipeList.size(); r++)
    {
        QTableWidgetItem *itemTitle = new QTableWidgetItem(m_recipeList[r].title);
        itemTitle->setFlags(itemTitle->flags() ^ Qt::ItemIsEditable);
        QTableWidgetItem *itemSubSamples = new QTableWidgetItem(m_recipeList[r].subSamples);
        QTableWidgetItem *itemOutputDimensions = new QTableWidgetItem(m_recipeList[r].outputDimensions);
        m_tableWidget->setItem(r, 0, itemTitle);
        m_tableWidget->setItem(r, 1, itemSubSamples);
        m_tableWidget->setItem(r, 2, itemOutputDimensions);
    }

    // put a grid layout underneath
    QGridLayout *gridLayout = new QGridLayout();
    topLayout->addLayout(gridLayout);

    // and put QLabel, QLineEdit, QButton in a row
    QLabel *label = new QLabel();
    label->setText("Working Folder");
    gridLayout->addWidget(label, 0, 0);
    m_lineEdit = new QLineEdit();
    m_lineEdit->setMinimumWidth(200);
    m_lineEdit->setText(m_workingFolder);
    gridLayout->addWidget(m_lineEdit, 0, 1);
    QPushButton *pushButton = new QPushButton();
    pushButton->setText("Browse...");
    gridLayout->addWidget(pushButton, 0, 2);
    connect(pushButton, SIGNAL(clicked()), this, SLOT(browseButtonClicked()));

    // and put the OK and Cancel buttons at the bottom of the dialog
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    topLayout->addWidget(buttonBox);

    restoreGeometry(Settings::value("_AboutDialogGeometry", QByteArray()).toByteArray());
}

RecipesDialog::~RecipesDialog()
{
    Settings::setValue("_PreferencesDialogGeometry", saveGeometry());
}

int RecipesDialog::ParseRecipes()
{
    QFile file(m_recipeFile);
    bool ok = file.open(QIODevice::ReadOnly);
    if (ok == false)
    {
        QMessageBox::warning(this, "Recipe File Read Error", QString("Could not open %1\n Click the button to return.").arg(m_recipeFile), QMessageBox::Ok);
        return __LINE__;
    }
    QString recipeData = QString(file.readAll());
    QStringList lines = recipeData.split(QRegularExpression("\n|\r\n|\r")); // this should split into lines no matter what sort of odd line ending we have

    Recipe recipe;
    int current_line_index = 0;
    while (current_line_index < lines.size())
    {
        recipe.title = ExtractTag(&current_line_index, lines, "TITLE");
        if (recipe.title.size() == 0) break;
        recipe.commandLine = ExtractTag(&current_line_index, lines, "COMMAND_LINE");
        recipe.programName = ExtractTag(&current_line_index, lines, "PROGRAM_NAME");
        recipe.outputImageRegexp = ExtractTag(&current_line_index, lines, "OUTPUT_IMAGE_REGEXP");
        recipe.subSamples = ExtractTag(&current_line_index, lines, "SUBSAMPLES");
        recipe.programText = ExtractTag(&current_line_index, lines, "PROGRAM_TEXT");
        recipe.outputDimensions = QString("1");

        m_recipeList.push_back(recipe);
    }
    return 0;
}

QString RecipesDialog::ExtractTag(int *lineIndex, const QStringList &lines, const QString &tag)
{
    QString outputString;

    QString openingTagMatchRegExp = QString("^\\s*<\\s*%1\\s*>\\s*$|^\\s*<\\s*%1\\s*>\\s*#.*$").arg(tag);
    QString closingTagMatchRegExp = QString("^\\s*</\\s*%1\\s*>\\s*$|^\\s*</\\s*%1\\s*>\\s*#.*$").arg(tag);

    int openingTagIndex = lines.indexOf(QRegularExpression(openingTagMatchRegExp), *lineIndex);
    if (openingTagIndex < 0)
    {
        *lineIndex = lines.size();
        return outputString;
    }
    int closingTagIndex = lines.indexOf(QRegularExpression(closingTagMatchRegExp), openingTagIndex + 1);
    if (closingTagIndex < 0)
    {
        *lineIndex = lines.size();
        return outputString;
    }

    // find the size of the output string
    int totalSize = 0;
    for (int i = openingTagIndex + 1; i < closingTagIndex; i++) totalSize += lines[i].size() + 1;
    outputString.reserve(totalSize + 1);
    for (int i = openingTagIndex + 1; i < closingTagIndex; i++)
    {
        outputString += lines[i];
        if (i < closingTagIndex - 1) outputString += QChar('\n');
    }
    *lineIndex = closingTagIndex + 1;
    return outputString;
}

void RecipesDialog::accept()
{
    QList<QTableWidgetItem *> selectedItems = m_tableWidget->selectedItems();
    if (selectedItems.size())
    {
        int selectedRow = selectedItems[0]->row();
        QTableWidgetItem *itemTitle = m_tableWidget->item(selectedRow, 0);
        QTableWidgetItem *itemSubSamples =  m_tableWidget->item(selectedRow, 1);
        QTableWidgetItem *itemOutputDimensions =  m_tableWidget->item(selectedRow, 2);
        qDebug() << itemTitle->text() << "\n";
        for (int i = 0; i < m_recipeList.size(); i++)
        {
            if (m_recipeList[i].title == itemTitle->text())
            {
                m_recipeList[i].outputDimensions = itemOutputDimensions->text();
                m_recipeList[i].subSamples = itemSubSamples->text();
                HandleRecipe(m_recipeList[i]);
                break;
            }
        }
    }

    QDialog::accept();
}

int RecipesDialog::HandleRecipe(const Recipe &recipe)
{
    // handle working folder
    QString currentWorkingFolder = QDir::currentPath();
    QDir folder(m_workingFolder);
    if (folder.exists() == false) folder.mkpath(".");
    if (QDir::setCurrent(m_workingFolder) == false)
    {
        QMessageBox::warning(this, "Change Folder Error", QString("Could not change folder to %1\n Click the button to return.").arg(m_workingFolder), QMessageBox::Ok);
        return __LINE__;
    }

    // need to substitute these strings in the programText
    // NUMBER_OF_IMAGES_SELECTED     number of images selected
    // INPUT_IMAGE_000               the names of the input images
    // INPUT_IMAGE_001
    // INPUT_IMAGE_002
    // ...
    // INPUT_IMAGES_MATLAB           input images in single quotes 'im1','im2','im3' etc
    // INPUT_IMAGES_R                input images in double quotes "im1","im2","im3" etc

    // OUTPUT_IMAGE_000              the names of the output images
    // OUTPUT_IMAGE_001
    // OUTPUT_IMAGE_002
    // ...
    // OUTPUT_IMAGES_MATLAB          output images in single quotes 'im1','im2','im3' etc
    // OUTPUT_IMAGES_R               output images in double quotes "im1","im2","im3" etc

    // LABEL_FILE                    file containing the labels and sample points

    // SUBSAMPLES_MATLAB             the number of random subsamples to be taken (0 means use all pixels)

    // OUTPUT_DIMENSIONS_MATLAB      the number of output dimensions wanted

#ifdef USE_STD_STRING_IN_RECIPES_DIALOG
    // convert to std::string to ensures a deep copy which is needed here
    std::string programText = recipe.programText.toUtf8().constData();
    ReplaceStringInPlace(programText, "NUMBER_OF_IMAGES_SELECTED", QString("%1").arg(m_inputImages->size()).toUtf8().constData());
    QString inputImagesMatlab('\'');
    QString inputImagesR('"');
    for (int i = 0; i < m_inputImages->size(); i++)
    {
        ReplaceStringInPlace(programText, QString("INPUT_IMAGE_%1").arg(i, 3, QChar('0')).toUtf8().constData(), m_inputImages->at(i)->localPath().toUtf8().constData());
        inputImagesMatlab += m_inputImages->at(i)->localPath() + QString("','");
        inputImagesR += m_inputImages->at(i)->localPath() + QString("\",\"");
    }
    inputImagesMatlab.chop(1);
    inputImagesR.chop(1);
    ReplaceStringInPlace(programText, "INPUT_IMAGES_MATLAB", inputImagesMatlab.toUtf8().constData());
    ReplaceStringInPlace(programText, "INPUT_IMAGES_R", inputImagesR.toUtf8().constData());
    ReplaceStringInPlace(programText, "SUBSAMPLES_MATLAB", recipe.subSamples.toUtf8().constData());
    ReplaceStringInPlace(programText, "OUTPUT_DIMENSIONS_MATLAB", recipe.outputDimensions.toUtf8().constData());

    QFile file(recipe.programName);
    bool ok = file.open(QIODevice::WriteOnly);
    if (ok == false) return __LINE__;
    file.write(programText.c_str());
    file.close();
#else
    QString programText = QString::fromUtf16(recipe.programText.utf16()); // this line forces a deep copy otherwise we just get a shallow copy
    programText.replace("NUMBER_OF_IMAGES_SELECTED", QString("%1").arg(m_inputImages->size()), Qt::CaseSensitive);
    QString inputImagesMatlab('\'');
    QString inputImagesR('"');
    for (int i = 0; i < m_inputImages->size(); i++)
    {
        programText.replace(QString("INPUT_IMAGE_%1").arg(i, 3, QChar('0')), m_inputImages->at(i)->localPath(), Qt::CaseSensitive);
        inputImagesMatlab += m_inputImages->at(i)->localPath() + QString("','");
        inputImagesR += m_inputImages->at(i)->localPath() + QString("\",\"");
    }
    inputImagesMatlab.chop(1);
    inputImagesR.chop(1);
    programText.replace("INPUT_IMAGES_MATLAB", inputImagesMatlab, Qt::CaseSensitive);
    programText.replace("INPUT_IMAGES_R", inputImagesR, Qt::CaseSensitive);
    programText.replace("SUBSAMPLES_MATLAB", recipe.subSamples, Qt::CaseSensitive);
    programText.replace("OUTPUT_DIMENSIONS_MATLAB", recipe.outputDimensions, Qt::CaseSensitive);

    QFile file(recipe.programName);
    bool ok = file.open(QIODevice::WriteOnly);
    if (ok == false) return __LINE__;
    file.write(programText.toUtf8());
    file.close();
#endif

    // use QProcess to run the command (this can work asynchronously if necessary)

    // synchronous version
    QProcess process;
    QStringList commandLineArgs = SplitCommandLine(recipe.commandLine);
    QString command =  commandLineArgs.takeFirst();
    if (m_searchLocations.contains(command)) command = m_searchLocations[command];

    process.start(command, commandLineArgs);
    if (!process.waitForStarted())
    {
        QMessageBox::warning(this, "Subprocess Run Error", QString("Could not start %1\n Click the button to return.").arg(command), QMessageBox::Ok);
        QDir::setCurrent(currentWorkingFolder);
        return __LINE__;
    }

    if (!process.waitForFinished(-1))
    {
        QMessageBox::warning(this, "Subprocess Run Error", QString("%1 produced an error\n Click the button to return.").arg(command), QMessageBox::Ok);
        QDir::setCurrent(currentWorkingFolder);
        return __LINE__;
    }

    // read the output information
    QByteArray result = process.readAll();
    Q_UNUSED(result);

    QStringList folderContents = folder.entryList(QDir::NoDotAndDotDot | QDir::Files, QDir::Name);
    m_outputImages.clear();
    QRegularExpression regExp(recipe.outputImageRegexp);
    for (int i = 0; i < folderContents.size(); i++)
        if (regExp.match(folderContents[i]).hasMatch()) m_outputImages.push_back(QDir::cleanPath(QFileInfo(folderContents[i]).absoluteFilePath()));

    QDir::setCurrent(currentWorkingFolder);
    return 0;
}

QStringList RecipesDialog::SplitCommandLine(const QString &cmdLine)
{
    QStringList list;
    QString arg;
    bool escape = false;
    enum { Idle, Arg, QuotedArg } state = Idle;
    foreach (QChar const c, cmdLine)
    {
        if (!escape && c == '\\') { escape = true; continue; }
        switch (state)
        {
        case Idle:
            if (!escape && c == '"') state = QuotedArg;
            else if (escape || !c.isSpace()) { arg += c; state = Arg; }
            break;
        case Arg:
            if (!escape && c == '"') state = QuotedArg;
            else if (escape || !c.isSpace()) arg += c;
            else { list << arg; arg.clear(); state = Idle; }
            break;
        case QuotedArg:
            if (!escape && c == '"') state = arg.isEmpty() ? Idle : Arg;
            else arg += c;
            break;
        }
        escape = false;
    }
    if (!arg.isEmpty()) list << arg;
    return list;
}

void RecipesDialog::ReplaceStringInPlace(std::string& subject, const std::string& search, const std::string& replace)
{
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos)
    {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
}

void RecipesDialog::browseButtonClicked()
{
    QString folder = QFileDialog::getExistingDirectory(this, "Select Folder", m_workingFolder, QFileDialog::ShowDirsOnly | QFileDialog::Options(EXTRA_FILE_DIALOG_OPTIONS));
    if (folder.isNull() == false)
    {
        m_workingFolder = folder;
        m_lineEdit->setText(folder);
    }
}

QStringList RecipesDialog::outputImages() const
{
    return m_outputImages;
}

QString RecipesDialog::workingFolder() const
{
    return m_workingFolder;
}

void RecipesDialog::setLabelledPoints(QList<LabelledPoints *> *labelledPoints)
{
    m_labelledPoints = labelledPoints;
}

void RecipesDialog::setInputImages(QList<SingleChannelImage *> *inputImages)
{
    m_inputImages = inputImages;
}

void RecipesDialog::setOutputDimensions(int outputDimension)
{
    for (int r = 0; r < m_recipeList.size(); r++)
    {
        m_recipeList[r].outputDimensions = QString("%1").arg(outputDimension);
        QTableWidgetItem *itemOutputDimensions = m_tableWidget->item(r, 2);
        itemOutputDimensions->setText(m_recipeList[r].outputDimensions);
    }
}


void RecipesDialog::setPreferences(Preferences *preferences)
{
    m_preferences = preferences;
}

