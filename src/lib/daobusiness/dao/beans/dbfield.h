/***************************************************************************
 *   Copyright (C) 2010 by David Pinelo   *
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
#ifndef DBFIELD_H
#define DBFIELD_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtCore>
#include <QtScript>
#include <alepherpglobal.h>
#include <aerpcommon.h>
#include "dao/beans/basebean.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/dbobject.h"

class BaseBean;
class DBFieldObserver;
class AERPScript;
class DBFieldPrivate;
class DBField;
class DBFieldMetadata;
class DBRelation;

typedef QPointer<DBField> DBFieldPointer;

/**
  * Clase que almacena un valor de una columna de base de datos. Contiene
  * también la interacción con los campos visibles de edición
  * @author David Pinelo
  */
class ALEPHERP_DLL_EXPORT DBField : public DBObject
{
    Q_OBJECT

    /** Valor del campo o registro */
    Q_PROPERTY(QVariant value READ value WRITE setValue)
    /** Valor que tenía el campo la última vez que se guardó correctamente en base de datos */
    Q_PROPERTY(QVariant oldValue READ oldValue)
    /**
     * Valor que tenía el campo antes de tener el valor actual (último valor antes de este). Este valor
     * se establece a través de asignar valores a la propiedad value
     */
    Q_PROPERTY(QVariant previousValue READ previousValue)
    /** Valor por defecto */
    Q_PROPERTY(QVariant defaultValue READ defaultValue)
    /** Valor que se visualiza */
    Q_PROPERTY(QVariant displayValue READ displayValue)
    /** Indica si el campo se ha modificado después de su última lectura o modificación
      en base de datos */
    Q_PROPERTY(bool modified READ modified WRITE setModified)
    /** Almacena una referencia al bean que contiene este campo */
    Q_PROPERTY(BaseBean * bean READ bean)
    /** Indica si se ha cargado el campo memo */
    Q_PROPERTY(bool memoLoaded READ memoLoaded WRITE setMemoLoaded)
    /** Nombre del campo en base de datos */
    Q_PROPERTY(QString dbFieldName READ dbFieldName)
    /** Valor del campo en modo sql */
    Q_PROPERTY(QString sqlValue READ sqlValue)
    /** Valor del campo en modo sql */
    Q_PROPERTY(QString sqlOldValue READ sqlOldValue)
    /** Si este field presenta un pixmap, aquí se puede obtener el mismo una vez descodificado. */
    Q_PROPERTY(QPixmap pixmapValue READ pixmapValue)
    /** Acceso directo a las relaciones de este field */
    Q_PROPERTY(QVariantMap relationsMap READ relationsMap)
    /** Si este field es de tipo fecha, devolverá las partes de la misma para su tratamiento de forma fácil */
    Q_PROPERTY(int day READ day)
    Q_PROPERTY(int month READ month)
    Q_PROPERTY(int year READ year)
    /** Permite bloquear el cálculo automático de contadores. Esto es útil cuando se realizan operaciones sobre
     * los mismos (por ejemplo, renumeraciones) y no interesa que varíen su valor hasta la transacción */
    Q_PROPERTY(bool counterBlocked READ counterBlocked WRITE setCounterBlocked)
    /** Tras validar el valor del campo con validate, aquí se puede obtener un texto con el resultado de la validación */
    Q_PROPERTY(QString validateMessages READ validateMessages)
    Q_PROPERTY(QString validateHtmlMessages READ validateHtmlMessages)
    Q_PROPERTY(DBFieldMetadata * metadata READ metadata)
    /** Devuelve una claúsula igual para este campo. Interesante para pedir beans. Por ejemplo
     * var bean = AERPScriptCommon.bean("empresas", otherBean.idempresa.sqlEqual);
     * Es equivalente a otherBean.idempresa.sqlWhere("=");
     */
    Q_PROPERTY(QString sqlEqual READ sqlEqual)
    /** Si el campo contiene coordenadas... */
    Q_PROPERTY(double latitude READ latitude WRITE setLatitude)
    Q_PROPERTY(double longitude READ longitude WRITE setLongitude)
    Q_PROPERTY(bool overwrite READ overwrite)
    /** Para campos de tipo contador, permite especificar el valor de la parte variable */
    Q_PROPERTY(int counterVariablePart READ counterVariablePart WRITE setCounterVariablePart)
    /** Valor a utilizar en los contadores, si este campo se usa para ello*/
    Q_PROPERTY(QString valueOnCounter READ valueOnCounter)
    Q_PROPERTY(bool hasToolTip READ hasToolTip)

    /** BaseDAO utiliza unas funciones específicas de DBField y BaseBean para así saber
      cuándo la lectura de un dato se ha producid por lectura de base de datos, de modo
      que no se modifique m_modified */
    friend class BaseDAO;
    friend class BaseBean;
    friend class BaseBeanPrivate;
    friend class DBFieldMetadata;
    friend class DBRelation;
    friend class DBRelationPrivate;
    friend class DBFieldPrivate;
    friend class AERPBuiltInExpressionCalculatorPrivate;
    friend class AERPBuiltInExpressionCalculator;

private:
    Q_DISABLE_COPY(DBField)

    /** Puntero d para mejorar la compatibilidad de binarios:
    http://techbase.kde.org/Policies/Library_Code_Policy#D-Pointers
    */
    DBFieldPrivate *d;
//    Q_DECLARE_PRIVATE(DBField)

