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
#include <QtCore>
#include <QtScript>
#include "configuracion.h"
#include "qlogger.h"
#include "dbfield.h"
#include <globales.h>
#include <aerpcommon.h>
#include "widgets/dbbasewidget.h"
#include "dao/basedao.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/basebean.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/aerptransactioncontext.h"
#include "dao/dbfieldobserver.h"
#include "dao/observerfactory.h"
#include "dao/database.h"
#include "dao/backgrounddao.h"
#include "scripts/perpscript.h"
#include "business/aerpbuiltinexpressioncalculator.h"
#include "qnumeric.h"

class DBFieldPrivate
{
//    Q_DECLARE_PUBLIC(DBFieldPrivate)
public:
    DBField *q_ptr;
    /** Valor del campo o registro */
    QVariant m_value;
    /** Los campos calculados pueden tener sus valores a capón si esta variable es true */
    bool m_overwriteValue;
    /** Valor justo antes */
    QVariant m_previousValue;
    /** Valor que tenía el campo la última vez que se almacenó correctamente en base de datos */
    QVariant m_oldValue;
    /** Indica si el campo se ha modificado después de su última lectura o modificación
      en base de datos */
    bool m_modified;
    /** Almacena una referencia al bean que contiene este campo */
    BaseBeanPointer m_bean;
    /** El valor de un DBField puede marcar la existencia de DBRelation hijos. De hecho en el XML se definen
      así. Por tanto se guarda una referencia a la relación que mantiene con sus hijos. BaseBean también
      guarda una referencia */
    QList<DBRelation* > m_relations;
    /** Metadatos */
    DBFieldMetadata *m;
    /** Si este es un campo memo, indica si se ha cargado de base de datos */
    bool m_memoLoaded;
    /** Si es un campo calculado, y se calcula sólo una vez, este flag guarda
      si se ha calculado o no*/
    bool m_hasBeenCalculated;
    /** La función setValue, es especialmente importante, y debemos procurar que el programador QS no genere llamadas
     * recursivas a la misma, lo que podría hacer con un código simple como este */
    bool m_settingNewValue;
    /** Vamos a tener una copia de cómo se visualiza el campo, para no estar constántemente invocando al posible script */
    QString m_displayValue;
    /** Evitamos anidamiento de signals/slots */
    QStack<EmittedSignals> m_emittedSignals;
    /** Para los cálculos agregados. Tendremos un cálculo por cada regla de agregado, para aprovechar la compilación en bytecode. */
    QList<AERPBuiltInExpressionCalculator *> m_calcAggregate;
    /** A veces, es interesante forzar un campo calculado. Esto se hace con la función buildCounterValue. Cuando se llama a esta función
     * se puede querer bloquear el contador. Este chivato lo indicará */
    bool m_counterBlocked;
    /** Mensaje que se va escribiendo una vez ejecutado validate */
    QString m_message;
    /** Mismo mensaje, pero en HTML */
    QString m_htmlMessage;
    /** Contiene una referencia al widget al que se le aplicará el foco, caso de que
      el dbfield asociado no pase la validación */
    QWidget *m_widget;
    double m_latitude;
    double m_longitude;
    QString m_backgroundUuid;
    int m_counterVariablePart;
    QMutex m_mutex;
    /** Indicará cuando el campo debe ser recalculado */
    bool m_valueIsOld;

    DBFieldPrivate(DBField *qq);
    QVariant calculateAggregateValue();
    void calculateAggregateScriptItem(const QString &relation, const QString &filter, const QString &script, double &sumResult, int &countResult);
    void calculateAggregateExpressionItem(int indexAggregate, const QString &relation, const QString &filter, double &sumResult, int &countResult);
    QVariant setDataToType(const QVariant &value);
    QString sqlValue(const QVariant &val, bool includeQuotesOnString, const QString &dialect);
    QVariant calculateCounterOldMethod(const QString &connection, bool searchOnTransaction);
    QVariant calculateCounter(const QString &connection, bool searchOnTransaction);
    QVariant calculateValue();
    QString filterForUniqueOnFilterField(const QString where);

    void emitValueModified(const QString &fieldName, const QVariant &);
    void emitValueModified(const QVariant &);

    bool checkNull();
    bool checkLength();
    bool checkUnique();
    bool checkUniqueOnFilterField();
};

DBFieldPrivate::DBFieldPrivate (DBField *qq) :
    q_ptr(qq),
    m_mutex(QMutex::Recursive)
{
    m_modified = false;
    m_overwriteValue = false;
    m = NULL;
    m_memoLoaded = false;
    m_hasBeenCalculated = false;
    m_settingNewValue = false;
    m_counterBlocked = false;
    m_widget = NULL;
    m_latitude = 0;
    m_longitude = 0;
    m_counterVariablePart = -1;
    m_valueIsOld = true;
}

void DBFieldPrivate::emitValueModified(const QString &fieldName, const QVariant &v)
{
    EmittedSignals st;
    st.signal = "valueModified";
    st.field = q_ptr;
    st.string = fieldName;
    st.variant = v;
    for ( int i = 0 ; i < m_emittedSignals.size() ; i++ )
    {
        if ( m_emittedSignals.at(i) == st )
        {
            qDebug() << "DBField::emitValueModifiedByUser: Anidamiento de señales";
            return;
        }
    }
    m_emittedSignals.push(st);
    emit q_ptr->valueModified(fieldName, v);
    m_emittedSignals.pop();

}

void DBFieldPrivate::emitValueModified(const QVariant &v)
{
    EmittedSignals st;
    st.signal = "valueModified";
    st.field = q_ptr;
    st.variant = v;
    for ( int i = 0 ; i < m_emittedSignals.size() ; i++ )
    {
        if ( m_emittedSignals.at(i) == st )
        {
            qDebug() << "DBField::emitValueModifiedByUser: Anidamiento de señales";
            return;
        }
    }
    m_emittedSignals.push(st);
    emit q_ptr->valueModified(v);
    m_emittedSignals.pop();
}

/*!
  Comprueba que la longitud del campo no exceda lo indicado
  */
bool DBFieldPrivate::checkLength()
{
    QMutexLocker lock(&m_mutex);

    bool result = true;
    if ( q_ptr->length() == -1 )
    {
        return true;
    }
    QString temp = q_ptr->value().toString();
    if ( temp.size() > q_ptr->length() )
    {
        result = false;
        m_message = QObject::trUtf8("%1\r\n%2: La longitud del texto introducido sobrepasa la permitida. Acorte el texto.").
                    arg(m_message).arg(q_ptr->metadata()->fieldName());
        m_htmlMessage = QObject::trUtf8("%1<p><strong>%2</strong>: La longitud del texto introducido sobrepasa la permitida. Acorte el texto.</p>").
                        arg(m_htmlMessage).arg(q_ptr->metadata()->fieldName());
        if ( m_widget == NULL )
        {
            // TODO esto no es muy elegante
            QList<QPointer<QObject> > widgets = q_ptr->observer()->viewWidgets();
            if ( widgets.size() > 0 )
            {
                m_widget = qobject_cast<QWidget *>(widgets.at(0));
            }
        }
    }
    return result;
}

/*!
  Comprueba que el valor del field es único
  */
bool DBFieldPrivate::checkUnique()
{
    QMutexLocker lock(&m_mutex);
    bool result = true;
    QString sql = QString("SELECT COUNT(*) as column1 FROM %1 WHERE %2=%3").arg(q_ptr->bean()->metadata()->tableName()).
                  arg(q_ptr->metadata()->dbFieldName()).arg(q_ptr->sqlValue(true));
    QVariant sqlValue;
    if ( BaseDAO::execute(sql, sqlValue, Database::databaseConnectionForThisThread()) )
    {
        int count = sqlValue.toInt();
        if ( count > 0 )
        {
            result = false;
            m_message = QObject::trUtf8("%1\r\n%2: Ya existe un registro que tiene ese valor. El valor debe ser único.").
                        arg(m_message).arg(q_ptr->metadata()->fieldName());
            m_htmlMessage = QObject::trUtf8("%1<p><strong>%2</strong>: Ya existe un registro que tiene ese valor. El valor debe ser único.</p>").
                            arg(m_htmlMessage).arg(q_ptr->metadata()->fieldName());
            if ( m_widget == NULL )
            {
                // TODO esto no es muy elegante
                QList<QPointer<QObject> > widgets = q_ptr->observer()->viewWidgets();
                if ( widgets.size() > 0 )
                {
                    m_widget = qobject_cast<QWidget *>(widgets.at(0));
                }
            }
        }
    }
    return result;
}

/*!
  Chequea si el valor de fld pasa la validación de NULL
  */
bool DBFieldPrivate::checkNull()
{
    QMutexLocker lock(&m_mutex);
    bool result = true;

    if ( q_ptr->metadata()->type() == QVariant::String ||
         q_ptr->metadata()->type() == QVariant::Double ||
         q_ptr->metadata()->type() == QVariant::Int ||
         q_ptr->metadata()->type() == QVariant::Date ||
         q_ptr->metadata()->type() == QVariant::DateTime )
    {
        bool canBeNullBecauseIsChildPartOfRelation = false;
        if ( q_ptr->value().isNull() || !q_ptr->value().isValid() )
        {
            // Si el campo apunta a una relación, no podra tener valor cero... o, si es cero, pero
            // apunta a un padre que va a ser guardado, entonces pasa la validación.
            QList<DBRelation *> relations = q_ptr->relations();
            foreach (DBRelation *rel, relations)
            {
                if ( rel->metadata()->type() == DBRelationMetadata::MANY_TO_ONE )
                {
                    if ( !rel->father()->modified() && rel->father()->dbState() == BaseBean::INSERT )
                    {
                        result = false;
                    }
                    else
                    {
                        canBeNullBecauseIsChildPartOfRelation = true;
                    }
                }
            }
        }

        if ( !canBeNullBecauseIsChildPartOfRelation )
        {
            QString temp = q_ptr->value().toString();
            // También puede ocurrir, que el campo sea un combobox.
            if ( temp.isEmpty() || ( q_ptr->metadata()->optionsList().size() > 0 && temp == "-1" ))
            {
                result = false;
            }
        }

        if ( ! result )
        {
            m_message = QObject::trUtf8("%1\r\n%2: No puede estar vacío.").arg(m_message).arg(q_ptr->metadata()->fieldName());
            m_htmlMessage = QObject::trUtf8("%1<p><strong>%2</strong>: No puede estar vac&iacute;o.</p>").
                            arg(m_htmlMessage).arg(q_ptr->metadata()->fieldName());
            if ( m_widget == NULL )
            {
                // TODO Esto no es muy elegante
                QList<QPointer<QObject> > widgets = q_ptr->observer()->viewWidgets();
                if ( widgets.size() > 0 )
                {
                    m_widget = qobject_cast<QWidget *>(widgets.at(0));
                }
            }
        }
    }
    else if ( q_ptr->metadata()->type() == QVariant::Bool || q_ptr->metadata()->type() == QVariant::Pixmap )
    {
        if ( q_ptr->value().isNull() || !q_ptr->value().isValid() )
        {
            result = false;
            m_message = QObject::trUtf8("%1\r\n%2: No puede estar vacío.").arg(m_message).arg(q_ptr->metadata()->fieldName());
            m_htmlMessage = QObject::trUtf8("%1<p><strong>%2</strong>: No puede estar vac&iacute;o.</p>").
                            arg(m_htmlMessage).arg(q_ptr->metadata()->fieldName());
            if ( m_widget == NULL )
            {
                // TODO Esto no es muy elegante
                QList<QPointer<QObject> > widgets = q_ptr->observer()->viewWidgets();
                if ( widgets.size() > 0 )
                {
                    m_widget = qobject_cast<QWidget *>(widgets.at(0));
                }
            }
        }
    }
    return result;
}


