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
#include "aerpspreadsheet.h"
#include <QtGlobal>
#include <QDebug>
#include "globales.h"
#include "models/filterbasebeanmodel.h"
#include "dao/beans/basebeanmetadata.h"

QList<AERPSpreadSheetIface *> AERPSpreadSheet::m_ifaces;

class AERPSpreadSheetPrivate
{
public:
    QString m_name;
    QMap<QString, AERPSheet *> m_sheets;
    QString m_lastMessage;
    QString m_type;
    QString m_path;

    AERPSpreadSheetPrivate() { }
};

class AERPSheetPrivate
{
public:
    QString m_name;
    int m_index;
    QHash<QString, QString> m_columnNames;
    QHash<QString, AERPSpreadSheet::Type> m_columnTypes;
    QHash<QString, int> m_columnLength;
    QHash<QString, int> m_columnDecimalPlaces;
    QList<AERPCell *> m_cells;
    QStringList m_rows;
    QStringList m_columns;
    bool m_hasColumnNames;

    AERPSheetPrivate()
    {
        m_index = 0;
        m_hasColumnNames = false;
    }
};

class AERPCellPrivate
{
public:
    QPointer<AERPSheet> m_sheet;
    QVariant m_value;
    QString m_row;
    QString m_column;
    int m_iRow;
    int m_iColumn;
    QString m_formula;
    AERPSpreadSheet::Type m_type;
    int m_length;
    int m_decimalPlaces;

    AERPCellPrivate()
    {
        m_type = AERPSpreadSheet::Invalid;
        m_length = -1;
        m_decimalPlaces = 0;
        m_iRow = -1;
        m_iColumn = -1;
    }
};

QVariant::Type AERPSpreadSheet::qtypeForAERPVariantType(AERPSpreadSheet::Type type)
{
    if ( type == AERPSpreadSheet::String )
    {
        return QVariant::String;
    }
    else if ( type == AERPSpreadSheet::StringList )
    {
        return QVariant::StringList;
    }
    else if ( type == AERPSpreadSheet::Int )
    {
        return QVariant::Int;
    }
    else if ( type == AERPSpreadSheet::LongLong )
    {
        return QVariant::LongLong;
    }
    else if ( type == AERPSpreadSheet::Double )
    {
        return QVariant::Double;
    }
    else if ( type == AERPSpreadSheet::Date )
    {
        return QVariant::Date;
    }
    else if ( type == AERPSpreadSheet::DateTime )
    {
        return QVariant::DateTime;
    }
    else if ( type == AERPSpreadSheet::Bool )
    {
        return QVariant::Bool;
    }
    return QVariant::Invalid;
}

AERPSpreadSheet::Type AERPSpreadSheet::aerptypeQVariantType(QVariant::Type type)
{
    if ( type == QVariant::String )
    {
        return AERPSpreadSheet::String;
    }
    else if ( type == QVariant::StringList )
    {
        return AERPSpreadSheet::StringList;
    }
    else if ( type == QVariant::Int )
    {
        return AERPSpreadSheet::Int;
    }
    else if ( type == QVariant::LongLong )
    {
        return AERPSpreadSheet::LongLong;
    }
    else if ( type == QVariant::Double )
    {
        return AERPSpreadSheet::Double;
    }
    else if ( type == QVariant::Date )
    {
        return AERPSpreadSheet::Date;
    }
    else if ( type == QVariant::DateTime )
    {
        return AERPSpreadSheet::DateTime;
    }
    else if ( type == QVariant::Bool )
    {
        return AERPSpreadSheet::Bool;
    }
    return AERPSpreadSheet::Invalid;
}

AERPSpreadSheet::AERPSpreadSheet(QObject *parent) :
    QObject(parent),
    d(new AERPSpreadSheetPrivate)
{
}

AERPSpreadSheet::~AERPSpreadSheet()
{
    delete d;
}

QString AERPSpreadSheet::lastMessage() const
{
    return d->m_lastMessage;
}

void AERPSpreadSheet::setLastMessage(const QString &value)
{
    d->m_lastMessage = value;
}

QString AERPSpreadSheet::name() const
{
    return d->m_name;
}

void AERPSpreadSheet::setName(const QString &value)
{
    d->m_name = value;
}

QList<AERPSheet *> AERPSpreadSheet::sheets() const
{
    return d->m_sheets.values();
}

