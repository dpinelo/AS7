/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo   *
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
#include "aerptransactioncontext.h"
#include "configuracion.h"
#include "qlogger.h"
#include "globales.h"
#include "dao/beans/basebean.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/relatedelement.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/basedao.h"
#include "dao/database.h"
#include "dao/beans/beansfactory.h"
#include "business/graph.h"

QThreadStorage<AERPTransactionContext *> threadContextData;

class BeansReferenceList
{
private:
    /** Utilizamos WeakPointers, ya que éste tipo de template NO borra el bean. Y no podemos permitir que estas
     * funciones borren los beans, una vez que han trabajado con ellas */
    QList<BaseBeanPointer> m_list;
    QStringList m_beforeSqls;
    QStringList m_afterSqls;

public:

    BeansReferenceList()
    {
    }

    ~BeansReferenceList()
    {
        m_list.clear();
    }

    void addPreviousSql(const QString &sql)
    {
        m_beforeSqls.append(sql);
    }

    void addFinalSql(const QString &sql)
    {
        m_afterSqls.append(sql);
    }

    void addBean(const BaseBeanPointer &bean)
    {
        foreach (BaseBeanPointer b, m_list)
        {
            if ( b.isNull() )
            {
                m_list.removeAll(b);
            }
        }
        if ( !bean.isNull() )
        {
            if ( m_list.contains(bean) )
            {
                return;
            }
            foreach (BaseBeanPointer b, m_list)
            {
                if ( !b.isNull() && b->objectName() == bean->objectName() )
                {
                    return;
                }
            }
            m_list.append(bean);
        }
    }

    bool removeBean(const BaseBeanPointer &bean)
    {
        foreach (BaseBeanPointer b, m_list)
        {
            if ( b.isNull() )
            {
                m_list.removeAll(b);
            }
        }
        if ( bean.isNull() )
        {
            return true;
        }
        int index = m_list.indexOf(bean);
        if ( index != -1 )
        {
            m_list.removeAt(index);
            return true;
        }
        else
        {
            for (int i = 0 ; i < m_list.size() ; i++)
            {
                if ( m_list.at(i).data()->objectName() == bean.data()->objectName() )
                {
                    m_list.removeAt(i);
                    return true;
                }
            }
        }
        return false;
    }

    QStringList previousSql() const
    {
        return m_beforeSqls;
    }

    QStringList finalsSql() const
    {
        return m_afterSqls;
    }

    QList<BaseBeanPointer> list() const
    {
        return m_list;
    }

    int size()
    {
        return m_list.size();
    }

    void clear()
    {
        m_list.clear();
        m_beforeSqls.clear();
        m_afterSqls.clear();
    }

    bool contains(const BaseBeanPointer &bean)
    {
        if ( bean.isNull() )
        {
            return false;
        }
        if ( m_list.contains(bean) )
        {
            return true;
        }
        foreach (BaseBeanPointer beanInList, m_list)
        {
            if ( !beanInList.isNull() && !bean.isNull() && beanInList.data()->objectName() == bean.data()->objectName() )
            {
                return true;
            }
        }
        return false;
    }
};

typedef struct DBFieldCounterStruct
{
    QPointer<DBField> fld;
    QVariant value;
} DBFieldCounter;

typedef QList<DBFieldCounter> DBFieldCounterList;

typedef struct BaseBeanStatePreviousCommitStruct
{
    BaseBeanPointer bean;
    BaseBean::DbBeanStates dbState;
    QHash<QString, bool> fieldsModifiedState;
} BaseBeanStatePreviousCommit;

class AERPTransactionContextPrivate
{
public:
    AERPTransactionContext *q_ptr;
    QHash<QString, BeansReferenceList*> m_contextObjects;
    QString m_lastError;
    QString m_database;
    QHash<QString, bool> m_cancel;
    QHash<QString, DBFieldCounterList> m_counterValues;
    QHash<QString, bool> m_doingCommit;
    // Antes de hacer un commit vamos a almacenar el dbState que tenían los beans, y así si hay algún tipo
    // de error se restaura. Lo mismo, para los modified.
    QList<BaseBeanStatePreviousCommit> m_beansStatePreviousCommit;

    AERPTransactionContextPrivate(AERPTransactionContext *qq) : q_ptr(qq)
    {
    }

    bool doPrepareTransaction(const QString &contextName);
    BaseBeanPointerList doOrderForTransaction(const QString &contextName);
    bool doValidationForTransaction(const BaseBeanPointerList &list);
    BaseBeanPointerList orderBeansForTransaction(const QString &contextName);
    void buildBeansStatePreviousCommit(const QList<BaseBeanPointer> &list);
};

AERPTransactionContext::AERPTransactionContext(QObject *parent) :
    QObject(parent),
    d(new AERPTransactionContextPrivate(this))
{
}

AERPTransactionContext::~AERPTransactionContext()
{
    delete d;
}

/**
 * @brief AERPTransactionContext::instance
 * Devuelve la instancia del objeto transacción.
 * @return
 */
AERPTransactionContext *AERPTransactionContext::instance()
{
    if ( !threadContextData.hasLocalData() )
    {
        AERPTransactionContext *context = new AERPTransactionContext();
        threadContextData.setLocalData(context);
    }
    return threadContextData.localData();
}

