/***************************************************************************
 *   Copyright (C) 2007 by David Pinelo                                    *
 *   alepherp@alephsistemas.es                                             *
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
#include <QtXml>
#include <qxtnamespace.h>
#include "configuracion.h"
#include "qlogger.h"
#include "dao/basedao.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/basebean.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/relatedelement.h"
#include "dao/beans/basebeanqsfunction.h"
#include "dao/dbfieldobserver.h"
#include "dao/basebeanobserver.h"
#include "dao/relateddao.h"
#include "dao/aerptransactioncontext.h"
#include "dao/beans/basebeanvalidator.h"
#include "dao/database.h"
#include "dao/userdao.h"
#include "widgets/dbbasewidget.h"
#include "business/aerploggeduser.h"
#ifdef ALEPHERP_DOC_MANAGEMENT
#include "dao/beans/aerpdocmngmntdocument.h"
#endif

int BaseBean::m_beansCount;
int BaseBean::m_maxBeansCount;

class BaseBeanPrivate
{
//	Q_DECLARE_PUBLIC(BaseBean)
public:
    BaseBean *q_ptr;
    /** Identificador único del registro en base de datos. OID en PostgreSQL, por ejemplo */
    qlonglong m_dbOid;
    /** Listado de campos de este field y sus valores */
    QList<DBField *> m_fields;
    QList<DBRelation *> m_relations;
    /** Flag que indica el estado en que se encuentra en base de datos */
    BaseBean::DbBeanStates m_dbState;
    /** Flag que indica si en este bean han modificado sus datos (los fields) */
    bool m_modified;
    /** Copia de seguridad de los valores de los fields del bean. Utilizado por backupValues y restoreValues */
    QVariantMap m_backup;
    /** Puntero a los metadatos que definen a este bean */
    BaseBeanMetadata *m;
    /** Indica el tiempo de la ultima carga de datos de BBDD */
    QDateTime m_loadTime;
    /** Contexto actual de la transacción */
    QString m_actualContext;
    /** Si este bean representa a un registro de una vista (y por tanto, ligado a otro de una tabla) aquí se establece la relación */
    BaseBeanPointer m_viewLinkedBean;
    RelatedElementPointerList m_relatedElements;
    QHash<AlephERP::RelatedElementTypes, int> m_relatedElementsCount;
    bool m_relatedElementsLoaded;
    bool m_loadingRelatedElements;
    bool m_relatedElementsModified;
    /** Cuando se está recalculando un campo, éste puede emitir la señal modificado. Si esto se hace, por ejemplo en la función "recalculate" es
     * posible que se empiece a llamar a sí mismo de forma recursiva. Esta variable lo evitará */
    QStack<DBField *> m_recalculatingField;
    /** Evitamos recursividad en la emisión de señales */
    QStack<EmittedSignals> m_emittedSignals;
    bool m_isBlockingSignals;
    /** Permisos de acceso por usuario */
    AERPUserRowAccessList m_access;
    /** Mensajes que devuelven los scripts tras la validación */
    QString m_validateMessage;
    bool m_allSignalsBlocked;
    bool m_readOnly;
    bool m_restoringValues;
    QMutex m_mutex;
    bool m_calculateFieldsEnabled;

    BaseBeanPrivate(BaseBean *qq);

    QString extractFilterOperator(const QString &filter);
    void setDefaultValues(BaseBeanPointerList fathers = BaseBeanPointerList());
    void connectCounterFields();
    void connectAggregateFields();
    QList<DBObject *> iterateNavigation(DBField *fld, const QStringList &relativePath, const QStringList &filters);
    QList<DBObject *> iterateNavigation(DBRelation *fld, const QStringList &relativePath, const QStringList &filters);
    QList<DBObject *> iterateNavigation(BaseBean *bean, const QStringList &relativePath, const QStringList &filters);

    void emitBeanModified(bool value);
    void emitFieldModified(const QString &dbFieldName, const QVariant &value);
    void emitDefaultValueFieldCalculated(const QString &dbFieldName, const QVariant &value);
    void emitDbStateModified(int val);
    void emitEndEdit(BaseBean *bean = NULL);
    void emitRelatedElementAdded(RelatedElementPointer el);
    void emitRelatedElementDeleted(RelatedElementPointer el);
    void emitRelatedElementModified(RelatedElementPointer el);
    void copy(BaseBean *copyBean);
    void removeRelatedElement(RelatedElementPointer element);
    bool checkAccess(QChar access);
};

BaseBeanPrivate::BaseBeanPrivate(BaseBean *qq) :
    q_ptr(qq),
    m_mutex(QMutex::Recursive)
{
    m_modified = false;
    m_dbState = BaseBean::INSERT;
    m_relatedElementsLoaded = false;
    m_relatedElementsModified = false;
    m_loadingRelatedElements = false;
    m_dbOid = 0;
    m = NULL;
    m_isBlockingSignals = false;
    m_allSignalsBlocked = false;
    m_readOnly = false;
    m_restoringValues = false;
    m_calculateFieldsEnabled = true;
}

void BaseBeanPrivate::setDefaultValues(BaseBeanPointerList fatherBeans)
{
    QMutexLocker lock(&m_mutex);

    QStringList rootFieldsSetted;
    // Si invocamos a este objeto con "padres" de relaciones, los asignamos
    foreach (BaseBeanPointer father, fatherBeans)
    {
        foreach (DBRelation *rel, q_ptr->relations(AlephERP::ManyToOne))
        {
            if ( father && rel->metadata()->tableName() == father->metadata()->tableName() )
            {
                rel->setFather(father);
                rootFieldsSetted.append(rel->metadata()->rootFieldName());
            }
        }
    }
    foreach ( DBField *fld, m_fields )
    {
        if ( fld->metadata()->hasDefaultValue() && !rootFieldsSetted.contains(fld->metadata()->dbFieldName()) )
        {
            bool blockSignalsState = fld->blockSignals(true);
            fld->setValue(fld->defaultValue());
            fld->blockSignals(blockSignalsState);
        }
    }
}

void BaseBeanPrivate::emitBeanModified(bool value)
{
    EmittedSignals st;
    st.signal = "beanModified";
    st.bean = q_ptr;
    for ( int i = 0 ; i < m_emittedSignals.size() ; i++ )
    {
        if ( m_emittedSignals.at(i) == st )
        {
            QLogger::QLog_Info(AlephERP::stLogDB, QString("BaseBean::emitEndEdit: Anidamiento de señales"));
            return;
        }
    }
    m_emittedSignals.push(st);
    emit q_ptr->beanModified(value);
    emit q_ptr->beanModified(q_ptr, value);
    m_emittedSignals.pop();
}

void BaseBeanPrivate::emitFieldModified(const QString &dbFieldName, const QVariant &value)
{
    EmittedSignals st;
    st.signal = "fieldModified";
    st.bean = q_ptr;
    st.string = dbFieldName;
    st.variant = value;
    for ( int i = 0 ; i < m_emittedSignals.size() ; i++ )
    {
        if ( m_emittedSignals.at(i) == st )
        {
            QLogger::QLog_Info(AlephERP::stLogDB, QString("BaseBean::emitFieldModified: Anidamiento de señales"));
            return;
        }
    }
    m_emittedSignals.push(st);
    emit q_ptr->fieldModified(dbFieldName, value);
    emit q_ptr->fieldModified(q_ptr, dbFieldName, value);
    m_emittedSignals.pop();
}

void BaseBeanPrivate::emitDefaultValueFieldCalculated(const QString &dbFieldName, const QVariant &value)
{
    EmittedSignals st;
    st.signal = "defaultValueFieldCalculated";
    st.bean = q_ptr;
    st.string = dbFieldName;
    st.variant = value;
    for ( int i = 0 ; i < m_emittedSignals.size() ; i++ )
    {
        if ( m_emittedSignals.at(i) == st )
        {
            QLogger::QLog_Info(AlephERP::stLogDB, QString("BaseBean::emitDefaultValueFieldCalculated: Anidamiento de señales"));
            return;
        }
    }
    m_emittedSignals.push(st);
    emit q_ptr->defaultValueCalculated(dbFieldName, value);
    emit q_ptr->defaultValueCalculated(q_ptr, dbFieldName, value);
    m_emittedSignals.pop();
}

void BaseBeanPrivate::emitDbStateModified(int value)
{
    EmittedSignals st;
    st.signal = "dbStateModified";
    st.bean = q_ptr;
    st.iValue = value;
    for ( int i = 0 ; i < m_emittedSignals.size() ; i++ )
    {
        if ( m_emittedSignals.at(i) == st )
        {
            QLogger::QLog_Info(AlephERP::stLogDB, QString("BaseBean::emitDbStateModified: Anidamiento de señales"));
            return;
        }
    }
    m_emittedSignals.push(st);
    emit q_ptr->dbStateModified(value);
    emit q_ptr->dbStateModified(q_ptr, value);
    m_emittedSignals.pop();
}


void BaseBeanPrivate::emitRelatedElementAdded(RelatedElementPointer el)
{
    EmittedSignals st;
    st.signal = "relatedElementAdded";
    st.element = el;
    for ( int i = 0 ; i < m_emittedSignals.size() ; i++ )
    {
        if ( m_emittedSignals.at(i) == st )
        {
            QLogger::QLog_Info(AlephERP::stLogDB, QString("BaseBean::emitRelatedElementAdded: Anidamiento de señales"));
            return;
        }
    }
    m_emittedSignals.push(st);
    emit q_ptr->relatedElementAdded(el);
    m_emittedSignals.pop();
}

void BaseBeanPrivate::emitRelatedElementDeleted(RelatedElementPointer el)
{
    EmittedSignals st;
    st.signal = "relatedElementDeleted";
    st.element = el;
    for ( int i = 0 ; i < m_emittedSignals.size() ; i++ )
    {
        if ( m_emittedSignals.at(i) == st )
        {
            QLogger::QLog_Info(AlephERP::stLogDB, QString("BaseBean::emitRelatedElementDeleted: Anidamiento de señales"));
            return;
        }
    }
    m_emittedSignals.push(st);
    emit q_ptr->relatedElementDeleted(el);
    m_emittedSignals.pop();
}

void BaseBeanPrivate::emitRelatedElementModified(RelatedElementPointer el)
{
    EmittedSignals st;
    st.signal = "relatedElementModified";
    st.element = el;
    for ( int i = 0 ; i < m_emittedSignals.size() ; i++ )
    {
        if ( m_emittedSignals.at(i) == st )
        {
            QLogger::QLog_Info(AlephERP::stLogDB, QString("BaseBean::emitRelatedElementModified: Anidamiento de señales"));
            return;
        }
    }
    m_emittedSignals.push(st);
    emit q_ptr->relatedElementModified(el);
    m_emittedSignals.pop();
}

void BaseBean::emitBeforeUpdate()
{
    EmittedSignals st;
    st.signal = "beforeUpdate";
    st.bean = this;
    for ( int i = 0 ; i < d->m_emittedSignals.size() ; i++ )
    {
        if ( d->m_emittedSignals.at(i) == st )
        {
            QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBean::emitBeforeUpdate: Anidamiento de señales"));
            return;
        }
    }
    d->m_emittedSignals.push(st);
    emit beforeUpdate();
    emit beforeUpdate(this);
    d->m_emittedSignals.pop();
}

void BaseBean::emitBeforeSave()
{
    EmittedSignals st;
    st.signal = "beforeSave";
    st.bean = this;
    for ( int i = 0 ; i < d->m_emittedSignals.size() ; i++ )
    {
        if ( d->m_emittedSignals.at(i) == st )
        {
            QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBean::emitBeforeSave: Anidamiento de señales"));
            return;
        }
    }
    d->m_emittedSignals.push(st);
    emit beforeSave();
    emit beforeSave(this);
    d->m_emittedSignals.pop();
}

void BaseBean::emitBeforeDelete()
{
    EmittedSignals st;
    st.signal = "beforeInsert";
    st.bean = this;
    for ( int i = 0 ; i < d->m_emittedSignals.size() ; i++ )
    {
        if ( d->m_emittedSignals.at(i) == st )
        {
            QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBean::emitBeforeDelete: Anidamiento de señales"));
            return;
        }
    }
    d->m_emittedSignals.push(st);
    emit beforeDelete();
    emit beforeDelete(this);
    d->m_emittedSignals.pop();
}

void BaseBean::emitBeforeInsert()
{
    EmittedSignals st;
    st.signal = "beforeInsert";
    st.bean = this;
    for ( int i = 0 ; i < d->m_emittedSignals.size() ; i++ )
    {
        if ( d->m_emittedSignals.at(i) == st )
        {
            QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBean::emitBeforeInsert: Anidamiento de señales"));
            return;
        }
    }
    d->m_emittedSignals.push(st);
    emit beforeInsert();
    emit beforeInsert(this);
    d->m_emittedSignals.pop();
}

void BaseBean::emitBeanSaved()
{
    EmittedSignals st;
    st.signal = "beanSaved";
    st.bean = this;
    for ( int i = 0 ; i < d->m_emittedSignals.size() ; i++ )
    {
        if ( d->m_emittedSignals.at(i) == st )
        {
            QLogger::QLog_Info(AlephERP::stLogDB, QString("BaseBean::emitBeanSaved: Anidamiento de señales"));
            return;
        }
    }
    d->m_emittedSignals.push(st);
    emit beanSaved();
    emit beanSaved(this);
    d->m_emittedSignals.pop();
}

void BaseBean::emitBeanCommitted()
{
    EmittedSignals st;
    st.signal = "beanCommitted";
    st.bean = this;
    for ( int i = 0 ; i < d->m_emittedSignals.size() ; i++ )
    {
        if ( d->m_emittedSignals.at(i) == st )
        {
            QLogger::QLog_Info(AlephERP::stLogDB, QString("BaseBean::emitBeanCommitted: Anidamiento de señales"));
            return;
        }
    }
    d->m_emittedSignals.push(st);
    emit beanCommitted();
    emit beanCommitted(this);
    d->m_emittedSignals.pop();
}

void BaseBean::emitEndEdit()
{
    EmittedSignals st;
    st.signal = "endEdit";
    st.bean = this;
    for ( int i = 0 ; i < d->m_emittedSignals.size() ; i++ )
    {
        if ( d->m_emittedSignals.at(i) == st )
        {
            QLogger::QLog_Info(AlephERP::stLogDB, QString("BaseBean::emitEndEdit: Anidamiento de señales"));
            return;
        }
    }
    d->m_emittedSignals.push(st);
    emit endEdit();
    emit endEdit(this);
    d->m_emittedSignals.pop();
}

BaseBean::BaseBean(QObject *parent) : DBObject(parent), d(new BaseBeanPrivate(this))
{
    if ( ObserverFactory::instance() )
    {
        m_observer = ObserverFactory::instance()->newObserver(this);
    }
    BaseBean::m_beansCount++;
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("BaseBean::BaseBean: Nuevo bean creado. Beans que existen: [%1]").arg(BaseBean::m_beansCount));
    if ( BaseBean::m_beansCount > BaseBean::m_maxBeansCount )
    {
        BaseBean::m_maxBeansCount = BaseBean::m_beansCount;
    }
    connect(this, SIGNAL(relatedElementAdded(RelatedElement*)), this, SLOT(setRelatedElementsModified()));
    connect(this, SIGNAL(relatedElementDeleted(RelatedElement*)), this, SLOT(setRelatedElementsModified()));
    connect(this, SIGNAL(relatedElementModified(RelatedElement*)), this, SLOT(setRelatedElementsModified()));
    connect(this, SIGNAL(beanCommitted()), this, SLOT(adjustOldValues()));
}

BaseBean::~BaseBean()
{
    BaseBean::m_beansCount--;
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("BaseBean::~BaseBean: Bean destruido. Beans que siguen existiendo: [%1]").arg(BaseBean::m_beansCount));
    delete d;
}

/**
 * @brief BaseBean::init
 * @param m
 * @param hastToSetDefaultValue
 * @param fatherBeans
 * Inicializa la estructura básica del bean o registro de base de datos. Puede establecer los valores por defecto
 * y también asignar los padres de las relaciones M1
 */
