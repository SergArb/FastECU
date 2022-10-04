#include "calibration_maps.h"
#include <ui_calibrationmaptable.h>

CalibrationMaps::CalibrationMaps(FileActions::EcuCalDefStructure *ecuCalDef, int romIndex, int mapIndex, QRect mdiAreaSize, QWidget *parent)
    : QWidget(parent),
      ui(new Ui::CalibrationMaps)
{
    ui->setupUi(this);

    this->setParent(parent);
    this->setAttribute(Qt::WA_DeleteOnClose);

    QString mapWindowObjectName = QString::number(romIndex) + "," + QString::number(mapIndex) + "," + ecuCalDef->NameList.at(mapIndex);

    this->setObjectName(mapWindowObjectName);
    this->setWindowTitle(ecuCalDef->NameList.at(mapIndex) + " - " + ecuCalDef->FileName);

    QString xScaleUnitsTitle = "";
    if (ecuCalDef->XScaleNameList.at(mapIndex) != " ")
    {
        if (ecuCalDef->XScaleUnitsList.at(mapIndex) != " ")
            xScaleUnitsTitle = ecuCalDef->XScaleNameList.at(mapIndex) + " (" + ecuCalDef->XScaleUnitsList.at(mapIndex) + ")";
        else
            xScaleUnitsTitle = ecuCalDef->XScaleNameList.at(mapIndex);
    }
    ui->xScaleUnitsLabel->setText(xScaleUnitsTitle);

    ui->mapDataUnitsLabel->setText(ecuCalDef->UnitsList.at(mapIndex));

    if (ecuCalDef->TypeList.at(mapIndex) == "MultiSelectable")
    {
        mapWindowObjectName = mapWindowObjectName + "," + "MultiSelectable";
        mapCellWidth = mapCellWidthSelectable;
        xSize = 1;
        ySize = 1;
        ui->xScaleUnitsLabel->setFixedHeight(0);
    }
    if (ecuCalDef->TypeList.at(mapIndex) == "Selectable")
    {
        mapWindowObjectName = mapWindowObjectName + "," + "Selectable";
        mapCellWidth = mapCellWidthSelectable;
        xSize = 1;
        ySize = 1;
        ui->xScaleUnitsLabel->setFixedHeight(0);
    }
    if (ecuCalDef->TypeList.at(mapIndex) == "1D")
    {
        mapWindowObjectName = mapWindowObjectName + "," + "1D";
        mapCellWidth = mapCellWidth1D;
        xSize = 1;
        ySize = 1;
        ui->xScaleUnitsLabel->setFixedHeight(0);
    }
    if (ecuCalDef->TypeList.at(mapIndex) == "2D")
    {
        if (ecuCalDef->YSizeList.at(mapIndex).toInt() > 1 || ecuCalDef->XSizeList.at(mapIndex).toInt() > 1)
            this->setWindowIcon(QIcon(":/icons/2D-64-W.png"));
        else
            this->setWindowIcon(QIcon(":/icons/1D-64-W.png"));
        if (ecuCalDef->XScaleTypeList.at(mapIndex) == "Static Y Axis")
            mapWindowObjectName = mapWindowObjectName + "," + "Static Y Axis 2D";
        else if (ecuCalDef->YSizeList.at(mapIndex).toInt() > 1)
            mapWindowObjectName = mapWindowObjectName + "," + "Y Axis 2D";
        else if (ecuCalDef->XSizeList.at(mapIndex).toInt() > 1)
            mapWindowObjectName = mapWindowObjectName + "," + "X Axis 2D";
        xSizeOffset = 0;
        ySizeOffset = 0;
        if (ecuCalDef->YSizeList.at(mapIndex).toInt() > 1 || ecuCalDef->YScaleTypeList.at(mapIndex) == "Static Y Axis")
            xSizeOffset = 1;
        if (ecuCalDef->XSizeList.at(mapIndex).toInt() > 1)
            ySizeOffset = 1;
        xSize = ecuCalDef->XSizeList.at(mapIndex).toInt() + xSizeOffset;
        ySize = ecuCalDef->YSizeList.at(mapIndex).toInt() + ySizeOffset;
    }
    if (ecuCalDef->TypeList.at(mapIndex) == "3D")
    {
        this->setWindowIcon(QIcon(":/icons/3D-64-W.png"));
        mapWindowObjectName = mapWindowObjectName + "," + "3D";
        xSizeOffset = 0;
        ySizeOffset = 0;
        if (ecuCalDef->YSizeList.at(mapIndex).toInt() > 1)
            xSizeOffset = 1;
        if (ecuCalDef->XSizeList.at(mapIndex).toInt() > 1)
            ySizeOffset = 1;
        xSize = ecuCalDef->XSizeList.at(mapIndex).toInt() + xSizeOffset;
        ySize = ecuCalDef->YSizeList.at(mapIndex).toInt() + ySizeOffset;

        VerticalLabel *yScaleUnitsLabel = new VerticalLabel();
        yScaleUnitsLabel->setAlignment(Qt::AlignCenter);
        yScaleUnitsLabel->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Maximum );
        ui->horizontalLayout_2->insertWidget(0, yScaleUnitsLabel);
        QString yScaleUnitsTitle = ecuCalDef->YScaleNameList.at(mapIndex) + " (" + ecuCalDef->YScaleUnitsList.at(mapIndex) + ")";
        yScaleUnitsLabel->setText(yScaleUnitsTitle);

    }

    //qDebug() << "Create map" << mapWindowObjectName;
    this->setObjectName(mapWindowObjectName);
    ui->mapDataTableWidget->setObjectName(mapWindowObjectName);
    ui->mapDataTableWidget->setColumnCount(xSize);
    ui->mapDataTableWidget->setRowCount(ySize);
    ui->mapNameLabel->setText(ecuCalDef->NameList.at(mapIndex));

    for (int col = 0; col < ui->mapDataTableWidget->columnCount(); col++){
        ui->mapDataTableWidget->horizontalHeader()->resizeSection(col, mapCellWidth);
    }
    for (int row = 0; row < ui->mapDataTableWidget->rowCount(); row++){
        ui->mapDataTableWidget->verticalHeader()->resizeSection(row, mapCellHeight);
    }

    if (ecuCalDef->TypeList.at(mapIndex) == "3D")
    {
        QTableWidgetItem *cellItem = new QTableWidgetItem;
        cellItem->setFlags(Qt::NoItemFlags);
        cellItem->setBackground(QBrush(QColor(0xf0, 0xf0, 0xf0, 255)));
        ui->mapDataTableWidget->setItem(0, 0, cellItem);
    }

    setMapTableWidgetItems(ecuCalDef, mapIndex);
    setMapTableWidgetSize(mdiAreaSize.width(), mdiAreaSize.height(), xSize);

    if (ecuCalDef->TypeList.at(mapIndex) != "Selectable")
    {
        connect(ui->mapDataTableWidget, SIGNAL(cellClicked(int, int)), this, SLOT (cellClicked(int, int)));
        connect(ui->mapDataTableWidget, SIGNAL(cellPressed(int, int)), this, SLOT (cellPressed(int, int)));
        ///connect(ui->mapDataTableWidget, SIGNAL(cellEntered(int, int)), this, SLOT (cellActivated(int, int)));
        connect(ui->mapDataTableWidget, SIGNAL(currentCellChanged(int, int, int, int)), this, SLOT (cellChanged(int, int, int, int)));
    }
}

