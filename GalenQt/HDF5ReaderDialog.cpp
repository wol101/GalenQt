#include "Settings.h"

#include "HDF5ReaderDialog.h"
#include "ui_HDF5ReaderDialog.h"

#include "Utilities.h"
#include "SingleChannelImage.h"

#include "hdf5.h"
#include "hdf5_hl.h"

#include <QFileDialog>
#include <QTableWidgetItem>
#include <QComboBox>
#include <QMessageBox>

const int MAX_NAME = 1024 * 64;
const int COL_PATH = 0;
const int COL_SIZE = 1;
const int COL_RANK = 2;
const int COL_DIMENSIONS = 3;

const char *permutationsText[] = {"XYZ","XZY","YXZ","YZX","ZXY","ZYX"};
enum permutations                { XYZ , XZY , YXZ , YZX , ZXY , ZYX};

HDF5ReaderDialog::HDF5ReaderDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HDF5ReaderDialog)
{
#ifdef Q_OS_MACOS
    // on MacOS High Sierra (and possibly others), the Qt::Dialog flag disables resizing
    // if I clear the Qt::Dialog flag and make sure the Qt::Window flag is set (it should be already)
    // then it seems to let me have resizing without any other side effects
    setWindowFlags(windowFlags() & (~Qt::Dialog) | Qt::Window);
#endif
    ui->setupUi(this);

    m_data = 0;
    m_rank = 0;
    m_dims = 0;
    m_nx = 0;
    m_ny = 0;
    m_nz = 0;

    connect(ui->pushButtonOK, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(ui->pushButtonCancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(ui->pushButtonInputFile, SIGNAL(clicked()), this, SLOT(inputFileClicked()));
    connect(ui->lineEditInputFile, SIGNAL(textChanged(const QString &)), this, SLOT(outputFileChanged(const QString &)));
    connect(ui->tableWidgetDataSets, SIGNAL(itemSelectionChanged()), this, SLOT(infoTableItemSelected()), Qt::QueuedConnection);
    connect(ui->pushButtonOutputFolder, SIGNAL(clicked()), this, SLOT(outputFolderClicked()));
    connect(ui->lineEditOutputFolder, SIGNAL(textChanged(const QString &)), this, SLOT(outputFolderChanged(const QString &)));

    QFont infoFont = qvariant_cast<QFont>(Settings::value("HDF Information Font", QVariant::fromValue(QFontDatabase::systemFont(QFontDatabase::FixedFont))));
    ui->plainTextEditInformation->setFont(infoFont);
    QString lastInputFile = Settings::value("_HDF5ReaderInputFile", QString("")).toString();
    ui->lineEditInputFile->setText(lastInputFile);
    QString lastOutputFolder = Settings::value("_HDF5ReaderOutputFolder", QString("")).toString();
    ui->lineEditOutputFolder->setText(lastOutputFolder);

    ui->pushButtonOK->setEnabled(false);
    ui->checkBoxReverseX->setChecked(Settings::value("_HDF5ReaderXReverse", false).toBool());
    ui->checkBoxReverseY->setChecked(Settings::value("_HDF5ReaderYReverse", false).toBool());
    ui->checkBoxReverseZ->setChecked(Settings::value("_HDF5ReaderZReverse", false).toBool());

    for (int i = 0; i < 6; i++)
    {
        ui->comboBoxSourceXYZ->setCurrentText(permutationsText[i]);
        ui->comboBoxDestinationXYZ->setCurrentText(permutationsText[i]);
    }
    ui->comboBoxSourceXYZ->setCurrentIndex(Settings::value("_HDF5ReaderSourceXYZ", 0).toInt());
    ui->comboBoxDestinationXYZ->setCurrentIndex(Settings::value("_HDF5ReaderDestinationXYZ", 0).toInt());

    restoreGeometry(Settings::value("_HDF5ReaderDialogGeometry", QByteArray()).toByteArray());
}

HDF5ReaderDialog::~HDF5ReaderDialog()
{
    if (m_data) delete [] m_data;
    if (m_dims) delete [] m_dims;
    Settings::setValue("_HDF5ReaderInputFile", ui->lineEditInputFile->text());
    Settings::setValue("_HDF5ReaderOutputFolder", ui->lineEditOutputFolder->text());
    Settings::setValue("_HDF5ReaderXReverse", ui->checkBoxReverseX->isChecked());
    Settings::setValue("_HDF5ReaderYReverse", ui->checkBoxReverseY->isChecked());
    Settings::setValue("_HDF5ReaderZReverse", ui->checkBoxReverseZ->isChecked());
    Settings::setValue("_HDF5ReaderSourceXYZ", ui->comboBoxSourceXYZ->currentIndex());
    Settings::setValue("_HDF5ReaderDestinationXYZ", ui->comboBoxDestinationXYZ->currentIndex());
    Settings::setValue("_HDF5ReaderDialogGeometry", saveGeometry());
    delete ui;
}

void HDF5ReaderDialog::okClicked()
{
    readData();
    writeTiffFiles();
    accept();
}

void HDF5ReaderDialog::cancelClicked()
{
    reject();
}

void HDF5ReaderDialog::inputFileClicked()
{
    QString lastInputFile = Settings::value("_HDF5ReaderInputFile", QString("")).toString();
    QString fileName = QFileDialog::getOpenFileName(this, "Open HDF File", lastInputFile, "HDF Files (*.HDF *.hdf *.HDF5 *.hdf5 *.HD5 *hd5);;Any File (*.*)", 0,
                                                    static_cast<QFileDialog::Options>(EXTRA_FILE_DIALOG_OPTIONS));
    if (!fileName.isEmpty())
    {
        ui->lineEditInputFile->setText(fileName);
        Settings::setValue("_HDF5ReaderInputFile", fileName);
    }
}

void HDF5ReaderDialog::outputFolderClicked()
{
    QString lastHDFOutputFolder = Settings::value("_HDF5ReaderOutputFolder", QString("")).toString();
    QString dir = QFileDialog::getExistingDirectory(this, "Select HDF output folder", lastHDFOutputFolder, QFileDialog::ShowDirsOnly | QFileDialog::Options(EXTRA_FILE_DIALOG_OPTIONS));
    if (!dir.isEmpty())
    {
        ui->lineEditOutputFolder->setText(dir);
    }
}

void HDF5ReaderDialog::outputFolderChanged(const QString &text)
{
    Q_UNUSED(text);
    if (!ui->lineEditHDF5InternalPath->text().isEmpty() && !ui->lineEditInputFile->text().isEmpty()&& !ui->lineEditOutputFolder->text().isEmpty()) ui->pushButtonOK->setEnabled(true);
    else  ui->pushButtonOK->setEnabled(false);
}


void HDF5ReaderDialog::outputFileChanged(const QString &text)
{
    if (text.size() == 0) return;
    if (QFileInfo(text).isFile() == false) return;
    ui->plainTextEditInformation->clear();
    m_informationLines.clear();
    m_dataSetNames.clear();
    m_dataSetSizes.clear();
    int rank;
    hsize_t *dims;


    print_info(text.toUtf8());
    QString line;
    for (int i = 0; i < m_informationLines.size(); i++)
    {
        line = Utilities::rstrip(m_informationLines[i]);
        ui->plainTextEditInformation->appendPlainText(line);
        ui->tableWidgetDataSets->clearContents();
        ui->tableWidgetDataSets->setRowCount(m_dataSetNames.size());
        if (m_dataSetNames.size())
        {
            hid_t file_id = H5Fopen (text.toUtf8(), H5F_ACC_RDONLY, H5P_DEFAULT);
            QTableWidgetItem *newItem;
            hsize_t biggestSize = 0;
            int biggestSizeIndex = -1;
            for (int i = 0; i < m_dataSetNames.size(); i++)
            {
                newItem = new QTableWidgetItem(m_dataSetNames[i]);
                ui->tableWidgetDataSets->setItem(i, COL_PATH, newItem);
                newItem = new QTableWidgetItem(QString("%1").arg(m_dataSetSizes[i]));
                ui->tableWidgetDataSets->setItem(i, COL_SIZE, newItem);
                H5LTget_dataset_ndims (file_id, m_dataSetNames[i].toUtf8(), &rank);
                newItem = new QTableWidgetItem(QString("%1").arg(rank));
                ui->tableWidgetDataSets->setItem(i, COL_RANK, newItem);
                dims = new hsize_t[rank];
                H5LTget_dataset_info(file_id, m_dataSetNames[i].toUtf8(), dims, 0, 0);
                QString dimsString = QString("%1").arg(dims[0]);
                for (int j = 1; j < rank; j++) dimsString = QString("%1,%2").arg(dims[j]).arg(dimsString);
                delete [] dims;
                newItem = new QTableWidgetItem(dimsString);
                ui->tableWidgetDataSets->setItem(i, COL_DIMENSIONS, newItem);
                if (m_dataSetSizes[i] > biggestSize)
                {
                    biggestSize = m_dataSetSizes[i];
                    biggestSizeIndex = i;
                }
            }

            if (biggestSizeIndex >= 0) ui->tableWidgetDataSets->setCurrentCell(biggestSizeIndex, 0);
            else ui->lineEditHDF5InternalPath->setText(QString());
            H5Fclose (file_id);
            for (int i = 0; i < ui->tableWidgetDataSets->columnCount(); i++) ui->tableWidgetDataSets->resizeColumnToContents(i);
        }
    }
}

void HDF5ReaderDialog::infoTableItemSelected()
{
    QList<QTableWidgetItem *> items = ui->tableWidgetDataSets->selectedItems();
    if (items.size() == 0) return;
//    int rank = ui->tableWidgetDataSets->item(items[0]->row(), COL_RANK)->text().toInt();
//    m_rank = rank;
//    m_datasetPath = ui->tableWidgetDataSets->item(items[0]->row(), COL_PATH)->text();
//    QStringList tokens = ui->tableWidgetDataSets->item(items[0]->row(), COL_DIMENSIONS)->text().split(QChar(','));
//    if (m_dims) delete m_dims;
//    m_dims = new hsize_t[m_rank];
//    for (int i = 0; i < m_rank; i++) m_dims[i] = tokens.at(m_rank - i -1).toLongLong();
    m_datasetPath = ui->tableWidgetDataSets->item(items[0]->row(), COL_PATH)->text();
    ui->lineEditHDF5InternalPath->setText(m_datasetPath);
    if (!ui->lineEditHDF5InternalPath->text().isEmpty() && !ui->lineEditInputFile->text().isEmpty()&& !ui->lineEditOutputFolder->text().isEmpty()) ui->pushButtonOK->setEnabled(true);
    else  ui->pushButtonOK->setEnabled(false);
}

void HDF5ReaderDialog::writeTiffFiles()
{
    size_t size = (size_t)m_nx * (size_t)m_ny;
    for (int i = 0; i < m_nz; i++)
    {
        SingleChannelImage *image = new SingleChannelImage();
        image->AllocateMemory(m_nx, m_ny, false);
        std::copy(m_data + i * size, m_data + (i + 1) * size, image->data());
        image->UpdateMinMax();
        image->setNumBins(Settings::value("Number of Histogram Bins", int(32)).toInt());
        image->UpdateHistogram();
        image->UpdateDisplay();
        QString name = QString("HDF%1").arg(i, 4, 10, QChar('0'));
        image->setName(name);
        QString filename = QString("HDF_Output_Image_%1.tif").arg(i, 4, 10, QChar('0'));
        QDir outputFolder(ui->lineEditOutputFolder->text());
        QString localPath = QDir::cleanPath(outputFolder.absoluteFilePath(filename));
        image->setLocalPath(localPath);
        if (!outputFolder.exists()) { outputFolder.mkpath("."); }
        QFileInfo outputFolderInfo(ui->lineEditOutputFolder->text());
        if (outputFolderInfo.isDir() == false)
        {
            QMessageBox::warning(this, "Warning", QString("Could not create output folder: %1").arg(QDir::cleanPath(outputFolder.absolutePath())));
        }
        if (image->SaveImageToTiffFile(localPath) == false)
        {
            QMessageBox::warning(this, "Warning", QString("Could not save image: %1").arg(filename));
        }
        m_ouputImages.append(image);
    }
}

QList<SingleChannelImage *> HDF5ReaderDialog::ouputImages() const
{
    return m_ouputImages;
}

void HDF5ReaderDialog::mapData(float *inputData, float **outputData)
{
    permutations srcPerms = (permutations)ui->comboBoxSourceXYZ->currentIndex();
    permutations dstPerms = (permutations)ui->comboBoxDestinationXYZ->currentIndex();
    bool rx = ui->checkBoxReverseX->isChecked();
    bool ry = ui->checkBoxReverseY->isChecked();
    bool rz = ui->checkBoxReverseZ->isChecked();

    // get the sizes of the dimensions and the strides
    // in HDF (as in C) the highest dimension always changes the most rapidly
    // so standard image as a stack might be [Z][Y][X] since X changes quickest followed by Y (line number) and Z (image number)
    // but common usage puts the fastes changing item first so this would be XYZ
    switch (srcPerms)
    {
    case XYZ:
        m_nx = m_dims[2];
        m_ny = m_dims[1];
        m_nz = m_dims[0];
        m_xs = 1;
        m_ys = m_dims[2];
        m_zs = m_dims[1] * m_dims[2];
        break;
    case XZY:
        m_nx = m_dims[2];
        m_ny = m_dims[0];
        m_nz = m_dims[1];
        m_xs = 1;
        m_ys = m_dims[1] * m_dims[2];
        m_zs = m_dims[2];
        break;
    case YXZ:
        m_nx = m_dims[1];
        m_ny = m_dims[2];
        m_nz = m_dims[0];
        m_xs = m_dims[2];
        m_ys = 1;
        m_zs = m_dims[1] * m_dims[2];
        break;
    case YZX:
        m_nx = m_dims[0];
        m_ny = m_dims[2];
        m_nz = m_dims[1];
        m_xs = m_dims[1] * m_dims[2];
        m_ys = 1;
        m_zs = m_dims[2];
        break;
    case ZXY:
        m_nx = m_dims[1];
        m_ny = m_dims[0];
        m_nz = m_dims[2];
        m_xs = m_dims[2];
        m_ys = m_dims[1] * m_dims[2];
        m_zs = 1;
        break;
    case ZYX:
        m_nx = m_dims[0];
        m_ny = m_dims[1];
        m_nz = m_dims[2];
        m_xs = m_dims[1] * m_dims[2];
        m_ys = m_dims[2];
        m_zs = 1;
        break;
    }

    int xs, ys, zs;
    switch (dstPerms)
    {
    case XYZ:
        xs = 1;
        ys = m_nx;
        zs = m_nx * m_ny;
        break;
    case XZY:
        xs = 1;
        ys = m_nx * m_nz;
        zs = m_nx;
        break;
    case YXZ:
        xs = m_ny;
        ys = 1;
        zs = m_ny * m_nx;
        break;
    case YZX:
        xs = m_ny * m_nz;
        ys = 1;
        zs = m_ny;
        break;
    case ZXY:
        xs = m_nz;
        ys = m_nz * m_nx;
        zs = 1;
        break;
    case ZYX:
        xs = m_nz * m_ny;
        ys = m_nz;
        zs = 1;
        break;
    }

    // loop through the dimensions
    int indexSrc, indexDst;
    int ox, oy, oz;
    for (int iz = 0; iz < m_nz; iz++)
    {
        if (rz) oz = m_nz - iz - 1;
        else oz = iz;
        for (int iy = 0; iy < m_ny; iy++)
        {
            if (ry) oy = m_ny - iy - 1;
            else oy = iy;
            for (int ix = 0; ix < m_nx; ix++)
            {
                if (rx) ox = m_nx - ix - 1;
                else ox = ix;

                indexSrc = ix * m_xs + iy * m_ys + iz * m_zs;
                indexDst = ox * xs + oy * ys + oz * zs;
                (*outputData)[indexDst] = inputData[indexSrc];
            }
        }
    }
}

bool HDF5ReaderDialog::readData()
{
    if (m_data) delete [] m_data;
    if (m_dims) delete [] m_dims;
    m_rank = 0;
    m_dims = 0;

    QString fileName = ui->lineEditInputFile->text();
    if (QFileInfo(fileName).isFile() == false) return false;
    QString hdf5Path = ui->lineEditHDF5InternalPath->text();
    if (hdf5Path.isEmpty()) return false;
    m_datasetPath = hdf5Path;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();

    // open file
    hid_t file_id = H5Fopen (fileName.toUtf8(), H5F_ACC_RDONLY, H5P_DEFAULT);

    // get the dataset rank and size
    H5LTget_dataset_ndims (file_id, m_datasetPath.toUtf8(), &m_rank);
    m_dims = new hsize_t[std::max(m_rank, 3)];
    H5LTget_dataset_info(file_id, m_datasetPath.toUtf8(), m_dims, 0, 0);
    if (m_rank < 3) for (int i = m_rank; i < 3; i++) m_dims[i] = 1; // fill in the rest of the dimensions with 1

    // calculate size
    m_size = 1;
    for (int i = 0; i < m_rank; i++) m_size *= m_dims[i];
    float *tempData = new float[m_size];

    // read the data
    H5LTread_dataset_float(file_id, m_datasetPath.toUtf8(), tempData);

    // close file
    H5Fclose (file_id);

    // remap the data
    m_data = new float[m_size];
//    m_nx = m_dims[1];
//    m_ny = m_dims[0];
//    m_nz = m_dims[2];
//    int indexSrc, indexDst;
//    for (int iz = 0; iz < m_nz; iz++)
//    {
//        for (int iy = 0; iy < m_ny; iy++)
//        {
//            for (int ix = 0; ix < m_nx; ix++)
//            {
//                indexSrc = iz + ix * m_nz + iy * m_nz * m_nx;
//                indexDst = ix + iy * m_nx + iz * m_nx * m_ny;
//                m_data[indexDst] = tempData[indexSrc];
//            }
//        }
//    }

    mapData(tempData, &m_data);
    delete [] tempData;

    QApplication::restoreOverrideCursor();
    QApplication::processEvents();
    return true;
}

hsize_t HDF5ReaderDialog::size() const
{
    return m_size;
}

hsize_t *HDF5ReaderDialog::dims() const
{
    return m_dims;
}

int HDF5ReaderDialog::rank() const
{
    return m_rank;
}

float *HDF5ReaderDialog::data() const
{
    return m_data;
}

int HDF5ReaderDialog::print_info(const char *filename)
{

    hid_t    file;
    hid_t    grp;

    herr_t   status;

    /*
     *  Example: open a file, open the root, scan the whole file.
     */
    file = H5Fopen(filename, H5F_ACC_RDWR, H5P_DEFAULT);

    grp = H5Gopen(file,"/", H5P_DEFAULT);
    scan_group(grp);

    status = H5Fclose(file);

    return 0;
}

/*
 * Process a group and all its members
 *
 *   This can be used as a model to implement different actions and
 *   searches.
 */

void HDF5ReaderDialog::scan_group(hid_t gid) {
    int i;
    ssize_t len;
    hsize_t nobj;
    herr_t err;
    int otype;
    hid_t grpid, type_id, dsid;
    char group_name[MAX_NAME];
    char memb_name[MAX_NAME];

    /*
         * Information about the group:
         *  Name and attributes
         *
         *  Other info., not shown here: number of links, object id
         */
    len = H5Iget_name (gid, group_name, MAX_NAME);

    m_informationLines.append(QString::asprintf("Group Name: %s\n",group_name));

    /*
     *  process the attributes of the group, if any.
     */
    scan_attrs(gid);

    /*
         *  Get all the members of the groups, one at a time.
         */
    err = H5Gget_num_objs(gid, &nobj);
    for (i = 0; i < nobj; i++) {
        /*
                 *  For each object in the group, get the name and
                 *   what type of object it is.
                 */
        m_informationLines.append(QString::asprintf("  Member: %d ",i));
        len = H5Gget_objname_by_idx(gid, (hsize_t)i,
                                    memb_name, (size_t)MAX_NAME );
        m_informationLines.append(QString::asprintf("   %llu ",len));
        m_informationLines.append(QString::asprintf("  Member: %s ",memb_name));
        otype =  H5Gget_objtype_by_idx(gid, (size_t)i );

        /*
                 * process each object according to its type
                 */
        switch(otype) {
        case H5G_LINK:
            m_informationLines.append(QString::asprintf(" SYM_LINK:\n"));
            do_link(gid,memb_name);
            break;
        case H5G_GROUP:
            m_informationLines.append(QString::asprintf(" GROUP:\n"));
            grpid = H5Gopen(gid,memb_name, H5P_DEFAULT);
            scan_group(grpid);
            H5Gclose(grpid);
            break;
        case H5G_DATASET:
            m_informationLines.append(QString::asprintf(" DATASET:\n"));
            dsid = H5Dopen(gid,memb_name, H5P_DEFAULT);
            do_dset(dsid);
            H5Dclose(dsid);
            break;
        case H5G_TYPE:
            m_informationLines.append(QString::asprintf(" DATA TYPE:\n"));
            type_id = H5Topen(gid,memb_name, H5P_DEFAULT);
            do_dtype(type_id);
            H5Tclose(type_id);
            break;
        default:
            m_informationLines.append(QString::asprintf(" unknown?\n"));
            break;
        }

    }
}

/*
 *  Retrieve information about a dataset.
 *
 *  Many other possible actions.
 *
 *  This example does not read the data of the dataset.
 */
void HDF5ReaderDialog::do_dset(hid_t did)
{
    hid_t tid;
    hid_t pid;
    hid_t sid;
    hsize_t size;
    char ds_name[MAX_NAME];

    /*
         * Information about the group:
         *  Name and attributes
         *
         *  Other info., not shown here: number of links, object id
         */
    H5Iget_name(did, ds_name, MAX_NAME  );
    m_informationLines.append(QString::asprintf("Dataset Name : "));
    m_informationLines.append(ds_name);
    m_dataSetNames.append(ds_name);
    m_informationLines.append(QString::asprintf("\n"));

    /*
     *  process the attributes of the dataset, if any.
     */
    scan_attrs(did);

    /*
     * Get dataset information: dataspace, data type
     */
    sid = H5Dget_space(did); /* the dimensions of the dataset (not shown) */
    tid = H5Dget_type(did);
    m_informationLines.append(QString::asprintf(" DATA TYPE:\n"));
    do_dtype(tid);

    /*
     * Retrieve and analyse the dataset properties
     */
    pid = H5Dget_create_plist(did); /* get creation property list */
    do_plist(pid);
    size = H5Dget_storage_size(did);
    m_informationLines.append(QString::asprintf("Total space currently written in file: %d\n",(int)size));
    m_dataSetSizes.append(size);

    /*
     * The datatype and dataspace can be used to read all or
     * part of the data.  (Not shown in this example.)
     */

    /* ... read data with H5Dread, write with H5Dwrite, etc. */

    H5Pclose(pid);
    H5Tclose(tid);
    H5Sclose(sid);
}

/*
 *  Analyze a data type description
 */
void HDF5ReaderDialog::do_dtype(hid_t tid) {

    H5T_class_t t_class;
    t_class = H5Tget_class(tid);
    if(t_class < 0){
        m_informationLines.append(" Invalid datatype.\n");
    } else {
        /*
         * Each class has specific properties that can be
         * retrieved, e.g., size, byte order, exponent, etc.
         */
        if(t_class == H5T_INTEGER) {
            m_informationLines.append(" Datatype is 'H5T_INTEGER'.\n");
            /* display size, signed, endianess, etc. */
        } else if(t_class == H5T_FLOAT) {
            m_informationLines.append(" Datatype is 'H5T_FLOAT'.\n");
            /* display size, endianess, exponennt, etc. */
        } else if(t_class == H5T_STRING) {
            m_informationLines.append(" Datatype is 'H5T_STRING'.\n");
            /* display size, padding, termination, etc. */
        } else if(t_class == H5T_BITFIELD) {
            m_informationLines.append(" Datatype is 'H5T_BITFIELD'.\n");
            /* display size, label, etc. */
        } else if(t_class == H5T_OPAQUE) {
            m_informationLines.append(" Datatype is 'H5T_OPAQUE'.\n");
            /* display size, etc. */
        } else if(t_class == H5T_COMPOUND) {
            m_informationLines.append(" Datatype is 'H5T_COMPOUND'.\n");
            /* recursively display each member: field name, type  */
        } else if(t_class == H5T_ARRAY) {
            m_informationLines.append(" Datatype is 'H5T_COMPOUND'.\n");
            /* display  dimensions, base type  */
        } else if(t_class == H5T_ENUM) {
            m_informationLines.append(" Datatype is 'H5T_ENUM'.\n");
            /* display elements: name, value   */
        } else  {
            m_informationLines.append(" Datatype is 'Other'.\n");
            /* eg. Object Reference, ...and so on ... */
        }
    }
}


/*
 *  Analyze a symbolic link
 *
 * The main thing you can do with a link is find out
 * what it points to.
 */
void HDF5ReaderDialog::do_link(hid_t gid, char *name) {
    herr_t status;
    char target[MAX_NAME];

    status = H5Gget_linkval(gid, name, MAX_NAME, target  ) ;
    m_informationLines.append(QString::asprintf("Symlink: %s points to: %s\n", name, target));
}

/*
 *  Run through all the attributes of a dataset or group.
 *  This is similar to iterating through a group.
 */
void HDF5ReaderDialog::scan_attrs(hid_t oid) {
    int na;
    hid_t aid;
    int i;

    na = H5Aget_num_attrs(oid);

    for (i = 0; i < na; i++) {
        aid =	H5Aopen_idx(oid, (unsigned int)i );
        do_attr(aid);
        H5Aclose(aid);
    }
}

/*
 *  Process one attribute.
 *  This is similar to the information about a dataset.
 */
void HDF5ReaderDialog::do_attr(hid_t aid) {
    ssize_t len;
    hid_t atype;
    hid_t aspace;
    char buf[MAX_NAME];

    /*
     * Get the name of the attribute.
     */
    len = H5Aget_name(aid, MAX_NAME, buf );
    m_informationLines.append(QString::asprintf("    Attribute Name : %s\n",buf));

    /*
     * Get attribute information: dataspace, data type
     */
    aspace = H5Aget_space(aid); /* the dimensions of the attribute data */

    atype  = H5Aget_type(aid);
    do_dtype(atype);

    /*
     * The datatype and dataspace can be used to read all or
     * part of the data.  (Not shown in this example.)
     */

    /* ... read data with H5Aread, write with H5Awrite, etc. */

    H5Tclose(atype);
    H5Sclose(aspace);
}

/*
 *   Example of information that can be read from a Dataset Creation
 *   Property List.
 *
 *   There are many other possibilities, and there are other property
 *   lists.
 */
void HDF5ReaderDialog::do_plist(hid_t pid) {
    hsize_t chunk_dims_out[2];
    int  rank_chunk;
    int nfilters;
    H5Z_filter_t  filtn;
    int i;
    unsigned int   filt_flags, filt_conf;
    size_t cd_nelmts;
    unsigned int cd_values[32] ;
    char f_name[MAX_NAME];
    H5D_fill_time_t ft;
    H5D_alloc_time_t at;
    H5D_fill_value_t fvstatus;
    unsigned int szip_options_mask;
    unsigned int szip_pixels_per_block;

    /* zillions of things might be on the plist */
    /*  here are a few... */

    /*
     * get chunking information: rank and dimensions.
     *
     *  For other layouts, would get the relevant information.
     */
    if(H5D_CHUNKED == H5Pget_layout(pid)){
        rank_chunk = H5Pget_chunk(pid, 2, chunk_dims_out);
        m_informationLines.append(QString::asprintf("chunk rank %d, dimensions %lu x %lu\n", rank_chunk,
                                                    (unsigned long)(chunk_dims_out[0]),
                                  (unsigned long)(chunk_dims_out[1])));
    } /* else if contiguous, etc. */

    /*
     *  Get optional filters, if any.
     *
     *  This include optional checksum and compression methods.
     */

    nfilters = H5Pget_nfilters(pid);
    for (i = 0; i < nfilters; i++)
    {
        /* For each filter, get
         *   filter ID
         *   filter specific parameters
         */
        cd_nelmts = 32;
        filtn = H5Pget_filter(pid, (unsigned)i,
                              &filt_flags, &cd_nelmts, cd_values,
                              (size_t)MAX_NAME, f_name, &filt_conf);
        /*
         *  These are the predefined filters
         */
        switch (filtn) {
        case H5Z_FILTER_DEFLATE:  /* AKA GZIP compression */
            m_informationLines.append(QString::asprintf("DEFLATE level = %d\n", cd_values[0]));
            break;
        case H5Z_FILTER_SHUFFLE:
            m_informationLines.append(QString::asprintf("SHUFFLE\n")); /* no parms */
            break;
        case H5Z_FILTER_FLETCHER32:
            m_informationLines.append(QString::asprintf("FLETCHER32\n")); /* Error Detection Code */
            break;
        case H5Z_FILTER_SZIP:
            szip_options_mask=cd_values[0];;
            szip_pixels_per_block=cd_values[1];

            m_informationLines.append(QString::asprintf("SZIP COMPRESSION: "));
            m_informationLines.append(QString::asprintf("PIXELS_PER_BLOCK %d\n",
                                                        szip_pixels_per_block));
            /* print SZIP options mask, etc. */
            break;
        default:
            m_informationLines.append(QString::asprintf("UNKNOWN_FILTER\n" ));
            break;
        }
    }

    /*
     *  Get the fill value information:
     *    - when to allocate space on disk
     *    - when to fill on disk
     *    - value to fill, if any
     */

    m_informationLines.append(QString::asprintf("ALLOC_TIME "));
    H5Pget_alloc_time(pid, &at);

    switch (at)
    {
    case H5D_ALLOC_TIME_EARLY:
        m_informationLines.append(QString::asprintf("EARLY\n"));
        break;
    case H5D_ALLOC_TIME_INCR:
        m_informationLines.append(QString::asprintf("INCR\n"));
        break;
    case H5D_ALLOC_TIME_LATE:
        m_informationLines.append(QString::asprintf("LATE\n"));
        break;
    default:
        m_informationLines.append(QString::asprintf("unknown allocation policy"));
        break;
    }

    m_informationLines.append(QString::asprintf("FILL_TIME: "));
    H5Pget_fill_time(pid, &ft);
    switch ( ft )
    {
    case H5D_FILL_TIME_ALLOC:
        m_informationLines.append(QString::asprintf("ALLOC\n"));
        break;
    case H5D_FILL_TIME_NEVER:
        m_informationLines.append(QString::asprintf("NEVER\n"));
        break;
    case H5D_FILL_TIME_IFSET:
        m_informationLines.append(QString::asprintf("IFSET\n"));
        break;
    default:
        m_informationLines.append(QString::asprintf("?\n"));
        break;
    }


    H5Pfill_value_defined(pid, &fvstatus);

    if (fvstatus == H5D_FILL_VALUE_UNDEFINED)
    {
        m_informationLines.append(QString::asprintf("No fill value defined, will use default\n"));
    } else {
        /* Read  the fill value with H5Pget_fill_value.
         * Fill value is the same data type as the dataset.
         * (details not shown)
         **/
    }

    /* ... and so on for other dataset properties ... */
}