void BaseBean::init(BaseBeanMetadata *m, bool hastToSetDefaultValue, BaseBeanPointerList fatherBeans)
{
    QMutexLocker lock(&d->m_mutex);
    d->m = m;
    QList<DBFieldMetadata *> fieldsMetadata = d->m->fields();
    QList<DBRelationMetadata *>relationsMetadata = d->m->relations();
    foreach (DBFieldMetadata *metadata, fieldsMetadata)
    {
        newField(metadata);
    }
    foreach (DBRelationMetadata *metadata, relationsMetadata)
    {
        DBRelation *rel = newRelation(metadata);
        DBField *fld = field(rel->metadata()->rootFieldName());
        if ( fld != NULL )
        {
            fld->addRelation(rel);
        }
    }
    // Funciones asociadas definidas en QS. Importante definirlas aquí para que estén presentes en los
    // scripts de los valores por defecto.
    foreach (const QString &functionName, m->associatedScriptFunctions())
    {
        BaseBeanQsFunction *function = new BaseBeanQsFunction(functionName, this);
        QByteArray ba = functionName.toUtf8();
        QVariant pointer = QVariant::fromValue(function);
        setProperty(ba.constData(), pointer);
    }

    makeCalculatedFieldsConnections();
    if ( hastToSetDefaultValue )
    {
        d->setDefaultValues(fatherBeans);
    }
    uncheckModifiedFields();
    uncheckModifiedRelatedElements();
    d->connectCounterFields();
    d->connectAggregateFields();

    d->m->createInternalConnections(const_cast<BaseBean*>(this));
    if ( hastToSetDefaultValue )
    {
        metadata()->onCreateScriptExecute(this);
    }
    d->m_readOnly = m->readOnly();
    if ( observer() )
    {
        observer()->setEntity(this);
    }
}

BaseBeanMetadata * BaseBean::metadata() const
{
    return d->m;
}

qlonglong BaseBean::dbOid() const
{
    return d->m_dbOid;
}

void BaseBean::setDbOid(qlonglong value)
{
    QMutexLocker lock(&d->m_mutex);
    d->m_dbOid = value;
}

QString BaseBean::actualContext() const
{
    return d->m_actualContext;
}

void BaseBean::setActualContext(const QString &value)
{
    QMutexLocker lock(&d->m_mutex);
    d->m_actualContext = value;
}

void BaseBean::setRelatedElementsModified()
{
    QMutexLocker lock(&d->m_mutex);
    d->m_relatedElementsModified = true;
    setModified();
}

/*!
  Método factoría para crear DBFields.
  Agrega el field al listado de fields, y conecta las señales de modificación
  */
DBField *BaseBean::newField(DBFieldMetadata *m)
{
    QMutexLocker lock(&d->m_mutex);
    // Poniendo como padre a este objeto, garantizamos que cuando
    // este bean se borre, se borrarán los campos
    DBField *field = new DBField(this);
    field->setParent(this);
    field->setMetadata(m);
    d->m_fields << field;
    connect (field, SIGNAL(valueModified(QVariant)), this, SLOT(setModified()));
    connect (field, SIGNAL(valueModified(QString,QVariant)), this, SLOT(emitFieldModified(QString, QVariant)));
    connect (field, SIGNAL(defaultValueCalculated(QString,QVariant)), this, SLOT(emitDefaultValueFieldCalculated(QString,QVariant)));
    /** Vamos a publicar propiedades dinámicas, que serán los propios DBField. De ese modo, desde el codigo
      QS se podrá acceder a los métodos así: bean.dbFieldName_del_campo.Value; */
    QVariant pointer = QVariant::fromValue(field);
    QByteArray ba = field->metadata()->dbFieldName().toUtf8();
    setProperty(ba.constData(), pointer);
    return field;
}

void BaseBean::makeCalculatedFieldsConnections(const QString &fieldToCalc, const QStringList &fieldsOnCalcArg)
{
    foreach (DBField *field, d->m_fields)
    {
        // Vamos a conectarnos a los objetos que deben recalculares
        if ( field->metadata()->calculated() && (fieldToCalc.isEmpty() || field->metadata()->dbFieldName() == fieldToCalc) )
        {
            QStringList fieldsOnCalc;
            if ( fieldsOnCalcArg.isEmpty() )
            {
                fieldsOnCalc = d->m->fieldsNecessaryToCalculate(field->metadata()->dbFieldName());
            }
            else
            {
                fieldsOnCalc = fieldsOnCalcArg;
            }
            foreach (const QString &path, fieldsOnCalc)
            {
                DBObject *obj = navigateThroughProperties(path);
                if ( obj != NULL )
                {
                    DBField *fieldOnCalc = qobject_cast<DBField *>(obj);
                    if ( fieldOnCalc )
                    {
                        connect(fieldOnCalc, SIGNAL(valueModified(QVariant)), field, SLOT(recalculate()));
                    }
                    DBRelation *rel = qobject_cast<DBRelation *> (obj);
                    if ( rel )
                    {
                        connect (rel, SIGNAL(childDbStateModified(BaseBean*,int)), field, SLOT(recalculate()));
                        connect (rel, SIGNAL(childModified(BaseBean*,bool)), field, SLOT(recalculate()));
                        connect (rel, SIGNAL(fatherLoaded(BaseBean*)), field, SLOT(recalculate()));
                        connect (rel, SIGNAL(fatherUnloaded()), field, SLOT(recalculate()));
                        connect (field, SIGNAL(valueModified(QVariant)), field, SLOT(recalculate()));
                    }
                }
            }
            // Nos conectamos también a las relaciones agregadas
            if ( field->metadata()->aggregate() )
            {
                foreach (const QString &rel, field->metadata()->aggregateCalc().relation)
                {
                    DBRelation *r = relation(rel);
                    if ( r != NULL )
                    {
                        connect (r, SIGNAL(childDbStateModified(BaseBean*,int)), field, SLOT(recalculate()));
                        connect (r, SIGNAL(childModified(BaseBean*,bool)), field, SLOT(recalculate()));
                    }
                }
            }
        }
    }
}

void BaseBean::emitFieldModified(const QString &dbFieldName, const QVariant &value)
{
    d->emitFieldModified(dbFieldName, value);
}

void BaseBean::emitDefaultValueFieldCalculated(const QString &dbFieldName, const QVariant &value)
{
    d->emitDefaultValueFieldCalculated(dbFieldName, value);
}

void BaseBean::emitRelatedElementModified(RelatedElement *element)
{
    d->emitRelatedElementModified(element);
    d->m->modifiedRelatedElementScriptExecute(this, element);
}

/*!
  Método factoría para generar nuevas relaciones en el bean. Crea el objeto y lo agrega
  al listado de DBRelation
  */
DBRelation *BaseBean::newRelation(DBRelationMetadata *m)
{
    QMutexLocker lock(&d->m_mutex);
    // Poniendo como padre a este objeto, garantizamos que cuando
    // este bean se borre, se borrarán los campos
    DBRelation *rel = new DBRelation(this);
    rel->setParent(this);
    rel->setOwner(this);
    rel->setMetadata(m);
    d->m_relations << rel;
    connect (rel, SIGNAL(childInserted(BaseBean*,int)), this, SLOT(setModified()));
    connect (rel, SIGNAL(childModified(BaseBean *, bool)), this, SLOT(setModified(BaseBean *, bool)));
    connect (rel, SIGNAL(childDeleted(BaseBean *,int)), this, SLOT(setModified()));
    connect (rel, SIGNAL(childDbStateModified(BaseBean *,int)), this, SLOT(setModified()));
    /** Vamos a publicar propiedades dinámicas, que serán los propios DBRelation. De ese modo, desde el codigo
      QS se podrá acceder a los métodos así: bean.relationName.childs; */
    QVariant pointer = QVariant::fromValue(rel);
    QByteArray ba = rel->metadata()->name().toUtf8();
    setProperty(ba.constData(), pointer);
    return rel;
}

QList<DBField *> BaseBean::fields() const
{
    return d->m_fields;
}

QVariantMap BaseBean::fieldsMap() const
{
    QVariantMap map;
    foreach (DBField *fld, d->m_fields)
    {
        map[fld->metadata()->dbFieldName()] = QVariant::fromValue(fld);
    }
    return map;
}

/*!
  Conjunto de todas las relaciones del basebean. Pueden ser relaciones con hijos o con padres
  */
QList<DBRelation *> BaseBean::relations(AlephERP::RelationTypes type) const
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

QVariantMap BaseBean::relationsMap() const
{
    QVariantMap map;
    foreach (DBRelation *rel, d->m_relations)
    {
        map[rel->metadata()->name()] = QVariant::fromValue(rel);
    }
    return map;
}

/*!
  Devuelve el orden de esta relación entre todas las del bean. Muy utilizado para construir alias
  */
int BaseBean::relationIndex(const QString &relationName)
{
    QList<DBRelationMetadata *> relations = d->m->relations();
    for ( int i = 0 ; i < relations.size() ; i++ )
    {
        if ( relationName == relations.at(i)->name() )
        {
            return i;
        }
    }
    return -1;
}

/*!
  Devuelve los hijos de la relación dada. Busca el objeto relation, y obtiene directamente los hijos
  */
BaseBeanPointerList BaseBean::relationChildren(const QString &relationName, const QString &order, bool includeToBeDeleted)
{
    DBRelation *rel = relation(relationName);
    if ( rel != NULL )
    {
        return rel->children(order, includeToBeDeleted);
    }
    return BaseBeanPointerList();
}

/*!
  Devuelve el número de beans hijos de la relación
  */
int BaseBean::relationChildrenCount(const QString &relationName, bool includeToBeDeleted)
{
    DBRelation *rel = relation(relationName);
    if ( rel != NULL )
    {
        return rel->childrenCount(includeToBeDeleted);
    }
    return 0;
}

/*!
  Para la relación relationName, devuelve el bean cuyo field fieldName es id
  */
BaseBeanPointer BaseBean::relationChildByField(const QString &relationName, const QString &fieldName, const QVariant &id, bool includeToBeDeleted)
{
    BaseBeanPointer bean;
    DBRelation *rel = relation(relationName);
    if ( rel != NULL )
    {
        bean = rel->childByField(fieldName, id, includeToBeDeleted);
    }
    return bean;
}

/*!
  Devuelve los hijos de la relación \a relationName según un filtro elaborado en \a filter
  y ordenados según \a order
  */
BaseBeanPointerList BaseBean::relationChildrenByFilter(const QString &relationName, const QString &filter, const QString &order, bool includeToBeDeleted)
{
    BaseBeanPointerList beans;
    DBRelation *rel = relation(relationName);
    if ( rel != NULL )
    {
        beans = rel->childrenByFilter(filter, order, includeToBeDeleted);
    }
    return beans;
}

BaseBeanPointer BaseBean::relationChildByOid(const QString &relationName, qlonglong oid, bool includeToBeDeleted)
{
    BaseBeanPointer bean;
    DBRelation *rel = relation(relationName);
    if ( rel != NULL )
    {
        bean = rel->childByOid(oid, includeToBeDeleted);
    }
    return bean;
}

void BaseBean::deleteAllChildren(const QString &relationName)
{
    DBRelation *rel = relation(relationName);
    if ( rel != NULL )
    {
        rel->deleteAllChildren();
    }
}

BaseBeanSharedPointer BaseBean::newRelationChild(const QString &relationName)
{
    DBRelation *rel = relation(relationName);
    if ( rel != NULL )
    {
        return rel->newChild();
    }
    return BaseBeanSharedPointer ();
}

void BaseBean::setFieldValue(int index, const QVariant &value)
{
    if ( index >= 0 && index < d->m_fields.size() )
    {
        d->m_fields.at(index)->setValue(value);
    }
}

void BaseBean::setFieldValueFromSqlRawData(const QString &dbFieldName, const QString &data)
{
    foreach ( DBField *field, d->m_fields )
    {
        if ( field->metadata()->dbFieldName() == dbFieldName )
        {
            field->setValueFromSqlRawData(data);
        }
    }
}

void BaseBean::setOverwriteFieldValue(const QString &dbFieldName, const QVariant &value)
{
    foreach ( DBField *field, d->m_fields )
    {
        if ( field->metadata()->dbFieldName() == dbFieldName )
        {
            field->setOverwriteValue(value);
        }
    }
}

void BaseBean::setFieldValue(const QString &dbFieldName, const QVariant &value)
{
    foreach ( DBField *field, d->m_fields )
    {
        if ( field->metadata()->dbFieldName() == dbFieldName )
        {
            field->setValue(value);
        }
    }
}

void BaseBean::setInternalFieldValue(int index, const QVariant &value, bool overwriteOnReadOnly, bool updateChildren)
{
    if ( index >= 0 && index < d->m_fields.size() )
    {
        d->m_fields.at(index)->setInternalValue(value, overwriteOnReadOnly, updateChildren);
    }
}

void BaseBean::setOldValue(const QString &dbFieldName, const QVariant &oldValue)
{
    foreach ( DBField *field, d->m_fields )
    {
        if ( field->metadata()->dbFieldName() == dbFieldName )
        {
            field->setOldValue(oldValue);
        }
    }
}

void BaseBean::setOldValue(int index, const QVariant &oldValue)
{
    if ( index >= 0 && index < d->m_fields.size() )
    {
        d->m_fields.at(index)->setOldValue(oldValue);
    }
}

void BaseBean::setInternalFieldValue(const QString &dbFieldName, const QVariant &value, bool overwriteOnReadOnly, bool updateChildren)
{
    foreach ( DBField *field, d->m_fields )
    {
        if ( field->metadata()->dbFieldName() == dbFieldName )
        {
            field->setInternalValue(value, overwriteOnReadOnly, updateChildren);
            return;
        }
    }
}

DBField *BaseBean::field(const QString &dbFieldName)
{
    DBField *field = NULL;
    // Veamos si el campo se refiere a un objeto de un padre o de una relación
    if ( dbFieldName.contains(QStringLiteral(".")) )
    {
        DBObject *obj = navigateThroughProperties(dbFieldName);
        field = qobject_cast<DBField *>(obj);
    }
    else
    {
        for ( int i = 0 ; i < d->m_fields.size() ; i++ )
        {
            if ( d->m_fields.at(i)->metadata()->dbFieldName() == dbFieldName )
            {
                field = d->m_fields.at(i);
                return field;
            }
        }
    }
    if ( field == NULL && !dbFieldName.isEmpty() && dbFieldName != "editable" )
    {
        QLogger::QLog_Info(AlephERP::stLogOther, QString("BaseBean::field: [%1]. No existe el campo: [%2]").arg(d->m->tableName()).arg(dbFieldName));
    }
    return field;
}

DBField * BaseBean::field(int index)
{
    if ( index < 0 || index >= d->m_fields.size() )
    {
        return NULL;
    }
    return d->m_fields.at(index);
}

/**
  Devuelve todos aquellos campos que componen la primaryKey
  */
QList<DBField *> BaseBean::pkFields()
{
    QList<DBField *> fields;
    for ( int i = 0 ; i < d->m_fields.size() ; i++ )
    {
        if ( d->m_fields.at(i)->metadata()->primaryKey() )
        {
            fields.append(d->m_fields.at(i));
        }
    }
    if ( fields.isEmpty() )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBean::pkFields: [%1]. No existe primary key.").arg(d->m->tableName()));
    }
    return fields;
}

int BaseBean::fieldIndex(const QString &dbFieldName)
{
    int index = -1;
    for ( int i = 0 ; i < d->m_fields.size() ; i++ )
    {
        if ( d->m_fields.at(i)->metadata()->dbFieldName() == dbFieldName )
        {
            index = i;
        }
    }
    if ( index == -1 && !dbFieldName.isEmpty() && dbFieldName != "editable" )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBean::fieldIndex: [%1]. No existe el campo: [%2]").arg(d->m->tableName()).arg(dbFieldName));
    }
    return index;
}

int BaseBean::fieldCount() const
{
    return d->m_fields.size();
}

bool BaseBean::modifiedRelatedElements() const
{
    return d->m_relatedElementsModified;
}

QColor BaseBean::rowColor()
{
    return d->m->rowColorScriptExecute(this);
}

QString BaseBean::toolTip()
{
    return d->m->toolTipExecute(this);
}

DBRelation * BaseBean::relation(const QString &relationName)
{
    DBRelation *rel = NULL;
    for ( int i = 0 ; i < d->m_relations.size() ; i++ )
    {
        if ( d->m_relations.at(i)->metadata()->name() == relationName )
        {
            rel = d->m_relations.at(i);
        }
    }
    if ( rel == NULL && !relationName.isEmpty() )
    {
        QLogger::QLog_Info(AlephERP::stLogOther, QString("BaseBean::relation: [%1]. No existe la relación: [%2]").arg(d->m->tableName()).arg(relationName));
    }
    return rel;
}