CalibrationMaps::~CalibrationMaps()
{
    delete ui;
}

void CalibrationMaps::setMapTableWidgetSize(int maxWidth, int maxHeight, int xSize){
    int w = 0;
    int h = 0;

    w += ui->mapDataTableWidget->contentsMargins().left() + ui->mapDataTableWidget->contentsMargins().right();
    h += ui->mapDataTableWidget->contentsMargins().top() + ui->mapDataTableWidget->contentsMargins().bottom();
    w += ui->mapDataTableWidget->verticalHeader()->width();
    if (xSize > 17)
        h += ui->mapDataTableWidget->horizontalHeader()->height() + 15;
    else
        h += ui->mapDataTableWidget->horizontalHeader()->height();

    for (int i = 0; i < ui->mapDataTableWidget->columnCount(); i++)
        w += ui->mapDataTableWidget->columnWidth(i);
    for (int i=0; i < ui->mapDataTableWidget->rowCount(); i++)
        h += ui->mapDataTableWidget->rowHeight(i);

    if (w > maxWidth)
        w = maxWidth - 55;
    if (h > maxHeight)
        h = maxHeight - 55;

    ui->mapDataTableWidget->setMinimumWidth(w);
    ui->mapDataTableWidget->setMaximumWidth(w);
    ui->mapDataTableWidget->setMinimumHeight(h);
    ui->mapDataTableWidget->setMaximumHeight(h);

    ui->mapDataTableWidget->setFixedWidth(w);
    ui->mapDataTableWidget->setFixedHeight(h);
}