/**
  Comprueba que el valor del campo es único para el filtro que impone el valor del campo uniqueOnFilterField
*/
bool DBFieldPrivate::checkUniqueOnFilterField()
{
    QMutexLocker lock(&m_mutex);
    if ( m_bean.isNull() )
    {
        return false;
    }

    bool result = true;
    BaseBean *bean = q_ptr->bean();
    QStringList filterFields = q_ptr->metadata()->uniqueOnFilterField().split(QRegExp(";|,"));
    QString sqlFilterField;
    foreach (const QString &filterField, filterFields)
    {
        DBField *dbField = bean->field(filterField.trimmed());
        if ( dbField != NULL )
        {
            if ( sqlFilterField.isEmpty() )
            {
                sqlFilterField = QString("%1=%2").arg(dbField->metadata()->dbFieldName()).
                                 arg(dbField->sqlValue(true));
            }
            else
            {
                sqlFilterField = QString("%1 AND %2=%3").arg(sqlFilterField).arg(dbField->metadata()->dbFieldName()).
                                 arg(dbField->sqlValue(true));
            }
        }
    }
    if ( sqlFilterField.isEmpty() )
    {
        return true;
    }
    // Hay que distinguir el caso en el que se está insertando, o editando.
    QString sql = QString("SELECT COUNT(*) as column1 FROM %1 WHERE %2=%3 AND %4").
                        arg(q_ptr->bean()->metadata()->sqlTableName()).
                        arg(q_ptr->metadata()->dbFieldName()).
                        arg(q_ptr->sqlValue(true)).
                        arg(sqlFilterField);
    if ( m_bean->dbState() == BaseBean::UPDATE )
    {
        sql.append(" AND NOT (").append(m_bean->sqlWherePk()).append(")");
    }
    QVariant sqlValue;
    if ( BaseDAO::execute(sql, sqlValue, Database::databaseConnectionForThisThread()) )
    {
        int count = sqlValue.toInt();
        if ( count > 0 )
        {
            result = false;
            if ( q_ptr->metadata()->hasCounterDefinition() )
            {
                if ( (q_ptr->metadata()->counterDefinition()->calculateOnlyOnInsert && bean->dbState() == BaseBean::INSERT) || !q_ptr->metadata()->counterDefinition()->calculateOnlyOnInsert )
                {
                    int ret = QMessageBox::question(m_widget, qApp->applicationName(),
                                                    QObject::trUtf8("Existe ya un registro con este mismo valor: <strong>%1</strong>. "
                                                            "El valor debe ser único. ¿Desea volver a calcularlo?").arg(q_ptr->displayValue()),
                                                    QMessageBox::Yes | QMessageBox::No);
                    if ( ret == QMessageBox::Yes )
                    {
                        q_ptr->setValue(q_ptr->calculateCounter());
                        m_message = QObject::trUtf8("%1\r\n%2: Se ha calculado un nuevo valor.").
                                    arg(m_message).arg(q_ptr->metadata()->fieldName());
                        m_htmlMessage = QObject::trUtf8("%1<p><strong>%2</strong>: Se ha calculado un nuevo valor.</p>").
                                        arg(m_htmlMessage).arg(q_ptr->metadata()->fieldName());
                        return true;
                    }
                }
            }
            m_message = QObject::trUtf8("%1\r\n%2: Ya existe un registro que tiene ese valor. El valor debe ser único.").
                        arg(m_message).arg(q_ptr->metadata()->fieldName());
            m_htmlMessage = QObject::trUtf8("%1<p><strong>%2</strong>: Ya existe un registro que tiene ese valor. El valor debe ser único.</p>").
                            arg(m_htmlMessage).arg(q_ptr->metadata()->fieldName());
            if ( m_widget == NULL )
            {
                // TODO esto no es muy elegante
                QList<QPointer<QObject> > widgets = q_ptr->observer()->viewWidgets();
                if ( widgets.size() > 0 )
                {
                    m_widget = qobject_cast<QWidget *>(widgets.at(0));
                }
            }
        }
    }
    return result;
}

DBField::DBField(QObject *parent) : DBObject(parent), d(new DBFieldPrivate(this))
{
    d->m_bean = qobject_cast<BaseBean *>(parent);
    connect(this, SIGNAL(valueModified(QVariant)), this, SLOT(onChangeEvent()));
}

DBField::~DBField()
{
    qDeleteAll(d->m_calcAggregate);
    delete d;
}

DBFieldMetadata * DBField::metadata() const
{
    return d->m;
}

void DBField::setMetadata(DBFieldMetadata *m)
{
    QMutexLocker lock(&d->m_mutex);

    d->m = m;
    // Inicialmente, el campo editable siempre estará a true.
    if ( d->m->dbFieldName() == AlephERP::stFieldEditable )
    {
        d->m_value = true;
    }
    for(int i = 0 ; i < d->m->aggregateCalc().expression.size() ; i++)
    {
        AERPBuiltInExpressionCalculator *calc = new AERPBuiltInExpressionCalculator();
        calc->setExpression(d->m->aggregateCalc().expression.at(i));
        calc->setFieldType(d->m->type());
        d->m_calcAggregate.append(calc);
    }
}

/**
 * @brief DBField::rawValue
 * Devuelve el contenido interno de la variable que calcula valores
 * @return
 */
QVariant DBField::rawValue() const
{
    return d->m_value;
}

/**
 * @brief DBField::registerCalculatingOnBean
 * Informamos al bean padre, que se está procediendo a calcular este valor, para que lo tenga en cuenta
 * de cara a recálculos
 */
void DBField::registerCalculatingOnBean()
{
    EmittedSignals st;
    st.signal = "calculatedValueInit";
    st.field = this;
    for ( int i = 0 ; i < d->m_emittedSignals.size() ; i++ )
    {
        if ( d->m_emittedSignals.at(i) == st )
        {
            qDebug() << "DBField::registerCalculatingOnBean: Anidamiento de señales";
            return;
        }
    }
    d->m_emittedSignals.push(st);
    emit calculatedValueInit(this);
    d-> m_emittedSignals.pop();
}

void DBField::unregisterCalculatingOnBean()
{
    EmittedSignals st;
    st.signal = "calculatedValueEnd";
    st.field = this;
    for ( int i = 0 ; i < d->m_emittedSignals.size() ; i++ )
    {
        if ( d->m_emittedSignals.at(i) == st )
        {
            qDebug() << "DBField::unregisterCalculatingOnBean: Anidamiento de señales";
            return;
        }
    }
    d->m_emittedSignals.push(st);
    emit calculatedValueEnd(this);
    d->m_emittedSignals.pop();
}

void DBField::setOldValue(const QVariant &value)
{
    QMutexLocker lock(&d->m_mutex);

    d->m_oldValue = value;
}

QList<DBRelation *> DBField::relations(const AlephERP::RelationTypes &type)
{
    if ( type.testFlag(AlephERP::All) )
    {
        return d->m_relations;
    }
    QList<DBRelation *> rels;
    foreach ( DBRelation *rel, d->m_relations )
    {
        if ( type.testFlag(AlephERP::OneToMany) && rel->metadata()->type() == DBRelationMetadata::ONE_TO_MANY )
        {
            rels << rel;
        }
        else if ( type.testFlag(AlephERP::ManyToOne) && rel->metadata()->type() == DBRelationMetadata::MANY_TO_ONE )
        {
            rels << rel;
        }
        else if ( type.testFlag(AlephERP::OneToOne) && rel->metadata()->type() == DBRelationMetadata::ONE_TO_ONE )
        {
            rels << rel;
        }
    }
    return rels;

}

DBRelation * DBField::relation(const QString &relationName)
{
    foreach ( DBRelation *rel, d->m_relations )
    {
        if ( rel->metadata()->name() == relationName)
        {
            return rel;
        }
    }
    return NULL;
}

void DBField::addRelation (DBRelation *rel)
{
    QMutexLocker lock(&d->m_mutex);

    d->m_relations << rel;
    /** Aquí exponemos las relaciones al motor Qs */
    QVariant pointer = QVariant::fromValue(rel);
    QByteArray ba = rel->metadata()->name().toUtf8();
    setProperty(ba.constData(), pointer);
}

void DBField::setRelations(const QList<DBRelation *> rels)
{
    QMutexLocker lock(&d->m_mutex);

    d->m_relations = rels;
    foreach (DBRelation *rel, rels)
    {
        /** Aquí exponemos las relaciones al motor Qs */
        QVariant pointer = QVariant::fromValue(rel);
        QByteArray ba = rel->metadata()->name().toUtf8();
        setProperty(ba.constData(), pointer);
    }
}

bool DBField::hasM1Relation() const
{
    // Si el campo contiene una relación a un padre (relación M1), y el padre no está establecido, en ese caso, no se incluye
    foreach (DBRelation *rel, d->m_relations)
    {
        if ( rel->metadata()->type() == DBRelationMetadata::MANY_TO_ONE )
        {
            return true;
        }
    }
    return false;
}

bool DBField::hasBrotherRelation() const
{
    // Si el campo contiene una relación a un padre (relación M1), y el padre no está establecido, en ese caso, no se incluye
    foreach (DBRelation *rel, d->m_relations)
    {
        if ( rel->metadata()->type() == DBRelationMetadata::ONE_TO_ONE )
        {
            return true;
        }
    }
    return false;
}

QString DBField::sqlEqual()
{
    return sqlWhere("=");
}

/**
  Puede existir un problema: Obtenemos de base de datos un double. El valor es cero. El QVariant
  contiene 0, pero al ponerse el valor, quizás QVariant entienda 0 como false, y su type se
  pone a QVariant::Bool. Esta función obliga a QVariant a contener el tipo definido en el metadato
  */
QVariant DBFieldPrivate::setDataToType(const QVariant &v)
{
    QVariant result;
    bool ok;
    if ( m->type() == QVariant::Int )
    {
        int temp = v.toInt(&ok);
        if ( ok )
        {
            result = QVariant(temp);
        }
    }
    else if ( m->type() == QVariant::Double )
    {
        double temp = v.toDouble(&ok);
        if ( ok )
        {
            temp = CommonsFunctions::round(temp, m->partD());
            result = QVariant(temp);
        }
    }
    else if ( m->type() == QVariant::Date )
    {
        QDate temp = v.toDate();
        result = QVariant(temp);
    }
    else if ( m->type() == QVariant::DateTime )
    {
        QDateTime temp = v.toDateTime();
        result = QVariant(temp);
    }
    else if ( m->type() == QVariant::Bool )
    {
        bool temp = v.toBool();
        result = QVariant(temp);
    }
    else if ( m->type() == QVariant::String )
    {
        QString temp = v.toString();
        result = QVariant(temp);
    }
    else if ( m->type() == QVariant::Pixmap )
    {
        result = QVariant(v.toByteArray());
    }
    else
    {
        qDebug() << "DBField::setDataToType: field " << m->dbFieldName() << " tiene un tipo no determinado: " << v.typeName();
        result = v;
    }
    return result;
}