/**
 * @brief AERPTransactionContext::save
 * Esta función se invoca cuando se quiere que un bean se guarde, desde un conjunto de transacciones.
 * Por ejemplo, cuando se abre un formulario de edición de registros y todos los cambios que se realizan
 * dentro, se van guardando hasta que se almacenan definitivamente.
 * Un bean, sólo podrá estar asociado a un contexto.
 * Esta función, de forma recursiva llama a los hijos (de estar cargados), para agregarlos igualmente al contexto.
 * @param contextName Pueden definirse diferentes contextos en los que almacenar los datos.
 * @param bean
 * @return
 */
bool AERPTransactionContext::addToContext(const QString &contextName, const BaseBeanPointer &bean)
{
    if ( bean.isNull() )
    {
        return false;
    }
    // Las vistas, por defecto, no estarán dentro de una transacción
    if ( bean->metadata()->dbObjectType() == AlephERP::NotValid || bean->metadata()->dbObjectType() == AlephERP::View )
    {
        return false;
    }

    // Comprobamos primero si debemos crear el contexto
    if ( !d->m_contextObjects.contains(contextName) )
    {
        d->m_contextObjects[contextName] = new BeansReferenceList();
    }
    // Recorremos todos los beans agregados para comprobar que no está previamente agregado
    QList<BaseBeanPointer> list = beansOnContext(contextName);
    foreach(BaseBeanPointer b, list)
    {
        if ( !b.isNull() && b->objectName() == bean->objectName() )
        {
            QLogger::QLog_Debug(AlephERP::stLogDB, QString("AERPTransactionContext::addToContext: El bean ya estaba agregado al contexto: %1").arg(contextName));
            return false;
        }
    }
    // ¿Estaba ya el bean agregado en otro contexto? De ser así, sale de ese contexto.
    if ( bean->actualContext() != contextName && !bean->actualContext().isEmpty() )
    {
        discardFromContext(bean);
    }
    d->m_contextObjects[contextName]->addBean(bean);
    bean->setActualContext(contextName);
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("AERPTransactionContext::addToContext: Bean agregado al contexto: [%1] tabla [%2]. Número de beans en el contexto: [%3]").
                        arg(contextName).
                        arg(bean->metadata()->tableName()).
                        arg(d->m_contextObjects[contextName]->list().size()));

    emit beanAddedToContext(contextName, bean);
    /*
    connect(bean.data(), static_cast<void (BaseBean::*)(BaseBean *, bool value)>(&BaseBean::beanModified), [=](BaseBean *bean, bool modified) {
        emit beanModified(contextName, bean, modified);
    });
    */
    connect(bean.data(), SIGNAL(beanModified(BaseBean*,bool)), this, SLOT(emitBeanModified(BaseBean*,bool)));

    // Si tiene campos con contadores, registramos el valor que tengan calculado
    foreach(DBField *fld, bean->fields())
    {
        if ( fld->metadata()->hasCounterDefinition() && fld->hasBeenCalculated() )
        {
            registerCounterValue(fld, fld->value());
        }
    }

    // Si tiene hijos que están cargados, se añaden al contexto. Los que se vayan cargando sólos se irán agregando
    // autónomamente a la transacción.
    foreach(DBRelation *relation, bean->relations(AlephERP::OneToMany | AlephERP::OneToOne | AlephERP::ManyToOne))
    {
        // Obtener los hijos no debe en este caso de desencadenar ningún tipo de proceso. Por eso accedemos a ellos
        // con la estructura interna.
        bool previousState = bean->blockAllSignals(true);
        BaseBeanPointerList list = relation->internalChildren();
        bean->blockAllSignals(previousState);
        foreach (BaseBeanPointer child, list)
        {
            if ( !child.isNull() )
            {
                AERPTransactionContext::addToContext(contextName, child.data());
                emit beanAddedToContext(contextName, child);
                connect(child.data(), SIGNAL(beanModified(BaseBean*,bool)), this, SLOT(emitBeanModified(BaseBean*,bool)));
            }
        }
    }
    // Si además, tiene relaciones, éstas también se agregan al contexto
    if ( bean->relatedElementsLoaded() )
    {
        RelatedElementPointerList elements = bean->getRelatedElements(AlephERP::Record, AlephERP::PointToChild, true);
        foreach (RelatedElementPointer element, elements)
        {
            if ( !element.isNull() && element->relatedIsLoaded() && !element->relatedBean().isNull() && element->relatedBean()->objectName() != bean->objectName() )
            {
                AERPTransactionContext::addToContext(contextName, element->relatedBean());
                emit beanAddedToContext(contextName, element->relatedBean());
                connect(element->relatedBean().data(), SIGNAL(beanModified(BaseBean*,bool)), this, SLOT(emitBeanModified(BaseBean*,bool)));
            }
        }
    }
    return true;
}

/**
 * @brief AERPTransactionContext::discard
 * Elimina un bean del contexto de almacenamiento. Hace lo mismo, de forma recursiva con los hijos
 * que tuviera agregados.
 * @param contextName
 * @param bean
 * @return
 */