QStringList AERPSpreadSheet::sheetNames() const
{
    return d->m_sheets.keys();
}

QString AERPSpreadSheet::type() const
{
    return d->m_type;
}

void AERPSpreadSheet::setType(const QString &type)
{
    d->m_type = type;
}

QString AERPSpreadSheet::path() const
{
    return d->m_path;
}

void AERPSpreadSheet::setPath(const QString &path)
{
    d->m_path = path;
}

void AERPSpreadSheet::loadPlugins()
{
    QString pluginDirPath = QString("%1/plugins/spreadsheet").arg(QApplication::applicationDirPath());
    QDir pluginDir(pluginDirPath);
#if defined(Q_OS_WIN)
    QStringList plugins = pluginDir.entryList(QStringList() << QString("*.dll"), QDir::Files);
#else
    QStringList plugins = pluginDir.entryList(QStringList() << QString("*.so"), QDir::Files);
#endif

    foreach (const QString &pluginFile, plugins)
    {
        AERPSpreadSheetIface *iface = NULL;
        QString pathPluginFile = QString("%1/%2").arg(pluginDirPath).arg(pluginFile);

        // Este comando puede dar "not real memory leaks" en valgrind. La razón, se puede encontrar aquí:
        // https://bugreports.qt-project.org/browse/QTBUG-25279
        QPluginLoader *pluginLoader = new QPluginLoader(pathPluginFile, qApp);
        CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
        if ( !pluginLoader->isLoaded() )
        {
            if ( !pluginLoader->load() )
            {
                CommonsFunctions::restoreOverrideCursor();
                qDebug() << trUtf8("Ha ocurrido un error cargando el plugin: %1. \nEl error es: %2").
                        arg(pathPluginFile).arg(pluginLoader->errorString());
            }
            else
            {
                iface = qobject_cast<AERPSpreadSheetIface *>(pluginLoader->instance());
                CommonsFunctions::restoreOverrideCursor();
                if ( !iface )
                {
                    qDebug() << trUtf8("No se cargó el plugin: %1. \nRazón desconocida.").arg(pathPluginFile);
                }
                else
                {
                    m_ifaces << iface;
                }
            }
        }
        else
        {
            iface = qobject_cast<AERPSpreadSheetIface *>(pluginLoader->instance());
            CommonsFunctions::restoreOverrideCursor();
            if ( !iface )
            {
                qDebug() << QObject::trUtf8("No se cargó el plugin: %1. \nRazón desconocida.").arg(pathPluginFile);
            }
            else
            {
                m_ifaces << iface;
            }
        }
    }
}

AERPSpreadSheet *AERPSpreadSheet::openSpreadSheet(const QString &file, const QString &type, int init, int offset)
{
    QString fileType = type;
    if ( type.isEmpty() )
    {
        QFileInfo fi(file);
        if ( fi.exists() )
        {
            fileType = fi.suffix().toLower();
        }
    }
    if ( AERPSpreadSheet::m_ifaces.isEmpty() )
    {
        AERPSpreadSheet::loadPlugins();
    }
    foreach (AERPSpreadSheetIface *iface, AERPSpreadSheet::m_ifaces)
    {
        if ( iface->type() == fileType )
        {
            if ( !iface->wasInited() && !iface->init() )
            {
                qDebug () << trUtf8("AERPSpreadSheet::openSpreadSheet: No se ha podido inicializar el driver. Error: [%1]").arg(iface->lastMessage());
                return NULL;
            }
            return iface->openFile(file, init, offset);
        }
    }
    return NULL;
}

QScriptValue AERPSpreadSheet::toScriptValue(QScriptEngine *engine, AERPSpreadSheet * const &in)
{
    QScriptValue qsBean = engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
    return qsBean;
}

void AERPSpreadSheet::fromScriptValue(const QScriptValue &object, AERPSpreadSheet *&out)
{
    out = qobject_cast<AERPSpreadSheet *>(object.toQObject());
}

QScriptValue AERPSpreadSheet::toScriptValueType(QScriptEngine *engine, const AERPSpreadSheet::Type &in)
{
    Q_UNUSED(engine)
    QScriptValue v = static_cast<int>(in);
    return v;
}