/*!
Obtiene, de una relación de tipo M1, el bean padre. Si no existe, devuelve NULL.
*/
BaseBeanPointer BaseBean::father(const QString &relationName)
{
    BaseBean *bean = NULL;
    DBRelation *rel = relation(relationName);
    if ( rel == NULL && !relationName.isEmpty() )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBean::father: [%1]. No existe la relación: [%2]").arg(d->m->tableName()).arg(relationName));
    }
    else
    {
        if ( rel->metadata()->type() == DBRelationMetadata::MANY_TO_ONE )
        {
            bean = rel->father();
        }
    }
    return bean;
}

QVariant BaseBean::fatherFieldValue(const QString &relationName, const QString &field)
{
    QVariant v;
    BaseBean *f = father(relationName);
    if ( f != NULL )
    {
        v = f->fieldValue(field);
    }
    return v;
}

QString BaseBean::fatherDisplayFieldValue(const QString &relationName, const QString &field)
{
    QString v;
    BaseBean *f = father(relationName);
    if ( f != NULL )
    {
        v = f->displayFieldValue(field);
    }
    return v;
}

/*!
  Devuelve el valor del campo dbFieldName
  */
QVariant BaseBean::fieldValue(int dbField)
{
    if ( dbField > -1 && dbField < d->m_fields.size() )
    {
        return d->m_fields.at(dbField)->value();
    }
    QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBean::fieldValue: [%1]. No existe el campo con índice: [%2]").arg(d->m->tableName()).arg(dbField));
    return QVariant();
}

/*!
  Devuelve el valor del campo dbFieldName
  */
QVariant BaseBean::fieldValue(const QString &dbFieldName)
{
    QVariant value;

    if ( dbFieldName.isEmpty() )
    {
        return value;
    }
    DBField *fld = field(dbFieldName);
    if ( fld == NULL && !dbFieldName.isEmpty() && dbFieldName != "editable" )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBean::fieldValue: [%1]. No existe el campo: [%2]").arg(d->m->tableName()).arg(dbFieldName));
    }
    else
    {
        value = fld->value();
    }
    return value;
}

bool  BaseBean::isFieldEmpty(const QString &dbFieldName)
{
    if ( dbFieldName.isEmpty() )
    {
        return true;
    }
    DBField *fld = field(dbFieldName);
    if ( fld == NULL && !dbFieldName.isEmpty() && dbFieldName != "editable" )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBean::isFieldEmpty: [%1]. No existe el campo: [%2]").arg(d->m->tableName()).arg(dbFieldName));
        return true;
    }
    return fld->isEmpty();
}

QPixmap BaseBean::pixmapFieldValue(int dbField)
{
    QPixmap pixmap;
    DBField *fld = field(dbField);
    if ( fld->metadata()->type() == QVariant::Pixmap )
    {
        pixmap = fld->pixmapValue();
    }
    return pixmap;
}

QPixmap BaseBean::pixmapFieldValue(const QString &dbFieldName)
{
    QPixmap pixmap;
    DBField *fld = field(dbFieldName);
    if ( fld != NULL && fld->metadata()->type() == QVariant::Pixmap )
    {
        pixmap = fld->pixmapValue();
    }
    return pixmap;
}

/*!
  Devuelve el valor formateado del campo dbFieldName
  */
QString BaseBean::displayFieldValue(int iField)
{
    if ( iField > -1 && iField < d->m_fields.size() )
    {
        return d->m_fields.at(iField)->displayValue();
    }
    QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBean::displayFieldValue: [%1]. No existe el campo: [%2]").arg(d->m->tableName()).arg(iField));
    return QString();
}

/*!
  Devuelve el valor formateado del campo dbFieldName
  */
QString BaseBean::displayFieldValue(const QString & dbFieldName)
{
    QString value;

    if ( dbFieldName.isEmpty() )
    {
        return value;
    }
    DBField *fld = field(dbFieldName);
    if ( fld == NULL && !dbFieldName.isEmpty() && dbFieldName != "editable" )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBean::displayFieldValue: [%1]. No existe el campo: [%2]").arg(d->m->tableName()).arg(dbFieldName));
    }
    else
    {
        value = fld->displayValue();
    }
    return value;
}

/*!
  Devuelve el valor formateado del campo dbFieldName para ser insertado en base de datos
  */
QString BaseBean::sqlFieldValue(int iField)
{
    if ( iField > -1 && iField < d->m_fields.size() )
    {
        return d->m_fields.at(iField)->sqlValue(true);
    }
    QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBean::sqlFieldValue: [%1]. No existe el campo: [%2]").arg(d->m->tableName()).arg(iField));
    return QString();
}

/*!
  Devuelve el valor formateado del campo dbFieldName para ser insertado en base de datos
  */
QString BaseBean::sqlFieldValue(const QString & dbFieldName)
{
    QString value;

    if ( dbFieldName.isEmpty() )
    {
        return value;
    }
    DBField *fld = field(dbFieldName);
    if ( fld == NULL && !dbFieldName.isEmpty() && dbFieldName != "editable" )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBean::sqlFieldValue: [%1]. No existe el campo: [%2]").arg(d->m->tableName()).arg(dbFieldName));
    }
    else
    {
        value = fld->sqlValue(true);
    }
    return value;
}

/**
  Ejecuta el script para obtener el tooltip de un field, y lo devuelve
  */
QString BaseBean::toolTipField(const QString &dbFieldName)
{
    QString value;

    if ( dbFieldName.isEmpty() )
    {
        return value;
    }
    DBField *fld = field(dbFieldName);
    if ( fld == NULL && !dbFieldName.isEmpty() && dbFieldName != "editable" )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBean::toolTipField: [%1]. No existe el campo: [%2]").arg(d->m->tableName()).arg(dbFieldName));
    }
    else
    {
        value = fld->toolTip();
    }
    return value;

}

QString BaseBean::toolTipField(int iField)
{
    if ( iField > -1 && iField < d->m_fields.size() )
    {
        return d->m_fields.at(iField)->toolTip();
    }
    QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBean::toolTipField: [%1]. No existe el campo: [%2]").arg(d->m->tableName()).arg(iField));
    return QString();
}

QVariant BaseBean::defaultFieldValue(const QString &dbFieldName)
{
    QVariant value;

    if ( dbFieldName.isEmpty() )
    {
        return value;
    }
    DBField *fld = field(dbFieldName);
    if ( fld == NULL && !dbFieldName.isEmpty() && dbFieldName != "editable" )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBean::defaultFieldValue: [%1]. No existe el campo: [%2]").arg(d->m->tableName()).arg(dbFieldName));
    }
    else
    {
        value = fld->metadata()->calculateDefaultValue(fld);
    }
    return value;
}


/*!
 Proporciona una claúsula where
*/
QString BaseBean::sqlWhere(const QString &fieldName, const QString &op)
{
    QString sql;
    DBField *fld = field(fieldName);
    if ( fld == NULL && !fieldName.isEmpty() && fieldName != "editable" )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBean::sqlWhere: [%1]. No existe el campo: [%2]").arg(d->m->tableName()).arg(fieldName));
    }
    else
    {
        sql = fld->sqlWhere(op);
    }
    return sql;
}


/*!
 Proporciona una claúsula where para la primary key de este bean.
*/
QString BaseBean::sqlWherePk()
{
    QString where;

    QList<DBField *> pk = pkFields();
    if ( pk.isEmpty() )
    {
        return where;
    }
    foreach ( DBField *fld, pk )
    {
        if ( where.isEmpty() )
        {
            where = fld->sqlWhere("=");
        }
        else
        {
            where = where + " AND " + fld->sqlWhere("=");
        }
    }
    return where;
}

QVariant BaseBean::pkValue()
{
    QVariantMap pkValues;
    QList<DBField *> pk = pkFields();
    if ( pk.isEmpty() )
    {
        return QVariant();
    }
    foreach ( DBField *fld, pk )
    {
        pkValues[fld->metadata()->dbFieldName()] = fld->value();
    }
    return pkValues;
}

QVariantList BaseBean::pkListValue()
{
    QVariantList list;
    QList<DBField *> pk = pkFields();
    if ( pk.isEmpty() )
    {
        return list;
    }
    foreach ( DBField *fld, pk )
    {
        list.append(fld->value());
    }
    return list;
}

/*!
  Devuelve el valor serializado de la primary key
  */
QString BaseBean::pkSerializedValue()
{
    return BaseDAO::serializePk(pkValue());
}

/*!
  Realiza una comparación entre la primary key de este bean, y
  la pasada en value
  */
bool BaseBean::pkEqual(const QVariant &value)
{
    QVariantMap pkThis, pkOther;
    pkThis = pkValue().toMap();
    pkOther = value.toMap();
    QMapIterator<QString, QVariant> it (pkThis);
    while ( it.hasNext() )
    {
        it.next();
        if ( !pkOther.contains(it.key()) || pkOther.value(it.key()) != it.value() )
        {
            return false;
        }
    }
    return true;
}

void BaseBean::setPkValue(const QVariant &id)
{
    QVariantMap pkValues = id.toMap();
    QMapIterator<QString, QVariant> i(pkValues);
    while ( i.hasNext() )
    {
        i.next();
        DBField *fld = field(i.key());
        fld->setValue(i.value());
    }
}

void BaseBean::setPkValueFromInternal(const QVariant &id)
{
    if ( id.canConvert<QVariantMap>() ) {
        QVariantMap pkValues = id.toMap();
        QMapIterator<QString, QVariant> i(pkValues);
        while ( i.hasNext() )
        {
            i.next();
            DBField *fld = field(i.key());
            fld->setInternalValue(i.value());
        }
    } else {
        QList<DBField *> pks = pkFields();
        if ( pks.size() > 0 ) {
            pks[0]->setInternalValue(id);
        }
    }
}

void BaseBean::setDbState(BaseBean::DbBeanStates value)
{
    QMutexLocker lock(&d->m_mutex);
    bool previousStateWasInsert = d->m_dbState == BaseBean::INSERT;
    d->m_dbState = value;
    /** Por definición, si se marca un bean para ser borrado, pasa a estar modificado */
    if ( value == BaseBean::TO_BE_DELETED )
    {
        d->m_modified = true;
        if ( previousStateWasInsert && !d->m_actualContext.isEmpty() )
        {
            // OJO: Aquí no descartamos al padre. Por eso ponemos false.
            AERPTransactionContext::instance()->discardFromContext(this, false);
        }
        // Marcamos también para borrar todos los hijos de las relaciones que hubiese
        foreach (DBRelation *rel, d->m_relations)
        {
            if ( rel->metadata()->deleteCascade() && (rel->metadata()->type() == DBRelationMetadata::ONE_TO_MANY || rel->metadata()->type() == DBRelationMetadata::ONE_TO_ONE) )
            {
                foreach (BaseBeanPointer b, rel->internalChildren())
                {
                    b->setDbState(BaseBean::TO_BE_DELETED);
                }
            }
        }

        // Se marcan también para borrar los elementos relacionados de ese bean. Esto invocará a los scripts
        // de borrado
        foreach (RelatedElement *elem, d->m_relatedElements)
        {
            if ( elem->state() != RelatedElement::TO_BE_DELETED && elem->state() != RelatedElement::DELETED )
            {
                deleteRelatedElement(elem);
            }
        }
    }
    d->emitDbStateModified(value);
    d->emitBeanModified(true);
}

BaseBean::DbBeanStates BaseBean::dbState() const
{
    // Si el registro es una vista, jamás podrá estar modificado
    if ( d->m->dbObjectType() == AlephERP::View )
    {
        return BaseBean::UPDATE;
    }
    return d->m_dbState;
}

QString BaseBean::dbStateDisplayName() const
{
    if ( d->m_dbState == BaseBean::INSERT )
    {
        return tr("Insert");
    }
    else if ( d->m_dbState == BaseBean::UPDATE )
    {
        return tr("Update");
    }
    else if ( d->m_dbState == BaseBean::TO_BE_DELETED )
    {
        return tr("To be deleted");
    }
    else if ( d->m_dbState == BaseBean::DELETED )
    {
        return tr("Deleted");
    }
    return QString();
}

/*!
  Chivato de modificación: Indica si el bean se ha modificado desde que se creó
  o desde que se leyó de base de datos
  */
bool BaseBean::modified () const
{
    if ( dbState() != BaseBean::INSERT && d->m_readOnly )
    {
        return false;
    }
    // Si el registro es una vista, jamás podrá estar modificado
    if ( d->m->dbObjectType() == AlephERP::View )
    {
        return false;
    }
    return d->m_modified;
}

/*!
  Recorre todos los fields, y recalcula todos los los fields hijos
  */
void BaseBean::setModified()
{
    QMutexLocker lock(&d->m_mutex);
    d->m_modified = true;
    d->emitBeanModified(d->m_modified);
}

void BaseBean::setModified(BaseBean *child, bool value)
{
    Q_UNUSED(child)
    QMutexLocker lock(&d->m_mutex);
    d->m_modified = d->m_modified | value;
    d->emitBeanModified(d->m_modified);
}

/*!
  Recorre todos los fields, y recalcula todos los los fields hijos
  */
void BaseBean::recalculateCalculatedFields()
{
    QMutexLocker lock(&d->m_mutex);
    if ( !d->m_calculateFieldsEnabled )
    {
        return;
    }
    QElapsedTimer timer;
    timer.start();
    foreach ( DBField *fld, d->m_fields )
    {
        if ( fld->metadata()->calculated() && fld != sender() && !fld->isWorking() )
        {
            if ( (dbState() == BaseBean::UPDATE && !fld->metadata()->calculatedOnlyOnInsert()) ||
                 dbState() == BaseBean::INSERT )
            {
                if ( d->m_recalculatingField.isEmpty() || !d->m_recalculatingField.contains(fld) )
                {
                    d->m_recalculatingField.push(fld);
                    QLogger::QLog_Info(AlephERP::stLogOther, QString("BaseBean::recalculateCalculatedFields: Forzando el recalculo de [%1][%2]. Bean número: [%3]").
                                       arg(d->m->tableName()).
                                       arg(fld->metadata()->dbFieldName()).
                                       arg(objectName()));
                    // Si la señal viene dada por la carga de un padre, no tendríamos porqué emitir ningúna señal (si es por ejemplo
                    // una modificación, la carga del padre implica necesariamente que se haya dado un valor al campo que apunta a ese padre
                    QString senderSignal;
                    if ( sender() != NULL )
                    {
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
                        senderSignal = sender()->metaObject()->method(senderSignalIndex()).signature();
#else
                        senderSignal = sender()->metaObject()->method(senderSignalIndex()).methodSignature();
#endif
                    }
                    bool blockSignalState = fld->signalsBlocked();
                    if ( senderSignal.contains(QStringLiteral("fatherLoaded")) || senderSignal.contains(QStringLiteral("fatherUnloaded")) )
                    {
                        fld->blockSignals(true);
                    }
                    fld->recalculate();
                    if ( senderSignal.contains(QStringLiteral("fatherLoaded")) || senderSignal.contains(QStringLiteral("fatherUnloaded")) )
                    {
                        fld->blockSignals(blockSignalState);
                    }
                    d->m_recalculatingField.pop();
                }
                else
                {
                    QLogger::QLog_Info(AlephERP::stLogOther, QString("BaseBean::recalculateCalculatedFields: Intentando calcular un campo que estaba en una llamada anterior.[%1] La pila es: ").arg(fld->metadata()->dbFieldName()));
                    QLogger::QLog_Info(AlephERP::stLogOther, QString("BaseBean::recalculateCalculatedFields: --------------------------------------"));
                    QLogger::QLog_Info(AlephERP::stLogOther, QString("BaseBean::recalculateCalculatedFields: FIELDS EN PILA "));
                    foreach (DBField *element, d->m_recalculatingField)
                    {
                        QLogger::QLog_Info(AlephERP::stLogOther, QString("BaseBean::recalculateCalculatedFields: [%1]").arg(element->metadata()->dbFieldName()));
                    }
                    QLogger::QLog_Info(AlephERP::stLogOther, QString("BaseBean::recalculateCalculatedFields: --------------------------------------"));
                }
            }
        }
    }
    QLogger::QLog_Debug(AlephERP::stLogOther, QString("BaseBean::recalculateCalculatedFields: Time: [%1] ms").
                        arg(timer.elapsed()));
}