void CalibrationMaps::setMapTableWidgetItems(FileActions::EcuCalDefStructure *ecuCalDef, int mapIndex)
{
    int xSize = ecuCalDef->XSizeList.at(mapIndex).toInt();
    int ySize = ecuCalDef->YSizeList.at(mapIndex).toInt();
    int mapSize = xSize * ySize;
    int maxWidth = 0;
    //qDebug() << "Map size:" << xSize << "x" << ySize << "=" << mapSize;

    QFont cellFont = ui->mapDataTableWidget->font();
    cellFont.setPointSize(cellFontSize);
    //cellFont.setBold(true);
    cellFont.setFamily("Franklin Gothic");
    //qDebug() << "Cell font size =" << cellFont.pointSize();

    if (xSize > 1)
    {
        QStringList xScaleCellText = ecuCalDef->XScaleData.at(mapIndex).split(",");
        for (int i = 0; i < xSize; i++)
        {
            QTableWidgetItem *cellItem = new QTableWidgetItem;
            QString xScaleCellDataText = xScaleCellText.at(i);
            if (ecuCalDef->XScaleTypeList.at(mapIndex) == "Static Y Axis")
                xScaleCellDataText = xScaleCellText.at(i);
            else
                xScaleCellDataText = QString::number(xScaleCellText.at(i).toFloat(), 'f', getMapValueDecimalCount(ecuCalDef->XScaleFormatList.at(mapIndex)));

            cellItem->setTextAlignment(Qt::AlignCenter);
            cellItem->setFont(cellFont);
            if (i < xScaleCellText.count())
                cellItem->setText(xScaleCellDataText);
            ui->mapDataTableWidget->setItem(0, i + xSizeOffset, cellItem);
/*
            if (ySize > 1)
                ui->mapDataTableWidget->setItem(0, i + 1, cellItem);
            else
                ui->mapDataTableWidget->setItem(0, i, cellItem);
*/
            QFontMetrics fm(cellFont);
            int width = fm.horizontalAdvance(xScaleCellDataText) + 20;
            if (width > maxWidth)
                maxWidth = width;
            ui->mapDataTableWidget->horizontalHeader()->resizeSection(i, maxWidth);
        }
    }
    if (ySize > 1)
    {
        QStringList yScaleCellText = ecuCalDef->YScaleData.at(mapIndex).split(",");
        int maxWidth = 0;
        for (int i = 0; i < ySize; i++)
        {
            QTableWidgetItem *cellItem = new QTableWidgetItem;
            QString yScaleCellDataText = yScaleCellText.at(i);
            yScaleCellDataText = QString::number(yScaleCellText.at(i).toFloat(), 'f', getMapValueDecimalCount(ecuCalDef->YScaleFormatList.at(mapIndex)));

            cellItem->setTextAlignment(Qt::AlignCenter);
            cellItem->setFont(cellFont);
            if (i < yScaleCellText.count())
                cellItem->setText(yScaleCellDataText);
            ui->mapDataTableWidget->setItem(i + ySizeOffset, 0, cellItem);

            QFontMetrics fm(cellFont);
            int width = fm.horizontalAdvance(yScaleCellDataText) + 20;
            if (width > maxWidth)
                maxWidth = width;

        }
        ui->mapDataTableWidget->horizontalHeader()->resizeSection(0, maxWidth);
    }
    if (ecuCalDef->TypeList.at(mapIndex) == "MultiSelectable")
    {
        QStringList mapDataCellText = ecuCalDef->MapData.at(mapIndex).split(",");
        QStringList selectionsList = ecuCalDef->SelectionsList.at(mapIndex).split(",");
        QStringList yScaleCellText = ecuCalDef->YScaleUnitsList.at(mapIndex).split(",");

        //qDebug() << "Selections count =" << mapDataCellText.size();
        for (int i = 0; i < yScaleCellText.length(); i++)
        {
            QTableWidgetItem *cellItem = new QTableWidgetItem;
            cellItem->setTextAlignment(Qt::AlignCenter);
            cellItem->setFont(cellFont);
            cellItem->setText(yScaleCellText.at(i));
            ui->mapDataTableWidget->setItem(i, 0, cellItem);

            QComboBox *selectableComboBox = new QComboBox();
            selectableComboBox->setFont(cellFont);
            //selectableComboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
            selectableComboBox->setFixedWidth(mapCellWidthSelectable);
            for (int j = 0; j < selectionsList.length(); j++){
                //if (mapDataCellText.at(j) != "")
                    selectableComboBox->addItem(selectionsList.at(j));
            }
            selectableComboBox->setObjectName("selectableComboBox");
            selectableComboBox->setCurrentIndex(mapDataCellText.at(0).toInt());
        //        qDebug() << "Selection: " << QString::number(mapData[romNumber][mapAddress]);
            ui->mapDataTableWidget->setCellWidget(i, 1, selectableComboBox);
            connect(selectableComboBox, SIGNAL(currentTextChanged(QString)), this, SIGNAL(selectableComboBoxItemChanged(QString)));
        }
    }
    if (ecuCalDef->TypeList.at(mapIndex) == "Selectable")
    {
        QStringList mapDataCellText = ecuCalDef->MapData.at(mapIndex).split(",");
        QStringList selectionsList = ecuCalDef->SelectionsList.at(mapIndex).split(",");
        QStringList selectionsListSorted = ecuCalDef->SelectionsListSorted.at(mapIndex).split(",");
        int currentIndex = 0;
        //qDebug() << selectionsList;
        //qDebug() << selectionsListSorted;

        //qDebug() << "Selections count =" << mapDataCellText.size();

        QComboBox *selectableComboBox = new QComboBox();
        selectableComboBox->setFont(cellFont);
        //selectableComboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        selectableComboBox->setFixedWidth(mapCellWidthSelectable);
        for (int j = 0; j < selectionsListSorted.length(); j++){
            if (selectionsListSorted.at(j) != "")
                selectableComboBox->addItem(selectionsListSorted.at(j));
        }
        selectableComboBox->setObjectName("selectableComboBox");
        for (int i = 0; i < selectionsListSorted.length(); i++)
        {
            if (selectionsListSorted.at(i) == selectionsList.at(mapDataCellText.at(0).toInt()))
            {
                //qDebug() << selectionsListSorted.at(i) << ":" << selectionsList.at(mapDataCellText.at(0).toInt());
                currentIndex = i;
            }
        }
        selectableComboBox->setCurrentIndex(currentIndex);
        //qDebug() << "Selection: " << currentIndex << ":" << mapDataCellText.at(0).toInt();
        ui->mapDataTableWidget->setCellWidget(0, 0, selectableComboBox);
        connect(selectableComboBox, SIGNAL(currentTextChanged(QString)), this, SIGNAL(selectableComboBoxItemChanged(QString)));
    }

    if (ecuCalDef->TypeList.at(mapIndex) == "1D")
    {
        QStringList mapDataCellText = ecuCalDef->MapData.at(mapIndex).split(",");
        //qDebug() << "Set 1D map item to" << mapDataCellText;
        QTableWidgetItem *cellItem = new QTableWidgetItem;
        cellItem->setTextAlignment(Qt::AlignCenter);
        cellItem->setFont(cellFont);
        cellItem->setForeground(Qt::black);

        //qDebug() << "1D data count =" << mapDataCellText.size();

        cellItem->setText(QString::number(mapDataCellText.at(0).toFloat(), 'f', getMapValueDecimalCount(ecuCalDef->FormatList.at(mapIndex))));
        ui->mapDataTableWidget->setItem(0, 0, cellItem);

    }

    if (ecuCalDef->TypeList.at(mapIndex) == "2D" || ecuCalDef->TypeList.at(mapIndex) == "3D")
    {
        QStringList mapDataCellText = ecuCalDef->MapData.at(mapIndex).split(",");
        ecuCalDef->MaxValueList[mapIndex] = "-1000";
        ecuCalDef->MinValueList[mapIndex] = "1000";

        for (int i = 0; i < mapDataCellText.length(); i++)
        {
            if (mapDataCellText.at(i).toFloat() < ecuCalDef->MinValueList.at(mapIndex).toFloat())
                ecuCalDef->MinValueList[mapIndex] = QString::number(mapDataCellText.at(i).toFloat() * 2);
            if (mapDataCellText.at(i).toFloat() > ecuCalDef->MaxValueList.at(mapIndex).toFloat())
                ecuCalDef->MaxValueList[mapIndex] = QString::number(mapDataCellText.at(i).toFloat() * 2);
        }

        for (int i = 0; i < xSize; i++)
        {
            QString cellDataText;
            cellDataText = QString::number(mapDataCellText.at(i).toFloat(), 'f', getMapValueDecimalCount(ecuCalDef->FormatList.at(mapIndex)));
            QFontMetrics fm(cellFont);
            int width = fm.horizontalAdvance(cellDataText) + 20;
            if (width > maxWidth)
                maxWidth = width;
        }
        for (int i = 0; i < xSize; i++)
        {
            //if (xSize > 1)
                ui->mapDataTableWidget->horizontalHeader()->resizeSection(i + xSizeOffset, maxWidth);
            //else
                //ui->mapDataTableWidget->horizontalHeader()->resizeSection(i, maxWidth);
        }

        for (int i = 0; i < mapSize; i++)
        {
            QTableWidgetItem *cellItem = new QTableWidgetItem;
            cellItem->setTextAlignment(Qt::AlignCenter);
            cellItem->setFont(cellFont);
            int mapItemColor = getMapCellColors(ecuCalDef, mapDataCellText.at(i).toFloat(), mapIndex);
            int mapItemColorRed = (mapItemColor >> 16) & 0xff;
            int mapItemColorGreen = (mapItemColor >> 8) & 0xff;
            int mapItemColorBlue = mapItemColor & 0xff;
            //qDebug() << mapItemColorRed << mapItemColorGreen << mapItemColorBlue;
            cellItem->setBackground(QBrush(QColor(mapItemColorRed , mapItemColorGreen, mapItemColorBlue, 255)));
            if (ecuCalDef->TypeList.at(mapIndex) == "1D")
                cellItem->setForeground(Qt::black);
            else
                cellItem->setForeground(Qt::white);

            if (i < mapDataCellText.count())
                cellItem->setText(QString::number(mapDataCellText.at(i).toFloat(), 'f', getMapValueDecimalCount(ecuCalDef->FormatList.at(mapIndex))));
            int yPos = 0;
            int xPos = 0;
            if (ecuCalDef->XSizeList.at(mapIndex) > 1)
                yPos = i / xSize + ySizeOffset;
            else
                yPos = i / xSize;
            if (ecuCalDef->YSizeList.at(mapIndex) > 1)
                xPos = i - (yPos - ySizeOffset) * xSize + xSizeOffset;
            else
                xPos = i - (yPos - ySizeOffset) * xSize;

            /*
            xPos = i - (yPos - 1) * xSize + xSizeOffset;//0;
            if (ySize > 1 && xSize > 1)
                xPos = i - (yPos - 1) * xSize + 1;
            else
                xPos = i - (yPos - 1) * xSize + 1;
                */
            ui->mapDataTableWidget->setItem(yPos, xPos, cellItem);
        }
    }

}

