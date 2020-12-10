#include <QtWidgets>
#include <QPushButton>

#include "ParameterDefinitions.h"
#include "Parameter.h"
#include "ParametersManager.h"
#include "ParametersManagerDialog.h"

ParametersManagerDialog::ParametersManagerDialog(ParametersManager* ptrParametersManager,
                                                 QString command,
                                                 QWidget * parent)
    : QDialog(parent)
{
    if(ptrParametersManager==NULL)
    {
        QString title="Error en la construccion de un objeto ParametersManagerDialog:";
        QString msg="El puntero al objeto ptrParametersManager es nulo";
        QMessageBox::information(this,title,msg);
        return;
    }
    _ptrParametersManager=ptrParametersManager;
    int frameStyle = QFrame::Sunken | QFrame::Panel;

    _command=command;

    QGridLayout *gridLayout = new QGridLayout;
    gridLayout->setColumnStretch(1, 1);
    gridLayout->setColumnMinimumWidth(1, 150);
    int pos=0;
    
//    QString file;
//    file=_ptrParametersManager->getFileName();
//    QLabel* fileTag=new QLabel(("Fichero XML"));
//    fileLabel=new QLabel(file);
//    fileLabel->setFrameStyle(frameStyle);
//    gridLayout->addWidget(fileTag, pos, 0,1,1);
//    gridLayout->addWidget(fileLabel, pos, 1,1,3);
    
    createTabWidget();
//    gridLayout->addWidget(tabWidget, 3, 0,2,4);
    gridLayout->addWidget(tabWidget, 1, 0,2,4);
    //gridLayout->addWidget(deletePointButton, 5, 0);

//    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
//                                     | QDialogButtonBox::Cancel);
//    connect(buttonBox, SIGNAL(accepted()), this, SLOT(onOk()));
//    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QVBoxLayout *mainLayout=new QVBoxLayout();
    //mainLayout->addLayout(projectNameHBoxLayout);
    mainLayout->addLayout(gridLayout);
//    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);

    QString title="Gestor de parametros";
    if(!command.isEmpty())
    {
        title+=" del comando: ";
        title+=command;
    }
    setWindowTitle(title);

    fillTabWidget();

    setMinimumSize(850,400);
    this->exec();
}