    void setMetadata(DBFieldMetadata *m);
    QVariant rawValue() const;
    void registerCalculatingOnBean();
    void unregisterCalculatingOnBean();
    void setOldValue(const QVariant &value);
    void adjustOldValue();

public:
    explicit DBField(QObject *parent = 0);
    ~DBField();

    DBFieldMetadata * metadata() const;

    /** Este operador sobrecargado, permitirá ordenaciones */
    bool operator < (DBField &field);

    QVariant value();
    QString displayValue();
    QVariant previousValue() const;
    QVariant oldValue() const;
    QPixmap pixmapValue();
    bool modified() const;
    void setModified(bool value);
    BaseBeanPointer bean() const;
    QVariant defaultValue();
    bool memoLoaded() const;
    void setMemoLoaded(bool value);
    QString dbFieldName() const;
    QVariantMap relationsMap();
    QString sqlValue(bool includeQuotesOnString = true, const QString &dialect = "", bool useRawValue = false);
    QString sqlOldValue(bool includeQuotesOnString = true, const QString &dialect = "");
    double latitude () const;
    void setLatitude(double value);
    double longitude() const;
    void setLongitude(double value);
    bool overwrite() const;
    int counterVariablePart() const;
    void setCounterVariablePart(int value);
    int length();
    QString valueOnCounter();

    bool hasBeenCalculated() const;

    int day();
    int month();
    int year();

    QList<DBRelation *> relations(const AlephERP::RelationTypes &type = AlephERP::All);
    Q_INVOKABLE DBRelation *relation(const QString &relationName);
    void addRelation (DBRelation *rel);
    void setRelations(const QList<DBRelation *> rels);
    bool hasM1Relation() const;
    bool hasBrotherRelation() const;

    QString sqlEqual();

    Q_INVOKABLE QString sqlWhere(const QString &op, const QString &dialect = "");
    Q_INVOKABLE QString toolTip();
    bool hasToolTip() const;

    Q_INVOKABLE bool checkValue(const QVariant &value, const QString &op = QString("="), Qt::CaseSensitivity cs = Qt::CaseInsensitive);
    Q_INVOKABLE bool checkValue(const QVariant &value1, const QVariant &value2);

    Q_INVOKABLE bool blockSignals ( bool block );
    Q_INVOKABLE QVariant calculateCounter(const QString &connection = "", bool searchOnTransaction = true);
    Q_INVOKABLE QVariant buildCounterValue();

    bool counterBlocked();
    void setCounterBlocked(bool value);

    Q_INVOKABLE bool isEmpty();
    Q_INVOKABLE int nextSerial();

    bool insertFieldOnUpdateSql(BaseBean::DbBeanStates state);

    static QScriptValue toScriptValue(QScriptEngine *engine, DBField * const &in);
    static void fromScriptValue(const QScriptValue &object, DBField * &out);
    static QScriptValue toScriptValueSharedPointer(QScriptEngine *engine, const QSharedPointer<DBField> &in);
    static void fromScriptValueSharedPointer(const QScriptValue &object, QSharedPointer<DBField> &out);

    int scheduleDurationValue();
    void setScheduleDurationValue(int val);

    QString validateMessages() const;
    QString validateHtmlMessages() const;
    QWidget *widgetOnBadValidate() const;

public slots:
    /** Distinguimos entre estas funciones, que son utilizada por funciones */
    void setValue(const QVariant &value);
    void setValue(const QString &string)
    {
        QVariant dato(string);
        setValue(dato);
    }
    void setValue(const QDate &date)
    {
        QVariant dato(date);
        setValue(dato);
    }
    void setValue(int number)
    {
        QVariant dato(number);
        setValue(dato);
    }
    void setValue(double number)
    {
        QVariant dato(number);
        setValue(dato);
    }
    void setValueFromSqlRawData(const QString &data);
    void setOverwriteValue(const QVariant &value);
    void setInternalValue(const QVariant &value, bool overwriteOnReadOnly = false, bool updateChildren = true);
    void resetOverwriteValue();
    void recalculateCounterField(const QString &connection = "");
    void recalculate();
    bool validate();
    void loadValueOnBackground();
    bool isLoadingOnBackground() const;
    void touch();

    static DBField *fromVariant(const QVariant &v);

signals:
    /** Indica que el usuario ha modificado un valor, al editar los datos directamente */
    void valueModified(const QString &fieldName, const QVariant &);
    void valueModified(const QVariant &);
    void valueModified(const QString &);
    void valueModified(QDate);
    void valueModified(int);
    void valueModified(double);
    void valueModified(bool);
    void defaultValueCalculated(QString fieldName, QVariant);

    void calculatedValueInit(DBField *fld);
    void calculatedValueEnd(DBField *fld);

    void dataAvailable(const QVariant &v);
    void errorBackgroundLoad(const QString &error);

private slots:
    void emitValueModifiedByUser();
    void onChangeEvent();
    void backgroundDataAvailable(const QString &id, bool result);
    void setValueIsOld();
};

// Para comparar campos calculados
class CompareCalculatedDBField {
public:
    bool operator() (const DBField* e1, const DBField* e2) const
    {
        return e1->metadata()->calculatedOrder() < e2->metadata()->calculatedOrder();
    }
};

Q_DECLARE_METATYPE(QList<DBField*>)
Q_DECLARE_METATYPE(DBField*)
Q_DECLARE_METATYPE(QSharedPointer<DBField>)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBField, QObject*)

#endif // DBFIELD_H