int CalibrationMaps::getMapValueDecimalCount(QString valueFormat)
{
    //qDebug() << "Value format" << valueFormat;
    if (valueFormat.contains("."))
        return valueFormat.count(QLatin1Char('0')) - 1;
    else
        return valueFormat.count(QLatin1Char('0'));
}

int CalibrationMaps::getMapCellColors(FileActions::EcuCalDefStructure *ecuCalDef, float mapDataValue, int mapIndex)
{

    int mapCellColors;
    int mapCellColorRed = 0;
    int mapCellColorGreen = 0;
    int mapCellColorBlue = 0;
    float mapMinValue = 0;
    float mapMaxValue = 0;

    mapMinValue = ecuCalDef->MinValueList.at(mapIndex).toFloat();
    mapMaxValue = ecuCalDef->MaxValueList.at(mapIndex).toFloat();
    //qDebug() << ecuCalDef->TypeList.at(mapIndex);

    if (ecuCalDef->TypeList.at(mapIndex) != "1D"){
        mapCellColorRed = 255 - (int)(mapDataValue / (mapMaxValue - mapMinValue) * 255);
        if (mapCellColorRed < 0)
            mapCellColorRed = 0;
        if (mapCellColorRed > 255)
            mapCellColorRed = 255;
        //mapCellColorRed = 0;
        mapCellColorGreen = 160 - (int)(mapDataValue / (mapMaxValue - mapMinValue) * 64);
        if (mapCellColorGreen < 0)
            mapCellColorGreen = 0;
        if (mapCellColorGreen > 255)
            mapCellColorGreen = 255;
        mapCellColorGreen = 0;
        mapCellColorBlue = (int)(mapDataValue / (mapMaxValue - mapMinValue) * 255);
        if (mapCellColorBlue < 0)
            mapCellColorBlue = 0;
        if (mapCellColorBlue > 255)
            mapCellColorBlue = 255;
        //mapCellColorBlue = 0;
    }
    else{
        mapCellColorRed = 255;
        mapCellColorGreen = 255;
        mapCellColorBlue = 255;

    }

    mapCellColors = (mapCellColorRed << 16) + (mapCellColorGreen << 8) + (mapCellColorBlue);
    return mapCellColors;
}