bool AERPTransactionContext::discardFromContext(const BaseBeanPointer &bean, bool includeFatherBeansOnDiscard, QStack<BaseBean *> *discardStack)
{
    bool stackCreatedHere = false;
    QString contextName = bean->actualContext();
    if ( bean.isNull() || contextName.isEmpty() )
    {
        return false;
    }
    if ( discardStack == NULL )
    {
        discardStack = new QStack<BaseBean *>();
        stackCreatedHere = true;
    }
    else
    {
        if ( discardStack->contains(bean.data()) )
        {
            return true;
        }
    }
    QLogger::QLog_Debug(AlephERP::stLogOther, QString("AERPTransactionContext::discardFromContext: Descartando: %1").arg(bean->metadata()->tableName()));
    discardStack->push(bean.data());
    if ( !d->m_contextObjects.contains(contextName) )
    {
        QLogger::QLog_Trace(AlephERP::stLogDB, QString("AERPTransactionContext::discard: No existe el contexto: [%1]").arg(contextName));
    }
    if ( !d->m_contextObjects[contextName]->removeBean(bean) )
    {
        QLogger::QLog_Trace(AlephERP::stLogDB, QString("AERPTransactionContext::discard:  El bean no existe en el contexto: [%1]").arg(contextName));
    }
    else
    {
        bean.data()->setActualContext("");

        disconnect(bean.data(), SLOT(emitBeanModified(BaseBean*,bool)));

        // Eliminamos del contexto los hijos o padres de este bean que estuviesen relacionados
        QList<DBRelation *> relations;
        if ( includeFatherBeansOnDiscard )
        {
            relations = bean.data()->relations(AlephERP::OneToMany | AlephERP::OneToOne | AlephERP::ManyToOne);
        }
        else
        {
            relations = bean.data()->relations(AlephERP::OneToMany | AlephERP::OneToOne);
        }
        foreach(DBRelation *relation, relations)
        {
            // Obtener los hijos no debe en este caso de desencadenar ningún tipo de proceso.
            bool previousState = bean->blockAllSignals(true);
            BaseBeanPointerList list = relation->internalChildren();
            bean->blockAllSignals(previousState);
            foreach (BaseBeanPointer child, list)
            {
                if ( !child.isNull() )
                {
                    AERPTransactionContext::discardFromContext(child.data(), includeFatherBeansOnDiscard, discardStack);
                    disconnect(child.data(), SLOT(emitBeanModified(BaseBean*,bool)));
                }
            }
        }
    }
    discardStack->pop();
    if ( stackCreatedHere )
    {
        delete discardStack;
    }
    return true;
}

/**
 * @brief AERPTransactionContext::discardContext
 * Elimina todos los objetos del contexto. A diferencia de rollback, deja los beans en su estado original
 * y no hace nada.
 * @param contextName
 * @return
 */
bool AERPTransactionContext::discardContext(const QString &contextName)
{
    if ( d->m_counterValues.contains(contextName) )
    {
        d->m_counterValues[contextName].clear();
    }
    if ( !d->m_contextObjects.contains(contextName) )
    {
        QLogger::QLog_Info(AlephERP::stLogDB, QString("AERPTransactionContext::discard: No existe el contexto: [%1]").arg(contextName));
        return false;
    }
    foreach (BaseBeanPointer bean, d->m_contextObjects[contextName]->list())
    {
        if ( !bean.isNull() )
        {
            bean.data()->setActualContext("");
        }
    }
    d->m_contextObjects[contextName]->clear();
    delete d->m_contextObjects[contextName];
    d->m_contextObjects.remove(contextName);
    return true;
}

/**
 * @brief AERPTransactionContext::commit
 * Dentro de una transacción, realizará todas las inserciones, ediciones y borrados de los beans
 * que se hayan agregado al contexto.
 * @param contextName Nombre del contexto
 * @param connectionName Conexión en la que se realizarán los cambios
 * @return
 */