bool BaseBean::enableCalculateFields(bool value)
{
    bool v = d->m_calculateFieldsEnabled;
    d->m_calculateFieldsEnabled = value;
    return v;
}

/*!
  Marca como no modificados los campos e hijos de este bean
  */
void BaseBean::uncheckModifiedFields(bool uncheckChildren)
{
    QMutexLocker lock(&d->m_mutex);
    bool signalsWasBlocked = blockAllSignals(true);
    foreach (DBField *fld, d->m_fields)
    {
        fld->setModified(false);
    }
    if ( uncheckChildren )
    {
        foreach (DBRelation *rel, d->m_relations)
        {
            rel->uncheckChildrensModified();
        }
    }
    blockAllSignals(signalsWasBlocked);
    if ( d->m_modified )
    {
        d->m_modified = false;
        d->emitBeanModified(d->m_modified);
    }
}

void BaseBean::uncheckModifiedRelatedElements()
{
    QMutexLocker lock(&d->m_mutex);
    d->m_relatedElementsModified = false;
}

/*!
  Guarda en BBDD el contenido de este bean. Ejecuta una inserción o update. Para que sea
  una inserción, insert debe estar a true. Si se guarda en base de datos bien, se pondrá
  insert a false. Sólo almacenará en base de datos este registro. Si tiene hijos descendientes,
  o padres modificados, el programador QS deberá llamar a la función save de esos hijos o padres.
  Esta función NO se recomienda para el programador QS. Lo ideal es añadir los beans a transacciones
  a través del objeto AERPScriptCommon con el método addToContext, y persistir después los cambios
  en base de datos a través de commit.
  */
bool BaseBean::save(const QString &idTransaction, bool recalculateFieldsBefore)
{
    QMutexLocker lock(&d->m_mutex);
    bool result = false;

    if ( dbState() != BaseBean::INSERT && d->m_readOnly )
    {
        return true;
    }

    // Un bean marcado como borrado, no puede ser "salvado". Ponemos esto aquí simplemente para
    // que quede claro a nivel de funcionalidad y documentación
    if ( dbState() == BaseBean::DELETED )
    {
        return false;
    }

    // Los beans que se crean desde el motor QS y son manipulados ahí no tienen porqué disparar
    // eventos de recalculo de campos. Por eso, es importante, antes de guardar asegurarse que
    // se tienen todos los cálculos al día
    if ( recalculateFieldsBefore )
    {
        if ( d->m_modified && ( d->m_dbState == BaseBean::INSERT || d->m_dbState == BaseBean::UPDATE ) )
        {
            recalculateCalculatedFields();
        }
    }

    QString transaction;
    if (idTransaction.isEmpty())
    {
        transaction = QUuid::createUuid().toString();
        if ( dbState() == BaseBean::INSERT )
        {
            if ( d->m_modified && (!metadata()->beforeInsertScriptExecute(this) || !metadata()->beforeSaveScriptExecute(this)) )
            {
                QLogger::QLog_Error(AlephERP::stLogOther, trUtf8("No se han cumplido las condiciones necesarias para la inserción de este registro '%1'").arg(metadata()->alias()));
                return false;
            }
            emitBeforeInsert();
            emitBeforeSave();
        }
        else if ( dbState() == BaseBean::UPDATE )
        {
            if ( d->m_modified && !(metadata()->beforeUpdateScriptExecute(this) || metadata()->beforeSaveScriptExecute(this)) )
            {
                QLogger::QLog_Error(AlephERP::stLogOther, trUtf8("No se han cumplido las condiciones necesarias para la edición de este registro '%1'").arg(metadata()->alias()));
                return false;
            }
            emitBeforeUpdate();
            emitBeforeSave();
        }
        else if ( dbState() == BaseBean::TO_BE_DELETED )
        {
            if ( !metadata()->beforeDeleteScriptExecute(this) )
            {
                return false;
            }
            emitBeforeDelete();
            prepareToDeleteRelatedElements();
        }
    }
    else
    {
        transaction = idTransaction;
    }

    // ¿Hay algún campo calculado que el usuario no puede modificar o bien, que puede modificar
    // pero no lo ha hecho?
    foreach ( DBField *fld, d->m_fields )
    {
        if ( fld->metadata()->hasCounterDefinition() )
        {
            if ( !fld->metadata()->counterDefinition()->userCanModified ||
                 (fld->metadata()->counterDefinition()->userCanModified && !fld->modified()) )
            {
                if ( (fld->metadata()->counterDefinition()->calculateOnlyOnInsert && dbState() == BaseBean::INSERT) || !fld->metadata()->counterDefinition()->calculateOnlyOnInsert )
                {
                    if ( !fld->counterBlocked() && !fld->overwrite() )
                    {
                        fld->setValue(fld->calculateCounter("", false));
                    }
                }
            }
        }
    }

    if ( dbState() == BaseBean::INSERT )
    {
        result = BaseDAO::insert(this, transaction);
    }
    else if ( dbState() == BaseBean::UPDATE )
    {
        result = BaseDAO::update(this, transaction);
    }
    else if ( dbState() == BaseBean::TO_BE_DELETED )
    {
        result = BaseDAO::remove(this, transaction);
    }

    if ( result )
    {
        if ( dbState() == BaseBean::INSERT )
        {
            d->m_dbState = BaseBean::UPDATE;
            // Ejecutamos las acciones tras insertar.
            metadata()->afterInsertScriptExecute(this);
        }
        else if ( dbState() == BaseBean::UPDATE )
        {
            metadata()->afterUpdateScriptExecute(this);
        }
        else if ( dbState() == BaseBean::TO_BE_DELETED )
        {
            d->m_dbState = BaseBean::DELETED;
            metadata()->afterDeleteScriptExecute(this);
        }
        // Optimización: Tras una inserción, no queremos eventos que se propagen y generen recálculos innecesarios...
        bool deleteBeanStateBlockSignals = blockAllSignals(true);
        uncheckModifiedFields();
        blockAllSignals(deleteBeanStateBlockSignals);
        // Ejecutamos las acciones tras insertar.
        metadata()->afterSaveScriptExecute(this);
    }
    if ( result && idTransaction.isEmpty() )
    {
        emitBeanSaved();
        emitBeanCommitted();
    }
    return result;
}

/**
 * @brief BaseBean::saveRelatedElements
 * Almacena en base de datos los posibles elementos relacionados que hubiera. Esta función es útil para los siguientes casos:
 * 1.- Se almacena un bean, y tras ello se realizan algunas acciones, que implican la creación de un elemento relacionado.
 * 2.- Como previamente el bean debe haberse guardado, como requisito para crear el elemento relacionado, es absurdo volver
 * a llamar al método save, y reiniciar el proceso (que podría generar en recursividad).
 * 3.- Se proporciona esta función.
 * @return
 */
bool BaseBean::saveRelatedElements(const QString &idTransaction)
{
    bool blockSignals = blockAllSignals(true);
    bool result = RelatedDAO::saveRelatedElements(this, idTransaction);
    if ( result )
    {
        uncheckModifiedRelatedElements();
    }
    blockAllSignals(blockSignals);
    return result;
}

/*!
  Realiza en memoria una copia de los Values de los DBFields del bean. Es muy útil cuando se
  va a editar los datos de un bean, se modifica y se cancela la edición. Se restaura la copia
  con restoreValues.
  @see restoreValues
  */
void BaseBean::backupValues()
{
    QMutexLocker lock(&d->m_mutex);

    bool beanSignalsState = blockAllSignals(true);
    foreach ( DBField *fld, d->m_fields )
    {
        if ( fld->metadata()->calculated() && (!fld->metadata()->memo() || fld->memoLoaded()) )
        {
            d->m_backup[fld->metadata()->dbFieldName()] = fld->value();
        }
    }
    blockAllSignals(beanSignalsState);
}

/*!
  Restaura la copia de seguridad de los datos que se hizo con backupValues.
  @see backupValues;
  */
void BaseBean::restoreValues(bool blockSignals)
{
    QMutexLocker lock(&d->m_mutex);
    if ( d->m_restoringValues )
    {
        return;
    }
    d->m_restoringValues = true;
    bool blockSignalsState = false;
    if ( blockSignals )
    {
        blockSignalsState = blockAllSignals(true);
    }
    QMapIterator<QString, QVariant> it(d->m_backup);
    while ( it.hasNext() )
    {
        it.next();
        setFieldValue(it.key(), it.value());
        DBField *fld = field(it.key());
        if ( fld != NULL )
        {
            fld->setModified(false);
        }
    }

    // Además, borramos los posibles hijos agregados...
    foreach (DBRelation *rel, d->m_relations)
    {
        rel->restoreValues(blockSignals);
    }

    foreach (DBField *fld, d->m_fields)
    {
        if ( fld->metadata()->calculated() && !(dbState() == BaseBean::UPDATE && fld->metadata()->calculatedOnlyOnInsert()) )
        {
            fld->setValueIsOld();
        }
    }
    foreach (DBField *fld, d->m_fields)
    {
        if ( fld->metadata()->calculated() && !(dbState() == BaseBean::UPDATE && fld->metadata()->calculatedOnlyOnInsert()) )
        {
            bool b = fld->blockSignals(true);
            fld->recalculate();
            fld->blockSignals(b);
            fld->setModified(false);
        }
    }

    uncheckModifiedFields();
    uncheckModifiedRelatedElements();

    if ( blockSignals )
    {
        blockAllSignals(blockSignalsState);
    }
    d->m_restoringValues = false;
}

/*!
  Cuando se inserta un nuevo bean, y se trabaja con él, puede ocurrir que sea necesario
  que los campos seriales tengan un valor único para poderlos identificar (pueden ser un
  primary key). Esta función se encarga de esa asignación
  @see backupValues;
  */
void BaseBean::setSerialUniqueId()
{
    QMutexLocker lock(&d->m_mutex);
    QList<DBFieldMetadata *> pkFields = d->m->pkFields();
    foreach ( DBField *fld, d->m_fields )
    {
        if ( fld->metadata()->serial() && pkFields.contains(fld->metadata()) )
        {
            fld->setInternalValue(alephERPSettings->uniqueId());
        }
    }
}

/*!
  Tenemos que decirle al motor de scripts, que BaseBean se convierte de esta forma a un valor script
  */
QScriptValue BaseBean::toScriptValue(QScriptEngine *engine, BaseBean * const &in)
{
    QScriptValue qsBean = engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
    return qsBean;
}

void BaseBean::fromScriptValue(const QScriptValue &object, BaseBean * &out)
{
    out = qobject_cast<BaseBean *>(object.toQObject());
}

