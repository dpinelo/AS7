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
#ifndef AERPSPREADSHEET_H
#define AERPSPREADSHEET_H

#include <QtCore>
#include <QtScript>
#include <alepherpglobal.h>
#include "aerpspreadsheetiface.h"

class AERPSpreadSheetPrivate;
class AERPSheetPrivate;
class AERPCellPrivate;
class FilterBaseBeanModel;

class AERPSheet;
class AERPCell;

class ALEPHERP_DLL_EXPORT AERPSpreadSheetUtil : public QObject
{
    Q_OBJECT

private:
    explicit AERPSpreadSheetUtil(QObject *parent);

public:
    virtual ~AERPSpreadSheetUtil();

    static AERPSpreadSheetUtil *instance();

public slots:
    void exportSpreadSheet(FilterBaseBeanModel *filterModel, QWidget *uiParent);
};

/**
 * @brief The AERPSpreadSheet class
 * Representa una hoja de cálculo estándar para su interacción con el sistema
 *
 */
class ALEPHERP_DLL_EXPORT AERPSpreadSheet : public QObject
{
    Q_OBJECT
    Q_ENUMS(Type)

    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(QList<AERPSheet *> sheets READ sheets)
    Q_PROPERTY(QStringList sheetNames READ sheetNames)
    Q_PROPERTY(QString lastMessage READ lastMessage WRITE setLastMessage)
    Q_PROPERTY(QString type READ type WRITE setType)
    Q_PROPERTY(QString path READ path WRITE setPath)

private:
    AERPSpreadSheetPrivate *d;

    static QList<AERPSpreadSheetIface *> m_ifaces;

public:
    explicit AERPSpreadSheet(QObject *parent = 0);
    virtual ~AERPSpreadSheet();

    enum Type {
        Invalid = QMetaType::UnknownType,
        Bool = QMetaType::Bool,
        Int = QMetaType::Int,
        LongLong = QMetaType::LongLong,
        Double = QMetaType::Double,
        Char = QMetaType::QChar,
        String = QMetaType::QString,
        StringList = QMetaType::QStringList,
        Date = QMetaType::QDate,
        DateTime = QMetaType::QDateTime,
    };
    static QVariant::Type qtypeForAERPVariantType(Type type);
    static Type aerptypeQVariantType(QVariant::Type type);
    static QString columnStringName(int column);

    QString lastMessage() const;
    void setLastMessage(const QString &value);
    QString name() const;
    void setName(const QString &value);
    QList<AERPSheet *> sheets() const;
    QStringList sheetNames() const;
    QString type() const;
    void setType(const QString &type);
    QString path() const;
    void setPath(const QString &path);

    static void loadPlugins();
    static AERPSpreadSheet *openSpreadSheet(const QString &file, const QString &type = "", int init = -1, int offset = -1);
    static bool appCanWriteToSomeFile();
    static QList<AERPSpreadSheetIface *> ifaces();

    static QScriptValue toScriptValue(QScriptEngine *engine, AERPSpreadSheet * const &in);
    static void fromScriptValue(const QScriptValue &object, AERPSpreadSheet * &out);
    static QScriptValue toScriptValueType(QScriptEngine *engine, Type const &in);
    static void fromScriptValueType(const QScriptValue &object, Type  &out);

signals:

public slots:
    AERPSheet *createSheet(const QString &value, int index = 0);
    AERPSheet *sheet(const QString &value);
    AERPSheet *sheet(int index);
    bool save();
    bool saveAs(const QString &path = "", const QString &typeToSave = "");
};

/**
 * @brief The AERPSheet class
 * Representa una hoja dentro de una hoja de cálculo
 */
class ALEPHERP_DLL_EXPORT AERPSheet : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(int index READ index)
    Q_PROPERTY(int rowCount READ rowCount)
    Q_PROPERTY(int columnCount READ columnCount)
    Q_PROPERTY(QList<AERPCell *> cells READ cells)
    Q_PROPERTY(QStringList columns READ columns)
    Q_PROPERTY(QStringList rows READ rows)
    Q_PROPERTY(QStringList columnNames READ columnNames)
    Q_PROPERTY(bool hasColumnNames READ hasColumnNames)

private:
    AERPSheetPrivate *d;

public:
    explicit AERPSheet(QObject *parent = 0);
    virtual ~AERPSheet();

    QString name() const;
    void setName(const QString &value);
    int index() const;
    void setIndex(int idx);
    int rowCount();
    int columnCount();
    QList<AERPCell *> cells () const;
    QStringList columnNames() const;
    QStringList columns() const;
    QStringList rows() const;
    bool hasColumnNames() const;

    static QScriptValue toScriptValue(QScriptEngine *engine, AERPSheet * const &in);
    static void fromScriptValue(const QScriptValue &object, AERPSheet * &out);