bool AERPTransactionContext::commit(const QString &contextName, bool discardContextOnSuccess)
{
    BaseBeanPointerList beansSaved;
    QString idTransaction = QUuid::createUuid().toString();

    d->m_doingCommit[contextName] = true;
    d->m_lastError.clear();

    if ( !d->m_cancel.contains(contextName) )
    {
        d->m_cancel[contextName] = false;
    }

    if ( !d->m_contextObjects.contains(contextName) )
    {
        d->m_lastError = trUtf8("AERPTransactionContext::commit: No hay ningún contexto llamado %1").arg(contextName);
        QLogger::QLog_Error(AlephERP::stLogDB, d->m_lastError);
        d->m_counterValues[contextName].clear();
        // Devolvemos true porque no pasa nada...
        emit transactionCommited(contextName);
        d->m_doingCommit[contextName] = false;
        return true;
    }

    emit preparationInited(contextName, d->m_contextObjects[contextName]->list().size());
    bool result = d->doPrepareTransaction(contextName);
    emit preparationFinished(contextName);
    if ( !result )
    {
        if ( d->m_lastError.isEmpty() )
        {
            if ( discardContextOnSuccess )
            {
                discardContext(contextName);
            }
            emit transactionCommited(contextName);
        }
        else
        {
            emit transactionAborted(contextName);
            d->m_doingCommit[contextName] = false;
        }
        return d->m_lastError.isEmpty();
    }

    emit orderingInited(contextName, d->m_contextObjects[contextName]->list().size());
    BaseBeanPointerList list = d->doOrderForTransaction(contextName);
    emit orderingFinished(contextName);
    if ( list.isEmpty() )
    {
        emit transactionCommited(contextName);
        d->m_doingCommit[contextName] = false;
        return true;
    }

    emit validationInited(contextName, list.size());
    result = d->doValidationForTransaction(list);
    emit validationFinished(contextName);
    if ( !result )
    {
        emit transactionAborted(contextName);
        d->m_doingCommit[contextName] = false;
        return false;
    }

    d->buildBeansStatePreviousCommit(list);

    emit transactionInited(contextName, list.size());

    // Iniciamos la transacción
    if ( !BaseDAO::transaction(d->m_database) )
    {
        d->m_lastError = trUtf8("AERPTransactionContext::commit: No se ha podido iniciar la transacción. \n%1").arg(BaseDAO::lastErrorMessage());
        QLogger::QLog_Error(AlephERP::stLogDB, d->m_lastError);
        emit transactionAborted(contextName);
        d->m_doingCommit[contextName] = false;
        return false;
    }

    // Ejecutamos las SQL Previas
    foreach (const QString &sql, d->m_contextObjects[contextName]->previousSql())
    {
        if ( !BaseDAO::execute(sql) )
        {
            d->m_lastError = trUtf8("AERPTransactionContext::commit: No se ha podido completar la transacción. \n%1").arg(BaseDAO::lastErrorMessage());
            QLogger::QLog_Error(AlephERP::stLogDB, d->m_lastError);
            BaseDAO::rollback(d->m_database);
            emit transactionAborted(contextName);
            d->m_doingCommit[contextName] = false;
            return false;
        }
    }

    // Guardamos los objetos del contexto
    foreach (BaseBeanPointer bean, list)
    {
        if ( d->m_cancel[contextName] )
        {
            BaseDAO::rollback(d->m_database);
            d->m_lastError = trUtf8("Cancelado por el usuario");
            emit transactionAborted(contextName);
            d->m_cancel[contextName] = false;
            d->m_doingCommit[contextName] = false;
            return false;
        }
        if ( !bean.isNull() )
        {
            emit workingWithBean(bean);
            if ( !bean->save(idTransaction, false, false) )
            {
                d->m_lastError = trUtf8("AERPTransactionContext::commit: No se ha podido completar la transacción. \n%1").arg(BaseDAO::lastErrorMessage());
                QLogger::QLog_Error(AlephERP::stLogDB, d->m_lastError);
                BaseDAO::rollback(d->m_database);
                emit transactionAborted(contextName);
                d->m_doingCommit[contextName] = false;
                return false;
            }
            if ( bean->dbState() == BaseBean::INSERT || bean->dbState() == BaseBean::UPDATE )
            {
                emit beanSaved(bean);
                bean->emitBeanSaved();
                beansSaved.append(bean);
            }
            if ( bean->dbState() == BaseBean::DELETED )
            {
                emit beanDeleted(bean);
            }
        }
    }
    // Ahora almacenamos todos los elementos relacionados de ese bean. Lo hacemos aquí ya que así nos garantizamos que todo bean relacionado
    // que haya podido crearse, quede guardado.
    foreach (BaseBeanPointer bean, list)
    {
        if ( !bean.isNull() && bean->modifiedRelatedElements() && bean->countRelatedElements(AlephERP::NoneAll, AlephERP::PointToChild, true) > 0 )
        {
            bean->saveRelatedElements(idTransaction, false);
        }
    }

    // Las relaciones 1 a 1 tienen un tratamiento especial, y es que debemos asegurar que ambos registros se han guardado, para entonces, ajustar
    // los valores de los campos
    foreach (BaseBeanPointer bean, list)
    {
        if ( !saveOneToOneIds(bean, contextName) )
        {
            d->m_lastError = trUtf8("AERPTransactionContext::commit: No se ha podido completar la transacción. \n%1").arg(BaseDAO::lastErrorMessage());
            QLogger::QLog_Error(AlephERP::stLogDB, d->m_lastError);
            BaseDAO::rollback(d->m_database);
            emit transactionAborted(contextName);
            d->m_doingCommit[contextName] = false;
            return false;
        }
    }

    // Ejecutamos las SQL Finales
    foreach (const QString &sql, d->m_contextObjects[contextName]->finalsSql())
    {
        if ( !BaseDAO::execute(sql) )
        {
            d->m_lastError = trUtf8("AERPTransactionContext::commit: No se ha podido completar la transacción. \n%1").arg(BaseDAO::lastErrorMessage());
            QLogger::QLog_Error(AlephERP::stLogDB, d->m_lastError);
            BaseDAO::rollback(d->m_database);
            emit transactionAborted(contextName);
            d->m_doingCommit[contextName] = false;
            return false;
        }
    }

    // Realizamos el commit a base de datos
    bool transactionSuccess = BaseDAO::commit(d->m_database);
    if ( !transactionSuccess )
    {
        BaseDAO::rollback(d->m_database);
        d->m_lastError = trUtf8("AERPTransactionContext::commit: No se ha podido completar la transacción. \n%1").arg(BaseDAO::lastErrorMessage());
        QLogger::QLog_Error(AlephERP::stLogDB, d->m_lastError);
        emit transactionAborted(contextName);
        d->m_doingCommit[contextName] = false;
        return false;
    }

    // Recargamos todos los registros desde base de datos. Implantaciones complejas pueden tener triggers que generen nuevos
    // registros, cambien datos o tengan columnas calculadas... Esto implica un decremento en el rendimiento en accesos lentos
    // pero da seguridad en la integridad de datos.
    BaseDAO::reloadBeansFromDB(beansSaved);
    foreach (BaseBeanPointer bean, beansSaved)
    {
        // Tras una transacción y como puede haber triggers deferred, es necesario recargar los datos de ellos
        if ( bean )
        {
            BaseDAO::reloadFieldChangedAfterSave(bean);
            BaseDAO::reloadRelationsChangedAfterSave(bean);
            if ( !bean.isNull() )
            {
                bean->emitBeanCommitted();
            }
            if ( transactionSuccess )
            {
                bean->uncheckModifiedFields();
                bean->uncheckModifiedRelatedElements();
            }
        }
    }

    if ( discardContextOnSuccess )
    {
        discardContext(contextName);
    }
    emit transactionCommited(contextName);
    d->m_doingCommit[contextName] = false;
    return true;
}