QScriptValue BaseBean::toScriptValuePointer(QScriptEngine *engine, const BaseBeanPointer &in)
{
    QScriptValue qsBean = engine->newQObject(in.data(), QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
    return qsBean;
}

void BaseBean::fromScriptValuePointer(const QScriptValue &object, BaseBeanPointer &out)
{
    out = BaseBeanPointer(qobject_cast<BaseBean *>(object.toQObject()));
}

QScriptValue BaseBean::toScriptValueSharedPointer(QScriptEngine *engine, const BaseBeanSharedPointer &in)
{
    QScriptValue qsBean = engine->newQObject(in.data(), QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
    return qsBean;
}

void BaseBean::fromScriptValueSharedPointer(const QScriptValue &object, BaseBeanSharedPointer &out)
{
    out = BaseBeanSharedPointer(qobject_cast<BaseBean *>(object.toQObject()));
}

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
QScriptValue BaseBean::toScriptValueWeakPointer(QScriptEngine *engine, const BaseBeanWeakPointer &in)
{
    QScriptValue qsBean = engine->newQObject(in.data(), QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
    return qsBean;
}

void BaseBean::fromScriptValueWeakPointer(const QScriptValue &object, BaseBeanWeakPointer &out)
{
    out = QWeakPointer<BaseBean>(qobject_cast<BaseBean *>(object.toQObject()));
}
#endif

QString BaseBeanPrivate::extractFilterOperator(const QString &filter)
{
    QStringList ops;
    QString op;
    ops << "=" << "<" << ">" << "!=";

    foreach ( QString temp, ops )
    {
        if ( filter.indexOf(temp) != -1 )
        {
            op = temp;
        }
    }
    return op;
}

/*!
  Pasa un checkeo de filtro. Pero este chequeo permite anidaciones en relacion M1. Por ejemplo, si hacemos
  "categoria.id_categoria = 2" buscará la relación categoria. Pero también puede hacer
  "subfamilia.familia.id_familia = 2" y devolverá si id_familia de familias es 2.
  También aceptará expresiones como subfamilia.familia.id_familia < 2.
  Por otro lado se pueden chequear varios campos. En ese caso hay que separar los filtros
  con la expresión " AND " o " and "
  Un ejemplo de filtro
  actividad.nombre='prueba' and id_tipo_trabajo='FO'
  */
bool BaseBean::checkFilter(const QString &filterExpression, Qt::CaseSensitivity sensivity)
{
    QString op, fields;
    QStringList parts, relations;
    BaseBean *bean = this;
    DBRelation *rel = NULL;
    QString data;
    QDate filterDate, filterDate2;

    if ( filterExpression.isEmpty() )
    {
        return true;
    }

    QStringList conditions = filterExpression.split(QRegExp(" (AND|and) "));

    foreach ( QString filter, conditions )
    {
        op = d->extractFilterOperator(filter);
        if ( op.isEmpty() )
        {
            return false;
        }
        parts = filter.split(op);
        if ( parts.size() != 2 )
        {
            return false;
        }
        fields = parts.at(0).trimmed();
        data = parts.at(1).trimmed();
        relations = fields.split(".");
        // Si es una relación, obtenemos el bean padre de esa relación
        for ( int i = 0 ; i < relations.size() - 1; i++ )
        {
            if ( bean != NULL )
            {
                rel = bean->relation(relations.at(i));
                if ( rel != NULL )
                {
                    BaseBean *shBean = NULL;
                    if ( rel->metadata()->type() == DBRelationMetadata::MANY_TO_ONE )
                    {
                        shBean = rel->father();
                    }
                    else
                    {
                        BaseBeanPointerList children = rel->children();
                        if ( children.size() > 0 && !children.at(0).isNull() )
                        {
                            shBean = children.at(0).data();
                        }
                    }
                    bean = shBean;
                }
                else
                {
                    bean = NULL;
                }
            }
        }
        if ( bean == NULL )
        {
            return false;
        }
        DBField *fld = bean->field(relations.last());
        if ( fld == NULL )
        {
            return false;
        }
        if ( fld->metadata()->type() == QVariant::Bool )
        {
            if ( op == "=" )
            {
                bool v = (data == "true" ? true : false);
                if (!fld->checkValue(v))
                {
                    return false;
                }
            }
            else if ( op == "!=" )
            {
                bool v = (data == "true" ? true : false);
                if (fld->checkValue(v))
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
        else if ( fld->metadata()->type() == QVariant::Date )
        {
            /** ¿es un intervalo de fechas?. Los intervalos de fecha se ponen
            fecha=2010-11-21;2010-12-30 */
            if ( data.contains(';') )
            {
                QStringList dates = data.split(';');
                if ( dates.size() == 2 )
                {
                    filterDate = QDate::fromString(dates.at(0), Qt::ISODate);
                    filterDate2 = QDate::fromString(dates.at(1), Qt::ISODate);
                    if ( !filterDate.isValid() || !filterDate2.isValid() )
                    {
                        return false;
                    }
                    if ( filterDate > filterDate2 )
                    {
                        QDate tmp = filterDate2;
                        filterDate2 = filterDate;
                        filterDate = tmp;
                    }
                    if ( ! ( fld->value().toDate() > filterDate && fld->value().toDate() < filterDate2 ) )
                    {
                        return false;
                    }
                }
            }
            else
            {
                filterDate = QDate::fromString(data, Qt::ISODate);
                if ( !filterDate.isValid() )
                {
                    return false;
                }
                if ( !fld->checkValue(filterDate, op) )
                {
                    return false;
                }
            }
        }
        else
        {
            if ( !fld->checkValue(data, op, sensivity) )
            {
                return false;
            }
        }
    }

    return true;
}

/*!
  Esta función es una que encuentra la/s relación/es adecuada o el field adecuado según el filtro explicado
  indicado en \a relationFilter y \a path. Por ejemplo:
  path: presupuestos_ejemplares.presupuestos_actividades.importe
  relationFilter: id_numejemplares=1234.
  El path generalmente será: relacion1.relacion2.field donde relacion1, y relacion2 serán relaciones
  M1. Pero puede existir múltiples instancias de una relación, lo que lleva a confusión: Por ejemplo,
  en Artículos tenemos idimpuestocompra e idimpuestoventa, ambos apuntan a la tabla impuestos en relación M1.
  Para identificar si accedermos por un lado u otro, se utiliza:
  idimpuestocompra.impuestos.nombre
  idimpuestoventa.impuestos.nombre
  Además, se podrán obtener hijos de hijos en relaciones 1M, por ejemplo:
  terceros.facturascli.lineasserviciosfacturascli
  que devolverá todos los DBRelation (y por tanto la posibilidad de obtener todos los hijos) de facturascli relacionando
  a lineasserviciosfacturascli, pero haciendo un cartesiano de todos cada nodo.
  */
QList<DBObject *> BaseBean::navigateThrough(const QString &path, const QString &relationFilters)
{
    QStringList filters = relationFilters.split(';');
    QStringList paths = path.split(".");
    if ( relationFilters.isEmpty() )
    {
        filters.clear();
        for (int i = 0 ; i < paths.size() ; i++)
        {
            filters.append("");
        }
    }
    bool previousState = blockAllSignals(true);
    QList<DBObject *> list = d->iterateNavigation(this, paths, filters);
    blockAllSignals(previousState);
    return list;
}

/**
 * @brief BaseBean::navigateThroughProperties
 * Función conceptualmente parecida a navigateThrough, y que debería sustituirla a futuro, ya que ésta hace una navegación
 * más lógica a través de las propiedades de los beans.
 * @param path
 * @param returnIntermediate: Si el path falla a la mitad, devuelve el punto en el que se hubiera encontrado. Si es false, devuelve NULL
 * @return
 */
DBObject *BaseBean::navigateThroughProperties(const QString &path, bool returnIntermediate)
{
    QStringList items = path.split(".");
    DBObject *obj = this, *backupObj = this;

    if ( path.isEmpty() )
    {
        return this;
    }

    for (int i = 0 ; i < items.size() ; i++)
    {
        if ( obj == NULL )
        {
            if ( returnIntermediate )
            {
                return this;
            }
            else
            {
                return NULL;
            }
        }
        QByteArray ba = items.at(i).toUtf8();
        QVariant v = obj->property(ba.constData());
        if ( !v.isValid() )
        {
            QLogger::QLog_Debug(AlephERP::stLogOther, QString("BaseBean::navigateThroughProperties: La ruta [%1] no conduce a ningún lugar. Item: [%2]").arg(path).arg(items.at(i)));
            return NULL;
        }
        // http://liveblue.wordpress.com/2012/10/29/qobject-multiple-inheritance-and-smart-delegators/
        // Ahí está la razón para estas cosas...
        // TODO: The sad thing is: in Qt4, values can be extracted from QVariant’s only by using the same
        // type used when constructing the QVariant. Qt5 introduces a nifty feature where you can safely
        // extract a QObject * from any QVariant built from a QObjectDerived *.
        obj = (DBField *)(v.value<DBField *>());
        if ( obj == NULL )
        {
            obj = (DBRelation *)(v.value<DBRelation *>());
            if ( obj == NULL )
            {
                obj = (BaseBean *)(v.value<BaseBean *>());
                if ( obj == NULL )
                {
                    BaseBeanPointer beanPointer = (BaseBeanPointer) (v.value<BaseBeanPointer>());
                    if ( !beanPointer.isNull() )
                    {
                        obj = beanPointer.data();
                        backupObj = obj;
                    }
                    else
                    {
                        if ( returnIntermediate )
                        {
                            return backupObj;
                        }
                        else
                        {
                            QLogger::QLog_Debug(AlephERP::stLogOther, QString("BaseBean::navigateThroughProperties: La ruta [%1] no ha podido ser convertida a un objeto valido..").arg(path));
                            return NULL;
                        }
                    }
                }
                else
                {
                    backupObj = obj;
                }
            }
            else
            {
                backupObj = obj;
            }
        }
        else
        {
            backupObj = obj;
        }
    }
    return obj;
}

/**
 * @brief BaseBeanPrivate::iterateNavigation
 * @param relativePath Parte de la ruta relativa: idimpuestoventa.impuestos.periodosimpuestos.iva
 * En cada iteración se irá eliminado el primero... impuestos.periodosimpuestos.iva será la siguiente llamada
 * @param filters
 * @return
 */
QList<DBObject *> BaseBeanPrivate::iterateNavigation(BaseBean *bean, const QStringList &relativePath, const QStringList &filters)
{
    QList<DBObject *> list;

    if ( bean == NULL )
    {
        return list;
    }

    if ( relativePath.size() > 0 )
    {
        QString objectName = relativePath.at(0);
        QStringList nextRelativePath = relativePath;
        QStringList nextFilters = filters;

        nextRelativePath.removeAt(0);

        // Veamos que es el siguiente item que viene...
        DBField *fld = bean->field(objectName);
        if ( fld == NULL )
        {
            DBRelation *rel = bean->relation(objectName);
            if ( rel != NULL )
            {
                if ( relativePath.size() > 1 )
                {
                    list.append(iterateNavigation(rel, nextRelativePath, nextFilters));
                }
                else
                {
                    list.append(rel);
                }
            }
        }
        else
        {
            if ( relativePath.size() == 1 )
            {
                list.append(fld);
            }
            else
            {
                list.append(iterateNavigation(fld, nextRelativePath, nextFilters));
            }
        }
    }
    return list;
}

/**
 * @brief BaseBeanPrivate::iterateNavigation
 * Realizar
 * @param relativePath Parte de la ruta relativa: idimpuestoventa.impuestos.periodosimpuestos.iva
 * En cada iteración se irá eliminado el primero... impuestos.periodosimpuestos.iva será la siguiente llamada
 * @param filters
 * @return
 */
QList<DBObject *> BaseBeanPrivate::iterateNavigation(DBField *fld, const QStringList &relativePath, const QStringList &filters)
{
    QList<DBObject *> list;
    if ( relativePath.size() > 0 )
    {
        QString objectName = relativePath.at(0);
        QString filter;
        QStringList nextRelativePath = relativePath;
        QStringList nextFilters = filters;

        nextRelativePath.removeAt(0);
        if ( filters.size() > 0 )
        {
            filter = filters.at(0);
            nextFilters.removeAt(0);
        }

        // La siguiente ruta tiene que ser un relation
        DBRelation *rel = fld->relation(objectName);
        if ( rel != NULL )
        {
            QList<BaseBean *> beanList;
            if ( rel->metadata()->type() == DBRelationMetadata::MANY_TO_ONE )
            {
                bool previous = rel->blockAllSignals(true);
                beanList.append(rel->father());
                rel->blockAllSignals(previous);
            }
            else
            {
                BaseBeanPointerList children = rel->childrenByFilter(filter);
                foreach (BaseBeanPointer pointerBean, children)
                {
                    if ( !pointerBean.isNull() )
                    {
                        beanList.append(pointerBean.data());
                    }
                }
            }
            foreach (BaseBean *bean, beanList)
            {
                list.append(iterateNavigation(bean, nextRelativePath, nextFilters));
            }
        }
    }
    return list;
}

QList<DBObject *> BaseBeanPrivate::iterateNavigation(DBRelation *rel, const QStringList &relativePath, const QStringList &filters)
{
    QString filter;
    QStringList nextFilters = filters;
    QList<DBObject *> list;

    if ( filters.size() > 0 )
    {
        filter = filters.at(0);
        nextFilters.removeAt(0);
    }

    if ( rel != NULL )
    {
        QList<BaseBean *> beanList;
        if ( rel->metadata()->type() == DBRelationMetadata::MANY_TO_ONE && rel->father() )
        {
            bool previousState = rel->father()->blockAllSignals(true);
            beanList.append(rel->father());
            rel->father()->blockAllSignals(previousState);
        }
        else
        {
            foreach (BaseBeanPointer pointerBean, rel->children(filter))
            {
                if ( !pointerBean.isNull() )
                {
                    beanList.append(pointerBean.data());
                }
            }
        }
        foreach (BaseBean *bean, beanList)
        {
            if ( bean != NULL )
            {
                list.append(iterateNavigation(bean, relativePath, nextFilters));
            }
        }
    }

    return list;
}


/*!
  Se borra el observador actual, y además los de los fields hijos
  */
void BaseBean::deleteObserver()
{
    QMutexLocker lock(&d->m_mutex);
    if ( m_observer )
    {
        delete m_observer;
    }
    foreach ( DBField *fld, d->m_fields )
    {
        fld->deleteObserver();
    }
}

/*!
  Valida si los datos del bean, cumplen con las reglas de validación establecidas
  */
bool BaseBean::validate()
{
    QMutexLocker lock(&d->m_mutex);
    d->m_validateMessage = d->m->validateScriptExecute(this);
    if ( !d->m_validateMessage.isEmpty() )
    {
        return false;
    }
    if ( m_observer )
    {
        BaseBeanObserver *obs = qobject_cast<BaseBeanObserver *>(m_observer);
        if ( obs != NULL )
        {
            return obs->validate();
        }
        else
        {
            QLogger::QLog_Warning(AlephERP::stLogOther, "BaseBean:: validate. Observador nulo");
            return false;
        }
    }
    else
    {
        QScopedPointer<BaseBeanValidator> val (new BaseBeanValidator(this));
        val->setBean(this);
        return val->validate();
    }
}

/*!
  Devuelve los mensajes de validación incorrectos que pudiesen surgir tras obtener validate() = false.
  Es decir, lo normal sera tener algo asi en codigo javascript:
  if ( !bean.validate() ) {
	QMessageBox.warning(bean.validateMessagesHtml();
  }
 */
QString BaseBean::validateMessages()
{
    QString returnExp;
    if ( m_observer )
    {
        BaseBeanObserver *obs = qobject_cast<BaseBeanObserver *>(m_observer);
        if ( obs != NULL )
        {
            returnExp = obs->validateMessages();
        }
        else
        {
            QLogger::QLog_Warning(AlephERP::stLogOther, "BaseBean::validateMessages: observador nulo.");
        }
    }
    else
    {
        QScopedPointer<BaseBeanValidator> val (new BaseBeanValidator(this));
        val->setBean(this);
        val->validate();
        returnExp = val->validateMessages();
    }
    if ( !d->m_validateMessage.isEmpty() )
    {
        if ( !returnExp.isEmpty() )
        {
            returnExp.append("\n");
        }
        returnExp.append(d->m_validateMessage);
    }
    return returnExp;
}

/*!
  Devuelve los mensajes de validación incorrectos que pudiesen surgir tras obtener validate() = false.
  Los devuelve correctamente formateados en HTML.
  Es decir, lo normal sera tener algo asi en codigo javascript:
  if ( !bean.validate() ) {
	QMessageBox.warning(bean.validateMessagesHtml();
  }
 */
QString BaseBean::validateHtmlMessages()
{
    QString returnExp;
    if ( m_observer )
    {
        BaseBeanObserver *obs = qobject_cast<BaseBeanObserver *>(m_observer);
        if ( obs != NULL )
        {
            returnExp = obs->validateHtmlMessages();
        }
        else
        {
            QLogger::QLog_Warning(AlephERP::stLogOther, "BaseBean::validateHtmlMessages: observador nulo.");
        }
    }
    else
    {
        QScopedPointer<BaseBeanValidator> val (new BaseBeanValidator(this));
        val->setBean(this);
        returnExp = val->validateHtmlMessages();
    }
    if ( !d->m_validateMessage.isEmpty() )
    {
        if ( !returnExp.isEmpty() )
        {
            returnExp.append("<br/>");
        }
        returnExp.append(d->m_validateMessage);
    }
    return returnExp;
}

QWidget *BaseBean::focusWidgetOnBadValidate()
{
    if ( m_observer )
    {
        BaseBeanObserver *obs = qobject_cast<BaseBeanObserver *>(m_observer);
        if ( obs != NULL )
        {
            return obs->focusWidgetOnBadValidate();
        }
        else
        {
            QLogger::QLog_Warning(AlephERP::stLogOther, "BaseBean::focusWidgetOnBadValidate: observador nulo.");
            return NULL;
        }
    }
    else
    {
        QScopedPointer<BaseBeanValidator> val (new BaseBeanValidator(this));
        val->setBean(this);
        return val->widgetOnBadValidate();
    }
}

/**
 * @brief BaseBean::isAccessible Ejecuta la función Qs del metadata (accessible) para determinar si este bean
 * es accesible por la aplicación. Será de aplicación sólo al Filtro FilterBaseBeanModel, pero se podrá
 * acceder a él, por parte del programador Qs, o bien, por parte de los DBRecord...
 * @return
 */
bool BaseBean::isAccessible()
{
    return d->m->isAccessible(this);
}

/**
 * @brief BaseBean::toString
 * Escoge todos los DBFields con showDefault a true y construye una representación de este bean con los
 * fields pasados
 * @return
 */
QString BaseBean::toString()
{
    QString exp = d->m->toStringExecute(this);
    if ( exp.isEmpty() && !d->m->defaultVisualizationField().isEmpty() )
    {
        return displayFieldValue(d->m->defaultVisualizationField());
    }
    return exp;
}

RelatedElementPointer BaseBean::newRelatedElement(AlephERP::RelatedElementTypes type)
{
    QMutexLocker lock(&d->m_mutex);
    RelatedElement *element = new RelatedElement(this);
    element->setType(type);
    element->setCardinality(AlephERP::PointToChild);
    addRelatedElement(element);
    setModified();
    connect(element, SIGNAL(hasBeenModified(RelatedElement *)), this, SLOT(emitRelatedElementModified(RelatedElement*)));
    return element;
}

AERPUserRowAccessList BaseBean::access()
{
    return d->m_access;
}

void BaseBean::setAccess(AERPUserRowAccessList list)
{
    QMutexLocker lock(&d->m_mutex);
    d->m_access = list;
}

void BaseBean::appendAccess(const AERPUserRowAccess &item)
{
    QMutexLocker lock(&d->m_mutex);
    d->m_access.append(item);
}

/**
 * @brief BaseBean::checkAccess
 * Devuelve true si el presente usuario tiene permisos para acceder a esta fila
 * @return
 */
bool BaseBean::checkAccess(QChar access)
{
    return d->checkAccess(access);
}

bool BaseBeanPrivate::checkAccess(QChar access)
{
    if ( AERPLoggedUser::instance()->isSuperAdmin() )
    {
        return true;
    }
    if ( !AERPLoggedUser::instance()->checkMetadataAccess('r', m->tableName()) )
    {
        return false;
    }
    if ( !m->filterRowByUser() )
    {
        return true;
    }
    if ( m_dbState == BaseBean::INSERT )
    {
        return true;
    }
    // Se hacen dos bucles para tener en cuenta la granularidad de los permisos.
    foreach (AERPUserRowAccess item, q_ptr->access())
    {
        if ( (item.userName() == AERPLoggedUser::instance()->userName() ||
                AERPLoggedUser::instance()->hasRole(item.idRole()) )
                && !item.access().contains(access) )
        {
            return false;
        }
    }
    foreach (AERPUserRowAccess item, q_ptr->access())
    {
        if ( item.access().contains(access) )
        {
            if ( item.userName() == "*" || item.userName() == AERPLoggedUser::instance()->userName() )
            {
                return true;
            }
            if ( item.roleName() == "*" || AERPLoggedUser::instance()->hasRole(item.idRole() ) )
            {
                return true;
            }
        }
    }
    if ( !m->accessibleRule().isEmpty() )
    {
        return q_ptr->isAccessible();
    }
    return false;
}

/**
 * @brief BaseBean::newRelatedElement
 * Crea un nuevo elemento relacionado, que será un registro de otra taba.
 * @param tableName
 * @param category
 * @return
 */
RelatedElementPointer BaseBean::newRelatedElement(const QString &tableName, const QStringList &category)
{
    if ( !d->m->canHaveRelatedElements() )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBean::newRelatedElement: La tabla [%1] no puede tener elementos relacionados").arg(d->m->tableName()));
        return NULL;
    }

    if ( BeansFactory::instance()->metadataBean(tableName) == NULL )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBean::newRelatedElement: No existe ninguna tabla de nombre: [%1]").arg(tableName));
        return NULL;
    }
    RelatedElementPointer element = newRelatedElement(AlephERP::Record);
    setModified();
    element->setParent(this);
    element->setRootBean(this);
    element->setCategories(category);
    element->newRelatedBean(tableName);
    if ( !actualContext().isEmpty() )
    {
        AERPTransactionContext::instance()->addToContext(actualContext(), element->relatedBean().data());
    }
    d->emitRelatedElementAdded(element);
    d->m->newRelatedElementScriptExecute(this, element);
    return element;
}

/**
 * @brief BaseBean::newRelatedElement
 * Esta función será la invocada por el motor QS, cuando desde el mismo se quiera crear un elemento relacionado.
 * Se puede invocar de varias formas:
 *
 * var nuevoBean = bean.newRelatedElement("efectospago", "categoria1", "categoria2");
 * En nuevo bean tendremos un objeto de tipo RelatedElement, en el que a su vez, se le ha creado un bean
 * de tipo "efectospago" en su interior. Esta función será equivalente a
 *
 * var nuevoBean = AERPScriptCommon.createBean("efectospago");
 * var relatedElement = bean.newRelatedElement(nuevoBean, "categoria1", "categoria2");
 *
 * También vale
 * var categorias = new Array();
 * categorias[0]="categoria1";
 *
 * var relatedElement = bean.newRelatedElement(nuevoBean, categorias);
 *
 * @param args
 * @return
 */
QScriptValue BaseBean::newRelatedElement()
{
    QScriptValue result = QScriptValue(QScriptValue::NullValue);

    if ( !d->m->canHaveRelatedElements() )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBean::newRelatedElement: La tabla [%1] no puede tener elementos relacionados").arg(d->m->tableName()));
        return result;
    }

    if ( engine() == NULL || context() == NULL )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBean::newRelatedElement: Invocada función sin motor ni contexto."));
        return result;
    }

    QStringList categories;
    if ( argumentCount() > 1 )
    {
        for (int i = 1; i < argumentCount() ; i++)
        {
            QScriptValue arg = argument(i);
            if ( arg.isString() )
            {
                categories.append(arg.toString());
            }
            else if ( arg.isArray() )
            {
                for (int j = 0 ; j < arg.property("length").toInteger() ; j++)
                {
                    QScriptValue arrayElement = arg.property(j);
                    if ( arrayElement.isString() )
                    {
                        categories.append(arrayElement.toString());
                    }
                }
            }
        }
    }
    if ( argumentCount() > 0 )
    {
        // Tratemos los argumentos
        QScriptValue arg1 = argument(0);
        // Si el primer argumento es una cadena, creamos un bean
        if ( arg1.isString() )
        {
            QString tableName = arg1.toString();
            if ( BeansFactory::instance()->metadataBean(tableName) == NULL )
            {
                QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBean::newRelatedElement: No existe ninguna tabla: [%1]").arg(tableName));
                return QScriptValue(QScriptValue::NullValue);
            }
            RelatedElementPointer element = newRelatedElement(tableName, categories);
            result = engine()->newQObject(element, QScriptEngine::QtOwnership);
        }
        else if ( arg1.isQObject() )
        {
            // Si es un bean, guay
            BaseBean *b = qobject_cast<BaseBean *>(arg1.toQObject());
            RelatedElement *otherElem = qobject_cast<RelatedElement *>(arg1.toQObject());
            if ( b != NULL )
            {
                RelatedElementPointer rel = newRelatedElement(b, categories);
                if ( rel != NULL )
                {
                    result = engine()->newQObject(rel, QScriptEngine::QtOwnership);
                }
            }
            else if ( otherElem != NULL )
            {
                RelatedElementPointer rel = newRelatedElement(otherElem);
                if ( rel != NULL )
                {
                    result = engine()->newQObject(rel.data(), QScriptEngine::QtOwnership);
                }
            }
            else
            {
                QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBean::newRelatedElement: No está permitido crear ningun elemento relacionado desde un objeto"));
            }
        }
    }
    return result;
}

RelatedElementPointer BaseBean::newRelatedElement(BaseBeanPointer related, const QStringList &category, bool takeOwnerShip)
{
    if ( !d->m->canHaveRelatedElements() )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBean::newRelatedElement: La tabla [%1] no puede tener elementos relacionados").arg(d->m->tableName()));
        return NULL;
    }
    RelatedElementPointer element = newRelatedElement(AlephERP::Record);
    setModified();
    element->setParent(this);
    element->setRootBean(this);
    element->setCategories(category);
    element->setRelatedBean(related, takeOwnerShip);
    if ( !actualContext().isEmpty() )
    {
        AERPTransactionContext::instance()->addToContext(actualContext(), element->relatedBean().data());
    }
    d->emitRelatedElementAdded(element);
    d->m->newRelatedElementScriptExecute(this, element);
    return element;
}