void AERPSpreadSheet::fromScriptValueType(const QScriptValue &object, AERPSpreadSheet::Type &out)
{
    out = static_cast<AERPSpreadSheet::Type>(static_cast<int>(object.toInteger()));
}

bool AERPSpreadSheet::appCanWriteToSomeFile()
{
    if ( AERPSpreadSheet::m_ifaces.isEmpty() )
    {
        AERPSpreadSheet::loadPlugins();
    }
    foreach (AERPSpreadSheetIface *iface, AERPSpreadSheet::m_ifaces)
    {
        if ( iface->canWriteFiles() )
        {
            return true;
        }
    }
    return false;
}

QList<AERPSpreadSheetIface *> AERPSpreadSheet::ifaces()
{
    if ( AERPSpreadSheet::m_ifaces.isEmpty() )
    {
        AERPSpreadSheet::loadPlugins();
    }
    return m_ifaces;
}

AERPSheet *AERPSpreadSheet::createSheet(const QString &value, int index)
{
    if ( !d->m_sheets.contains(value) )
    {
        d->m_sheets[value] = new AERPSheet(this);
        d->m_sheets[value]->setName(value);
        d->m_sheets[value]->setIndex(index);
    }
    return d->m_sheets[value];
}

AERPSheet *AERPSpreadSheet::sheet(const QString &value)
{
    if ( !d->m_sheets.contains(value) )
    {
        return NULL;
    }
    return d->m_sheets[value];
}

AERPSheet *AERPSpreadSheet::sheet(int index)
{
    foreach (AERPSheet *sheet, d->m_sheets.values())
    {
        if ( sheet->index() == index )
        {
            return sheet;
        }
    }
    return NULL;
}

bool AERPSpreadSheet::save()
{
    return saveAs();
}

bool AERPSpreadSheet::saveAs(const QString &path, const QString &typeToSave)
{
    QString file = path.isEmpty() ? d->m_path : path;
    QString type = typeToSave.isEmpty() ? d->m_type : typeToSave;

    if ( type.isEmpty() )
    {
        d->m_lastMessage = trUtf8("Debe especificar un tipo.");
        return false;
    }
    QScopedPointer<AERPSpreadSheet> spread(new AERPSpreadSheet());
    AERPSpreadSheetIface *iface = NULL;
    foreach (AERPSpreadSheetIface *i, AERPSpreadSheet::ifaces())
    {
        if ( i->type() == type )
        {
            iface = i;
            break;
        }
    }
    if ( iface == NULL )
    {
        d->m_lastMessage = trUtf8("Tipo desconocido.");
        return false;
    }
    if ( !iface->canWriteFiles() )
    {
        d->m_lastMessage = trUtf8("No es posible escribir en este tipo.");
        return false;
    }
    return iface->writeFile(this, file);
}

AERPSheet::AERPSheet(QObject *parent) :
    QObject(parent), d(new AERPSheetPrivate)
{

}

AERPSheet::~AERPSheet()
{
    delete d;
}

QString AERPSheet::name() const
{
    return d->m_name;
}

void AERPSheet::setName(const QString &value)
{
    d->m_name = value;
}

int AERPSheet::index() const
{
    return d->m_index;
}

void AERPSheet::setIndex(int idx)
{
    d->m_index = idx;
}

int AERPSheet::rowCount()
{
    return d->m_rows.count();
}

int AERPSheet::columnCount()
{
    return d->m_columns.size();
}

QList<AERPCell *> AERPSheet::cells() const
{
    return d->m_cells;
}

QStringList AERPSheet::columnNames() const
{
    if ( d->m_columnNames.isEmpty() )
    {
        return d->m_columns;
    }
    QStringList temp;
    foreach (const QString &col, d->m_columns)
    {
        temp.append(d->m_columnNames[col]);
    }
    return temp;
}

QStringList AERPSheet::columns() const
{
    return d->m_columns;
}

QStringList AERPSheet::rows() const
{
    return d->m_rows;
}

bool AERPSheet::hasColumnNames() const
{
    return d->m_hasColumnNames;
}

QScriptValue AERPSheet::toScriptValue(QScriptEngine *engine, AERPSheet * const &in)
{
    QScriptValue qsBean = engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
    return qsBean;
}

void AERPSheet::fromScriptValue(const QScriptValue &object, AERPSheet *&out)
{
    out = qobject_cast<AERPSheet *>(object.toQObject());
}