QString DBFieldPrivate::sqlValue(const QVariant &val, bool includeQuotesOnString, const QString &dialect)
{
    QString result;
    QString driverName;
    if ( dialect.isEmpty() )
    {
        driverName = QSqlDatabase::database(Database::databaseConnectionForThisThread()).driverName();
    }
    else
    {
        driverName = dialect;
    }
    if ( m->type() == QVariant::Int )
    {
        result = QString("%1").arg(val.toInt());
    }
    else if ( m->type() == QVariant::Double )
    {
        result = QString::number(val.toDouble(), 'f', DB_DOUBLE_PRECISION);
    }
    else if ( m->type() == QVariant::Date )
    {
        QDate fecha = val.toDate();
        if ( includeQuotesOnString )
        {
            result = QString("\'%1\'").arg(fecha.toString("yyyy-MM-dd"));
        }
        else
        {
            result = fecha.toString("yyyy-MM-dd");
        }
    }
    else if ( m->type() == QVariant::DateTime )
    {
        QDate fecha = val.toDate();
        if ( includeQuotesOnString )
        {
            result = QString("\'%1\'").arg(fecha.toString("yyyy-MM-dd HH:mm:ss"));
        }
        else
        {
            result = fecha.toString("yyyy-MM-dd HH:mm:ss");
        }
    }
    else if ( m->type() == QVariant::Bool )
    {
        if ( driverName == "QIBASE" )
        {
            result = (val.toBool() ? QString("1") : QString("0"));
        }
        else
        {
            result = (val.toBool() ? QString("true") : QString("false"));
        }
    }
    else if ( m->type() == QVariant::String )
    {
        // Hay que escapar los caracters ' internos
        QString v = val.toString();
        v.replace('\'', "\'\'");
        if ( includeQuotesOnString )
        {
            result = QString("\'%1\'").arg(v);
        }
        else
        {
            result = v;
        }
    }
    else if ( m->type() == QVariant::Pixmap )
    {
        result = "";
    }
    else
    {
        qCritical() << "DBFieldPrivate: sqlValue: [" << m->dbFieldName() << "]. No tiene definido un tipo de datos. Asignando el tipo por defecto.";
        result = QString("\'%1\'").arg(val.toString());
    }
    return result;
}

/*!
  Formatea la salida de value. Es muy útil para presentar los datos en TableView y demás. Si value
  es nulo, entonces, formatea la salida de d->m_value;
  */
QString DBField::displayValue()
{
    QMutexLocker lock(&d->m_mutex);

    if ( !d->m_displayValue.isNull() )
    {
        return d->m_displayValue;
    }
    d->m_displayValue = d->m->displayValue(value(), this);
    return d->m_displayValue;
}

QVariant DBField::previousValue() const
{
    if ( !d->m_bean->checkAccess('r') )
    {
        return trUtf8(AlephERP::stNoAccess);
    }
    return d->m_previousValue;
}

QVariant DBField::oldValue() const
{
    if ( !d->m_bean->checkAccess('r') )
    {
        return trUtf8(AlephERP::stNoAccess);
    }
    return d->m_oldValue;
}

QPixmap DBField::pixmapValue()
{
    QPixmap pixmap;
    if ( d->m->type() == QVariant::Pixmap )
    {
        if ( !pixmap.loadFromData(d->m_value.toByteArray()) )
        {
            QLogger::QLog_Debug(AlephERP::stLogOther, QString("DBField::pixmapValue: Pixmap no valido"));
        }
    }
    return pixmap;
}

/**
 * @brief DBField::value
 * Generación o lectura del valor que almacena este campo.
 * @return
 */
QVariant DBField::value()
{
    QMutexLocker lock(&d->m_mutex);

    if ( d->m_bean.isNull() )
    {
        return QVariant();
    }

    // Función para construir la estructura de llamadas de los fields calculados
    BaseBeanMetadata::fieldIsAccesed(this);

    if ( d->m_overwriteValue )
    {
        return d->m_value;
    }

    if ( m_executingStack.contains(AlephERP::CalculateValue) ||
         m_executingStack.contains(AlephERP::AggregateValue) ||
         m_executingStack.contains(AlephERP::CounterValue) )
    {
        return d->m_value;
    }
    if ( !d->m_bean->checkAccess('r') )
    {
        return trUtf8(AlephERP::stNoAccess);
    }
    if ( d->m->coordinates() )
    {
        return d->m_value;
    }
    if ( hasM1Relation() )
    {
        // Si el campo contiene una relación a un padre (relación M1), y el padre no está establecido, en ese caso, no se incluye
        foreach (DBRelation *rel, d->m_relations)
        {
            if ( rel != NULL && rel->metadata()->type() == DBRelationMetadata::MANY_TO_ONE )
            {
                if ( !rel->fatherSetted() && !d->m_modified )
                {
                    return QVariant();
                }
            }
        }
    }
    if ( hasBrotherRelation() )
    {
        // Si el campo contiene una relación a un padre (relación M1), y el padre no está establecido, en ese caso, no se incluye
        foreach (DBRelation *rel, d->m_relations)
        {
            if ( rel != NULL && rel->metadata()->type() == DBRelationMetadata::ONE_TO_ONE )
            {
               if ( !rel->brotherSetted() )
               {
                   return QVariant();
               }
            }
        }
        return false;
    }
    if ( d->m_bean->dbState() == BaseBean::UPDATE &&
            (d->m->type() == QVariant::String || d->m->type() == QVariant::Pixmap)
            && d->m->memo() && !d->m_memoLoaded && !d->m->calculated() )
    {
        d->m_memoLoaded = BaseDAO::selectField(this);
    }
    if ( d->m->serial() && d->m_bean->dbState() == BaseBean::INSERT && Database::getQDatabase().driverName() == "QSQLITE" )
    {
        /** Para SQLite debemos calcular y generar los valores de las secuencias desde AlephERP. Lo podemos hacer
         * en este punto, ya que se supone que las base de datos SQLite son locales */
        if ( !d->m_hasBeenCalculated )
        {
            QVariant value = d->setDataToType(nextSerial());
            if ( value.toInt() > -1 )
            {
                setInternalValue(value);
                d->m_hasBeenCalculated = true;
                d->m_previousValue = -1;
                d->m_displayValue.clear();
            }
        }
    }
    else
    {
        if ( d->m->calculated() && !d->m_overwriteValue )
        {
            // Si el campo no es válido se fuerza un primer cálculo, pero si es válido, las conexiones que hace el BaseBean
            // para comprobar qué relaciones y campos intervienen en el cálculo de éste, hacen que no sea necesario recalcular
            if ( d->m_value.isNull() || !d->m_value.isValid() || d->m_valueIsOld )
            {
                bool hasToEmitModified = false;
                if ( d->m->aggregate() )
                {
                    if ( !d->m_hasBeenCalculated || d->m_bean->modified() )
                    {
                        QVariant r = d->setDataToType(d->calculateAggregateValue());
                        if ( r != d->m_value )
                        {
                            d->m_previousValue = d->m_value;
                            d->m_value = d->setDataToType(r);
                            d->m_displayValue.clear();
                            hasToEmitModified = true;
                        }
                    }
                }
                else if ( d->m->hasCounterDefinition() )
                {
                    if ( !d->m_hasBeenCalculated )
                    {
                        if ( (d->m->counterDefinition()->calculateOnlyOnInsert && d->m_bean->dbState() == BaseBean::INSERT) || !d->m->counterDefinition()->calculateOnlyOnInsert )
                        {
                            if ( !d->m_counterBlocked )
                            {
                                QVariant r = d->setDataToType(calculateCounter());
                                if ( r != d->m_value )
                                {
                                    d->m_previousValue = d->m_value;
                                    d->m_value = r;
                                    d->m_displayValue.clear();
                                    d->m_hasBeenCalculated = true;
                                    hasToEmitModified = true;
                                }
                            }
                        }
                    }
                }
                else
                {
                    if ( !d->m->calculatedOneTime() || (d->m->calculatedOneTime() && !d->m_hasBeenCalculated ))
                    {
                        if ( !d->m->calculatedOnlyOnInsert() || (d->m->calculatedOnlyOnInsert() && d->m_bean->dbState() == BaseBean::INSERT) )
                        {
                            QVariant data = d->setDataToType(d->m->calculateValue(this));
                            if ( data.isValid() && d->m_value != data )
                            {
                                QString temp = data.toString();
                                if ( temp.toLower() != "nan" )
                                {
                                    // Vamos a contemplar un caso particular: Un campo calculado, que no se guarda en base de datos, y que
                                    // d->m_value es invalid. En este caso, puede ocurrir: El bean se ha cargado y está como "no modified".
                                    // Pero al pedir este campo (que se calcula a partir de los otros datos simplemente), se marca el bean como
                                    // modified, cuando realmente no lo está: simplemente se ha dado un nuevo valor.
                                    if ( !d->m->calculatedSaveOnDb() && !d->m_value.isValid() )
                                    {
                                        hasToEmitModified = false;
                                    }
                                    else
                                    {
                                        hasToEmitModified = true;
                                    }
                                    d->m_previousValue = d->m_value;
                                    d->m_value = data;
                                    d->m_displayValue.clear();
                                    if ( d->m->calculatedOneTime() )
                                    {
                                        d->m_hasBeenCalculated = true;
                                    }
                                }
                            }
                        }
                    }
                }
                if ( hasToEmitModified )
                {
                    // Este es el punto al que están enganchados los observadores, y el que hace que se refresquen
                    // los controles.
                    emitValueModifiedByUser();
                }
            }
            d->m_valueIsOld = false;
        }
        else
        {
            // Si no hay un valor definido (m_value no es válido), porque no se ha establecido ninguno antes
            // y sí hay un valor por defecto, se le da éste mismo. Pero ojo, sólo en nuevas inserciones.
            if ( !d->m_value.isValid() && d->m_bean->dbState() == BaseBean::INSERT )
            {
                QVariant defaultValue = d->m->calculateDefaultValue(this);
                if ( defaultValue.isValid() )
                {
                    d->m_value = d->setDataToType(defaultValue);
                }
                else
                {
                    d->m_value = d->setDataToType(d->m_value);
                }
                d->m_previousValue = d->m_value;
                d->m_displayValue.clear();
                emit defaultValueCalculated(d->m->dbFieldName(), d->m_value);
            }
        }
    }
    return d->m_value;
}

QVariant DBFieldPrivate::calculateValue()
{
    QVariant v;
    // Si el campo no es válido se fuerza un primer cálculo, pero si es válido, las conexiones que hace el BaseBean
    // para comprobar qué relaciones y campos intervienen en el cálculo de éste, hacen que no sea necesario recalcular
    if ( m->aggregate() )
    {
        v = setDataToType(calculateAggregateValue());
    }
    else if ( m->hasCounterDefinition() )
    {
        v = setDataToType(q_ptr->calculateCounter());
    }
    else
    {
        v = setDataToType(m->calculateValue(q_ptr));
    }
    return v;
}