/**
 * @brief BaseBean::newRelatedElement
 * Crea un elemento relacionado a partir de otro elemento relacionado, quizás de otra tabla.
 * @param otherElement
 * @return
 */
RelatedElementPointer BaseBean::newRelatedElement(RelatedElementPointer otherElement)
{
    if ( otherElement.isNull() )
    {
        return NULL;
    }
    RelatedElementPointer element = newRelatedElement(otherElement->type());
    setModified();
    element->setParent(this);
    element->setRootBean(this);
    element->setCategories(otherElement->categories());
    if ( otherElement->type() == AlephERP::Record )
    {
        element->setRelatedBean(otherElement->relatedBean());
    }
#ifdef ALEPHERP_SMTP_SUPPORT
    else if ( otherElement->type() == AlephERP::Email )
    {
        element->setEmail(otherElement->email());
    }
#endif
#ifdef ALEPHERP_DOC_MANAGEMENT
    else if ( otherElement->type() == AlephERP::Document )
    {
        element->setDocument(otherElement->document());
    }
#endif
    return element;
}

#ifdef ALEPHERP_SMTP_SUPPORT
RelatedElementPointer BaseBean::newRelatedElement(const EmailObject &emailContent, const QStringList &category)
{
    if ( !d->m->canHaveRelatedElements() )
    {
        return NULL;
    }
    RelatedElementPointer element = newRelatedElement(AlephERP::Email);
    setModified();
    element->setParent(this);
    element->setRootBean(this);
    element->setType(AlephERP::Email);
    element->setEmail(emailContent);
    element->setCategories(category);
    d->emitRelatedElementAdded(element);
    d->m->newRelatedElementScriptExecute(this, element);
    return element;
}
#endif

#ifdef ALEPHERP_DOC_MANAGEMENT
RelatedElementPointer BaseBean::newRelatedElement(AERPDocMngmntDocument *doc, const QStringList &category)
{
    if ( !d->m->canHaveRelatedElements() )
    {
        return NULL;
    }
    RelatedElementPointer element = newRelatedElement(AlephERP::Document);
    setModified();
    element->setParent(this);
    element->setRootBean(this);
    element->setType(AlephERP::Document);
    element->setDocument(doc);
    element->setCategories(category);
    d->emitRelatedElementAdded(element);
    d->m->newRelatedElementScriptExecute(this, element);
    return element;
}
#endif

/**
 * @brief BaseBean::getRelatedElements
 * Devuelve los elementos relacionados. No devuelve los marcados para ser borrados.
 * @return
 */
RelatedElementPointerList BaseBean::getRelatedElements(AlephERP::RelatedElementTypes type, AlephERP::RelatedElementCardinalities cardinality, bool includeToBeDelete)
{
    QMutexLocker lock(&d->m_mutex);
    RelatedElementPointerList elements;
    if ( !d->m_relatedElementsLoaded )
    {
        bool modifiedState = d->m_modified;
        d->m_loadingRelatedElements = true;
        // La carga de elementos relacionados no implica la modificación del bean. loadRelatedElements llama a newRelatedElement
        // que invoca a setModified (y emite una señal). La inhibimos.
        bool blockSignalsState = blockSignals(true);
        if ( dbState() != BaseBean::INSERT )
        {
            if ( RelatedDAO::loadRelatedElements(this) )
            {
                d->m_relatedElementsLoaded = true;
            }
        }
        else
        {
            d->m_relatedElementsLoaded = true;
        }
        blockSignals(blockSignalsState);
        d->m_loadingRelatedElements = false;
        d->m_modified = modifiedState;
    }
    foreach (RelatedElementPointer element, d->m_relatedElements)
    {
        if ( !element.isNull() )
        {
            if ( (includeToBeDelete || element->state() != RelatedElement::TO_BE_DELETED) && cardinality.testFlag(element->cardinality()) )
            {
                if ( type == 0 || type == element->type() )
                {
                    elements.append(element);
                }
            }
        }
    }
    return elements;
}

RelatedElementPointerList BaseBean::getRelatedElementsByRelatedTableName(const QString &tableName, AlephERP::RelatedElementCardinalities cardinality, bool includeToBeDelete)
{
    RelatedElementPointerList elements;
    RelatedElementPointerList allElements = getRelatedElements(AlephERP::Record, cardinality, includeToBeDelete);
    foreach (RelatedElementPointer element, allElements)
    {
        if ( !element.isNull() )
        {
            if ( (includeToBeDelete || element->state() != RelatedElement::TO_BE_DELETED) && cardinality.testFlag(element->cardinality()) )
            {
                if ( !element->relatedBean().isNull() && element->relatedBean()->metadata()->tableName() == tableName )
                {
                    elements.append(element);
                }
            }
        }
    }
    return elements;
}

RelatedElementPointerList BaseBean::getRelatedElementsByCategory(const QString &category, AlephERP::RelatedElementTypes type, AlephERP::RelatedElementCardinalities cardinality, bool includeToBeDelete)
{
    RelatedElementPointerList elements;
    RelatedElementPointerList allElements = getRelatedElements(type, cardinality, includeToBeDelete);
    foreach (RelatedElementPointer element, allElements)
    {
        if ( !element.isNull() )
        {
            if ( (includeToBeDelete || element->state() != RelatedElement::TO_BE_DELETED) && cardinality.testFlag(element->cardinality()) )
            {
                if ( element->categories().contains(category) )
                {
                    elements.append(element);
                }
            }
        }
    }
    return elements;
}

RelatedElementPointerList BaseBean::getRelatedElementsByCategoryAndRelatedTableName(const QString &tableName, const QString &category, AlephERP::RelatedElementCardinalities cardinality, bool includeToBeDelete)
{
    RelatedElementPointerList elements;
    RelatedElementPointerList allElements = getRelatedElements(AlephERP::Record, cardinality, includeToBeDelete);
    foreach (RelatedElementPointer element, allElements)
    {
        if ( !element.isNull() )
        {
            if ( (includeToBeDelete || element->state() != RelatedElement::TO_BE_DELETED) && cardinality.testFlag(element->cardinality()) )
            {
                if ( !element->relatedBean().isNull() && element->relatedBean()->metadata()->tableName() == tableName && element->categories().contains(category) )
                {
                    elements.append(element);
                }
            }
        }
    }
    return elements;
}

RelatedElementPointerList BaseBean::getRelatedElementsByCategoriesAndRelatedTableNames(const QStringList &tableNames, const QStringList &categories, AlephERP::RelatedElementCardinalities cardinality, bool includeToBeDelete)
{
    RelatedElementPointerList elements;
    RelatedElementPointerList allElements = getRelatedElements(AlephERP::Record, cardinality, includeToBeDelete);
    foreach (RelatedElementPointer element, allElements)
    {
        if ( !element.isNull() )
        {
            if ( !element->relatedBean().isNull() &&
                    tableNames.contains(element->relatedBean()->metadata()->tableName()) && element->hasCategories(categories) )
            {
                elements.append(element);
            }
        }
    }
    return elements;
}

#ifdef ALEPHERP_DOC_MANAGEMENT
RelatedElementPointerList BaseBean::getRelatedElementsByDocumentId(int id)
{
    RelatedElementPointerList elements;
    RelatedElementPointerList allElements = getRelatedElements(AlephERP::Document);
    foreach (RelatedElementPointer element, allElements)
    {
        if ( !element.isNull() )
        {
            if ( element->document() != NULL && element->document()->id() == id )
            {
                elements.append(element);
            }
        }
    }
    return elements;
}
#endif

int BaseBean::countRelatedElementsByCategory(const QString &category, AlephERP::RelatedElementTypes type, AlephERP::RelatedElementCardinalities cardinality, bool includeToBeDelete)
{
    int count = 0;
    RelatedElementPointerList allElements = getRelatedElements(type, cardinality, includeToBeDelete);
    foreach (RelatedElementPointer element, allElements)
    {
        if ( !element.isNull() )
        {
            if ( element->categories().contains(category) )
            {
                count++;
            }
        }
    }
    return count;
}

int BaseBean::countRelatedElementsByRelatedTableName(const QString &tableName, AlephERP::RelatedElementCardinalities cardinality, bool includeToBeDelete)
{
    int count = 0;
    RelatedElementPointerList allElements = getRelatedElements(AlephERP::Record, cardinality, includeToBeDelete);
    foreach (RelatedElementPointer element, allElements)
    {
        if ( !element.isNull() )
        {
            if ( !element->relatedBean().isNull() && element->relatedBean()->metadata()->tableName() == tableName )
            {
                count++;
            }
        }
    }
    return count;
}

int BaseBean::countRelatedElementsByCategoryAndRelatedTableName(const QString &tableName, const QString &category, AlephERP::RelatedElementCardinalities cardinality, bool includeToBeDelete)
{
    int count = 0;
    RelatedElementPointerList allElements = getRelatedElements(AlephERP::Record, cardinality, includeToBeDelete);
    foreach (RelatedElementPointer element, allElements)
    {
        if ( !element.isNull() )
        {
            if ( !element->relatedBean().isNull() && element->relatedBean()->metadata()->tableName() == tableName && element->categories().contains(category) )
            {
                count++;
            }
        }
    }
    return count;
}

int BaseBean::countRelatedElementsByCategoriesAndRelatedTableNames(const QStringList &tableNames, const QStringList &categories, AlephERP::RelatedElementCardinalities cardinality, bool includeToBeDelete)
{
    int count = 0;
    RelatedElementPointerList allElements = getRelatedElements(AlephERP::Record, cardinality, includeToBeDelete);
    foreach (RelatedElementPointer element, allElements)
    {
        if ( !element.isNull() )
        {
            if ( !element->relatedBean().isNull() && tableNames.contains(element->relatedBean()->metadata()->tableName()) && element->hasCategories(categories) )
            {
                count++;
            }
        }
    }
    return count;
}

/**
 * @brief BaseBean::deleteRelatedElement
 * Borrar la relación con el elemento relacionado, pero también el propio elemento relacionado (si es un registro, borra ese registro).
 * @param element
 */
void BaseBean::deleteRelatedElement(RelatedElementPointer element)
{
    d->m->deletedRelatedElementScriptExecute(this, element);
    if ( element->state() == RelatedElement::UPDATE )
    {
        element->setState(RelatedElement::TO_BE_DELETED);
    }
    else
    {
        d->removeRelatedElement(element);
    }
    if ( !element->relatedBean().isNull() && element->relatedBean()->dbState() == BaseBean::INSERT )
    {
        // Con esta llamada, vamos a fozar a borrar a él, y sus hijos en cascada que se hubiesen creado.
        element->relatedBean()->setDbState(BaseBean::TO_BE_DELETED);
        AERPTransactionContext::instance()->discardFromContext(element->relatedBean());
    }
    d->emitRelatedElementDeleted(element);
    setRelatedElementsModified();
    setModified();
}

bool BaseBean::relatedElementsLoaded()
{
    return d->m_relatedElementsLoaded;
}

int BaseBean::countRelatedElements(AlephERP::RelatedElementTypes type, AlephERP::RelatedElementCardinalities cardinality, bool includeToBeDelete)
{
    QMutexLocker lock(&d->m_mutex);
    if ( !d->m_relatedElementsLoaded )
    {
        if ( d->m_relatedElementsCount.isEmpty() )
        {
            d->m_relatedElementsCount = RelatedDAO::countRelatedElements(this, cardinality);
            if ( d->m_relatedElementsCount.contains(type) )
            {
                return d->m_relatedElementsCount.value(type);
            }
        }
    }
    RelatedElementPointerList elements = getRelatedElements(type, cardinality, includeToBeDelete);
    return elements.size();
}

void BaseBean::deleteRelatedElementByCategory(const QString &category, AlephERP::RelatedElementTypes type, AlephERP::RelatedElementCardinalities cardinality)
{
    RelatedElementPointerList allElements = getRelatedElements(type, cardinality, false);
    foreach (RelatedElementPointer element, allElements)
    {
        if ( !element.isNull() )
        {
            if ( element->categories().contains(category) && cardinality.testFlag(element->cardinality()) )
            {
                deleteRelatedElement(element);
            }
        }
    }
}