void CalibrationMaps::cellClicked(int row, int col)
{
    startCol = col;
    startRow = row;
    //qDebug() << "Cell" << col << ":" << row << "clicked";

    QStringList objectName = ui->mapDataTableWidget->objectName().split(",");
    int cols = ui->mapDataTableWidget->columnCount();
    int rows = ui->mapDataTableWidget->rowCount();

    for (int i = 0; i < cols; i++)
    {
        for (int j = 0; j < rows; j++)
        {
            ui->mapDataTableWidget->item(j, i)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsEnabled);
        }
    }
    ui->mapDataTableWidget->item(row, col)->setSelected(true);
    if (objectName.at(3) == "3D")
        ui->mapDataTableWidget->item(0, 0)->setFlags(Qt::ItemIsEditable);

    if (objectName.at(3) == "Static Y Axis 2D" && rows > 1)
    {
        for (int j = 0; j < cols; j++)
        {
            ui->mapDataTableWidget->item(0, j)->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled);
        }
    }
}

void CalibrationMaps::cellPressed(int row, int col)
{
    startCol = col;
    startRow = row;
    //qDebug() << "Cell" << col << ":" << row << "pressed";

    int cols = ui->mapDataTableWidget->columnCount();
    int rows = ui->mapDataTableWidget->rowCount();

    QStringList objectName = ui->mapDataTableWidget->objectName().split(",");
    for (int i = 0; i < cols; i++)
    {
        for (int j = 0; j < rows; j++)
        {
            ui->mapDataTableWidget->item(j, i)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsEnabled);
        }
    }
    ui->mapDataTableWidget->item(row, col)->setSelected(true);
    if (objectName.at(3) == "3D")
        ui->mapDataTableWidget->item(0, 0)->setFlags(Qt::ItemIsEditable);

    if (objectName.at(3) == "Static Y Axis 2D" && rows > 1)
    {
        for (int j = 0; j < cols; j++)
        {
            ui->mapDataTableWidget->item(0, j)->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled);
        }
    }
}