/**
 * @brief AERPTransactionContextPrivate::doPreparationForTransaction
 * @param contextName
 * @return
 * Realiza los trabajos previos al commit, que son
 * 1.- Conformar la lista definitiva de registros a meter en la transacción a partir del contextName
 * 2.- Ordernar los registros para que los SQL tengan sentido en base de datos
 * 3.- Comprobar la validación de los registros que van en la transacción
 */
bool AERPTransactionContextPrivate::doPrepareTransaction(const QString &contextName)
{
    QLogger::QLog_Debug(AlephERP::stLogDB,
                        QString("AERPTransactionContext::commit: Inicio de transacción. Existen [%1] beans en el contexto [%2]").
                            arg(m_contextObjects[contextName]->list().size()).arg(contextName));

    // Esto debemos hacerlo ANTES de iniciar los save. La razón es la siguiente: Imaginemos que desde una función beforeSave se crean nuevos beans
    // que deberán ser incluídos en esta transacción. Por ejemplo, se están creando las líneas de IVA antes de guardar el registro. Estas líneas
    // deberán estar en el contexto y en la transacción. Por eso, necesitamos dos bucles. TODO: Y es probable que más...
    BaseBeanPointerList firstList;
    BaseBeanPointerList proccesedBeans;

    while ( firstList.size() != m_contextObjects[contextName]->list().size() )
    {
        firstList = m_contextObjects[contextName]->list();
        foreach (BaseBeanPointer bean, firstList)
        {
            emit q_ptr->workingWithBean(bean);
            if ( !bean.isNull() && bean->modified() && AERPListContainsBean<BaseBeanPointerList>(proccesedBeans, bean) == -1 )
            {
                if ( bean->dbState() == BaseBean::INSERT )
                {
                    if ( !bean->metadata()->beforeInsertScriptExecute(bean) || !bean->metadata()->beforeSaveScriptExecute(bean) )
                    {
                        m_lastError = QObject::trUtf8("No se han cumplido las condiciones necesarias para la inserción de este registro '%1'").arg(bean->metadata()->alias());
                        return false;
                    }
                    emit q_ptr->beforeSaveBean(bean);
                    bean->emitBeforeInsert();
                    bean->emitBeforeSave();
                }
                else if ( bean->dbState() == BaseBean::UPDATE )
                {
                    if ( !bean->metadata()->beforeUpdateScriptExecute(bean) || !bean->metadata()->beforeSaveScriptExecute(bean) )
                    {
                        m_lastError = QObject::trUtf8("No se han cumplido las condiciones necesarias para la edición de este registro '%1'").arg(bean->metadata()->alias());
                        return false;
                    }
                    emit q_ptr->beforeSaveBean(bean);
                    bean->emitBeforeUpdate();
                    bean->emitBeforeSave();
                }
                else if ( bean->dbState() == BaseBean::TO_BE_DELETED )
                {
                    if ( !bean->metadata()->beforeDeleteScriptExecute(bean) )
                    {
                        m_lastError = QObject::trUtf8("No se han cumplido las condiciones necesarias para eliminar este registro '%1'").arg(bean->metadata()->alias());
                        return false;
                    }
                    emit q_ptr->beforeDeleteBean(bean);
                    // Esta llamada es de especial importancia, ya que permitirá añadir al contexto los elementos relacionados que deban borrarse, por ejemplo.
                    // TODO: Puede estar duplicada con la llamada bean->removeConfiguredRelatedElements, que se hace desde BaseDAO::remove
                    bean->prepareToDeleteRelatedElements();
                    bean->emitBeforeDelete();
                }
                emit q_ptr->beforeAction(bean);
            }
            proccesedBeans.append(bean);
        }
    }
    return true;
}

/**
 * @brief AERPTransactionContextPrivate::doOrderForTransaction
 * @param contextName
 * @return
 * Devuelve una lista ordenada de los registros en la transacción. Utilizará topological sort para ello.
 */
BaseBeanPointerList AERPTransactionContextPrivate::doOrderForTransaction(const QString &contextName)
{
    // El orden en el que se hace el commit es muy importante, y debe tener en cuenta las relaciones 1->M y demás.
    BaseBeanPointerList list = orderBeansForTransaction(contextName);
    if ( list.size() == 0 )
    {
        return list;
    }

    // Algo de debug
    QLogger::QLog_Info(AlephERP::stLogDB, QObject::tr("----- INFORMACIÓN DE TRANSACCIÓN ----"));
    foreach (BaseBeanPointer bean, list)
    {
        QLogger::QLog_Info(AlephERP::stLogDB,
                           QObject::tr("Bean: %1. [%2] Estado: %3. OID: %4").arg(bean->metadata()->alias()).
                                                                    arg(bean->objectName()).
                                                                    arg(bean->dbStateDisplayName()).
                                                                    arg(bean->dbOid()));
        foreach (DBField *fld, bean->fields())
        {
            QLogger::QLog_Debug(AlephERP::stLogDB,
                                QObject::tr("   %1: [%2]").
                                    arg(fld->dbFieldName()).
                                    arg(fld->value().toString()));
        }
    }
    QLogger::QLog_Info(AlephERP::stLogDB, QObject::tr("----- FIN ----"));
    return list;
}

/**
 * @brief AERPTransactionContextPrivate::doValidationForTransaction
 * @param list
 * @return
 * Recalcula los campos calculados de los registros pasados como argumentos, así como ejecuta
 * el método validate de ellos. Todo ello para preparar la transacción.
 */