public slots:
    AERPCell *cell(int rowId, int columnId);
    AERPCell *cell(const QString &row, const QString &column);
    AERPCell *cell(int rowId, const QString &column);
    AERPCell *createCell(int row, int column, const QVariant value = QVariant());
    AERPCell *createCell(const QString &row, const QString &column, const QVariant value = QVariant());
    QVariant cellValue(int rowId, int columnId);
    QVariant cellValue(const QString &row, const QString &column);
    QVariant cellValue(int rowId, const QString &column);
    QVariant dateTimeCellValue(int rowId, int columnId);
    QVariant dateTimeCellValue(const QString &row, const QString &column);
    QVariant dateTimeCellValue(int rowId, const QString &column);
    AERPSpreadSheet::Type cellType(int rowId, int columnId);
    AERPSpreadSheet::Type cellType(const QString &row, const QString &column);
    AERPSpreadSheet::Type cellType(int rowId, const QString &column);
    int cellLength(int rowId, int columnId);
    int cellLength(const QString &row, const QString &column);
    int cellLength(int rowId, const QString &column);
    int cellDecimalPlaces(int rowId, int columnId);
    int cellDecimalPlaces(const QString &row, const QString &column);
    int cellDecimalPlaces(int rowId, const QString &column);

    void addColumn(const QString &columnName);
    void setColumn(int index, const QString &columnName, AERPSpreadSheet::Type type, int length = -1, int decimalPlaces = 0);
    void setColumnName(const QString &col, const QString &columnName);
    void setColumnName(int index, const QString &columnName);
    QString columnName(int index);
    QString columnName(const QString &column);
    bool containsColumn(const QString &column);
    bool containsRow(const QString &row);
    void setColumnType(int columnId, AERPSpreadSheet::Type type);
    void setColumnType(const QString &col, AERPSpreadSheet::Type type);
    AERPSpreadSheet::Type columnType(int columnId);
    AERPSpreadSheet::Type columnType(const QString &col);
    void setColumnLength(int columnId, int type);
    void setColumnLength(const QString &col, int type);
    int columnLength(int columnId);
    int columnLength(const QString &col);
    void setColumnDecimalPlaces(int columnId, int type);
    void setColumnDecimalPlaces(const QString &col, int type);
    int columnDecimalPlaces(int columnId);
    int columnDecimalPlaces(const QString &col);
};

/**
 * @brief The AERPCell class
 * Representa una celda, y su valor
 */
class ALEPHERP_DLL_EXPORT AERPCell : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariant value READ value WRITE setValue)
    Q_PROPERTY(QDateTime dateTimeValue READ dateTimeValue)
    Q_PROPERTY(int rowIndex READ rowIndex)
    Q_PROPERTY(QString row READ row)
    Q_PROPERTY(int columnIndex READ rowIndex)
    Q_PROPERTY(QString column READ column)
    Q_PROPERTY(QString columnName READ columnName)
    Q_PROPERTY(QString formula READ formula WRITE setFormula)
    Q_PROPERTY(AERPSpreadSheet::Type type READ type WRITE setType)
    Q_PROPERTY(int length READ length WRITE setLength)
    Q_PROPERTY(int decimalPlaces READ decimalPlaces WRITE setDecimalPlaces)
    Q_PROPERTY(QFont font READ font WRITE setFont)

    friend class AERPSheet;

private:
    AERPCellPrivate *d;

    void setRow(const QString &value);
    void setColumn(const QString &value);

public:
    explicit AERPCell(QObject *parent = 0);
    virtual ~AERPCell();

    QVariant value() const;
    void setValue(const QVariant &v);
    QDateTime dateTimeValue();
    QString row() const;
    int rowIndex() const;
    QString column() const;
    int columnIndex() const;
    QString formula() const;
    void setFormula(const QString &value);
    AERPSpreadSheet::Type type() const;
    void setType(AERPSpreadSheet::Type type);
    int length() const;
    void setLength(int value);
    int decimalPlaces() const;
    void setDecimalPlaces(int value);
    QString columnName() const;
    QFont font() const;
    void setFont(const QFont &font);

    static QScriptValue toScriptValue(QScriptEngine *engine, AERPCell * const &in);
    static void fromScriptValue(const QScriptValue &object, AERPCell * &out);

};

Q_DECLARE_METATYPE(AERPSpreadSheet*)
Q_SCRIPT_DECLARE_QMETAOBJECT(AERPSpreadSheet, QObject*)
Q_DECLARE_METATYPE(AERPSpreadSheet::Type)

Q_DECLARE_METATYPE(AERPSheet*)
Q_SCRIPT_DECLARE_QMETAOBJECT(AERPSheet, QObject*)
Q_DECLARE_METATYPE(QList<AERPSheet *>)

Q_DECLARE_METATYPE(AERPCell*)
Q_SCRIPT_DECLARE_QMETAOBJECT(AERPCell, QObject*)
Q_DECLARE_METATYPE(QList<AERPCell *>)

#endif // AERPSPREADSHEET_H