/**
 * @brief BaseBean::addRelatedElement
 * Esta función sólo estará accesible para ciertas funciones.
 * @param element
 */
void BaseBean::addRelatedElement(RelatedElement *element)
{
    QMutexLocker lock(&d->m_mutex);
    // Como política, si los elementos no están cargados, aquí se cargan
    if ( dbState() != BaseBean::INSERT )
    {
        if ( !d->m_relatedElementsLoaded && !d->m_loadingRelatedElements )
        {
            d->m_loadingRelatedElements = true;
            RelatedDAO::loadRelatedElements(this);
            d->m_relatedElementsLoaded = true;
            d->m_loadingRelatedElements = false;
        }
    }
    element->setParent(this);
    element->setRootBean(this);
    d->m_relatedElements.append(element);
    if ( d->m_relatedElementsCount.contains(element->type()) )
    {
        d->m_relatedElementsCount[element->type()] = d->m_relatedElementsCount[element->type()] + 1;
    }
    else
    {
        d->m_relatedElementsCount[element->type()] = 1;
    }
}

QDateTime BaseBean::loadTime() const
{
    return d->m_loadTime;
}

bool BaseBean::readOnly()
{
    if ( d->m_readOnly )
    {
        return true;
    }
    if ( !d->m->readOnlyScript().isEmpty() )
    {
        return d->m->readOnlyScriptExecute(this);
    }
    DBField *fldEditable = field(AlephERP::stFieldEditable);
    if ( fldEditable != NULL )
    {
        return !fldEditable->value().toBool();
    }
    return false;
}

bool BaseBean::setReadOnly(bool value)
{
    QMutexLocker lock(&d->m_mutex);
    bool previousReadOnly = readOnly();
    d->m_readOnly = value;
    DBField *fldEditable = field(AlephERP::stFieldEditable);
    if ( fldEditable != NULL )
    {
        fldEditable->setInternalValue(!value);
    }
    return previousReadOnly;
}

bool BaseBean::calculatedFieldsEnabled() const
{
    return d->m_calculateFieldsEnabled;
}

void BaseBean::setLoadTime(const QDateTime &time)
{
    QMutexLocker lock(&d->m_mutex);
    d->m_loadTime = time;
}

/*!
  Limpia toda la estructura de datos de este bean. OJO: Si hay hijos o padres con datos pendientes de guardar,
  estos se perderán. Si hay fields modificados, estos se perderán también
  @params onlyFields Excluyente, si está a true, nunca limpiará hijos ni padres, aunque children o fathers esté a true
  @params children Incluye en el limpiado el borrado de los hijos
  @params fathers Incluye en el limpiado el borrado de los padres.
  */
void BaseBean::clean(bool onlyFields, bool children, bool fathers)
{
    QMutexLocker lock(&d->m_mutex);
    foreach (DBField *fld, d->m_fields)
    {
        bool blockState = fld->blockSignals(true);
        fld->setValue(QVariant());
        fld->blockSignals(blockState);
        fld->setModified(false);
    }
    if ( onlyFields )
    {
        d->m_modified = false;
        return;
    }
    if ( children )
    {
        foreach (DBRelation *rel, d->m_relations)
        {
            if ( rel->metadata()->type() == DBRelationMetadata::ONE_TO_MANY || rel->metadata()->type() == DBRelationMetadata::ONE_TO_ONE )
            {
                rel->removeAllChildren();
            }
        }
    }
    if ( fathers )
    {
        foreach (DBRelation *rel, d->m_relations)
        {
            if ( rel->metadata()->type() == DBRelationMetadata::MANY_TO_ONE  )
            {
                rel->father()->clean(true);
            }
        }
    }
    d->m_modified = false;
    d->m_relatedElementsModified = false;
}

/**
 * @brief BaseBean::reloadFromDb
 * El bean se recarga desde base de datos, sobreescribiendo los posibles cambios que se hubiesen podido hacer
 * sobre el mismo, y quedándose el bean como modified = false. Es necesario que esté en estado UPDATE.
 * Devuelve true si todo ha ido correctamente
 */
bool BaseBean::reloadFromDb()
{
    QMutexLocker lock(&d->m_mutex);
    if ( dbState() == BaseBean::UPDATE )
    {
        if ( !BaseDAO::selectByPk(pkValue(), this) )
        {
            QLogger::QLog_Error(AlephERP::stLogDB, QString("BaseBean::reloadFromDb: No se pudo recargar. Error: [%1]").arg(BaseDAO::lastErrorMessage()));
            return false;
        }
    }
    return true;
}

/**
 * @brief BaseBean::reloadFromLinkedBean
 * El registro es un registro de una vista. Podemos recargar su valor según el registro linkado.
 * @return
 */
bool BaseBean::reloadFromLinkedBean()
{
    QMutexLocker lock(&d->m_mutex);
    if ( d->m_viewLinkedBean.isNull() )
    {
        return reloadFromDb();
    }
    else
    {
        bool blockSignalsState = blockSignals(true);
        setDbOid(d->m_viewLinkedBean->dbOid());
        setPkValue(d->m_viewLinkedBean->pkValue());
        copyValues(d->m_viewLinkedBean);
        setDbState(BaseBean::UPDATE);
        blockSignals(blockSignalsState);
        return reloadFromDb();
    }
}

/**
 * @brief BaseBean::copyValues
 * Permite al programador QS la inicialización de datos, a partir de los datos de otro bean. Puede invocarse así
 * nuevoBean.copyValues(otherBean)
 * nuevoBean.copyValues(otherBean, "field1", "field2" ...)
 * @return
 */
QScriptValue BaseBean::copyValues()
{
    if ( context() == NULL )
    {
        return QScriptValue(QScriptValue::UndefinedValue);
    }
    // Debe tener al menos un argumento: el bean original. Tras eso, el listado de fields, si aplica
    if ( context()->argumentCount() < 1 )
    {
        return QScriptValue(false);
    }
    BaseBeanPointer originalBean = qobject_cast<BaseBean *> (context()->argument(0).toQObject());

    if ( originalBean.isNull() )
    {
        return QScriptValue(false);
    }
    if (context()->argumentCount() == 1)
    {
        copyValues(originalBean);
        return QScriptValue(true);
    }
    QStringList fields;
    for ( int i = 1; i < context()->argumentCount(); ++i )
    {
        fields.append(context()->argument(i).toString());
    }
    copyValues(originalBean, fields);
    return QScriptValue(true);
}

/**
 * @brief BaseBean::copyRelationChildren
 * Permite replicar la estructura de childs de relaciones. Puede invocarse
 * nuevoBean.copyRelationChildren(otherBean);
 * nuevoBean.copyRelationChildren(otherBean, "relation1", "relation2");
 * @return
 */
QScriptValue BaseBean::copyRelationChildren()
{
    // Debe tener al menos un argumento: el bean original. Tras eso, el listado de fields, si aplica
    if ( context()->argumentCount() < 1 )
    {
        return QScriptValue(false);
    }
    BaseBeanPointer originalBean = qobject_cast<BaseBean *> (context()->argument(0).toQObject());

    if ( originalBean.isNull() )
    {
        return QScriptValue(false);
    }
    QStringList relationNames;
    if (context()->argumentCount() == 1)
    {
        foreach (DBRelation *rel, relations(AlephERP::OneToMany | AlephERP::OneToOne))
        {
            relationNames.append(rel->metadata()->name());
        }
    }
    else
    {
        for ( int i = 1; i < context()->argumentCount(); ++i )
        {
            if ( context()->argument(i).isArray() )
            {
                QScriptValue arrayArg = context()->argument(i);
                for ( int idx = 0; idx < arrayArg.property("length").toInt32() ; ++idx )
                {
                    relationNames.append(arrayArg.property(idx).toString());
                }
            }
            else
            {
                relationNames.append(context()->argument(i).toString());
            }
        }
    }
    copyRelationChildren(originalBean, relationNames);
    return QScriptValue(true);
}

QScriptValue BaseBean::deepCopyValues()
{
    // Debe tener al menos un argumento: el bean original. Tras eso, el listado de fields, si aplica
    if ( context()->argumentCount() < 1 )
    {
        return QScriptValue(false);
    }

    BaseBeanPointer originalBean = qobject_cast<BaseBean *> (context()->argument(0).toQObject());
    if ( originalBean.isNull() )
    {
        return QScriptValue(false);
    }

    QStringList relationNames;
    if (context()->argumentCount() == 1)
    {
        foreach (DBRelation *rel, relations(AlephERP::OneToMany | AlephERP::OneToOne))
        {
            relationNames.append(rel->metadata()->name());
        }
    }
    else
    {
        for ( int i = 1; i < context()->argumentCount(); ++i )
        {
            if ( context()->argument(i).isArray() )
            {
                QScriptValue arrayArg = context()->argument(i);
                for ( int idx = 0; idx < arrayArg.property("length").toInt32() ; ++idx )
                {
                    relationNames.append(arrayArg.property(idx).toString());
                }
            }
            else
            {
                relationNames.append(context()->argument(i).toString());
            }
        }
    }
    deepCopyValues(originalBean, relationNames);
    return QScriptValue(true);
}

/**
 * @brief BaseBean::touch
 * Función específica para el entorno QScript. Pone el bean como "modified" de cara a su posible almacenamiento posterior
 */
void BaseBean::touch()
{
    setModified();
}

/**
 * @brief BaseBean::registerRecalculateFields
 * El sistema intenta detectar los campos calculados que se deben recalcular tras la modificación de un usuario.
 * Pero a veces no es capaz de detectarlos todos (por ejemplo, en un script donde haya varios if). El programador
 * QS puede establecerlos desde aquí, llamando así
 *
 * bean.registerRecalculateFields("fieldARecalcular", "fieldInvolucradoEnCalculo1", "fieldInvolucradoEnCalculo2", ...);
 */
void BaseBean::registerRecalculateFields()
{
    if ( engine() == NULL || context() == NULL )
    {
        return;
    }
    if ( context()->argumentCount() < 2 )
    {
        return;
    }
    QString fieldToRecalc = context()->argument(0).toString();
    QStringList fieldsOnCalc;
    for (int i = 1 ; i < context()->argumentCount() ; i++)
    {
        QString fieldOnCalc = context()->argument(i).toString();
        if ( !fieldsOnCalc.contains(fieldOnCalc) )
        {
            fieldsOnCalc.append(fieldOnCalc);
        }
    }
    if ( fieldsOnCalc.size() > 0 )
    {
        d->m->registerRecalculateFields(fieldToRecalc, fieldsOnCalc);
    }
    makeCalculatedFieldsConnections(fieldToRecalc, fieldsOnCalc);
}

void BaseBean::setValuesFromFilter(const QString &filter)
{
    if ( filter.toLower().contains(QStringLiteral(" or ")) )
    {
        return;
    }
    QStringList parts = filter.split(QRegExp(" (AND|and) "));
    foreach (const QString &part, parts)
    {
        QStringList subparts = part.split("=");
        if ( subparts.size() == 2 )
        {
            DBField *fld = field(subparts.at(0).trimmed());
            if ( fld != NULL )
            {
                fld->setValue(subparts.at(1).trimmed());
            }
        }
    }
}

void BaseBean::setValuesFromFilter(const QVariantMap &filter)
{
    QMapIterator<QString, QVariant> it (filter);
    while ( it.hasNext() )
    {
        it.next();
        if ( it.key() == "filter" )
        {
            setValuesFromFilter(it.value().toString());
        }
        else
        {
            setFieldValue(it.key(), it.value());
        }
    }
}

/**
 * @brief BaseBean::duplicate
 * Para el entorno QS, realiza un duplicado del registro.
 * @return
 */
QScriptValue BaseBean::duplicate()
{
    QMutexLocker lock(&d->m_mutex);
    QScriptValue result (QScriptValue::UndefinedValue);
    if ( engine() != NULL )
    {
        BaseBean *copyBean = BeansFactory::instance()->newBaseBean(d->m->tableName());
        d->copy(copyBean);
        // Los campos seriales se resetean
        foreach (DBField *fld, copyBean->fields())
        {
            if ( fld->metadata()->serial() )
            {
                fld->setInternalValue(0);
            }
        }
        copyBean->setDbState(BaseBean::INSERT);
        copyBean->setDbOid(0);
        result = engine()->newQObject(copyBean, QScriptEngine::ScriptOwnership, QScriptEngine::PreferExistingWrapperObject);
    }
    return result;
}

/*!
  Devuelve un objeto idéntico a este. Pero no realiza ningún tipo de acción sobre contexto ni nada parecido.
  */
BaseBeanSharedPointer BaseBean::clone()
{
    QMutexLocker lock(&d->m_mutex);
    if ( d->m->tableName().isEmpty() )
    {
        return BaseBeanSharedPointer();
    }
    BaseBeanSharedPointer copyBean = BeansFactory::instance()->newQBaseBean(d->m->tableName(), false);
    d->copy(copyBean.data());
    setReadOnly(readOnly());
    return copyBean;
}

BaseBeanPointer BaseBean::clone(QObject *parent)
{
    if ( d->m->tableName().isEmpty() )
    {
        return NULL;
    }
    BaseBeanPointer copyBean = BeansFactory::instance()->newBaseBean(d->m->tableName(), false);
    copyBean->setParent(parent);
    d->copy(copyBean.data());
    setReadOnly(readOnly());
    return copyBean;
}

void BaseBean::setDefaultValues(BaseBeanPointerList fathers)
{
    d->setDefaultValues(fathers);
}

void BaseBeanPrivate::copy(BaseBean *destCopyBean)
{
    QMutexLocker lock(&m_mutex);
    // Bloqueamos todas las señales, ya que el clonado debería ser una operación que no afectase
    // al bean principal. La emisión de señales puede poner en modified el bean.
    bool blockState = q_ptr->blockAllSignals(true);

    // Importante hacer este access aquí!!
    destCopyBean->setAccess(m_access);

    foreach (DBField *fld, m_fields)
    {
        DBField *fldCopy = destCopyBean->field(fld->dbFieldName());
        // Al hacer la copia, tenemos en cuenta si es un campo memo y no se ha obtenido, para no obtenerlo
        if ( fldCopy != NULL )
        {
            bool fldBlockState = fldCopy->blockSignals(true);
            if ( fld->metadata()->type() == QVariant::String || fld->metadata()->type() == QVariant::Pixmap )
            {
                if ( fld->metadata()->memo() )
                {
                    if ( fld->memoLoaded() )
                    {
                        fldCopy->setInternalValue(fld->rawValue());
                        fldCopy->setMemoLoaded(true);
                    }
                }
                else
                {
                    fldCopy->setInternalValue(fld->rawValue());
                }
            }
            else
            {
                fldCopy->setInternalValue(fld->rawValue());
            }
            fldCopy->blockSignals(fldBlockState);
        }
    }
    destCopyBean->setDbOid(q_ptr->dbOid());

    bool blockSignalsState = destCopyBean->blockSignals(true);
    destCopyBean->setDbState(q_ptr->dbState());
    if ( q_ptr->modified() )
    {
        destCopyBean->setModified();
    }
    else
    {
        destCopyBean->uncheckModifiedFields();
    }
    destCopyBean->blockSignals(blockSignalsState);

    q_ptr->blockAllSignals(blockState);
}

void BaseBeanPrivate::removeRelatedElement(RelatedElementPointer element)
{
    QMutexLocker lock(&m_mutex);
    for(int i = 0; i < m_relatedElements.size() ; i++)
    {
        if ( m_relatedElements.at(i)->objectName() == element->objectName() )
        {
            m_relatedElements.removeAt(i);
            return;
        }
    }
}

/*!
  Función util para profiling. Vamos a ver cuántos beans genera la aplicación, cuántos libera...
  */
int BaseBean::countBeans()
{
    return BaseBean::m_beansCount;
}

int BaseBean::maxCountBeans()
{
    return BaseBean::m_maxBeansCount;
}

/**
 * @brief BaseBean::blockAllSignals
 * Bloquea las señales de este bean, así como de las relaciones hijas y de los fields
 * @param value
 */