void ParametersManagerDialog::createTabWidget()
{
    int frameStyle = QFrame::Sunken | QFrame::Panel;
    tabWidget = new QTabWidget;
    tabWidget->setSizePolicy(QSizePolicy::Preferred,
                             QSizePolicy::Ignored);

    QWidget *tab1 = new QWidget;
    //int numberOfParameters=_ptrParametersManager->getNumberOfParameters();
    //int numberOfParametersNoDeleted=_ptrParametersManager->getNumberOfParametersNoDeleted();
    //tableWidget = new QTableWidget(numberOfParametersNoDeleted, 11);
    tableWidget = new QTableWidget(0, 12);

    connect(tableWidget->verticalHeader(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(selectColumn(int)));
    QString _unit;
    int _printWidth;
    int _printPrecision;
    QString _printUnit;
    QStringList tableHorizontalHeaders;
    tableHorizontalHeaders<<("Codigo");
    tableHorizontalHeaders<<("Habilitado");
    tableHorizontalHeaders<<("Tipo");
    tableHorizontalHeaders<<("Unidad");
    //tableHorizontalHeaders<<"Unidad\n de impresion";
    //tableHorizontalHeaders<<"Factor Conversion";
    tableHorizontalHeaders<<("Valor");
    tableHorizontalHeaders<<("Valor minimo");
    tableHorizontalHeaders<<("Valor maximo");
    tableHorizontalHeaders<<("Tamaño de impresion");
    tableHorizontalHeaders<<("Precision de impresion");
    tableHorizontalHeaders<<("Lista de valores");
    tableHorizontalHeaders<<("Etiqueta");
    tableHorizontalHeaders<<("Descripcion");
    tableWidget->setHorizontalHeaderLabels(tableHorizontalHeaders);
    QHBoxLayout *tab1hbox = new QHBoxLayout;
    tab1hbox->setMargin(5);
    tab1hbox->addWidget(tableWidget);
    tab1->setLayout(tab1hbox);
    connect(tableWidget,SIGNAL(itemClicked(QTableWidgetItem*)),this,SLOT(tableWidgetClicked(QTableWidgetItem*)));
    tabWidget->addTab(tab1,("Parametros"));
    tableWidget->resizeColumnToContents(0);
}

void ParametersManagerDialog::fillTabWidget()
{
    int rowsTableWidget=tableWidget->rowCount();
    for(int i=0;i<rowsTableWidget;i++)
        tableWidget->removeRow(0);

    QVector<Parameter*> ptrParameters;
    if(!_ptrParametersManager->getParametersByCommand(_command,
                                                      ptrParameters))
    {
        QString strError=tr("Error recuperando parametros para el comando: %1").arg(_command);
        QMessageBox::about(this, tr("arametersManagerDialog::fillTabWidget"),
            strError);
        return;
    }
    int numberOfParameters=ptrParameters.size();
    for(int i=0;i<numberOfParameters;i++)
    {
        Parameter* ptrParameter=ptrParameters.at(i);
        int precision=ptrParameter->getPrintPrecision();
        QString strCode=ptrParameter->getCode();
        QString strEnabled="Si";
        if(!ptrParameter->isEnabled())
            strEnabled="No";
        QString strTag=ptrParameter->getTag();
        QString strType=ptrParameter->getType();
        QString strExplanation=ptrParameter->getExplanation();
        QString strPrintWidth=QString::number(ptrParameter->getPrintWidth(),10);
        //QString strUnit,strPrintUnit,strDefaultValue,strMaxValue,strMinValue;
        QString strUnit,strValue,strMaxValue,strMinValue;
        QString strPrintPrecision;
        QString strEnumValues;
        if(strType.compare(PARAMETER_TYPE_STRING,Qt::CaseInsensitive)==0)
        {
            ptrParameter->getValue(strValue);
        }
        if(strType.compare(PARAMETER_TYPE_ENUM,Qt::CaseInsensitive)==0)
        {
            ptrParameter->getEnumValues(strEnumValues);
            ptrParameter->getValue(strValue);
        }
        if(strType.compare(PARAMETER_TYPE_DOUBLE,Qt::CaseInsensitive)==0)
        {
            strUnit=ptrParameter->getUnit();
            //strPrintUnit=ptrParameter->getPrintUnit();
            //QString strFactorConversion=QString::number(ptrParameter->getFactorConversion(),'f',20);
            double value;
            ptrParameter->getValue(value);
            strValue=QString::number(value,'f',precision);
            ptrParameter->getMinValue(value);
            strMinValue=QString::number(value,'f',precision);
            ptrParameter->getMaxValue(value);
            strMaxValue=QString::number(value,'f',precision);
            strPrintPrecision=QString::number(precision);
        }
        if(strType.compare(PARAMETER_TYPE_INTEGER,Qt::CaseInsensitive)==0)
        {
            int value;
            ptrParameter->getValue(value);
            strValue=QString::number(value);
            ptrParameter->getMinValue(value);
            strMinValue=QString::number(value);
            ptrParameter->getMaxValue(value);
            strMaxValue=QString::number(value);
        }
        if(strType.compare(PARAMETER_TYPE_VECTOR_INTEGER,Qt::CaseInsensitive)==0)
        {
            ptrParameter->getIntMaxValues(strMaxValue);
            ptrParameter->getIntMinValues(strMinValue);
            ptrParameter->getIntValues(strValue);
        }
        if(strType.compare(PARAMETER_TYPE_VECTOR_TAG_VALUES,Qt::CaseInsensitive)==0)
        {
            QMap<QString,QString> tagAndValues;
            ptrParameter->getValue(tagAndValues);
            QMap<QString,QString>::const_iterator iterTagAndValues=tagAndValues.begin();
            strValue="";
            int cont=0;
            while(iterTagAndValues!=tagAndValues.end())
            {
                strValue+=iterTagAndValues.key();
                strValue+=TAG_VALUE_CHARACTER_SEPARATOR;
                strValue+=iterTagAndValues.value();
                cont++;
                if(cont<tagAndValues.size())
                {
                    strValue+=ENUM_CHARACTER_SEPARATOR;
                }
                iterTagAndValues++;
            }
        }
        QTableWidgetItem *itemCode = new QTableWidgetItem(strCode);
        QTableWidgetItem *itemEnabled = new QTableWidgetItem(strEnabled);
        QTableWidgetItem *itemTag = new QTableWidgetItem(strTag);
        QTableWidgetItem *itemType = new QTableWidgetItem(strType);
        QTableWidgetItem *itemUnit = new QTableWidgetItem(strUnit);
        //QTableWidgetItem *itemPrintUnit = new QTableWidgetItem(strPrintUnit);
        //QTableWidgetItem *itemFactorConversion = new QTableWidgetItem(strFactorConversion);
        QTableWidgetItem *itemDefaultValue = new QTableWidgetItem(strValue);
        QTableWidgetItem *itemMinValue = new QTableWidgetItem(strMinValue);
        QTableWidgetItem *itemMaxValue= new QTableWidgetItem(strMaxValue);
        QTableWidgetItem *itemPrintWidth = new QTableWidgetItem(strPrintWidth);
        QTableWidgetItem *itemPrintPrecision = new QTableWidgetItem(strPrintPrecision);
        QTableWidgetItem *itemEnumValues = new QTableWidgetItem(strEnumValues);
        QTableWidgetItem *itemExplanation = new QTableWidgetItem(strExplanation);
        itemCode->setTextAlignment(Qt::AlignHCenter);
        itemCode->setFlags(Qt::ItemIsSelectable);
        itemCode->setBackground(QBrush(QColor(Qt::white),Qt::SolidPattern));
        itemCode->setForeground(QBrush(QColor(Qt::black),Qt::SolidPattern));
        itemEnabled->setTextAlignment(Qt::AlignHCenter);
        itemEnabled->setFlags(Qt::ItemIsSelectable);
        itemEnabled->setBackground(QBrush(QColor(Qt::white),Qt::SolidPattern));
        itemEnabled->setForeground(QBrush(QColor(Qt::black),Qt::SolidPattern));
        itemTag->setTextAlignment(Qt::AlignHCenter);
        itemTag->setFlags(Qt::ItemIsSelectable);
        itemTag->setBackground(QBrush(QColor(Qt::white),Qt::SolidPattern));
        itemTag->setForeground(QBrush(QColor(Qt::black),Qt::SolidPattern));
        itemType->setTextAlignment(Qt::AlignHCenter);
        itemType->setFlags(Qt::ItemIsSelectable);
        itemType->setBackground(QBrush(QColor(Qt::white),Qt::SolidPattern));
        itemType->setForeground(QBrush(QColor(Qt::black),Qt::SolidPattern));
        itemUnit->setTextAlignment(Qt::AlignHCenter);
        itemUnit->setFlags(Qt::ItemIsSelectable);
        itemUnit->setBackground(QBrush(QColor(Qt::white),Qt::SolidPattern));
        itemUnit->setForeground(QBrush(QColor(Qt::black),Qt::SolidPattern));
        //itemPrintUnit->setTextAlignment(Qt::AlignHCenter);
        //itemPrintUnit->setFlags(Qt::ItemIsSelectable);
        //itemPrintUnit->setBackground(QBrush(QColor(Qt::white),Qt::SolidPattern));
        //itemPrintUnit->setForeground(QBrush(QColor(Qt::black),Qt::SolidPattern));
        //itemFactorConversion->setTextAlignment(Qt::AlignHCenter);
        itemDefaultValue->setTextAlignment(Qt::AlignHCenter);
        itemDefaultValue->setFlags(Qt::ItemIsSelectable);
        itemDefaultValue->setBackground(QBrush(QColor(Qt::white),Qt::SolidPattern));
        itemDefaultValue->setForeground(QBrush(QColor(Qt::black),Qt::SolidPattern));
        itemMinValue->setTextAlignment(Qt::AlignHCenter);
        itemMinValue->setFlags(Qt::ItemIsSelectable);
        itemMinValue->setBackground(QBrush(QColor(Qt::white),Qt::SolidPattern));
        itemMinValue->setForeground(QBrush(QColor(Qt::black),Qt::SolidPattern));
        itemMaxValue->setTextAlignment(Qt::AlignHCenter);
        itemMaxValue->setFlags(Qt::ItemIsSelectable);
        itemMaxValue->setBackground(QBrush(QColor(Qt::white),Qt::SolidPattern));
        itemMaxValue->setForeground(QBrush(QColor(Qt::black),Qt::SolidPattern));
        itemPrintWidth->setTextAlignment(Qt::AlignHCenter);
        itemPrintWidth->setFlags(Qt::ItemIsSelectable);
        itemPrintWidth->setBackground(QBrush(QColor(Qt::white),Qt::SolidPattern));
        itemPrintWidth->setForeground(QBrush(QColor(Qt::black),Qt::SolidPattern));
        itemPrintPrecision->setTextAlignment(Qt::AlignHCenter);
        itemPrintPrecision->setFlags(Qt::ItemIsSelectable);
        itemPrintPrecision->setBackground(QBrush(QColor(Qt::white),Qt::SolidPattern));
        itemPrintPrecision->setForeground(QBrush(QColor(Qt::black),Qt::SolidPattern));
        itemEnumValues->setTextAlignment(Qt::AlignHCenter);
        itemEnumValues->setFlags(Qt::ItemIsSelectable);
        itemEnumValues->setBackground(QBrush(QColor(Qt::white),Qt::SolidPattern));
        itemEnumValues->setForeground(QBrush(QColor(Qt::black),Qt::SolidPattern));
        itemExplanation->setTextAlignment(Qt::AlignHCenter);
        itemExplanation->setFlags(Qt::ItemIsSelectable);
        itemExplanation->setBackground(QBrush(QColor(Qt::white),Qt::SolidPattern));
        itemExplanation->setForeground(QBrush(QColor(Qt::black),Qt::SolidPattern));
        tableWidget->insertRow(i);
        tableWidget->setItem(i, 0, itemCode);
        tableWidget->setItem(i, 1, itemEnabled);
        tableWidget->setItem(i, 2, itemType);
        tableWidget->setItem(i, 3, itemUnit);
        //tableWidget->setItem(i, 3, itemPrintUnit);
        //tableWidget->setItem(_nParametersShow, 3, itemFactorConversion);
        tableWidget->setItem(i, 4, itemDefaultValue);
        tableWidget->setItem(i, 5, itemMinValue);
        tableWidget->setItem(i, 6, itemMaxValue);
        tableWidget->setItem(i, 7, itemPrintWidth);
        tableWidget->setItem(i, 8, itemPrintPrecision);
        tableWidget->setItem(i, 9, itemEnumValues);
        tableWidget->setItem(i, 10, itemTag);
        tableWidget->setItem(i, 11, itemExplanation);
        tableWidget->resizeColumnToContents(0);
        tableWidget->resizeColumnToContents(1);
        tableWidget->resizeColumnToContents(2);
        //tableWidget->resizeColumnToContents(3);
        tableWidget->resizeColumnToContents(3);
        tableWidget->resizeColumnToContents(4);
        tableWidget->resizeColumnToContents(5);
        tableWidget->resizeColumnToContents(6);
        tableWidget->resizeColumnToContents(7);
        tableWidget->resizeColumnToContents(8);
        tableWidget->resizeColumnToContents(9);
        tableWidget->resizeColumnToContents(10);
        tableWidget->resizeColumnToContents(11);
        //tableWidget->resizeColumnToContents(11);
    }
    tableWidget->resizeColumnToContents(0);
}

void ParametersManagerDialog::onOk()
{
    bool allOk=false;
    QString noStr;
    QString fileName=fileLabel->text();
//    if(fileName.compare(noStr)==0)
//    {
//        fileName = QFileDialog::getSaveFileName(this,
//                                ("Seleccione fichero para guardar los parametros"),
//                                _initialPath,
//                                ("Fichero XML (*.xml)"));
//    }
    if (!fileName.isEmpty())
    {
        _ptrParametersManager->setFileName(fileName);
        if(!_ptrParametersManager->saveAsXml())
        {
            QString strError="Error";
            QMessageBox::about(this, tr("Grabacion de parametros:"),
                strError);
            return;
        }
        accept();
    }
}

void ParametersManagerDialog::tableWidgetClicked(QTableWidgetItem* twi)
{
    int row=tableWidget->row(twi);
    int column=tableWidget->column(twi);
    if(column!=1&&column!=4)
    {
        return;
    }
    QString strParameterCode=(tableWidget->item(row,0))->text();
    Parameter* ptrParameter=_ptrParametersManager->getParameter(strParameterCode);
    bool changed=false;
    if(column==1)
    {
        QStringList items;
        items << tr("Si") << tr("No");
        int defaultValue=0;
        if(!ptrParameter->isEnabled())
            defaultValue=1;
        bool ok;
        QString item = QInputDialog::getItem(this, tr("Parametro: %1").arg(strParameterCode),
                                             tr("Habilitado:"), items, defaultValue, false, &ok);
        if (ok && !item.isEmpty())
        {
            bool enabled=true;
            if(item.compare("No")==0)
                enabled=false;
            if((!ptrParameter->isEnabled()&&enabled)
                    ||(ptrParameter->isEnabled()&&!enabled))
            {
                changed=true;
                ptrParameter->setEnabled(enabled);
            }
        }
    }
    if(column==4)
    {
        QString type=ptrParameter->getType();
        if(type.compare(PARAMETER_TYPE_DOUBLE,Qt::CaseInsensitive)==0)
        {
            double initialValue,minValue,maxValue,value;
            ptrParameter->getValue(initialValue);
            ptrParameter->getMaxValue(maxValue);
            ptrParameter->getMinValue(minValue);
            int precision=ptrParameter->getPrintPrecision();
            bool control=true;
            bool ok=false;
            while(control)
            {
                QString msg=tr("Valor [%1,%2]:")
                        .arg(QString::number(minValue,'f',precision))
                        .arg(QString::number(maxValue,'f',precision));
                QString inputStrValue=QInputDialog::getText(this, tr("Parametro: %1").arg(strParameterCode),
                                                            msg,QLineEdit::Normal,
                                                            QString::number(initialValue,'f',precision),&ok);
                if(ok)
                {
                    value=inputStrValue.toDouble(&ok);
                    if(ok&&(value>=minValue&&value<=maxValue))
                        control=false;
                }
                else
                    control=false;
            }
            if(ok)
            {
                ptrParameter->setValue(value);
                changed=true;
            }
        }
        if(type.compare(PARAMETER_TYPE_INTEGER,Qt::CaseInsensitive)==0)
        {
            int initialValue,minValue,maxValue,value;
            ptrParameter->getValue(initialValue);
            ptrParameter->getMaxValue(maxValue);
            ptrParameter->getMinValue(minValue);
            bool control=true;
            bool ok=false;
            while(control)
            {
                QString msg=tr("Valor [%1,%2]:")
                        .arg(QString::number(minValue))
                        .arg(QString::number(maxValue));
                QString inputStrValue=QInputDialog::getText(this, tr("Parametro: %1").arg(strParameterCode),
                                                            msg,QLineEdit::Normal,
                                                            QString::number(initialValue),&ok);
                if(ok)
                {
                    value=inputStrValue.toInt(&ok);
                    if(ok&&(value>=minValue&&value<=maxValue))
                        control=false;
                }
                else
                    control=false;
            }
            if(ok)
            {
                ptrParameter->setValue(value);
                changed=true;
            }
        }
        if(type.compare(PARAMETER_TYPE_STRING,Qt::CaseInsensitive)==0)
        {
            QString initialValue,value;
            ptrParameter->getValue(initialValue);
            bool control=true;
            bool ok=false;
            while(control)
            {
                QString msg=tr("Valor:");
                QString inputStrValue=QInputDialog::getText(this, tr("Parametro: %1").arg(strParameterCode),
                                                            msg,QLineEdit::Normal,initialValue,&ok);
                if(ok)
                {
                    value=inputStrValue.trimmed();
                    control=false;
                }
                else
                    control=false;
            }
            if(ok)
            {
                ptrParameter->setValue(value);
                changed=true;
            }
        }
        if(type.compare(PARAMETER_TYPE_ENUM,Qt::CaseInsensitive)==0)
        {
            QString initialValue,value;
            ptrParameter->getValue(initialValue);
            QVector<QString> values=ptrParameter->getEnumValues();
            QStringList items;
            for(int nv=0;nv<values.size();nv++)
            {
                items.append(values.at(nv));
            }
            int pos=items.indexOf(initialValue);
            if(pos<0)
                pos=0;
            bool ok;
            QString msg="Valor:";
            QString item = QInputDialog::getItem(this,tr("Parametro: %1").arg(strParameterCode),
                                                  msg, items, pos, false, &ok);
            if (ok&&item.compare(initialValue,Qt::CaseInsensitive)!=0)
            {
                ptrParameter->setValue(item);
                changed=true;
            }
        }
        if(type.compare(PARAMETER_TYPE_VECTOR_TAG_VALUES,Qt::CaseInsensitive)==0)
        {
            QMap<QString,QString> tagAndValues;
            ptrParameter->getValue(tagAndValues);
            QMap<QString,QString>::const_iterator iterTagAndValues=tagAndValues.begin();
            QString strInitialValue="";
            int cont=0;
            while(iterTagAndValues!=tagAndValues.end())
            {
                strInitialValue+=iterTagAndValues.key();
                strInitialValue+=TAG_VALUE_CHARACTER_SEPARATOR;
                strInitialValue+=iterTagAndValues.value();
                cont++;
                if(cont<tagAndValues.size())
                {
                    strInitialValue+=ENUM_CHARACTER_SEPARATOR;
                }
                iterTagAndValues++;
            }
            bool control=true;
            bool ok=false;
            QMap<QString,QString> newTagAndValues;
            while(control)
            {
                QString msg=tr("Valor:");
                QString inputStrValue=QInputDialog::getText(this, tr("Parametro: %1").arg(strParameterCode),
                                                            msg,QLineEdit::Normal,strInitialValue,&ok);
                control=false;
                strInitialValue=strInitialValue.trimmed();
                if(ok&&inputStrValue.compare(strInitialValue)!=0&&!inputStrValue.isEmpty())
                {
                    QStringList tagAndValuesList=inputStrValue.trimmed().split(ENUM_CHARACTER_SEPARATOR);
                    if(tagAndValuesList.size()>0)
                    {
                        bool sucess=true;
                        for(int k=0;k<tagAndValuesList.size();k++)
                        {
                            QStringList tagAndValue=tagAndValuesList.at(k).split(TAG_VALUE_CHARACTER_SEPARATOR);
                            if(tagAndValue.size()!=2)
                            {
                                QString strError=tr("Debe introducir cadenas etiqueta/valor. Incorrecto en caso %2")
                                        .arg(QString::number(k+1));
                                QMessageBox::about(this, tr("Parametro: %1").arg(strParameterCode),strError);
                                sucess=false;
                                break;
                            }
                            newTagAndValues[tagAndValue.at(0).trimmed()]=tagAndValue.at(1).trimmed();
                        }
                        if(sucess)
                        {
                            ptrParameter->setValue(newTagAndValues);
                        }
                        else
                        {
                            control=true;
                        }
                    }
                }
                if(ok&&inputStrValue.isEmpty())
                {
                    ptrParameter->setValue(newTagAndValues); // porque acepto que esté vacío
                }
            }
        }
        if(type.compare(PARAMETER_TYPE_VECTOR_INTEGER,Qt::CaseInsensitive)==0)
        {
            QString initialValue,strMinValues,strMaxValues,inputStrValue;
            ptrParameter->getIntValues(initialValue);
            ptrParameter->getIntMinValues(strMinValues);
            ptrParameter->getIntMaxValues(strMaxValues);
            QStringList strListMinValues=strMinValues.split(ENUM_CHARACTER_SEPARATOR,QString::SkipEmptyParts);
            QStringList strListMaxValues=strMaxValues.split(ENUM_CHARACTER_SEPARATOR,QString::SkipEmptyParts);
            QVector<int> minValues,maxValues;
            for(int nv=0;nv<strListMinValues.size();nv++)
            {
                minValues.push_back(strListMinValues.at(nv).toInt());
                maxValues.push_back(strListMaxValues.at(nv).toInt());
            }
            bool control=true;
            bool ok=false;
            while(control)
            {
                QString msg=tr("Valores [[%1],[%2]:").arg(strMinValues).arg(strMaxValues);
                inputStrValue=QInputDialog::getText(this, tr("Parametro: %1").arg(strParameterCode),
                                                            msg,QLineEdit::Normal,
                                                            initialValue,&ok);
                inputStrValue=inputStrValue.trimmed();
                if(ok)
                {
                    QStringList strListInputValues=inputStrValue.split(ENUM_CHARACTER_SEPARATOR,QString::SkipEmptyParts);
                    if(strListInputValues.size()!=strListMinValues.size())
                    {
                        QString strError=tr("Debe introducir %1 valores en el dominio definido separados por %2")
                                .arg(QString::number(strListMinValues.size())).arg(ENUM_CHARACTER_SEPARATOR);
                        QMessageBox::about(this, tr("Parametro: %1").arg(strParameterCode),strError);
                    }
                    else
                    {
                        bool valuesInDomain=true;
                        for(int niv=0;niv<strListInputValues.size();niv++)
                        {
                            bool okToInt=false;
                            int inputValue=strListInputValues.at(niv).toInt(&okToInt);
                            if(okToInt)
                            {
                                if(inputValue<minValues.at(niv)
                                        ||inputValue>=maxValues.at(niv))
                                {
                                    valuesInDomain=false;
                                    break;
                                }
                            }
                            if(valuesInDomain)
                            {
                                control=false;
                            }
                        }
                    }
                }
                else
                    control=false;
            }
            if(ok)
            {
                ptrParameter->setIntValues(inputStrValue);
                changed=true;
            }
        }
    }
    if(changed)
    {
        if(!_ptrParametersManager->saveAsXml(this))
        {
            QString strError="Error guardando la modificacion de los parametros en el fichero:\n";
            strError+=_ptrParametersManager->getFileName();
            QMessageBox::about(this, tr("Grabacion de parametros:"),
                strError);
            return;
        }
    }
    fillTabWidget();
}