QString DBFieldPrivate::filterForUniqueOnFilterField(const QString where)
{
    QString tempSql;
    if ( m->uniqueOnFilterField().isEmpty() )
    {
        return where;
    }
    QStringList parts = m->uniqueOnFilterField().split(QRegExp(";|,"));
    if ( !parts.isEmpty() )
    {
        bool first = true;
        tempSql.append("(");
        foreach (const QString &part, parts)
        {
            if ( !first )
            {
                tempSql.append(" AND ");
            }
            QString sqlVal = m_bean->sqlFieldValue(part);
            if ( !sqlVal.isEmpty() )
            {
                first = false;
                tempSql.append(QString("%1=%2").arg(part).arg(sqlVal));
            }
        }
        tempSql.append(")");
    }
    if ( !tempSql.isEmpty() )
    {
        tempSql.append(" AND ").append(where);
    }
    return tempSql;
}

QVariant DBField::defaultValue()
{
    if ( !d->m_bean->checkAccess('r') )
    {
        return trUtf8(AlephERP::stNoAccess);
    }
    QVariant v = d->m->calculateDefaultValue(this);
    emit defaultValueCalculated(d->m->dbFieldName(), v);
    return v;
}

/*!
  Almacena el valor pasado en el parámetro. Emite señal de valueModified. Está pensada
  para ser el receptor de las señales de modificación de los diferentes controles de edición.
  */
void DBField::setValue(const QVariant &argNewValue)
{
    QVariant data, newValue;

    QMutexLocker lock(&d->m_mutex);

    newValue = argNewValue;

    if ( d->m->dbFieldName() != AlephERP::stFieldEditable && d->m_bean->readOnly() )
    {
        return;
    }

    // Esto permite asignaciones desde el motor javascript de este estilo:
    // bean.field.value = otherBean.otherField;
    DBField *fld = fromVariant(argNewValue);
    if ( fld != NULL )
    {
        newValue = fld->value();
    }

    if ( d->m_bean.isNull() || !d->m_bean->checkAccess('w') )
    {
        return;
    }

    if ( d->m_bean->metadata()->readOnly() )
    {
        return;
    }

    // Para evitar llamadas recursivas
    if ( d->m_settingNewValue )
    {
        qDebug() << "DBField::setValue: EXISTEN LLAMADAS RECURSIVAS EN EL CODIGO: bean: [" <<
                 d->m_bean->metadata()->tableName() << "]. Field: [" << d->m->dbFieldName() << "].";
        AERPScript::printScriptsStack();
        return;
    }

    // No se puede dar valor a campos calculados, salvo que estos sean contadores
    if ( d->m->calculated() && !d->m->hasCounterDefinition() )
    {
        return;
    }

    d->m_settingNewValue = true;

    // Puede ser que estemos estableciendo el valor desde un hash. En ese caso
    // buscamos la key que corresponde a este valor
    if ( newValue.type() == QVariant::Hash )
    {
        QHashIterator<QString, QVariant> it(newValue.toHash());
        while ( it.hasNext() )
        {
            it.next();
            if ( it.key() == d->m->dbFieldName() )
            {
                data = it.value();
            }
        }
    }
    else if ( newValue.type() == QVariant::Map )
    {
        QMapIterator<QString, QVariant> it(newValue.toMap());
        while ( it.hasNext() )
        {
            it.next();
            if ( it.key() == d->m->dbFieldName() )
            {
                data = it.value();
            }
        }
    }
    else if ( d->m->coordinates() )
    {
        QStringList parts = newValue.toString().split(",");
        if ( parts.size() != 2 )
        {
            data = QString("%1,%2").arg(d->m_latitude).arg(d->m_longitude);
        }
        else
        {
            bool okLat, okLng;
            double lat = parts.at(0).toDouble(&okLat);
            double lng = parts.at(1).toDouble(&okLng);
            if ( okLat && okLng )
            {
                d->m_latitude = lat;
                d->m_longitude = lng;
                data = newValue.toString();
            }
            else
            {
                data = QString("%1,%2").arg(d->m_latitude).arg(d->m_longitude);
            }
        }
    }
    else
    {
        data = d->setDataToType(newValue);
    }
    // Si el campo es de tipo serial, es seguro que el valor no se ha modificado por el usuario (estos valores
    // sólo los da la base de datos)
    if ( !d->m->serial() )
    {
        // Permitimos que se pueda reiniciar el valor de un field a un valor inválido, por eso se hace la comprobación
        if ( newValue.isValid() )
        {
            data = d->setDataToType(data);
            // Esto evita las llamadas recursivas entre widget y field
            if ( data.isValid() && d->m_value != data )
            {
                d->m_previousValue = d->m_value;
                d->m_value = data;
                d->m_displayValue.clear();
                d->m_modified = true;
                d->m_valueIsOld = false;
                if ( d->m->memo() )
                {
                    d->m_memoLoaded = true;
                }
                emitValueModifiedByUser();
            }
        }
        else
        {
            // La asignación a valor no válido se hace siempre. Pero no siempre se levantan las señales
            // de valor modificado: sólo se hará si el valor no válido, que se traduce a un valor almacenable
            // es diferente del m_value almacenado. Por ejemplo, el campo es de tipo int, newValue es invalid,
            // y m_value es cero. En ese caso, no se modifica el valor del campo.
            QVariant newValueData = d->setDataToType(newValue);
            d->m_previousValue = d->m_value;
            if ( newValueData != d->m_value )
            {
                d->m_value = newValueData;
                d->m_displayValue.clear();
                emitValueModifiedByUser();
                d->m_modified = true;
            }
            else
            {
                d->m_value = newValueData;
                d->m_displayValue.clear();
            }
            d->m_valueIsOld = false;
        }
    }
    d->m_settingNewValue = false;
}

/**
  Inserta el valor a partir de cadena de texto. Esta función es muy utilizada para las funciones
  de importación/exportación. Lo que viene en data, debe ser interpretable por las funciones
  de conversión de QVariant. Es decir, por ejemplo, una cifra, debe venir asi 1234.56
  o una fecha, 2012-12-24 (formato ISO)
  */
void DBField::setValueFromSqlRawData(const QString &data)
{
    setValue(d->m->variantValueFromSqlRawData(data));
}

/**
 * @brief DBField::setOverwriteValue
 * Establece el valor interno del campo, obviando cálculos, contadores...
 * Pero genera señales, y también establece el estado a modificado
 * @param value
 */
void DBField::setOverwriteValue(const QVariant &value)
{
    QMutexLocker lock(&d->m_mutex);
    if ( value != d->m_value && !d->m_bean->readOnly() )
    {
        setOnExecution(AlephERP::OverwriteValue);
        setInternalValue(value);
        d->m_overwriteValue = true;
        emitValueModifiedByUser();
        d->m_modified = true;
        restoreOverrideOnExecution();
    }
}

void DBField::resetOverwriteValue()
{
    QMutexLocker lock(&d->m_mutex);
    d->m_overwriteValue = false;
}

/*!
  Almacena el valor pasado en el parámetro. Pero no emite señales. Es una
  función de uso interno de la aplicación.
  */
void DBField::setInternalValue(const QVariant &newValue, bool overwriteOnReadOnly)
{
    QMutexLocker lock(&d->m_mutex);

    if ( d->m_settingNewValue )
    {
        return;
    }
    if ( d->m_bean->readOnly() && !overwriteOnReadOnly )
    {
        return;
    }
    d->m_settingNewValue = true;
    QVariant data = d->setDataToType(newValue);
    QVariant v = d->m_value;
    // Esto evita las llamadas recursivas entre widget y field
    if ( v != data )
    {
        if ( d->m_value.isValid() )
        {
            d->m_previousValue = d->m_value;
        }
        else
        {
            d->m_previousValue = data;
        }
        if ( d->m->coordinates() )
        {
            d->m_latitude = 0;
            d->m_longitude = 0;
            QStringList parts = newValue.toString().split(",");
            if ( parts.size() == 2 )
            {
                bool okLat, okLng;
                double lat = parts.at(0).toDouble(&okLat);
                double lng = parts.at(1).toDouble(&okLng);
                if ( okLat && okLng )
                {
                    d->m_latitude = lat;
                    d->m_longitude = lng;
                }
            }
        }
        d->m_value = data;
        d->m_displayValue.clear();
        d->m_valueIsOld = false;
        // ¿Hay que actualizar los valores de beans hijos?
        QList<DBRelation *> rels = relations();
        foreach (DBRelation *rel, rels)
        {
            if ( rel->metadata()->type() == DBRelationMetadata::ONE_TO_MANY )
            {
                BaseBeanPointerList otherChildren = rel->internalChildren();
                foreach (BaseBeanPointer child, otherChildren)
                {
                    if ( !child.isNull() )
                    {
                        DBField *fld = child->field(rel->metadata()->childFieldName());
                        if ( fld != NULL )
                        {
                            // TODO: OJO: esto puede provocar llamadas recursivas si se configuran mal los metadatos.
                            fld->setInternalValue(d->m_value, overwriteOnReadOnly);
                        }
                    }
                }
            }
            else if ( rel->metadata()->type() == DBRelationMetadata::ONE_TO_ONE )
            {
                if ( (d->m->serial() || d->m->unique()) &&
                     !rel->internalBrother().isNull() &&
                     rel->brother()->fieldValue(rel->metadata()->childFieldName()) != d->m_value )
                {
                    rel->brother()->setInternalFieldValue(rel->metadata()->childFieldName(), d->m_value);
                }
            }
            else if ( rel->metadata()->type() == DBRelationMetadata::MANY_TO_ONE )
            {
                bool previous = rel->blockAllSignals(true);
                rel->updateChildrens();
                rel->blockAllSignals(previous);
            }
        }
        // Pero hay que escalar más en los permisos... Si este es el campo ID de un father, que se acaba de insertar, debemos actualizar el
        // valor del campo padre...
        if ( d->m_bean )
        {
            DBRelation *relationParent = qobject_cast<DBRelation *>(d->m_bean->parent());
            if ( relationParent != NULL &&
                 relationParent->metadata()->type() == DBRelationMetadata::MANY_TO_ONE &&
                 relationParent->metadata()->childFieldName() == d->m->dbFieldName() )
            {
                BaseBean *ancestorBean = qobject_cast<BaseBean *>(relationParent->parent());
                if ( ancestorBean != NULL )
                {
                    ancestorBean->setInternalFieldValue(relationParent->metadata()->rootFieldName(), d->m_value, overwriteOnReadOnly);
                }
            }
        }
    }
    if ( d->m->calculatedSaveOnDb() )
    {
        d->m_hasBeenCalculated = true;
    }
    if ( d->m->memo() )
    {
        d->m_memoLoaded = true;
    }
    d->m_settingNewValue = false;
}

void DBField::onChangeEvent()
{
    d->m->onChangeEvent(this);
}

/**
 * @brief DBField::emitValueModifiedByUser
 * Esta función se invocará cuando se haya producido una modificación del valor del campo por una acción
 * del usuario (y no por una recarga interna del valor, o por la precarga de datos...)
 */
void DBField::emitValueModifiedByUser()
{
    if ( !signalsBlocked() )
    {
        d->emitValueModified(d->m_value);
        d->emitValueModified(d->m->dbFieldName(), d->m_value);
    }
}