void CalibrationMaps::cellChanged(int curRow, int curCol, int prevRow, int prevCol)
{
    //qDebug() << "Startcell =" << startCol << ":" << startRow << " and current cell is" << curCol << ":" << curRow;

    int cols = ui->mapDataTableWidget->columnCount();
    int rows = ui->mapDataTableWidget->rowCount();

    //qDebug() << ui->mapDataTableWidget->objectName().split(",").at(3);

    /* Check for 3D table */
    QStringList objectName = ui->mapDataTableWidget->objectName().split(",");
    if (startCol == 0 && objectName.at(3) == "3D")
    {
        for (int i = 1; i < cols; i++)
        {
            for (int j = 0; j < rows; j++)
            {
                //ui->mapDataTableWidget->item(0, i)->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled);
                ui->mapDataTableWidget->item(j, i)->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled);
            }
        }
    }
    else if (startRow == 0 && (objectName.at(3) == "3D" || objectName.at(3) == "X Axis 2D" || objectName.at(3) == "Y Axis 2D"))
    {
        for (int i = 0; i < cols; i++)
        {
            for (int j = 1; j < rows; j++)
            {
                //ui->mapDataTableWidget->item(j, 0)->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled);
                ui->mapDataTableWidget->item(j, i)->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled);
            }
        }
    }
    else
    {
        if (objectName.at(3) == "3D" || objectName.at(3) == "X Axis 2D")
        {
            for (int i = 0; i < cols; i++)
            {
                ui->mapDataTableWidget->item(0, i)->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled);
                ui->mapDataTableWidget->item(0, i)->setSelected(false);
            }
        }
        if (objectName.at(3) == "3D")
        {
            for (int j = 0; j < rows; j++)
            {
                ui->mapDataTableWidget->item(j, 0)->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled);
                ui->mapDataTableWidget->item(j, 0)->setSelected(false);
            }
        }
    }
    if (objectName.at(3) == "3D")
        ui->mapDataTableWidget->item(0, 0)->setFlags(Qt::ItemIsEditable);

    if (objectName.at(3) == "Static Y Axis 2D" && rows > 1)
    {
        for (int j = 0; j < cols; j++)
        {
            ui->mapDataTableWidget->item(0, j)->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled);
        }
    }
}