AERPCell *AERPSheet::cell(int rowId, int columnId)
{
    foreach (AERPCell *cell, d->m_cells)
    {
        if ( cell->rowIndex() == rowId && cell->columnIndex() == columnId )
        {
            return cell;
        }
    }
    return NULL;
}

AERPCell *AERPSheet::cell(const QString &row, const QString &column)
{
    foreach (AERPCell *cell, d->m_cells)
    {
        if ( cell->column() == column && cell->row() == row )
        return cell;
    }
    return NULL;
}

AERPCell *AERPSheet::cell(int rowId, const QString &column)
{
    foreach (AERPCell *cell, d->m_cells)
    {
        if ( cell->column() == column && cell->rowIndex() == rowId )
        {
            return cell;
        }
    }
    return NULL;
}

AERPCell *AERPSheet::createCell(int row, int column, const QVariant value)
{
    QString strRow(row);
    QString strColumn = QString('A' + column);
    return createCell(strRow, strColumn, value);
}

AERPCell *AERPSheet::createCell(const QString &row, const QString &column, const QVariant value)
{
    AERPCell *actualCell = cell(row, column);
    if ( actualCell == NULL )
   {
        AERPCell *c = new AERPCell(this);
        // Este orden es muy importante.
        if ( !d->m_columns.contains(column) )
        {
            d->m_columns.append(column);
        }
        if ( !d->m_rows.contains(row) )
        {
            d->m_rows.append(row);
        }
        c->setRow(row);
        c->setColumn(column);
        d->m_cells.append(c);
        c->setValue(value);
        return c;
    }
    return actualCell;
}

QVariant AERPSheet::cellValue(int rowId, int columnId)
{
    AERPCell *c = cell(rowId, columnId);
    if ( c != NULL )
    {
        return c->value();
    }
    return QVariant();
}

QVariant AERPSheet::cellValue(const QString &row, const QString &column)
{
    AERPCell *c = cell(row, column);
    if ( c != NULL )
    {
        return c->value();
    }
    return QVariant();
}

QVariant AERPSheet::cellValue(int rowId, const QString &column)
{
    AERPCell *c = cell(rowId, column);
    if ( c != NULL )
    {
        return c->value();
    }
    return QVariant();
}

QVariant AERPSheet::dateTimeCellValue(int rowId, int columnId)
{
    AERPCell *c = cell(rowId, columnId);
    if ( c != NULL )
    {
        return c->dateTimeValue();
    }
    return QVariant();
}

QVariant AERPSheet::dateTimeCellValue(const QString &row, const QString &column)
{
    AERPCell *c = cell(row, column);
    if ( c != NULL )
    {
        return c->dateTimeValue();
    }
    return QVariant();
}

QVariant AERPSheet::dateTimeCellValue(int rowId, const QString &column)
{
    AERPCell *c = cell(rowId, column);
    if ( c != NULL )
    {
        return c->dateTimeValue();
    }
    return QVariant();
}

AERPSpreadSheet::Type AERPSheet::cellType(int rowId, int columnId)
{
    AERPCell *c = cell(rowId, columnId);
    if ( c != NULL )
    {
        return c->type();
    }
    return AERPSpreadSheet::Invalid;
}

AERPSpreadSheet::Type AERPSheet::cellType(const QString &row, const QString &column)
{
    AERPCell *c = cell(row, column);
    if ( c != NULL )
    {
        return c->type();
    }
    return AERPSpreadSheet::Invalid;
}

AERPSpreadSheet::Type AERPSheet::cellType(int rowId, const QString &column)
{
    AERPCell *c = cell(rowId, column);
    if ( c != NULL )
    {
        return c->type();
    }
    return AERPSpreadSheet::Invalid;
}

int AERPSheet::cellLength(int rowId, int columnId)
{
    AERPCell *c = cell(rowId, columnId);
    if ( c != NULL )
    {
        return c->length();
    }
    return -1;
}

int AERPSheet::cellLength(const QString &row, const QString &column)
{
    AERPCell *c = cell(row, column);
    if ( c != NULL )
    {
        return c->length();
    }
    return -1;
}

int AERPSheet::cellLength(int rowId, const QString &column)
{
    AERPCell *c = cell(rowId, column);
    if ( c != NULL )
    {
        return c->length();
    }
    return -1;
}