bool AERPTransactionContextPrivate::doValidationForTransaction(const BaseBeanPointerList &list)
{
    // Vamos a validar con las reglas internas
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("AERPTransactionContext::commit: Se va a proceder a guardar los beans de las transacción. El número de beans a guardar es de: [%1]").arg(list.size()));
    foreach (BaseBeanPointer bean, list)
    {
        // Asegurémosnos de que los campos están adecuadamente calculados
        bean->recalculateCalculatedFields();
        // Validemos
        if ( !bean.isNull() && bean->dbState() != BaseBean::TO_BE_DELETED && !bean->validate() )
        {
            m_lastError.append(bean->metadata()->alias()).append(": ").append(bean->validateMessages()).append("\n");
            QLogger::QLog_Error(AlephERP::stLogDB, QString("AERPTransactionContext::commit: [%1]. Estado: [%2]").
                                arg(bean->metadata()->tableName()).
                                arg(bean->dbState() == BaseBean::INSERT ? "INSERT" : (bean->dbState() == BaseBean::UPDATE ? "UPDATE" : "DELETE")));
            QLogger::QLog_Error(AlephERP::stLogDB, QString("AERPTransactionContext::commit: ERROR: [%1]").arg(m_lastError));
        }
    }
    return m_lastError.isEmpty();
}

/**
 * @brief AERPTransactionContext::saveOneToOneIds
 * @param bean
 * @return
 * En las relaciones 1 a 1, ajustamos los valores de los ids
 */
bool AERPTransactionContext::saveOneToOneIds(BaseBeanPointer bean, const QString &contextName)
{
    if ( bean.isNull() )
    {
        return true;
    }
    QList<DBRelation *> rels = bean->relations(AlephERP::OneToOne);
    foreach (DBRelation *rel, rels)
    {
        if ( !rel->internalBrother().isNull() &&
             rel->internalBrother()->dbState() != BaseBean::TO_BE_DELETED &&
             rel->internalBrother()->dbState() != BaseBean::DELETED )
        {
            DBField *oneSideField = bean->field(rel->metadata()->rootFieldName());
            DBField *otherSideField = rel->internalBrother()->field(rel->metadata()->childFieldName());
            if ( oneSideField && otherSideField )
            {
                if ( !oneSideField->metadata()->unique() && !otherSideField->metadata()->unique() &&
                     !oneSideField->metadata()->serial() && !otherSideField->metadata()->serial() )
                {
                    QLogger::QLog_Error(AlephERP::stLogDB, trUtf8("En una relación 11, uno de los dos campos debe ser serial o único."));
                }
                else
                {
                    if ( (oneSideField->metadata()->unique() ||
                         oneSideField->metadata()->serial()) &&
                         !oneSideField->value().isNull() &&
                         oneSideField->value() != otherSideField->value() )
                    {
                        otherSideField->setInternalValue(oneSideField->value(), true);
                        otherSideField->setModified(true);
                        if ( !BaseDAO::update(otherSideField, contextName, d->m_database) )
                        {
                            return false;
                        }
                    }
                    if ( (otherSideField->metadata()->unique() ||
                         otherSideField->metadata()->serial()) &&
                         !otherSideField->value().isNull() &&
                         oneSideField->value() != otherSideField->value() )
                    {
                        oneSideField->setInternalValue(otherSideField->value(), true);
                        oneSideField->setModified(true);
                        if ( !BaseDAO::update(oneSideField, contextName, d->m_database) )
                        {
                            return false;
                        }
                    }
                }
            }
        }
    }
    return true;
}

/**
 * @brief AERPTransactionContext::waitToEnd
 * Esta función hace esperar al hilo principal hasta que el commit acaba. Se usa porque en el commit
 * hay mucha ejecución JS que puede ir por hilos separados e interferir con el Main Thread
 */
void AERPTransactionContext::waitCommitToEnd(const QString &contextName)
{
    while ( doingCommit(contextName) )
    {
        CommonsFunctions::processEvents();
    }
}

/**
 * @brief AERPTransactionContext::rollback
 * El rollback del contexto, analiza todos los beans, y los devuelve a su estado original.
 * @param contextName
 * @return
 */
bool AERPTransactionContext::rollback(const QString &contextName)
{
    if ( !d->m_contextObjects.contains(contextName) )
    {
        d->m_lastError = trUtf8("AERPTransactionContext::rollback: No hay ningún contexto llamado [%1]").arg(contextName);
        QLogger::QLog_Error(AlephERP::stLogDB, d->m_lastError);
        return false;
    }
    QList<BaseBeanPointer> list = d->m_contextObjects[contextName]->list();
    foreach (BaseBeanPointer bean, list)
    {
        if ( !bean.isNull() )
        {
            bean.data()->restoreValues();
            bean.data()->setActualContext("");
        }
    }
    discardContext(contextName);
    return true;
}

bool AERPTransactionContext::addPreviousSqlToContext(const QString &contextName, const QString &sql)
{
    // Comprobamos primero si debemos crear el contexto
    if ( !d->m_contextObjects.contains(contextName) )
    {
        d->m_contextObjects[contextName] = new BeansReferenceList();
    }
    d->m_contextObjects[contextName]->addPreviousSql(sql);
    return true;
}

bool AERPTransactionContext::addFinalSqlToContext(const QString &contextName, const QString &sql)
{
    // Comprobamos primero si debemos crear el contexto
    if ( !d->m_contextObjects.contains(contextName) )
    {
        d->m_contextObjects[contextName] = new BeansReferenceList();
    }
    d->m_contextObjects[contextName]->addFinalSql(sql);
    return true;
}