bool DBField::modified()
{
    return d->m_modified;
}

void DBField::setModified(bool value)
{
    QMutexLocker lock(&d->m_mutex);

    d->m_modified = value;
}

bool DBField::memoLoaded()
{
    return d->m_memoLoaded;
}

void DBField::setMemoLoaded(bool value)
{
    QMutexLocker lock(&d->m_mutex);

    d->m_memoLoaded = value;
}

QString DBField::dbFieldName()
{
    return d->m->dbFieldName();
}

QVariantMap DBField::relationsMap()
{
    QVariantMap map;
    foreach (DBRelation *rel, d->m_relations)
    {
        map[rel->metadata()->name()] = QVariant::fromValue(rel);
    }
    return map;
}

int DBField::day()
{
    if ( ! (d->m->type() == QVariant::Date || d->m->type() == QVariant::DateTime) )
    {
        return -1;
    }
    QDateTime dateTime = value().toDateTime();
    if ( dateTime.isNull() || !dateTime.isValid() )
    {
        return -1;
    }
    return dateTime.date().day();
}

int DBField::month()
{
    if ( ! (d->m->type() == QVariant::Date || d->m->type() == QVariant::DateTime) )
    {
        return -1;
    }
    QDateTime dateTime = value().toDateTime();
    if ( dateTime.isNull() || !dateTime.isValid() )
    {
        return -1;
    }
    return dateTime.date().month();
}

int DBField::year()
{
    if ( ! (d->m->type() == QVariant::Date || d->m->type() == QVariant::DateTime) )
    {
        return -1;
    }
    QDateTime dateTime = value().toDateTime();
    if ( dateTime.isNull() || !dateTime.isValid() )
    {
        return -1;
    }
    return dateTime.date().year();
}

BaseBeanPointer DBField::bean()
{
    return d->m_bean;
}

/*!
  Esta función formatea d->m_value de cara a que sea insertable en una SQL, obviando locales y demás.
  Así un string sería devuelto como 'Hola' con comillas incluídas si includeQuotesOnString está a true
  o sin comillas, si está a false
  */
QString DBField::sqlValue(bool includeQuotesOnString, const QString &dialect, bool useRawValue)
{
    if ( !d->m_bean->checkAccess('r') )
    {
        return trUtf8(AlephERP::stNoAccess);
    }
    QVariant v;
    if ( useRawValue )
    {
        v = rawValue();
    }
    else
    {
        v = value();
    }
    return d->sqlValue(v, includeQuotesOnString, dialect);
}

QString DBField::sqlOldValue(bool includeQuotesOnString, const QString &dialect)
{
    if ( !d->m_bean->checkAccess('r') )
    {
        return trUtf8(AlephERP::stNoAccess);
    }

    return d->sqlValue(d->m_oldValue, includeQuotesOnString, dialect);
}

double DBField::latitude() const
{
    return d->m_latitude;
}

void DBField::setLatitude(double val)
{
    QMutexLocker lock(&d->m_mutex);
    if ( d->m_bean->readOnly() )
    {
        return;
    }
    QVariant v = value();
    d->m_latitude = val;
    QString coords = QString("%1,%2").arg(d->m_latitude).arg(d->m_longitude);
    if ( v.toString() != coords )
    {
        QVariant data = d->setDataToType(coords);
        d->m_previousValue = d->m_value;
        d->m_value = data;
        d->m_displayValue.clear();
        d->m_modified = true;
        d->m_valueIsOld = false;
        emitValueModifiedByUser();
    }
}

double DBField::longitude() const
{
    return d->m_longitude;
}

void DBField::setLongitude(double val)
{
    QMutexLocker lock(&d->m_mutex);
    if ( d->m_bean->readOnly() )
    {
        return;
    }
    QVariant v = value();
    d->m_longitude = val;
    QString coords = QString("%1,%2").arg(d->m_latitude).arg(d->m_longitude);
    if ( v.toString() != coords )
    {
        QVariant data = d->setDataToType(coords);
        d->m_previousValue = d->m_value;
        d->m_value = data;
        d->m_displayValue.clear();
        d->m_modified = true;
        d->m_valueIsOld = false;
        emitValueModifiedByUser();
    }
}

bool DBField::overwrite() const
{
    return d->m_overwriteValue;
}

int DBField::counterVariablePart() const
{
    return d->m_counterVariablePart;
}

void DBField::setCounterVariablePart(int value)
{
    QMutexLocker lock(&d->m_mutex);
    d->m_counterVariablePart = value;
}

int DBField::length()
{
    if ( !d->m->lengthScript().isEmpty() )
    {
        return d->m->calculateLength(this);
    }
    return d->m->length();
}

QString DBField::valueOnCounter()
{
    if ( d->m->counterPrefix().isEmpty() )
    {
        QString v = displayValue();
        if ( !v.isEmpty() )
        {
            return v;
        }
        return defaultValue().toString();
    }
    if ( d->m->type() == QVariant::Bool )
    {
        if ( value().toBool() )
        {
            return d->m->counterPrefix();
        }
    }
    return QString();
}

bool DBField::hasBeenCalculated() const
{
    return d->m_hasBeenCalculated;
}

/*!
  Proporciona una claúsula where para la el campo. Útil para updates y deletes
  */
QString DBField::sqlWhere(const QString &op, const QString &dialect)
{
    QString result;
    QString sqlFieldName = d->m->dbFieldName();

    if ( d->m->type() == QVariant::Double )
    {
        result = QString ("round(%1::numeric, %2) %3 %4").arg(sqlFieldName).
                 arg(DB_DOUBLE_PRECISION).arg(op).
                 arg(sqlValue(true, dialect));
    }
    else
    {
        result = QString("%1 %2 %3").arg(sqlFieldName).
                 arg(op).arg(sqlValue(true, dialect));
    }
    return result;
}

/**
  Ejecuta el script asociado para obtener el tooltip
  */
QString DBField::toolTip()
{
    return d->m->toolTip(this);
}

bool DBField::hasToolTip() const
{
    return !d->m->toolTipScript().isEmpty() || d->m->toolTipStringExpression().valid;
}

/*!
  Pasa un checkeo de filtro. Indica si el valor pasado corresponde a parte del valor almacenado
  (caso de una cadena) o es igual, caso de números, fechas... Muy útil para proxys de filtros
  Existen algunas reglas:
  -Las cadenas pueden de ir entre comillas, pero serán eliminadas al principio y al final.
  -Para cadenas, se pueden utilizar expresiones regulares
  -Los rangos de fechas deberán ir separados por "-", en formato dd/MM/yyy de forma "01/01/2010-12/12/2012"
  TODO: Pasar a formatos de fechas internacionales
  */
bool DBField::checkValue(const QVariant &chkValue, const QString &op, Qt::CaseSensitivity cs)
{
    bool result;

    if ( d->m->type() == QVariant::Int )
    {
        if ( op == "<" )
        {
            return ( chkValue.toInt() > value().toInt() );
        }
        else if ( op == ">" )
        {
            return ( chkValue.toInt() < value().toInt() );
        }
        else if ( op == "=" )
        {
            return ( chkValue.toInt() == value().toInt() );
        }
        else if ( op == "!=" )
        {
            return ( chkValue.toInt() != value().toInt() );
        }
        else
        {
            return false;
        }
    }
    else if ( d->m->type() == QVariant::Double )
    {
        if ( op == "<" )
        {
            return ( chkValue.toDouble() > value().toDouble() );
        }
        else if ( op == ">" )
        {
            return ( chkValue.toDouble() < value().toDouble() );
        }
        else if ( op == "=" )
        {
            return ( chkValue.toDouble() == value().toDouble() );
        }
        else if ( op == "!=" )
        {
            return ( chkValue.toDouble() != value().toDouble() );
        }
        else
        {
            return false;
        }
    }
    else if ( d->m->type() == QVariant::Date )
    {
        if ( op == "<" )
        {
            return ( value().toDate() < chkValue.toDate());
        }
        else if ( op == ">" )
        {
            return ( value().toDate() > chkValue.toDate() );
        }
        else if ( op == "=" )
        {
            return ( value().toDate() == chkValue.toDate() );
        }
        else if ( op == "!=" )
        {
            return ( value().toDate() != chkValue.toDate() );
        }
        else
        {
            return false;
        }
    }
    else if ( d->m->type() == QVariant::DateTime )
    {
        if ( op == "<" )
        {
            return ( value().toDateTime() < chkValue.toDateTime());
        }
        else if ( op == ">" )
        {
            return ( value().toDateTime() > chkValue.toDateTime() );
        }
        else if ( op == "=" )
        {
            return ( value().toDateTime() == chkValue.toDateTime() );
        }
        else if ( op == "!=" )
        {
            return ( value().toDateTime() != chkValue.toDateTime() );
        }
        else
        {
            return false;
        }
    }
    else if ( d->m->type() == QVariant::Bool )
    {
        bool invert = ( op == "!=" ? true : false );
        result = (chkValue.toBool() == value().toBool());
        if ( invert )
        {
            result = !result;
        }
    }
    else if ( d->m->type() == QVariant::String )
    {        
        QString tmpChkValue = chkValue.toString().trimmed();
        if ( tmpChkValue.startsWith('\"', cs) || tmpChkValue.startsWith('\'', cs) )
        {
            tmpChkValue.remove(0, 1);
        }
        if ( tmpChkValue.endsWith('\"', cs) || tmpChkValue.endsWith('\'', cs) )
        {
            tmpChkValue.remove(tmpChkValue.size()-1, 1);
        }
        QRegExp tmp;
        QString val;
        if ( cs == Qt::CaseInsensitive )
        {
            tmp = QRegExp(tmpChkValue.toLower());
            val = value().toString().toLower();
        }
        else
        {
            tmp = QRegExp(tmpChkValue);
            val = value().toString();
        }
        bool invert = ( op == "!=" ? true : false );
        if ( tmp.isValid() )
        {
            int index = tmp.indexIn(val);
            result = ( index == -1 ? false : true );
        }
        else
        {
            result = value().toString().compare(chkValue.toString(), cs);
        }
        if ( invert )
        {
            result = !result;
        }
    }
    else
    {
        qCritical() << "checkValue: DBField: " << d->m->dbFieldName() << ". No tiene definido un tipo de datos. Asignando el tipo por defecto.";
        bool invert = ( op == "!=" ? true : false );
        result = value().toString().contains(chkValue.toString(), Qt::CaseInsensitive);
        if ( invert )
        {
            result = !result;
        }
    }
    return result;
}

/*!
  Función adecuada para realizar un chequeo BETWEEN entre dos valores. Tiene sentido
  para campos de tipo Numérico, o fecha
  */