int AERPSheet::cellDecimalPlaces(int rowId, int columnId)
{
    AERPCell *c = cell(rowId, columnId);
    if ( c != NULL )
    {
        return c->decimalPlaces();
    }
    return 0;
}

int AERPSheet::cellDecimalPlaces(const QString &row, const QString &column)
{
    AERPCell *c = cell(row, column);
    if ( c != NULL )
    {
        return c->decimalPlaces();
    }
    return 0;
}

int AERPSheet::cellDecimalPlaces(int rowId, const QString &column)
{
    AERPCell *c = cell(rowId, column);
    if ( c != NULL )
    {
        return c->decimalPlaces();
    }
    return 0;
}

void AERPSheet::addColumn(const QString &columnName)
{
    QString col = QString('A' + d->m_columns.size());
    d->m_columns.append(col);
    d->m_columnNames[col] = columnName;
    d->m_hasColumnNames = true;
}

void AERPSheet::setColumn(int index, const QString &columnName, AERPSpreadSheet::Type type, int length, int decimalPlaces)
{
    setColumnName(index, columnName);
    setColumnType(index, type);
    setColumnLength(index, length);
    setColumnDecimalPlaces(index, decimalPlaces);
}

void AERPSheet::setColumnName(const QString &col, const QString &columnName)
{
    if ( d->m_columns.contains(col) )
    {
        d->m_columnNames[col] = columnName;
        if ( col != columnName )
        {
            d->m_hasColumnNames = true;
        }
    }
    else
    {
        d->m_columns.append(col);
        d->m_columnNames[col] = columnName;
        if ( col != columnName )
        {
            d->m_hasColumnNames = true;
        }
    }
}

void AERPSheet::setColumnName(int index, const QString &columnName)
{
    if (AERP_CHECK_INDEX_OK(index, d->m_columns))
    {
        d->m_columnNames[d->m_columns.at(index)] = columnName;
        if ( d->m_columns.at(index) != columnName )
        {
            d->m_hasColumnNames = true;
        }
    }
    else
    {
        QString col = QString('A' + index);
        d->m_columns.append(col);
        d->m_columnNames[col] = columnName;
        d->m_hasColumnNames = true;
    }
}

QString AERPSheet::columnName(int index)
{
    if ( AERP_CHECK_INDEX_OK(index, d->m_columnNames) )
    {
        return d->m_columnNames.value(d->m_columns.at(index));
    }
    return QString("");
}

QString AERPSheet::columnName(const QString &column)
{
    return d->m_columnNames.value(column);
}

bool AERPSheet::containsColumn(const QString &column)
{
    return d->m_columns.contains(column);
}

bool AERPSheet::containsRow(const QString &row)
{
    return d->m_rows.contains(row);
}

void AERPSheet::setColumnType(int columnId, AERPSpreadSheet::Type type)
{
    if ( AERP_CHECK_INDEX_OK(columnId, d->m_columns) )
    {
        QString col = d->m_columns.at(columnId);
        d->m_columnTypes[col] = type;
    }
}

void AERPSheet::setColumnType(const QString &col, AERPSpreadSheet::Type type)
{
    d->m_columnTypes[col] = type;
}

AERPSpreadSheet::Type AERPSheet::columnType(int columnId)
{
    if ( AERP_CHECK_INDEX_OK(columnId, d->m_columns) )
    {
        QString col = d->m_columns.at(columnId);
        return d->m_columnTypes.value(col);
    }
    return AERPSpreadSheet::Invalid;
}

AERPSpreadSheet::Type AERPSheet::columnType(const QString &col)
{
    if ( !d->m_columnTypes.contains(col) )
    {
        return AERPSpreadSheet::Invalid;
    }
    return d->m_columnTypes.value(col);
}

void AERPSheet::setColumnLength(int columnId, int type)
{
    if ( AERP_CHECK_INDEX_OK(columnId, d->m_columns) )
    {
        QString col = d->m_columns.at(columnId);
        d->m_columnLength[col] = type;
    }
}

void AERPSheet::setColumnLength(const QString &col, int type)
{
    d->m_columnLength[col] = type;
}