/**
 * @brief AERPTransactionContext::isContextEmpty
 * Indicará si el contexto pasado en contextName tiene algún objeto asociado.
 * @param contextName
 * @return
 */
bool AERPTransactionContext::isContextEmpty(const QString &contextName)
{
    if ( d->m_contextObjects.contains(contextName) )
    {
        if ( d->m_contextObjects[contextName]->size() == 0 )
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    return true;
}

/**
 * @brief AERPTransactionContext::objectsContextSize
 * Devuelve el número de objetos en el contexto
 * @param contextName
 * @return
 */
int AERPTransactionContext::objectsContextSize(const QString &contextName)
{
    if ( d->m_contextObjects.contains(contextName) )
    {
        BeansReferenceList *list = d->m_contextObjects[contextName];
        return list->size();
    }
    return 0;
}

QString AERPTransactionContext::lastErrorMessage() const
{
    return d->m_lastError;
}

void AERPTransactionContext::setDatabase(const QString &name)
{
    d->m_database = name;
}

QString AERPTransactionContext::database() const
{
    return d->m_database;
}

BaseBeanPointerList AERPTransactionContext::beansOnContext(const QString &contextName)
{
    if ( !d->m_contextObjects.contains(contextName) )
    {
        QLogger::QLog_Debug(AlephERP::stLogDB, QString("AERPTransactionContext::beansOnContext: No existe el contexto [%1]").arg(contextName));
        return QList<BaseBeanPointer>();
    }
    return d->m_contextObjects[contextName]->list();
}

BaseBeanPointerList AERPTransactionContext::beansOrderedToPersist(const QString &contextName)
{
    BaseBeanPointerList list;
    if ( d->doPrepareTransaction(contextName) )
    {
        list = d->doOrderForTransaction(contextName);
    }
    return list;
}

/**
 * @brief AERPTransactionContext::isDirty
 * Indica si el contexto actual tiene beans marcados como modified, y que por tanto deberían
 * ser guardados en base de datos.
 * @param contextName
 * @return
 */
bool AERPTransactionContext::isDirty(const QString &contextName)
{
    foreach (BaseBeanPointer bean, beansOnContext(contextName))
    {
        if ( !bean.isNull() && bean->modified() )
        {
            return true;;
        }
    }
    return false;
}

bool AERPTransactionContext::isOnTransaction(BaseBeanPointer bean)
{
    QList<BaseBeanPointer> beans = AERPTransactionContext::instance()->beansOnContext(bean->actualContext());
    foreach (BaseBeanPointer b, beans)
    {
        if ( bean->dbState() == BaseBean::INSERT )
        {
            return bean->objectName() == b->objectName();
        }
        else
        {
            return bean->dbOid() == b->dbOid();
        }
    }
    return false;
}

QList<QVariant> AERPTransactionContext::registeredCounterValues(DBField *fld)
{
    if ( fld->bean()->actualContext().isEmpty() )
    {
        return QList<QVariant>();
    }
    QList<QVariant> result;
    if ( !d->m_counterValues.contains(fld->bean()->actualContext()) )
    {
        return result;
    }
    DBFieldCounterList list = d->m_counterValues[fld->bean()->actualContext()];
    for (int i = 0 ; i < list.size() ; i++)
    {
        if ( list.at(i).fld )
        {
            if ( list.at(i).fld->objectName() != fld->objectName() &&
                 list.at(i).fld->metadata()->dbFieldName() == fld->metadata()->dbFieldName() &&
                 list.at(i).fld->bean()->metadata()->tableName() == fld->bean()->metadata()->tableName() )
            {
                result.append(list.at(i).value);
            }
        }
        else
        {
            d->m_counterValues[fld->bean()->actualContext()].removeAt(i);
        }
    }
    return result;
}

void AERPTransactionContext::registerCounterValue(DBField *fld, const QVariant &counterValue)
{
    if ( fld->bean()->actualContext().isEmpty() )
    {
        return;
    }
    DBFieldCounterList list;
    if ( !d->m_counterValues.contains(fld->bean()->actualContext()) )
    {
        d->m_counterValues[fld->bean()->actualContext()] = list;
    }
    else
    {
        list = d->m_counterValues[fld->bean()->actualContext()];
    }
    for (int i = 0 ; i < list.size() ; i++)
    {
        if ( list[i].fld )
        {
            if ( list[i].fld == fld )
            {
                list[i].value = counterValue;
                return;
            }
        }
        else
        {
            d->m_counterValues[fld->bean()->actualContext()].removeAt(i);
        }
    }
    DBFieldCounter c;
    c.fld = fld;
    c.value = counterValue;
    d->m_counterValues[fld->bean()->actualContext()].append(c);
}

bool AERPTransactionContext::doingCommit(const QString &contextName)
{
    if ( contextName.isEmpty() && d->m_doingCommit.contains(contextName) )
    {
        return d->m_doingCommit.value(contextName);
    }
    bool result = false;
    QHashIterator<QString, bool> it(d->m_doingCommit);
    while (it.hasNext())
    {
        it.next();
        result = result | it.value();
    }
    return result;
}

void AERPTransactionContext::cancel(const QString &contextName)
{
    d->m_cancel[contextName] = true;
}

void AERPTransactionContext::emitBeanModified(BaseBean *bean, bool modified)
{
    BaseBeanPointer b(bean);
    emit beanModified(bean, modified);
}

/**
 * @brief AERPTransactionContextPrivate::orderAndFilterBeans
 * Los beans deben insertarse en un orden particular, dependiendo de las relaciones 1->M que tengan.
 * En el proceso de la aplicación se ha podido alterar ese orden, por lo que desde aquí, se realiza la
 * ordenación de los mismos.
 * @return
 */
BaseBeanPointerList AERPTransactionContextPrivate::orderBeansForTransaction(const QString &contextName)
{
    // TODO: Hay que ver el caso en el que los beans dependan de otro de su misma tabla!!!
    BaseBeanPointerList orderedBeans;
    BaseBeanPointerList beansOnContext = m_contextObjects[contextName]->list();
    Graph graph(beansOnContext.size());
    const char *graphIndexProperty = "graphIndex";

    // Primero asignemos índices a los beans para agregarlos al grafo
    for (int i = 0 ; i < beansOnContext.size() ; ++i)
    {
        beansOnContext[i]->setProperty(graphIndexProperty, i);
    }

    // Vamos a construir el grafo. Para ello recorremos todos los beans, y examinamos las relaciones o
    // Si la tienen, y el registro está en el contexto, se añade al grafo
    foreach (BaseBeanPointer bean, beansOnContext)
    {
        QList<DBRelation *> relations = bean->relations(AlephERP::ManyToOne);
        foreach (DBRelation *rel, relations)
        {
            if ( rel->isFatherLoaded() )
            {
                BaseBeanPointer father = rel->father(false);
                if ( father && father->actualContext() == contextName )
                {
                    QLogger::QLog_Debug(AlephERP::stLogDB,
                                        QString("AERPTransactionContextPrivate::orderBeansForTransaction: Camino del grafo [%1] [%2] [%3] [%4]").
                                        arg(father->metadata()->tableName()).
                                        arg(father->property(graphIndexProperty).toInt()).
                                        arg(bean->metadata()->tableName()).
                                        arg(bean->property(graphIndexProperty).toInt())
                                        );
                    graph.addEdge(father->property(graphIndexProperty).toInt(), bean->property(graphIndexProperty).toInt());
                }
            }
        }
        relations = bean->relations(AlephERP::OneToOne);
        foreach (DBRelation *rel, relations)
        {
            BaseBeanPointer brother = rel->brother();
            if ( brother && brother->actualContext() == contextName )
            {
                QLogger::QLog_Debug(AlephERP::stLogDB,
                                    QString("AERPTransactionContextPrivate::orderBeansForTransaction: Camino del grafo [%1] [%2] [%3] [%4]").
                                    arg(bean->metadata()->tableName()).
                                    arg(bean->property(graphIndexProperty).toInt()).
                                    arg(brother->metadata()->tableName()).
                                    arg(brother->property(graphIndexProperty).toInt())
                                    );
                graph.addEdge(bean->property(graphIndexProperty).toInt(), brother->property(graphIndexProperty).toInt());
            }
        }
        relations = bean->relations(AlephERP::OneToMany);
        foreach (DBRelation *rel, relations)
        {
            BaseBeanPointerList children;
            // Si no se han obtenido los hijos cuando llegamos a la transacción... ¡¡es que no hay que obtenerlos!!
            if ( bean->dbState() == BaseBean::INSERT ||
                 rel->childrenLoaded() )
            {
                children = rel->children();
            }
            foreach (BaseBeanPointer child, children)
            {
                if ( child->actualContext() == contextName )
                {
                    QLogger::QLog_Debug(AlephERP::stLogDB,
                                        QString("AERPTransactionContextPrivate::orderBeansForTransaction: Camino del grafo [%1] [%2] [%3] [%4]").
                                        arg(bean->metadata()->tableName()).
                                        arg(bean->property(graphIndexProperty).toInt()).
                                        arg(child->metadata()->tableName()).
                                        arg(child->property(graphIndexProperty).toInt())
                                        );
                    graph.addEdge(bean->property(graphIndexProperty).toInt(), child->property(graphIndexProperty).toInt());
                }
            }
        }
    }
    // Se realiza la ordenación topológica
    QList<int> indexOrdered = graph.topologicalSort();

    foreach(int graphId, indexOrdered)
    {
        // Busquemos el bean en el contexto, y agregémoslo
        foreach (BaseBeanPointer bean, beansOnContext)
        {
            if ( bean && bean->property(graphIndexProperty).toInt() == graphId )
            {
                emit q_ptr->workingWithBean(bean);
                if ( !orderedBeans.contains(bean) )
                {
                    if ( bean->dbState() != BaseBean::DELETED && bean->modified() )
                    {
                        QLogger::QLog_Debug(AlephERP::stLogDB,
                                            QString("AERPTransactionContextPrivate::orderBeansForTransaction: Ordenado: [%1] [%2] [%3]").
                                            arg(bean->metadata()->tableName()).
                                            arg(bean->dbOid()).
                                            arg(bean->property(graphIndexProperty).toInt()));
                        orderedBeans.append(bean);
                    }
                }
            }
        }

    }
    return orderedBeans;
}

/**
 * @brief AERPTransactionContextPrivate::buildBeansStatePreviousCommit
 * Antes de hacer el commit, guardamos el estado de los beans, y el de modificación de sus fields
 * @param list
 */
void AERPTransactionContextPrivate::buildBeansStatePreviousCommit(const QList<BaseBeanPointer> &list)
{
    m_beansStatePreviousCommit.clear();
    foreach (BaseBeanPointer bean, list)
    {
        BaseBeanStatePreviousCommit data;
        data.bean = bean;
        data.dbState = bean->dbState();
        foreach (DBField *fld, bean->fields())
        {
            data.fieldsModifiedState[fld->dbFieldName()] = fld->modified();
        }
        m_beansStatePreviousCommit.append(data);
    }
}