bool BaseBean::blockAllSignals(bool value)
{
    QMutexLocker lock(&d->m_mutex);
    // Evitemos recursividad
    bool previuos = d->m_allSignalsBlocked;
    d->m_allSignalsBlocked = value;
    if ( !d->m_isBlockingSignals )
    {
        d->m_isBlockingSignals = true;
        blockSignals(value);
        foreach (DBRelation *rel, d->m_relations)
        {
            rel->blockAllSignals(value);
        }
        foreach (DBField *fld, d->m_fields)
        {
            fld->blockSignals(value);
        }
        d->m_isBlockingSignals = false;
    }
    return previuos;
}

bool BaseBean::allSignalsBlocked() const
{
    return d->m_allSignalsBlocked;
}

DBField *BaseBean::fieldForRole(int role)
{
    foreach (DBField *f, d->m_fields)
    {
        if ( role == Qxt::ItemStartTimeRole )
        {
            if ( f->metadata()->scheduleStartTime() )
            {
                return f;
            }
        }
        if ( role == Qxt::ItemDurationRole )
        {
            if ( f->metadata()->scheduleDuration() )
            {
                return f;
            }
        }
    }
    return NULL;
}

QString BaseBean::scheduleTitle()
{
    return d->m->calculateScheduleTitleExpression(this);
}

void BaseBean::resetToDefaultValues()
{
    QMutexLocker lock(&d->m_mutex);
    clean();
    d->setDefaultValues();
    d->m->onCreateScriptExecute(this);
    setSerialUniqueId();
}

/**
 * @brief BaseBean::setViewLinkedBean
 * Este bean es un registro de una vista de base de datos. Por tanto, tiene que estar enlazado
 * a un bean de una tabla. Aquí se establece esa relación
 * @param bean
 */
void BaseBean::setViewLinkedBean(BaseBeanPointer bean)
{
    if ( bean.isNull() )
    {
        return;
    }
    QMutexLocker lock(&d->m_mutex);
    d->m_viewLinkedBean = BaseBeanPointer (bean);
    if ( !d->m_viewLinkedBean.isNull() )
    {
        connect (d->m_viewLinkedBean.data(), SIGNAL(beanCommitted()), this, SLOT(reloadFromDb()));
    }
}

/**
 * @brief BaseBean::copyValues
 * Esta es una función muy útil al motor QtScript, ya que permite copiar los valores de los dbfields que tienen el mismo
 * nombre (y mismo tipo) de otro bean de otra tabla. Esto permite una fácil replicación de datos
 * @param otherBean Bean DESDE el que se quieren copiar los datos
 */
void BaseBean::copyValues(BaseBeanPointer otherBean, bool saveValues)
{
    if ( otherBean.isNull() )
    {
        return;
    }
    if ( saveValues )
    {
        backupValues();
    }
    QStringList fieldNames;
    foreach (DBField *fld, otherBean->fields())
    {
        fieldNames << fld->metadata()->dbFieldName();
    }
    copyValues(otherBean, fieldNames);
}

/**
 * @brief BaseBean::copyValues
 * Igual que copyValues, pero sólo se copian los fields que se han incluído en fields
 * @param otherBean Bean DESDE el que se quieren copiar los datos
 * @param fields Fields a copiar
 */
void BaseBean::copyValues(BaseBeanPointer otherBean, const QStringList &fields)
{
    QMutexLocker lock(&d->m_mutex);
    if ( otherBean.isNull() )
    {
        return;
    }
    foreach ( DBField *fld, d->m_fields )
    {
        if ( fields.contains(fld->metadata()->dbFieldName()) )
        {
            DBField *otherFld = otherBean->field(fld->metadata()->dbFieldName());
            if ( otherFld != NULL && otherFld->metadata()->type() == fld->metadata()->type() &&
                    !fld->metadata()->serial() &&
                    otherFld->metadata()->isOnDb() )
            {
                fld->setValue(otherFld->value());
            }
        }
    }
    bool previousState = blockAllSignals(true);
    recalculateCalculatedFields();
    blockAllSignals(previousState);
}

/**
 * @brief BaseBean::copyRelationChildren
 * Permite la copia de la estructura de childs hijos de las relaciones pasadas en \relationNames
 * @param otherBean
 * @param relationNames
 */
void BaseBean::copyRelationChildren(BaseBeanPointer otherBean, QStringList &relationNames)
{
    QMutexLocker lock(&d->m_mutex);
    foreach (const QString &relationName, relationNames)
    {
        DBRelation *otherRel = otherBean->relation(relationName);
        DBRelation *thisRel = relation(relationName);
        if (otherRel != NULL && thisRel != NULL)
        {
            foreach (BaseBeanPointer child, otherRel->children("", false))
            {
                if (!child.isNull())
                {
                    BaseBeanSharedPointer newBean = thisRel->newChild();
                    newBean->copyValues(child);
                }
            }
        }
    }
    bool previousState = blockAllSignals(true);
    recalculateCalculatedFields();
    blockAllSignals(previousState);
}

/**
 * @brief BaseBean::deepCopyValues
 * Realiza una copia "profunda" con los hijos de relaciones incluídos.
 * @param otherBean
 * @param relationNames
 */
void BaseBean::deepCopyValues(BaseBeanPointer otherBean, const QStringList &relationNames)
{
    QStringList definitiveRelationsNames;
    if ( relationNames.isEmpty() )
    {
        foreach (DBRelation *rel, relations(AlephERP::OneToMany | AlephERP::OneToOne))
        {
            definitiveRelationsNames.append(rel->metadata()->name());
        }
    }
    else
    {
        definitiveRelationsNames = relationNames;
    }

    copyValues(otherBean);

    foreach (const QString &relationName, definitiveRelationsNames)
    {
        DBRelation *otherRel = otherBean->relation(relationName);
        DBRelation *thisRel = relation(relationName);
        if (otherRel != NULL && thisRel != NULL && thisRel->metadata()->type() == DBRelationMetadata::ONE_TO_MANY)
        {
            foreach (BaseBeanPointer child, otherRel->children("", false))
            {
                if (!child.isNull())
                {
                    BaseBeanSharedPointer newBean = thisRel->newChild();
                    // Importante no incluir en la lista de valores a copiar el del idservicio
                    QStringList fieldNames;
                    foreach (DBField *fld, newBean->fields())
                    {
                        if ( fld->metadata()->dbFieldName() != thisRel->metadata()->childFieldName() )
                        {
                            fieldNames << fld->metadata()->dbFieldName();
                        }
                    }
                    newBean->copyValues(child, fieldNames);
                    newBean->deepCopyValues(child, relationNames);
                }
            }
        }
    }
    bool previousState = blockAllSignals(true);
    recalculateCalculatedFields();
    blockAllSignals(previousState);
}

/**
 * @brief BaseBean::hash Devuelve un código HASH (SHA-1) que identifica unívocamente al registro, y permite conocer
 * si se modificó. Muy útil por ejemplo, para el modo de trabajo local. En el hash sólo se tienen en cuenta aquellos
 * fields que se almacenan en base de datos (ya que los calculados vendrán de una relación entre estos)
 * @return
 */
QString BaseBean::hash(bool useRawValue)
{
    QString totalData;
    foreach ( DBField *fld, d->m_fields )
    {
        if ( fld->metadata()->isOnDb() )
        {
            totalData = QString("%1%2;").arg(totalData).arg(fld->sqlValue(true, "", useRawValue));
        }
    }
    QByteArray ba = QCryptographicHash::hash(totalData.toUtf8(), QCryptographicHash::Sha1);
    QString result = ba.toHex();
    return result;
}

/**
 * @brief BaseBean::rawHash
 * @return
 * Esta función es análoga a la anterior \a hash pero utiliza valores sin procesar
 * y no fuerza a obtener ningún campo (como los memo que se solicitan bajo demanda)
 */
QString BaseBean::rawHash()
{
    QString totalData;
    foreach ( DBField *fld, d->m_fields )
    {
        if ( fld->metadata()->isOnDb() )
        {
            totalData = QString("%1%2;").arg(totalData).arg(fld->metadata()->sqlValue(fld->rawValue()));
        }
    }
    QByteArray ba = QCryptographicHash::hash(totalData.toUtf8(), QCryptographicHash::Sha1);
    QString result = ba.toHex();
    return result;
}

/**
 * @brief BaseBean::emailTemplate
 * Crea un correo electrónico con los datos del bean, si se ha creado el script correspondiente.
 * @return
 */
QString BaseBean::emailTemplate() const
{
    return d->m->emailTemplateScriptExecute(const_cast<BaseBean *>(this));
}

/**
 * @brief BaseBean::emailSubject
 * Devuelve el asunto de un correo electrónico a partir de los datos de este bean.
 * @return
 */
QString BaseBean::emailSubject() const
{
    return d->m->emailSubjectScriptExecute(const_cast<BaseBean *>(this));
}

QString BaseBean::repositoryPath() const
{
    return d->m->repositoryPathScriptExecute(const_cast<BaseBean *>(this));
}

QStringList BaseBean::repositoryKeywords() const
{
    return d->m->repositoryKeywordsScriptExecute(const_cast<BaseBean *>(this));
}

/**
 * @brief BaseBean::fieldCounter
 * Devuelve fields que son campos calculados, y en los que está involucrado en el cálculo @param fld
 * @param field
 * @return
 */
QList<DBField *> BaseBean::counterFields(const QString &fld)
{
    QList<DBFieldMetadata *> mList = d->m->counterFields(fld);
    QList<DBField *> list;
    foreach (DBFieldMetadata *mFld, mList)
    {
        list << field(mFld->dbFieldName());
    }
    return list;
}

/**
  Las modificaciones que se produzcan en un field que interviene en el cálculo de un campo
  calculado, deben ser tenidas en cuenta. Esto se hace a través de conexiones
*/
void BaseBeanPrivate::connectCounterFields()
{
    foreach ( DBField *fld, m_fields )
    {
        if ( fld->metadata()->hasCounterDefinition() )
        {
            QStringList fields, prefixs;
            fields = fld->metadata()->counterDefinition()->fields;
            prefixs = fld->metadata()->counterDefinition()->prefixOnRelations;
            if ( fields.size() > 0 )
            {
                for ( int i = 0 ; i < fields.size() ; i++ )
                {
                    DBField *otherField = q_ptr->field(fields.at(i));
                    if ( otherField != NULL )
                    {
                        QObject::connect(otherField, SIGNAL(valueModified(QVariant)), fld, SLOT(recalculateCounterField()));
                        if ( prefixs.size() > i && !prefixs.at(i).isEmpty() )
                        {
                            QList<DBRelation *> rels = otherField->relations(AlephERP::ManyToOne);
                            foreach ( DBRelation *rel, rels )
                            {
                                BaseBean *father = rel->father(false);
                                if ( father != NULL )
                                {
                                    DBField *remoteField = father->field(prefixs.at(i));
                                    if ( remoteField != NULL )
                                    {
                                        QObject::connect(remoteField, SIGNAL(valueModified(QVariant)), fld, SLOT(recalculateCounterField()));
                                    }
                                }
                            }
                        }
                    }
                }
            }
            // Tenemos que inferir los campos de la expresión
            else if ( !fld->metadata()->counterDefinition()->expression.isEmpty() )
            {
                StringExpression stringExpression;
                stringExpression.expression = fld->metadata()->counterDefinition()->expression;
                // Primero generamos la expresión sin el valor del contador (por si hay que aplicar trailing zeros)
                QList<DBField *> flds = stringExpression.fieldsInvolvedOnCalc(q_ptr);
                foreach (DBField *remoteField, flds)
                {
                    QObject::connect(remoteField, SIGNAL(valueModified(QVariant)), fld, SLOT(recalculateCounterField()));
                }
            }
        }
    }
}

/**
 * @brief BaseBeanPrivate::connectAggregateFields
 * Hay campos que presentan unas operaciones agregadas a modo de suma de registros hijos. A esto los afecta sobremanera
 * el que se inserten nuevos registros o modifiquen. Aquí se producen las conexiones con las relaciones
 */
void BaseBeanPrivate::connectAggregateFields()
{
    foreach ( DBField *fld, m_fields )
    {
        AggregateCalc calc = fld->metadata()->aggregateCalc();
        for ( int i = 0 ; i < calc.relation.size() ; i++ )
        {
            DBRelation *rel = q_ptr->relation(calc.relation.at(i));
            if ( rel != NULL )
            {
                QObject::connect(rel, SIGNAL(childDbStateModified(BaseBean*,int)), fld, SLOT(recalculate()));
                QObject::connect(rel, SIGNAL(childDeleted(BaseBean*,int)), fld, SLOT(recalculate()));
                QObject::connect(rel, SIGNAL(childInserted(BaseBean*,int)), fld, SLOT(recalculate()));
                QObject::connect(rel, SIGNAL(childModified(BaseBean*,bool)), fld, SLOT(recalculate()));
            }
        }
    }
}

/**
 * @brief BaseBean::cleanToBeDeletedRelatedElements
 * Esta función será llamada por RelatedDAO, cuando se hayan almacenado los elementos relacionados en base de datos
 * para eliminar aquellos marcados para ser borrados
 */
void BaseBean::cleanToBeDeletedRelatedElements()
{
    QMutexLocker lock(&d->m_mutex);
    for ( int i = 0 ; i < d->m_relatedElements.size() ; i++ )
    {
        if ( d->m_relatedElements.at(i)->state() == RelatedElement::TO_BE_DELETED )
        {
            d->m_relatedElements.removeAt(i);
        }
    }
}

/**
 * @brief BaseBean::adjustOldValues
 * Este slot se llamará cuando el bean se haya guardado definitivamente en base de datos, y haya que guardar
 * los nuevos valores guardados como oldValues
 */
void BaseBean::adjustOldValues()
{
    foreach (DBField *fld, d->m_fields)
    {
        fld->adjustOldValue();
    }
}

/**
 * @brief BaseBean::prepareToDelete
 * Puede ser necesario realizar algunas actividades antes de borrar un registro... Como por ejemplo, para eliminar
 * el contenido de ficheros remotos, o eliminar registros relacionados que así se han configurado en los metadatos.
 */
void BaseBean::prepareToDeleteRelatedElements()
{
    if ( d->m->relatedElementsContentToBeDelete().size() > 0 )
    {
        foreach (const AlephERP::RelatedElementsContentToBeDeleted &item, d->m->relatedElementsContentToBeDelete() )
        {
            RelatedElementPointerList itemsToBeDeleted;
            if ( item.category.isEmpty() || item.category == "*" )
            {
                itemsToBeDeleted = getRelatedElements(item.type, item.cardinality, true);
            }
            else
            {
                itemsToBeDeleted = getRelatedElementsByCategory(item.category, item.type, item.cardinality, true);
            }
            foreach (RelatedElementPointer element, itemsToBeDeleted)
            {
                if ( item.type == AlephERP::NoneAll || item.type == element->type() )
                {
                    if ( element->type() == AlephERP::Record )
                    {
                        if ( item.tableName == "*" || item.tableName == element->relatedTableName() )
                        {
                            element->relatedBean()->setDbState(BaseBean::TO_BE_DELETED);
                            element->relatedBean()->prepareToDeleteRelatedElements();
                        }
                    }
                }
            }
        }
    }
}

/**
 * @brief BaseBean::removeConfiguredRelatedElements
 * Elimina los elementos relacionados que se han configurado en los metadatos con las entradas
 * <relatedElementsContentToBeDelete>
 * TODO: ESTA ENTRADA PUEDE ENTRAR EN CONFLICTO O ESTAR DUPLICADA CON LA LLAMADA prepareToDeleteRelatedElements QUE SE HACE DESDE AERPTRANSACTIONCONTEXT
 */
bool BaseBean::removeConfiguredRelatedElements(const QString &idTransaction)
{
    if ( d->m->relatedElementsContentToBeDelete().size() > 0 )
    {
        foreach (const AlephERP::RelatedElementsContentToBeDeleted &item, d->m->relatedElementsContentToBeDelete() )
        {
            RelatedElementPointerList itemsToBeDeleted;
            if ( item.category.isEmpty() || item.category == "*" )
            {
                itemsToBeDeleted = getRelatedElements(item.type, item.cardinality, true);
            }
            else
            {
                itemsToBeDeleted = getRelatedElementsByCategory(item.category, item.type, item.cardinality, true);
            }
            foreach (RelatedElementPointer element, itemsToBeDeleted)
            {
                if ( item.type == AlephERP::NoneAll || item.type == element->type() )
                {
                    if ( element->type() == AlephERP::Record )
                    {
                        if ( item.tableName == "*" || item.tableName == element->relatedTableName() )
                        {
                            if (!BaseDAO::remove(element->relatedBean(), idTransaction))
                            {
                                return false;
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}