int AERPSheet::columnLength(int columnId)
{
    if ( AERP_CHECK_INDEX_OK(columnId, d->m_columns) )
    {
        QString col = d->m_columns.at(columnId);
        return d->m_columnLength.value(col);
    }
    return -1;
}

int AERPSheet::columnLength(const QString &col)
{
    if ( !d->m_columnLength.contains(col) )
    {
        return -1;
    }
    return d->m_columnLength.value(col);
}

void AERPSheet::setColumnDecimalPlaces(int columnId, int type)
{
    if ( AERP_CHECK_INDEX_OK(columnId, d->m_columns) )
    {
        QString col = d->m_columns.at(columnId);
        d->m_columnDecimalPlaces[col] = type;
    }
}

void AERPSheet::setColumnDecimalPlaces(const QString &col, int type)
{
    d->m_columnDecimalPlaces[col] = type;
}

int AERPSheet::columnDecimalPlaces(int columnId)
{
    if ( AERP_CHECK_INDEX_OK(columnId, d->m_columns) )
    {
        QString col = d->m_columns.at(columnId);
        return d->m_columnDecimalPlaces.value(col);
    }
    return -1;
}

int AERPSheet::columnDecimalPlaces(const QString &col)
{
    if ( !d->m_columnDecimalPlaces.contains(col) )
    {
        return -1;
    }
    return d->m_columnDecimalPlaces.value(col);
}

AERPCell::AERPCell(QObject *parent) :
    QObject(parent),
    d(new AERPCellPrivate)
{
    d->m_sheet = qobject_cast<AERPSheet *>(parent);
}

AERPCell::~AERPCell()
{
    delete d;
}

QVariant AERPCell::value() const
{
    return d->m_value;
}

void AERPCell::setValue(const QVariant &v)
{
    if ( type() == AERPSpreadSheet::String && length() > 0 )
    {
        d->m_value = v.toString().left(length());
    }
    else
    {
        d->m_value = v;
    }
}

QDateTime AERPCell::dateTimeValue()
{
    bool ok;
    unsigned int daysFrom = value().toInt(&ok);
    if ( !ok )
    {
        return QDateTime();
    }
    QDateTime origin;
    origin.setDate(QDate(1900, 1, 1));
    QDateTime result = origin.addDays(daysFrom - 2);
    return result;
}

QString AERPCell::row() const
{
    return d->m_row;
}

int AERPCell::rowIndex() const
{
    return d->m_iRow;
}

void AERPCell::setRow(const QString &value)
{
    d->m_row = value;
    AERPSheet *sheet = qobject_cast<AERPSheet *>(parent());
    if ( sheet )
    {
        d->m_iRow = sheet->rows().indexOf(d->m_row);
    }
}

QString AERPCell::column() const
{
    return d->m_column;
}

int AERPCell::columnIndex() const
{
    return d->m_iColumn;
}

void AERPCell::setColumn(const QString &value)
{
    d->m_column = value;
    AERPSheet *sheet = qobject_cast<AERPSheet *>(parent());
    if ( sheet )
    {
        d->m_iColumn = sheet->columns().indexOf(d->m_column);
    }
}

QString AERPCell::formula() const
{
    return d->m_formula;
}

void AERPCell::setFormula(const QString &value)
{
    d->m_formula = value;
}

AERPSpreadSheet::Type AERPCell::type() const
{
    if ( d->m_sheet && d->m_sheet->columnType(d->m_column) != AERPSpreadSheet::Invalid )
    {
        return d->m_sheet->columnType(d->m_column);
    }
    if ( d->m_value.isValid() )
    {
        return AERPSpreadSheet::aerptypeQVariantType(d->m_value.type());
    }
    return d->m_type;
}

void AERPCell::setType(AERPSpreadSheet::Type type)
{
    d->m_type = type;
}

int AERPCell::length() const
{
    if ( d->m_length > -1 )
    {
        return d->m_length;
    }
    if ( d->m_sheet && d->m_sheet->columnLength(d->m_column) != AERPSpreadSheet::Invalid )
    {
        return d->m_sheet->columnLength(d->m_column);
    }
    return -1;
}

void AERPCell::setLength(int value)
{
    d->m_length = value;
}