bool DBField::checkValue(const QVariant &value1, const QVariant &value2)
{
    if ( d->m->type() == QVariant::Int )
    {
        return ( value().toInt() >= value1.toInt() && value().toInt() <= value2.toInt() );
    }
    else if ( d->m->type() == QVariant::Double )
    {
        return ( value().toDouble() >= value1.toDouble() && value().toDouble() <= value2.toDouble() );
    }
    else if ( d->m->type() == QVariant::Date )
    {
        bool result = value().toDate() >= value1.toDate() && value().toDate() <= value2.toDate();
        return result;
    }
    else if ( d->m->type() == QVariant::DateTime )
    {
        bool result = value().toDateTime() >= value1.toDateTime() && value().toDateTime() <= value2.toDateTime();
        return result;
    }
    else if ( d->m->type() == QVariant::Bool )
    {
        return (value().toBool() == value1.toBool() && value().toBool() == value2.toBool() );
    }
    else if ( d->m->type() == QVariant::String )
    {
        return (value().toString() >= value1.toString() && value().toString() <= value2.toString() );
    }
    else
    {
        qCritical() << "checkValue: DBField: " << d->m->dbFieldName() << ". No tiene definido un tipo de datos. Asignando el tipo por defecto.";
        return false;
    }
}

/*!
  Si el field es calculated y aggregated, se ejecuta esta función para obtener su valor. La idea es que
  el valor de este field se calcula a partir de una operación con el resto de beans iguales a este
  y según un field: Por ejemplo, para la suma de un total
  */
QVariant DBFieldPrivate::calculateAggregateValue()
{
    int countResult = 0;
    double sumResult = 0;
    AggregateCalc calc = m->aggregateCalc();
    QVariant result;
    QElapsedTimer timer;

    if ( m_bean.isNull() )
    {
        return QVariant();
    }

    if ( q_ptr->isExecuting(AlephERP::AggregateValue) )
    {
        return QVariant();
    }

    timer.start();
    q_ptr->setOnExecution(AlephERP::AggregateValue);
    q_ptr->registerCalculatingOnBean();

    if ( calc.useExpression() )
    {
        for ( int i = 0 ; i < calc.expression.size() ; i++ )
        {
            calculateAggregateExpressionItem(i, calc.relation.at(i), calc.filter.at(i), sumResult, countResult);
        }
    }
    else if ( calc.useScript() )
    {
        for ( int i = 0 ; i < calc.script.size() ; i++ )
        {
            calculateAggregateScriptItem(calc.relation.at(i), calc.filter.at(i), calc.script.at(i), sumResult, countResult);
        }
    }
    else
    {
        for ( int i = 0 ; i < calc.relation.size() ; i++ )
        {
            DBRelation *rel = m_bean->relation(calc.relation.at(i));
            if ( rel != NULL )
            {
                BaseBeanPointerList beans = rel->childrenByFilter(calc.filter.at(i));
                foreach ( BaseBeanPointer bean, beans )
                {
                    if ( !bean.isNull() )
                    {
                        if ( bean->dbState() != BaseBean::TO_BE_DELETED && bean->dbState() != BaseBean::DELETED )
                        {
                            DBField *fld = bean->field(calc.field.at(i));
                            if ( fld != NULL )
                            {
                                countResult++;
                                sumResult = sumResult + fld->value().toDouble();
                            }
                        }
                    }
                    else
                    {
                        qDebug() << "DBFieldPrivate::calculateAggregateValue(): Algunos hijos de la relacion " << rel->metadata()->tableName() << " son nulos.";
                    }
                }
            }
            else
            {
                qDebug() << "DBField::calculateAggregateValue: nombre de la relación incorrecta: " << calc.relation.at(i);
            }
        }
    }
    if ( calc.operation == "sum" )
    {
        result = QVariant(sumResult);
    }
    else if ( calc.operation == "count" )
    {
        result = QVariant(countResult);
    }
    else if ( calc.operation == "avg" )
    {
        result = QVariant (sumResult / countResult);
    }
    q_ptr->unregisterCalculatingOnBean();
    q_ptr->restoreOverrideOnExecution();
    qint64 elapsed = timer.elapsed();
    QLogger::QLog_Debug(AlephERP::stLogOther, QString("DBFieldPrivate::calculateAggregateValue: fieldName: %1.%2. Calculate Aggregate: [%3] ms").
               arg(m_bean->metadata()->tableName()).
               arg(m->dbFieldName()).
               arg(elapsed));
    if ( elapsed > AlephERP::warningCalculatedTime )
    {
        QLogger::QLog_Info(AlephERP::stLogScript, "DBFieldPrivate::calculateAggregateValue: Ha tomado mucho tiempo!!!");
    }
    return result;
}

/** Si hay una estructura definida así
<aggregate>
    <script>
<![CDATA[
function value() {
return (relationChildRecord.importetotal.value * relationChildRecord.irpf.value) / 100;
}
]]>
    </script>
    <relation>lineasservicios${tableName}</relation>
</aggregate>
Se repetirá el script por cada línea o children de la relación determinada, y se pasará el script como un objeto llamado relationChildRecord
*/
void DBFieldPrivate::calculateAggregateScriptItem(const QString &relation, const QString &filter, const QString &script, double &sumResult, int &countResult)
{
    QVariant result;
    if ( !relation.isEmpty() )
    {
        DBRelation *rel = m_bean->relation(relation);
        if ( rel != NULL )
        {
            BaseBeanPointerList beans = rel->childrenByFilter(filter);
            foreach ( BaseBeanPointer bean, beans )
            {
                if ( !bean.isNull() )
                {
                    if ( bean->dbState() != BaseBean::TO_BE_DELETED && bean->dbState() != BaseBean::DELETED )
                    {
                        result = m->calculateAggregateScript(q_ptr, script, bean.data());
                        if ( result.isValid() )
                        {
                            countResult++;
                            sumResult = sumResult + result.toDouble();
                        }
                    }
                }
                else
                {
                    qDebug() << "DBFieldPrivate::calculateAggregateScriptItem(): Algunos hijos de la relacion " << rel->metadata()->tableName() << " son nulos.";
                }
            }
        }
        else
        {
            qDebug() << "DBField::calculateAggregateScriptItem: nombre de la relación incorrecta: " << relation;
        }
    }
    else
    {
        result = m->calculateAggregateScript(q_ptr, script);
        if ( result.isValid() )
        {
            countResult++;
            sumResult = sumResult + result.toDouble();
        }
    }
}

/** Si hay una estructura definida así
<aggregate>
    <builtInExpression>
        <expression>(importetotal * irpf) / 100</expression>
    </builtInExpression>
    <relation>lineasservicios${tableName}</relation>
</aggregate>
Se repetirá el script por cada línea o children de la relación determinada. Los objetos importetotal o irpf son fields del children de la relación.
*/
void DBFieldPrivate::calculateAggregateExpressionItem(int indexAggregate, const QString &relation, const QString &filter, double &sumResult, int &countResult)
{
    DBRelation *rel = m_bean->relation(relation);
    if ( rel != NULL )
    {
        BaseBeanPointerList beans = rel->childrenByFilter(filter);
        foreach ( BaseBeanPointer bean, beans )
        {
            if ( !bean.isNull() )
            {
                if ( bean->dbState() != BaseBean::TO_BE_DELETED && bean->dbState() != BaseBean::DELETED )
                {
                    m_calcAggregate[indexAggregate]->setBean(bean.data());
                    countResult++;
                    if ( m_calcAggregate[indexAggregate]->isWorking() )
                    {
                        sumResult = sumResult + 0;
                    }
                    else
                    {
                        sumResult = sumResult + m_calcAggregate[indexAggregate]->value().toDouble();
                    }
                }
            }
            else
            {
                qDebug() << "DBFieldPrivate::calculateAggregateExpressionItem(): Algunos hijos de la relacion " << rel->metadata()->tableName() << " son nulos.";
            }
        }
    }
    else
    {
        qDebug() << "DBField::calculateAggregateExpressionItem: nombre de la relación incorrecta: " << relation;
    }
}

/*!
  Tenemos que decirle al motor de scripts, que DBSearchDlg se convierte de esta forma a un valor script
  */
