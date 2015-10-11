/***************************************************************************
 *   Copyright (C) 2014 by David Pinelo   *
 *   alepherp@alephsistemas.es   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "dbbarcode.h"
#include "qzint.h"
#include "barcodeitem.h"
#include "dao/dbfieldobserver.h"
#include "dao/beans/basebean.h"
#include "dao/beans/dbfield.h"

static QHash<QString, int> m_allowedBarCodes;

static void insertBarCodesTypes()
{
    m_allowedBarCodes["Aztec Code (ISO 24778)"] = BARCODE_AZTEC;
    m_allowedBarCodes["Aztec Runes"] =
    m_allowedBarCodes["Channel Code"] = BARCODE_CHANNEL;
    m_allowedBarCodes["Codabar"] = BARCODE_CODABAR;
    m_allowedBarCodes["Code 11"] = BARCODE_CODE11;
    m_allowedBarCodes["Code 128 (ISO 15417)"] = BARCODE_CODE128B;
    m_allowedBarCodes["Code 16k"] = BARCODE_CODE16K;
    m_allowedBarCodes["Code 2 of 5 Data Logic"] = BARCODE_C25LOGIC;
    m_allowedBarCodes["Code 2 of 5 IATA"] = BARCODE_C25IATA;
    m_allowedBarCodes["Code 2 of 5 Industrial"] = BARCODE_C25IND;
    m_allowedBarCodes["Code 2 of 5 Interleaved"] = BARCODE_C25INTER;
    m_allowedBarCodes["Code 2 of 5 Matrix"] = BARCODE_C25MATRIX;
    m_allowedBarCodes["Code 32 (Italian Pharmacode)"] = BARCODE_CODE32;
    m_allowedBarCodes["Code 39 (ISO 16388)"] = BARCODE_HIBC_39;
    m_allowedBarCodes["Code 39 Extended"] = BARCODE_EXCODE39;
    m_allowedBarCodes["Code 49"] = BARCODE_CODE49;
    m_allowedBarCodes["Code 93"] = BARCODE_CODE93;
    m_allowedBarCodes["Code One"] = BARCODE_CODEONE;
    m_allowedBarCodes["Data Matrix (ISO 16022)"] = BARCODE_DATAMATRIX;
    m_allowedBarCodes["EAN-14"] = BARCODE_EAN14;
    m_allowedBarCodes["European Article Number (EAN)"] = BARCODE_EAN128;
    m_allowedBarCodes["Grid Matrix"] = BARCODE_GRIDMATRIX;
    m_allowedBarCodes["ITF-14"] = BARCODE_ITF14;
    m_allowedBarCodes["International Standard Book Number (ISBN)"] = BARCODE_ISBNX;
    m_allowedBarCodes["Maxicode (ISO 16023)"] = BARCODE_MAXICODE;
    m_allowedBarCodes["Micro QR Code"] = BARCODE_MICROQR;
    m_allowedBarCodes["MSI Plessey"] = BARCODE_MSI_PLESSEY;
    m_allowedBarCodes["NVE-18"] = BARCODE_NVE18;
    m_allowedBarCodes["Pharmacode"] = BARCODE_PHARMA;
    m_allowedBarCodes["Pharmacode 2-track"] = BARCODE_PHARMA_TWO;
    m_allowedBarCodes["Postnet"] = BARCODE_POSTNET;
    m_allowedBarCodes["QR Code (ISO 18004)"] = BARCODE_QRCODE;
    m_allowedBarCodes["Telepen"] = BARCODE_TELEPEN;
    m_allowedBarCodes["Telepen Numeric"] = BARCODE_TELEPEN_NUM;
    m_allowedBarCodes["Universal Product Code (UPC-A)"] = BARCODE_UPCA;
    m_allowedBarCodes["Universal Product Code (UPC-E)"] = BARCODE_UPCE;
}

class DBBarCodePrivate
{
public:
    DBBarCode *q_ptr;
    BarcodeItem *m_bc;
    QPointer<QGraphicsScene> m_scene;
    int m_format;
    QString m_barCodeTypeFieldName;
    QString m_barCodeType;

    DBBarCodePrivate(DBBarCode *qq) : q_ptr(qq)
    {
        m_format = -1;
        m_bc = NULL;
    }

    DBField *barCodeTypeField(BaseBeanPointer bean);
};

DBField *DBBarCodePrivate::barCodeTypeField(BaseBeanPointer bean)
{
    if ( bean.isNull() )
    {
        return NULL;
    }

    DBField *fld = NULL;
    QList<DBObject *> listObj = bean->navigateThrough(m_barCodeTypeFieldName, QString());
    if ( listObj.size() == 0 )
    {
        DBObject *obj = bean->navigateThroughProperties(m_barCodeTypeFieldName);
        if ( obj != NULL )
        {
            fld = qobject_cast<DBField *>(obj);
        }
    }
    else
    {
        fld = qobject_cast<DBField *>(listObj.at(0));
    }
    return fld;
}

void DBBarCode::showEvent(QShowEvent *event)
{
    if ( !property(AlephERP::stInited).toBool() )
    {
        DBField *fld = d->barCodeTypeField(DBBaseWidget::beanFromContainer());
        if ( fld != NULL )
        {
            connect(fld, SIGNAL(valueModified(QVariant)), this, SLOT(setBarCodeType(QVariant)));
            connect(fld, SIGNAL(valueModified(QVariant)), this, SLOT(refresh()));
            setProperty(AlephERP::stInited, true);
        }
    }
    DBBaseWidget::showEvent(event);
    d->m_bc->update();
    scene()->update();
}

DBBarCode::DBBarCode(QWidget *parent) :
    QGraphicsView(parent), DBBaseWidget(), d(new DBBarCodePrivate(this))
{
    d->m_scene = new QGraphicsScene(this);
    d->m_bc = new BarcodeItem;
    setScene(d->m_scene);
    scene()->addItem(d->m_bc);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameShape(QFrame::NoFrame);
}

DBBarCode::~DBBarCode()
{
    delete d;
}

void DBBarCode::applyFieldProperties()
{

}

QVariant DBBarCode::value()
{
    return QVariant();
}

QStringList DBBarCode::allowedBarCodes()
{
    if ( m_allowedBarCodes.isEmpty() )
    {
        insertBarCodesTypes();
    }

    return m_allowedBarCodes.keys();
}

QString DBBarCode::barCodeTypeFieldName() const
{
    return d->m_barCodeTypeFieldName;
}

void DBBarCode::setBarCodeTypeFieldName(const QString &value)
{
    d->m_barCodeTypeFieldName = value;
}

QString DBBarCode::barCodeType() const
{
    return d->m_barCodeType;
}

void DBBarCode::setValue(const QVariant &value)
{
    if ( d->m_format == -1 )
    {
        DBField *fld = d->barCodeTypeField(DBBaseWidget::beanFromContainer());
        if ( fld != NULL )
        {
            setBarCodeType(fld->value());
        }
        if ( d->m_format == -1 )
        {
            return;
        }
    }

    d->m_bc->ar = (Zint::QZint::AspectRatioMode) 1;

    d->m_bc->bc.setText(value.toString());
    d->m_bc->bc.setSecurityLevel(0);
    d->m_bc->bc.setWidth(0);
    d->m_bc->bc.setInputMode(UNICODE_MODE);
    d->m_bc->bc.setHideText(false);

    d->m_bc->bc.setSymbol(d->m_format);

    d->m_bc->bc.setFgColor(Qt::black);
    d->m_bc->bc.setBgColor(Qt::white);
    fitInView(d->m_bc);
    d->m_bc->update();
    scene()->update();
}

void DBBarCode::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    fitInView(d->m_bc);
    d->m_bc->update();
    scene()->update();
}

void DBBarCode::refresh()
{
    observer();
    if ( m_observer != NULL )
    {
        m_observer->sync();
    }
}

void DBBarCode::observerUnregistered()
{
    DBBaseWidget::observerUnregistered();
    bool blockState = blockSignals(true);
    qDeleteAll(d->m_scene->items());
    blockSignals(blockState);
}

void DBBarCode::setBarCodeType(QVariant value)
{
    if ( m_allowedBarCodes.isEmpty() )
    {
        insertBarCodesTypes();
    }

    if ( m_allowedBarCodes.contains(value.toString()) )
    {
        d->m_format = m_allowedBarCodes.value(value.toString());
    }
    else
    {
        d->m_format = -1;
    }
}

/*!
  Tenemos que decirle al motor de scripts, que DBFormDlg se convierte de esta forma a un valor script
  */
QScriptValue DBBarCode::toScriptValue(QScriptEngine *engine, DBBarCode * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBBarCode::fromScriptValue(const QScriptValue &object, DBBarCode * &out)
{
    out = qobject_cast<DBBarCode *>(object.toQObject());
}