int AERPCell::decimalPlaces() const
{
    if ( d->m_decimalPlaces > 0 )
    {
        return d->m_decimalPlaces;
    }
    if ( d->m_sheet && d->m_sheet->columnDecimalPlaces(d->m_column) != AERPSpreadSheet::Invalid )
    {
        return d->m_sheet->columnDecimalPlaces(d->m_column);
    }
    return 0;
}

void AERPCell::setDecimalPlaces(int value)
{
    d->m_decimalPlaces = value;
}

QString AERPCell::columnName() const
{
    QString r;
    if ( d->m_sheet )
    {
        r = d->m_sheet->columnName(d->m_column);
    }
    return r;
}

QScriptValue AERPCell::toScriptValue(QScriptEngine *engine, AERPCell * const &in)
{
    QScriptValue qsBean = engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
    return qsBean;
}

void AERPCell::fromScriptValue(const QScriptValue &object, AERPCell *&out)
{
    out = qobject_cast<AERPCell *>(object.toQObject());
}

AERPSpreadSheetUtil::AERPSpreadSheetUtil(QObject *parent) :
    QObject(parent)
{
    m_operationCanceled = false;
}

AERPSpreadSheetUtil::~AERPSpreadSheetUtil()
{

}

AERPSpreadSheetUtil *AERPSpreadSheetUtil::instance()
{
    static AERPSpreadSheetUtil *singleton;
    if ( singleton == NULL )
    {
        singleton = new AERPSpreadSheetUtil(qApp);
    }
    return singleton;
}

void AERPSpreadSheetUtil::operationCanceled()
{
    m_operationCanceled = true;
}

void AERPSpreadSheetUtil::exportSpreadSheet(FilterBaseBeanModel *filterModel, QWidget *uiParent)
{
    if ( filterModel == NULL )
    {
        return;
    }
    BaseBeanMetadata *metadata = filterModel->metadata();
    if ( metadata == NULL )
    {
        return;
    }
    QStringList displayTypes;

    foreach (AERPSpreadSheetIface *iface, AERPSpreadSheet::ifaces())
    {
        if ( iface->canWriteFiles() )
        {
            displayTypes << iface->displayType();
        }
    }

    bool ok;
    QString displayType = QInputDialog::getItem(uiParent,
                                                qApp->applicationName(),
                                                trUtf8("Seleccione el formato al que desea exportar la información."),
                                                displayTypes,
                                                0,
                                                false,
                                                &ok);
    if ( !ok )
    {
        return;
    }

    QString writeTo = QFileDialog::getExistingDirectory(uiParent,
                                                        trUtf8("Seleccione el directorio en el que guardar los datos"),
                                                        QDir::homePath());
    if ( writeTo.isEmpty() )
    {
        return;
    }
    AERPSpreadSheetIface *iface = NULL;
    foreach (AERPSpreadSheetIface *i, AERPSpreadSheet::ifaces())
    {
        if ( i->displayType() == displayType )
        {
            iface = i;
            break;
        }
    }
    if ( iface == NULL )
    {
        QMessageBox::warning(uiParent,
                             qApp->applicationName(),
                             trUtf8("Ha ocurrido un error exportando los datos. \nNo tiene configurado ningún plugin."));
        return;
    }
    writeTo.append("/").
            append(QDateTime::currentDateTime().toString("yyyyMMddHHmm")).
            append("-").
            append(metadata->alias()).
            append(".").
            append(iface->type());

    QProgressDialog dlg;
    m_operationCanceled = false;
    dlg.setMaximum(filterModel->rowCount());
    dlg.setMinimum(0);
    dlg.setLabelText(trUtf8("Exportando información... Por favor, espere."));
    dlg.setWindowTitle(trUtf8("%1 - Exportando datos").arg(qApp->applicationName()));
    dlg.setWindowModality(Qt::WindowModal);
    connect(&dlg, SIGNAL(canceled()), this, SLOT(operationCanceled()));
    connect(filterModel, SIGNAL(rowProcessed(int)), &dlg, SLOT(setValue(int)));
    dlg.show();
    qApp->processEvents();

    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    bool r = filterModel->exportToSpreadSheet(writeTo, iface->type());
    CommonsFunctions::restoreOverrideCursor();
    if ( !r )
    {
        QMessageBox::warning(uiParent,
                             qApp->applicationName(),
                             trUtf8("Ha ocurrido un error exportando los datos. \nEl error es: %1").arg(filterModel->lastErrorMessage()));
    }
}