QScriptValue DBField::toScriptValue(QScriptEngine *engine, DBField* const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBField::fromScriptValue(const QScriptValue &object, DBField* &out)
{
    out = qobject_cast<DBField *>(object.toQObject());
}

QScriptValue DBField::toScriptValueSharedPointer(QScriptEngine *engine, const QSharedPointer<DBField> &in)
{
    return engine->newQObject(in.data(), QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBField::fromScriptValueSharedPointer(const QScriptValue &object, QSharedPointer<DBField> &out)
{
    out = QSharedPointer<DBField>(qobject_cast<DBField *>(object.toQObject()));
}

/**
 * @brief DBField::scheduleDurationValue
 * Devuelve en segundos el valor indicado para ser representado en el schedule view
 * @return
 */
int DBField::scheduleDurationValue()
{
    bool ok;
    int v = value().toInt(&ok);

    if ( !ok )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, "DBField::scheduleDurationValue: El valor no ha podido ser convertido a entero.");
        return 0;
    }
    switch ( d->m->scheduleTimeUnit() )
    {
    case AlephERP::DayOfMonth:
    case AlephERP::DayOfWeek:
        return (v * 24 * 3600);
    case AlephERP::Hour:
        return v * 3600;
    case AlephERP::Minute:
        return v * 60;
    case AlephERP::Month:
        return (v * 24 * 3600) * 30;
    case AlephERP::Second:
        return v;
    case AlephERP::Week:
        return (v * 24 * 3600) * 7;
    case AlephERP::Year:
        return (v * 24 * 3600) * 365;
    default:
        return 0;
    }
}

void DBField::setScheduleDurationValue(int val)
{
    switch ( d->m->scheduleTimeUnit() )
    {
    case AlephERP::DayOfMonth:
    case AlephERP::DayOfWeek:
        setValue(val / (24 * 3600));
        break;
    case AlephERP::Hour:
        setValue (val / 3600);
        break;
    case AlephERP::Minute:
        setValue (val / 60);
        break;
    case AlephERP::Month:
        setValue (val / ((24 * 3600) * 30));
        break;
    case AlephERP::Second:
        setValue (val);
        break;
    case AlephERP::Week:
        setValue (val / ((24 * 3600) * 7));
        break;
    case AlephERP::Year:
        setValue (val / ((24 * 3600) * 365));
        break;
    default:
        setValue (0);
        break;
    }
}

/*!
  Para permitir hacer comparaciones entre campos
  */
bool DBField::operator < (DBField &field)
{
    if ( d->m->type() == QVariant::Int )
    {
        return ( value().toInt() < field.value().toInt() );
    }
    else if ( d->m->type() == QVariant::Double )
    {
        return (  value().toDouble() < field.value().toDouble() );
    }
    else if ( d->m->type() == QVariant::Date )
    {
        return ( value().toDate() < field.value().toDate());
    }
    else if ( d->m->type() == QVariant::DateTime )
    {
        return ( value().toDateTime() < field.value().toDateTime());
    }
    else if ( d->m->type() == QVariant::Bool )
    {
        return ( field.value().toBool() != value().toBool() );
    }
    else if ( d->m->type() == QVariant::String )
    {
        return ( value().toString() < field.value().toString() );
    }
    return false;
}

bool DBField::blockSignals ( bool block )
{
    return QObject::blockSignals(block);
}

/*!
  Si el field es calculated y es un contador, se ejecuta esta función para obtener su valor. Se tiene así la
  posibilidad de crear contadores complejos
  */
QVariant DBField::calculateCounter(const QString &connection, bool searchOnTransaction)
{
    QVariant data;
    QElapsedTimer timer;

    if ( d->m_bean.isNull() )
    {
        return QVariant();
    }

    if ( isExecuting(AlephERP::CounterValue) )
    {
        return QVariant();
    }

    timer.start();
    setOnExecution(AlephERP::CounterValue);
    if ( !d->m->counterDefinition()->expression.isEmpty() )
    {
        data = d->calculateCounter(connection, searchOnTransaction);
    }
    else
    {
        data = d->calculateCounterOldMethod(connection, searchOnTransaction);
    }
    restoreOverrideOnExecution();
    qint64 elapsed = timer.elapsed();
    QLogger::QLog_Debug(AlephERP::stLogOther, QString("DBField::calculateCounter: fieldName: %1.%2. Calculate Aggregate: [%3] ms").
               arg(d->m_bean->metadata()->tableName()).
               arg(d->m->dbFieldName()).
               arg(elapsed));
    if ( elapsed > AlephERP::warningCalculatedTime )
    {
        QLogger::QLog_Info(AlephERP::stLogScript, "DBField::calculateCounter: Ha tomado mucho tiempo!!!");
    }
    return data;
}

QVariant DBFieldPrivate::calculateCounterOldMethod(const QString &connection, bool searchOnTransaction)
{
    QVariant result;
    QString counterPrefix;
    QStringList fields, prefixs;
    fields = m->counterDefinition()->fields;
    prefixs = m->counterDefinition()->prefixOnRelations;
    for ( int i = 0 ; i < fields.size() ; i++ )
    {
        DBField *otherField = m_bean->field(fields.at(i));
        if ( otherField != NULL )
        {
            if ( prefixs.size() > i && !prefixs.at(i).isEmpty() )
            {
                QList<DBRelation *> rels = otherField->relations(AlephERP::ManyToOne);
                foreach ( DBRelation *rel, rels )
                {
                    BaseBean *father = rel->father();
                    if ( father != NULL )
                    {
                        DBField *fld = father->field(prefixs.at(i));
                        if ( fld != NULL )
                        {
                            QString fv = fld->valueOnCounter();
                            if ( counterPrefix.isEmpty() )
                            {
                                counterPrefix = fv;
                            }
                            else
                            {
                                counterPrefix.append(m->counterDefinition()->separator).append(fv);
                            }
                        }
                    }
                }
            }
            else
            {
                QString val = otherField->valueOnCounter();
                if ( counterPrefix.isEmpty() )
                {
                    counterPrefix = val;
                }
                else
                {
                    counterPrefix.append(m->counterDefinition()->separator).append(val);
                }
            }
        }
        else
        {
            // Si no se encuentra el FIELD adecuado, se inserta directamente el nombre
            counterPrefix.append(fields.at(i));
        }
    }
    counterPrefix.append(m->counterDefinition()->separator);
    if ( m_counterVariablePart == -1 )
    {
        QString where = QString("%1 LIKE '%2%%'").arg(m->dbFieldName()).arg(counterPrefix);
        where = filterForUniqueOnFilterField(where);
        where = m->beanMetadata()->processWhereSqlToIncludeEnvVars(where);
        QString sql = QString("SELECT max(%1) as column1 FROM %2 WHERE %3").
                        arg(m->dbFieldName()).
                        arg(m_bean->metadata()->sqlTableName()).
                        arg(where);
        if ( !BaseDAO::execute(sql, result, connection) )
        {
            return result;
        }
    }
    else
    {
        result = m_counterVariablePart - 1;
    }
    // Vamos a obtener los contadores que ya pudiesen estar registrados
    QList<QVariant> countersOnTransaction;
    if ( searchOnTransaction )
    {
        countersOnTransaction = AERPTransactionContext::instance()->registeredCounterValues(q_ptr);
    }
    int counter = 0;
    if ( !result.isNull() )
    {
        bool ok;
        QString stringResult = result.toString();
        stringResult = stringResult.remove(counterPrefix);
        counter = stringResult.toInt(&ok);
        if ( !ok )
        {
            QLogger::QLog_Error(AlephERP::stLogOther, QString("DBFieldPrivate::calculateCounterOldMethod: La base de datos ha devuelto un valor no numérico [%1]").arg(stringResult));
            return QVariant();
        }
    }
    do
    {
        counter++;
        QString final = QString("%1").arg(counter);
        QString tmp;
        if ( m->counterDefinition()->useTrailingZeros )
        {
            tmp = counterPrefix.leftJustified(q_ptr->length()-final.size(), '0');
        }
        else
        {
            tmp = counterPrefix;
        }
        result = QVariant(tmp.append(final));
    }
    while (countersOnTransaction.contains(result) && m_counterVariablePart == -1);
    if ( searchOnTransaction )
    {
        AERPTransactionContext::instance()->registerCounterValue(q_ptr, result);
    }
    return result;
}

QVariant DBFieldPrivate::calculateCounter(const QString &connection, bool searchOnTransaction)
{
    QVariant counterValue;
    StringExpression stringExpression;
    QString counterPart, result;
    int counterPartLength = 0;
    bool hasTrailingZeros = m->counterDefinition()->expression.indexOf("trailingZeros") == -1 ? false : true;
    QHash<QString, QString> otherValues;

    stringExpression.expression = m->counterDefinition()->expression;
    // Primero generamos la expresión sin el valor del contador (por si hay que aplicar trailing zeros)
    result = stringExpression.applyExpressionRules(q_ptr, QHash<QString, QString>(), true);

    if ( m_counterVariablePart )
    {
        QString sql, where;
        sql = QString("SELECT max(%1) as column1 FROM %2").
                      arg(m->dbFieldName()).
                      arg(m_bean->metadata()->sqlTableName());
        if ( m->type() == QVariant::String )
        {
            where = QString("%1 LIKE '%2%%'").
                      arg(m->dbFieldName()).
                      arg(result);
        }
        where = filterForUniqueOnFilterField(where);
        where = m_bean->metadata()->processWhereSqlToIncludeEnvVars(where);
        if ( !where.isEmpty() )
        {
            sql.append(" WHERE ").append(where);
        }
        if ( !BaseDAO::execute(sql, counterValue, connection) )
        {
            return result;
        }
    }
    else
    {
        counterValue = m_counterVariablePart -1;
    }
    if ( m->type() != QVariant::String )
    {
        qlonglong v = counterValue.toLongLong();
        v++;
        return v;
    }
    counterPartLength = q_ptr->length() - result.size();
    if ( counterPartLength <= 0 )
    {
        return result;
    }
    QList<QVariant> countersOnTransaction;
    if ( searchOnTransaction )
    {
        countersOnTransaction = AERPTransactionContext::instance()->registeredCounterValues(q_ptr);
    }
    long lCounterValue;
    if ( result.isNull() )
    {
        lCounterValue = 0;
    }
    else
    {
        bool ok;
        QString counterString;
        if ( !counterValue.toString().isEmpty() ) {
            counterString = counterValue.toString();
            // Eliminamos la parte común que no sea el propio valor numérico del contador.
            counterString = counterString.replace(result, QString(""));
            lCounterValue = counterString.toLong(&ok);
            if ( !ok )
            {
                QLogger::QLog_Error(AlephERP::stLogOther, QString("DBFieldPrivate::calculateCounter: Error. No se ha podido convertir a long [%1]").arg(counterString));
                return result;
            }
        }
        else
        {
            lCounterValue = 0;
        }
    }

    do
    {
        lCounterValue++;
        QString counterString = QString("%1").arg(lCounterValue);
        if ( hasTrailingZeros )
        {
            counterPart = counterString.rightJustified(counterPartLength, '0');
        }
        else
        {
            counterPart = counterString;
        }
        if ( hasTrailingZeros )
        {
            otherValues["value.trailingZeros"] = counterPart;
        }
        else
        {
            otherValues["value"] = counterPart;
        }
        result = stringExpression.applyExpressionRules(q_ptr, otherValues, true);
    }
    while (countersOnTransaction.contains(result) && m_counterVariablePart == -1);
    if ( searchOnTransaction )
    {
        AERPTransactionContext::instance()->registerCounterValue(q_ptr, result);
    }
    return result;
}


/**
 * @brief DBField::buildCounterValue
 * Construye el campo con el valor calculado, pero especificando un valor para el mismo... Por ejemplo
 * imaginemos un contador que tiene la forma PREFIX1/PREFIX2/13.
 * Es una función pensada para el motor QS, y debe recibir un único objeto, con los campos y valores. Así
 * var param = new Object();
 * param.field1 = value;
 * param.field2 = value2;
 * param.value = valorNumericoContador;
 * var calculo = dbField.buildCounterValue(param);
 * también
 * param.valueTrailingZeros=valorNumericoContador;
 * donde
 * @return
 */
QVariant DBField::buildCounterValue()
{
    if ( d->m_bean.isNull() )
    {
        return QVariant();
    }

    if ( d->m->type() != QVariant::String )
    {
        QLogger::QLog_Error(AlephERP::stLogScript, QString::fromUtf8("DBField::buildCounterValue: Los campos calculados deben ser obligatoriamente de typo STRING"));
        return QVariant();
    }
    if ( engine() == NULL || context() == NULL )
    {
        return QVariant();
    }
    if ( context()->argumentCount() == 0 )
    {
        return QVariant();
    }
    QScriptValue objectParam = context()->argument(0);
    if ( !objectParam.isObject() )
    {
        QLogger::QLog_Error(AlephERP::stLogScript, QString::fromUtf8("DBField::buildCounterValue. Parámetros mal pasados."));
        return QVariant();
    }

    QHash<QString, QString> otherValues;
    QScriptValueIterator it(objectParam);
    bool hasTrailingZeros = false;
    StringExpression stringExpression;
    QString counterString;

    stringExpression.expression = d->m->counterDefinition()->expression;

    while ( it.hasNext() )
    {
        it.next();
        otherValues[it.name()] = it.value().toString();
        if ( it.name().toLower().contains("trailingzeros") )
        {
            hasTrailingZeros = true;
            counterString = it.value().toString();
        }
    }
    if ( hasTrailingZeros )
    {
        QString result = stringExpression.applyExpressionRules(this, otherValues);
        int counterPartLength = length() - result.size();
        QString counterPart = counterString.rightJustified(counterPartLength, '0');
        otherValues["value.trailingZeros"] = counterPart;
    }
    QString result = stringExpression.applyExpressionRules(this, otherValues);
    return result;
}

bool DBField::counterBlocked()
{
    return d->m_counterBlocked;
}

void DBField::setCounterBlocked(bool value)
{
    QMutexLocker lock(&d->m_mutex);
    d->m_counterBlocked = value;
}

/**
  Este slot sólo actúa en campos calculados, y genera un nuevo cálculo del campo calculado.
  Recalcula incluso campos marcados como "calculatedOneTime". OJO: Esta función puede hacer
  que otros campos se recalculen a su vez (incluyendo a campos contadores) ya que internamente llama
  a "value" y por tanto, afectando a otros campos... Si sólo se quiere cambiar el valor de este campo,
  hay que bloquear antes las señales.
  */
void DBField::recalculate()
{
    QMutexLocker lock(&d->m_mutex);
    if ( d->m_bean->readOnly() || d->m_overwriteValue )
    {
        return;
    }
    if ( d->m_bean->dbState() == BaseBean::UPDATE &&
         (d->m->calculatedOnlyOnInsert() || d->m->counterDefinition()->calculateOnlyOnInsert) )
    {
        return;
    }
    if ( m_executingStack.contains(AlephERP::CalculateValue) ||
         m_executingStack.contains(AlephERP::AggregateValue) ||
         m_executingStack.contains(AlephERP::CounterValue) )
    {
        return;
    }
    d->m_valueIsOld = true;

    if ( d->m->calculated() && !isWorking() )
    {
        QVariant v = d->calculateValue();
        if ( v != d->m_value )
        {
            d->m_previousValue = d->m_value;
            d->m_value = v;
            d->m_displayValue.clear();
            emitValueModifiedByUser();
            // Un campo calculado, puede no emitir la señal de haber sido modificado, y es probable que los widgets que dependen de él
            // no se vean actualizados. Forzamos una actualización por si acaso.
            if ( observer() != NULL )
            {
                observer()->sync();
            }
        }
    }
}

QString DBField::validateMessages() const
{
    return d->m_message;
}

/*!
  Devuelve el mensaje correspondiente a la última llamada a validate, incluyendo
  los datos por los que no se pasó la validación. Lo hace en HTML
*/
QString DBField::validateHtmlMessages() const
{
    return d->m_htmlMessage;
}

/*!
  De una mala validación, devuelve el primer widget que tenía problemas
*/
QWidget *DBField::widgetOnBadValidate() const
{
    return d->m_widget;
}

/**
 * @brief DBField::validate
 * Comprueba si el valor del campo cumple los requisitos establecidos.
 * @return
 */
bool DBField::validate()
{
    bool result = true;

    QMutexLocker lock(&d->m_mutex);
    d->m_message.clear();
    d->m_htmlMessage.clear();
    d->m_widget = NULL;

    // En las validaciones NO emitimos señales!!. Eso podría hacer que el bean padre quede marcado como modificado
    // cuando realmente no lo ha sido.
    bool blockSignalsState = blockSignals(true);

    // El campo no puede ser nulo (salvo que sea serial)
    if ( !metadata()->serial() && !metadata()->canBeNull() )
    {
        result = result & d->checkNull();
    }
    // Comprobemos que no se excede la longitud establecida
    if ( metadata()->type() == QVariant::String )
    {
        if ( metadata()->isOnDb() )
        {
            result = result & d->checkLength();
        }
    }
    // Si hay alguna regla de validación en Javascript definida en el XML se comprueba
    if ( !metadata()->validationRuleScript().isEmpty() )
    {
        bool temp = false;
        QString message = metadata()->validateRule(this, temp);
        result = result & temp;
        if ( !temp && !message.isEmpty() )
        {
            d->m_htmlMessage = QString("%1<p><strong>%2</strong>: %3</p>").
                               arg(d->m_htmlMessage).arg(metadata()->fieldName()).arg(message);
        }
    }
    // Si el campo está marcado como único, no puede haber ningún valor igual
    if ( metadata()->unique() )
    {
        result = result & d->checkUnique();
    }
    if ( !metadata()->uniqueOnFilterField().isEmpty() )
    {
        result = result & d->checkUniqueOnFilterField();
    }

    blockSignals(blockSignalsState);

    return result;
}

/**
 * @brief DBField::loadValueOnBackground
 * Para columnas de tipo memo o tipo imágen, puede ser interesante cargar el valor en segundo plano, y no bloquear
 * la interfaz de usuario. Esta función se encarga de ello.
 * Al finalizar la carga, se emite la señal valueLoaded
 */
void DBField::loadValueOnBackground()
{
    QMutexLocker lock(&d->m_mutex);
    if ( !d->m->loadOnBackground() )
    {
        emit dataAvailable(value());
        return;
    }
    if ( !d->m_backgroundUuid.isEmpty() )
    {
        return;
    }
    QString sql = QString("SELECT %1 FROM %2 WHERE %3").arg(metadata()->dbFieldName()).
                  arg(bean()->metadata()->sqlTableName()).arg(bean()->sqlWherePk());

    d->m_backgroundUuid = BackgroundDAO::instance()->programQuery(sql);
    connect(BackgroundDAO::instance(), SIGNAL(queryExecuted(QString,bool)), this, SLOT(backgroundDataAvailable(QString,bool)));
}

bool DBField::isLoadingOnBackground() const
{
    return !d->m_backgroundUuid.isEmpty();
}

void DBField::touch()
{
    QMutexLocker lock(&d->m_mutex);
    d->m_modified = true;
}

void DBField::backgroundDataAvailable(const QString &id, bool result)
{
    QMutexLocker lock(&d->m_mutex);
    if ( d->m_backgroundUuid != id )
    {
        return;
    }
    if ( !result )
    {
        emit errorBackgroundLoad(BackgroundDAO::instance()->lastError(d->m_backgroundUuid));
    }
    else
    {
        QVector<QVariantList> v = BackgroundDAO::instance()->takeResults(d->m_backgroundUuid);
        QVariant val;
        if ( v.isEmpty() || v.at(0).size() == 0 )
        {
            emit dataAvailable(val);
        }
        else
        {
            val = v.at(0).at(0);
            if ( d->m->type() == QVariant::Pixmap )
            {
                QByteArray ba = val.toByteArray();
                val = QVariant(QByteArray::fromBase64(ba));
            }
            emit dataAvailable(val);
        }
        setInternalValue(val);
    }
    d->m_backgroundUuid.clear();;
    disconnect(BackgroundDAO::instance(), SIGNAL(queryExecuted(QString,bool)), this, SLOT(backgroundDataAvailable(QString,bool)));
}

/**
 * @brief DBField::valueIsOld
 * El campo se conecta a los campos de los que depende su valor. Cuando uno de estos cambia, se marca que ssu valor
 * se ha modificado. Esa comprobación se tendrá en cuenta a lo hora de generar el valor
 */
void DBField::setValueIsOld()
{
    d->m_valueIsOld = true;
}

DBField *DBField::fromVariant(const QVariant &v)
{
    if ( v.canConvert<QObject *>() )
    {
        QObject *obj = (QObject *)(v.value<QObject *>());
        if ( obj != NULL )
        {
            DBField *fld = qobject_cast<DBField *>(obj);
            return fld;
        }
    }
    return NULL;
}

/**
  Recalcula el valor de contador, si, el usuario NO puede modificar el texto, o si pudiéndolo modificar
  éste field no se ha modificado. Esto se hace así para que prevarezca el valor del usuario.
  Es posible indicar la conexión ya que estando en modo desconectado, podría ser interesante obtener
  el contador desde el servidor remoto.
  */
void DBField::recalculateCounterField(const QString &connection)
{
    QMutexLocker lock(&d->m_mutex);
    if ( d->m_overwriteValue )
    {
        return;
    }
    d->m_valueIsOld = true;
    if ( !d->m->hasCounterDefinition() || isWorking() )
    {
        return;
    }
    if ( !d->m->counterDefinition()->userCanModified ||
         (d->m->counterDefinition()->userCanModified && !modified()) )
    {
        if ( !d->m_bean.isNull() )
        {
            // Si se llama a este slot invocado por una modificación en otro campo, debemos comprobar que el contador pueda o no recalcularse.
            DBField *fldSender = qobject_cast<DBField *>(sender());
            if ( fldSender != NULL && d->m_bean->dbState() == BaseBean::UPDATE && d->m->counterDefinition()->calculateOnlyOnInsert )
            {
                return;
            }
            if ( !d->m_counterBlocked )
            {
                QVariant v = calculateCounter(connection);
                // Importante hacer el setOnExecution, ya que setValue emite la señal valueModified, que podría hacer
                // que éste metodo volviese a ejecutarse (conexiones).
                setOnExecution(AlephERP::CounterValue);
                setValue(v);
                restoreOverrideOnExecution();
                d->m_modified = false;
                d->m_overwriteValue = false;
                d->m_hasBeenCalculated = true;
            }
        }
    }
}

/**
 * @brief DBField::isEmpty
 * Validando por campo, nos indica si AlephERP considera el campo vacío.
 * @return
 */
bool DBField::isEmpty()
{
    if ( value().isNull() || !value().isValid() )
    {
        return true;
    }
    if ( d->m->type() == QVariant::Int )
    {
        return (value().toInt() == 0);
    }
    else if ( d->m->type() == QVariant::Double )
    {
        return (value().toDouble() == 0);
    }
    else if ( d->m->type() == QVariant::String )
    {
        return value().toString().isEmpty();
    }
    else if ( hasM1Relation() )
    {
        // Si el campo contiene una relación a un padre (relación M1), y el padre no está establecido, en ese caso, no se incluye
        foreach (DBRelation *rel, d->m_relations)
        {
            if ( rel != NULL && rel->metadata()->type() == DBRelationMetadata::MANY_TO_ONE )
            {
                return !rel->fatherSetted();
            }
        }
    }
    else if ( hasBrotherRelation() )
    {
        // Si el campo contiene una relación a un padre (relación M1), y el padre no está establecido, en ese caso, no se incluye
        foreach (DBRelation *rel, d->m_relations)
        {
            if ( rel != NULL && rel->metadata()->type() == DBRelationMetadata::ONE_TO_ONE )
            {
                return !rel->brotherSetted();
            }
        }
    }
    return false;
}

/**
 * @brief DBField::nextSerial
 * Devuelve el siguiente valor de una sequencia. Esta función es útil para bases de datos que no soportan secuencias,
 * como SQLite
 * @return
 */
int DBField::nextSerial()
{
    return d->m->nextSerial(this);
}

/*!
  Hay algunas condiciones en las que un field no entra en una cláusula INSERT o UPDATE, como por ejemplo
  si el campo es un serial, o un campo pixmap que está vacío, o un campo calculado. Esta función es útil
  para los objetos DAO que deben analizar los fields de un bean, para decidir si se utilizand en una
  sentencia SQL de actualización
  */
bool DBField::insertFieldOnUpdateSql(BaseBean::DbBeanStates state)
{
    QSqlDatabase db = Database::getQDatabase();

    if ( overwrite() && metadata()->isOnDb() )
    {
        return true;
    }

    /**
     * SQLite no soporta secuencias. Por ello, si es un campo secuencia, debemos incluirlo en la sentencia INSERT o UPDATE
     * y se le dará valor. La función \a value contiene el código necesario para buscar el siguiente valor serial.
     */
    if ( metadata()->serial() && db.driverName() != "QSQLITE" )
    {
        return false;
    }
    if ( metadata()->calculated() )
    {
        if ( metadata()->calculatedSaveOnDb() )
        {
            if ( modified() ||
                 (state == BaseBean::INSERT && !metadata()->canBeNull()) ||
                 (d->m_hasBeenCalculated && rawValue() != oldValue()) ||
                 (!d->m_hasBeenCalculated && value() != oldValue()) )
            {
                return true;
            }
        }
        return false;
    }
    if ( metadata()->type() == QVariant::Pixmap )
    {
        QByteArray ba = value().toByteArray();
        if ( ba.isNull() || ba.isEmpty() )
        {
            return false;
        }
    }
    if ( metadata()->canBeNull() && value().isNull() && !modified() )
    {
        return false;
    }
    if ( state == BaseBean::UPDATE && !modified() )
    {
        return false;
    }

    return true;
}


void DBField::adjustOldValue()
{
    QMutexLocker lock(&d->m_mutex);

    d->m_oldValue = d->m_value;
}
