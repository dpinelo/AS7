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

#include "relateddao.h"
#include "dao/database.h"
#include "dao/beans/basebean.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/relatedelement.h"
#include "dao/basedao.h"
#include "dao/aerptransactioncontext.h"
#include "configuracion.h"
#include "globales.h"
#include "qlogger.h"
#ifdef ALEPHERP_DOC_MANAGEMENT
#include "business/docmngmnt/aerpdocumentdaowrapper.h"
#endif

QThreadStorage<QString> m_relatedDaoThreadLastMessage;

RelatedDAO::RelatedDAO(QObject *parent) :
    QObject(parent)
{
}

QString RelatedDAO::lastErrorMessage()
{
    return m_relatedDaoThreadLastMessage.localData();
}

void RelatedDAO::writeDbMessages(QSqlQuery *qry)
{
    if ( qry->lastError().driverText() == qry->lastError().databaseText() )
    {
        m_relatedDaoThreadLastMessage.setLocalData(QString("Database Error: %2").arg(qry->lastError().driverText()));
    }
    else
    {
        m_relatedDaoThreadLastMessage.setLocalData(QString("Driver Error: %1\nDatabase Error: %2").arg(qry->lastError().driverText()).
                arg(qry->lastError().databaseText()));
    }
    QLogger::QLog_Error(AlephERP::stLogDB, QString("RelatedDAO: writeDbMessages: BBDD LastQuery: [%1]").arg(qry->lastQuery()));
    QLogger::QLog_Error(AlephERP::stLogDB, QString("RelatedDAO: writeDbMessages: BBDD Message(Error databaseText): [%1]").arg(qry->lastError().databaseText()));
    QLogger::QLog_Error(AlephERP::stLogDB, QString("RelatedDAO: writeDbMessages: BBDD Message(Error text): [%1]").arg(qry->lastError().text()));
}

/**
 * @brief RelatedDAO::saveRelatedElements
 * Guarda los elementos relacionados de un bean
 * @param bean
 * @return
 */
bool RelatedDAO::saveRelatedElements(BaseBean *bean, const QString &idTransaction)
{
    RelatedElementPointerList elements = bean->getRelatedElements(AlephERP::NoneAll, AlephERP::PointToChild, true);
    bool result = true;
    foreach (RelatedElementPointer element, elements)
    {
        if ( !element.isNull() && element->modified() )
        {
            result = result & RelatedDAO::saveRelatedElement(element, idTransaction);
        }
    }
    if ( result )
    {
        bean->cleanToBeDeletedRelatedElements();
    }
    return result;
}

bool RelatedDAO::saveRelatedElement(RelatedElementPointer element, const QString &idTransaction)
{
    bool result = true;
    QScopedPointer<QSqlQuery> qry(new QSqlQuery(Database::getQDatabase()));
    QString sql;
    if ( element->state() == RelatedElement::UPDATE && !element->modified() )
    {
        return true;
    }
    if ( element->state() == RelatedElement::LINK )
    {
        return true;
    }
    // Si el elemento relacionado tiene un bean en modo INSERT, primero debemos almacenar este
    if ( element->type() == AlephERP::Record && !element->relatedBean().isNull() && element->relatedBean()->dbState() == BaseBean::INSERT )
    {
        if ( !element->relatedBean()->validate() )
        {
            m_relatedDaoThreadLastMessage.setLocalData(element->relatedBean()->validateHtmlMessages());
            return false;
        }
        element->relatedBean()->save(idTransaction);
    }
    if ( element->state() == RelatedElement::INSERT )
    {
        sql = QString("INSERT INTO %1_relations (mastertablename, masteroid, relatedtablename, relatedoid, relationtype, data)"
                      "VALUES (:mastertablename, :masteroid, :relatedtablename, :relatedoid, :relationtype, :data)").arg(alephERPSettings->systemTablePrefix());
    }
    else if ( element->state() == RelatedElement::UPDATE )
    {
        sql = QString("UPDATE %1_relations SET mastertablename=:mastertablename, masteroid=:masteroid, relatedtablename=:relatedtablename, "
                      "relatedoid=:relatedoid, relationtype=:relationtype, data=:data WHERE id=:id").arg(alephERPSettings->systemTablePrefix());
    }
    else if ( element->state() == RelatedElement::TO_BE_DELETED )
    {
        sql = QString("DELETE FROM %1_relations WHERE id=:id").arg(alephERPSettings->systemTablePrefix());
    }
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("RelatedDAO::saveRelatedElement: [%1]").arg(sql));
    if ( !qry->prepare(sql) )
    {
        result = false;
        writeDbMessages(qry.data());
    }
    else
    {
        if ( element->state() == RelatedElement::INSERT || element->state() == RelatedElement::UPDATE )
        {
            qry->bindValue(":mastertablename", element->rootTableName());
            qry->bindValue(":masteroid", element->rootDbOid());
            qry->bindValue(":relatedtablename", element->relatedTableName());
            qry->bindValue(":relatedoid", element->relatedDbOid());
            qry->bindValue(":data", element->toXml());
            qry->bindValue(":relationtype", element->stringType());
        }
        if ( element->state() == RelatedElement::UPDATE || element->state() == RelatedElement::TO_BE_DELETED )
        {
            qry->bindValue(":id", element->id());
        }
        result = qry->exec();
        QLogger::QLog_Debug(AlephERP::stLogDB, QString("RelatedDAO::saveRelatedElements: [%1]").arg(qry->lastQuery()));
        if ( !result )
        {
            result = false;
            writeDbMessages(qry.data());
        }
        else
        {
            // Se actualiza el estado de los elementos que se han agregado nuevos
            if ( element->state() == RelatedElement::INSERT )
            {
                element->setState(RelatedElement::UPDATE);
            }
        }
    }
    return result;
}

/**
 * @brief RelatedDAO::loadRelatedElements
 * Carga los elementos relacionados de un bean
 * @param bean
 * @return
 */
bool RelatedDAO::loadRelatedElements(BaseBean *bean)
{
    bool result;
    // Si el bean aún no ha sido grabado a base de datos, entonces, si tiene elementos relacionados, ya están en memoria
    if ( bean->dbState() == BaseBean::INSERT || bean->dbOid() == 0 )
    {
        return true;
    }
    QScopedPointer<QSqlQuery> qry(new QSqlQuery(Database::getQDatabase()));
    QString sql = QString("SELECT id, relationtype, data, masteroid, relatedoid, mastertablename, masteroid, relatedtablename, relatedoid FROM %1_relations WHERE masteroid=:oid or relatedoid=:childoid ORDER by ts").arg(alephERPSettings->systemTablePrefix());
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("RelatedDAO::loadRelatedElements: [%1]").arg(sql));
    if ( !qry->prepare(sql) )
    {
        writeDbMessages(qry.data());
        return false;
    }
    qry->bindValue(":oid", bean->dbOid());
    qry->bindValue(":childoid", bean->dbOid());
    result = qry->exec();
    if ( !result )
    {
        writeDbMessages(qry.data());
        return false;
    }
    while (qry->next())
    {
        bool blockSignalsState = bean->blockSignals(true);
        RelatedElement *element = bean->newRelatedElement(RelatedElement::stringToType(qry->record().value("relationtype").toString()));
        if ( qry->record().value("masteroid").toInt() == bean->dbOid() )
        {
            element->setCardinality(AlephERP::PointToChild);
            element->setRelatedTableName(qry->record().value("relatedtablename").toString());
            element->setRelatedDbOid(qry->record().value("relatedoid").toLongLong());
        }
        else
        {
            element->setCardinality(AlephERP::PointToMaster);
            element->setRelatedTableName(qry->record().value("mastertablename").toString());
            element->setRelatedDbOid(qry->record().value("masteroid").toLongLong());
        }
        element->setXml(qry->record().value("data").toString());
        element->setId(qry->record().value("id").toInt());
        element->setState(RelatedElement::UPDATE);
        element->setModified(false);
        bean->blockSignals(blockSignalsState);
    }
    // Ahora buscamos los enlaces. Estos enlaces pueden ser relaciones PointToMaster entre objetos que estan en modo INSERT.
    if ( bean->dbState() == BaseBean::INSERT )
    {
        RelatedElement *parentElement = qobject_cast<RelatedElement *>(bean->parent());
        if ( parentElement != NULL && !parentElement->rootBean().isNull() )
        {
            bool blockState = bean->blockSignals(true);
            RelatedElement *elem = bean->newRelatedElement(AlephERP::Record);
            elem->setCardinality(AlephERP::PointToMaster);
            elem->setCategories(parentElement->categories());
            elem->setRelatedTableName(parentElement->rootBean()->metadata()->tableName());
            elem->setRelatedDbOid(parentElement->rootBean()->dbOid());
            elem->setRelatedBean(parentElement->rootBean(), false);
            elem->setState(RelatedElement::UPDATE);
            elem->setModified(false);
            bean->blockSignals(blockState);
        }
    }

    return true;
}

QHash<AlephERP::RelatedElementTypes, int> RelatedDAO::countRelatedElements(BaseBean *bean, AlephERP::RelatedElementCardinalities cardinality)
{
    bool result;
    QHash<AlephERP::RelatedElementTypes, int> hash;

    if ( bean->dbState() == BaseBean::INSERT )
    {
        RelatedElementPointerList list = bean->getRelatedElements(AlephERP::NoneAll, AlephERP::PointToAll, false);
        foreach (RelatedElement *element, list)
        {
            if ( cardinality.testFlag(element->cardinality()) )
            {
                if (!hash.contains(element->type()))
                {
                    hash[element->type()] = 0;
                }
                hash[element->type()] = hash.value(element->type()) + 1;
            }
        }
        return hash;
    }
    QScopedPointer<QSqlQuery> qry(new QSqlQuery(Database::getQDatabase()));
    QString sql = QString("SELECT count(*), relationtype as column1 FROM %1_relations WHERE %2 GROUP BY relationtype");
    QString where;

    if ( cardinality.testFlag(AlephERP::PointToChild) || cardinality.testFlag(AlephERP::PointToAll) )
    {
        where = QString("masteroid=:oid");
    }
    if ( cardinality.testFlag(AlephERP::PointToMaster) || cardinality.testFlag(AlephERP::PointToAll) )
    {
        if ( !where.isEmpty() )
        {
            where = where + " OR " ;
        }
        where = where + QString("relatedoid=:childoid");
    }
    sql = sql.arg(alephERPSettings->systemTablePrefix()).arg(where);
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("RelatedDAO::countRelatedElements: [%1]").arg(sql));
    if ( !qry->prepare(sql) )
    {
        writeDbMessages(qry.data());
        return hash;
    }
    qry->bindValue(":oid", bean->dbOid());
    qry->bindValue(":childoid", bean->dbOid());
    result = qry->exec();
    if ( !result )
    {
        writeDbMessages(qry.data());
        return hash;
    }
    while (qry->next())
    {
        hash[RelatedElement::stringToType(qry->value(1).toString())] = qry->value(0).toInt();
    }
    if ( !hash.contains(AlephERP::Record) )
    {
        hash[AlephERP::Record] = 0;
    }
    if ( !hash.contains(AlephERP::Email) )
    {
        hash[AlephERP::Email] = 0;
    }
    if ( !hash.contains(AlephERP::Document) )
    {
        hash[AlephERP::Document] = 0;
    }
    return hash;
}

int RelatedDAO::countRelatedElements(AlephERP::RelatedElementTypes type, BaseBean *bean, AlephERP::RelatedElementCardinalities cardinality)
{
    bool result;

    if ( bean->dbState() == BaseBean::INSERT )
    {
        int count = 0;
        RelatedElementPointerList list = bean->getRelatedElements(AlephERP::NoneAll, AlephERP::PointToAll, false);
        foreach (RelatedElement *element, list)
        {
            if ( cardinality.testFlag(element->cardinality()) && element->type() == type )
            {
                count++;
            }
        }
        return count;
    }
    QScopedPointer<QSqlQuery> qry(new QSqlQuery(Database::getQDatabase()));
    QString sql = QString("SELECT count(*) as column1 FROM %1_relations WHERE %2 AND relationtype=:relationtype");
    QString where;
    if ( cardinality.testFlag(AlephERP::PointToChild) || cardinality.testFlag(AlephERP::PointToAll) )
    {
        where = QString("masteroid=:oid");
    }
    if ( cardinality.testFlag(AlephERP::PointToMaster) || cardinality.testFlag(AlephERP::PointToAll) )
    {
        if ( !where.isEmpty() )
        {
            where = where + " OR ";
        }
        where = where + QString("relatedoid=:childoid");
    }
    sql = sql.arg(alephERPSettings->systemTablePrefix()).arg(where);
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("RelatedDAO::countRelatedElements: [%1]").arg(sql));
    if ( !qry->prepare(sql) )
    {
        writeDbMessages(qry.data());
        return -1;
    }
    qry->bindValue(":oid", bean->dbOid());
    qry->bindValue(":childoid", bean->dbOid());
    qry->bindValue(":relationtype", RelatedElement::typeToString(type));
    result = qry->exec();
    if ( !result )
    {
        writeDbMessages(qry.data());
        return -1;
    }
    qry->first();
    return qry->value(0).toInt();
}

bool RelatedDAO::deleteRelations(BaseBean *bean, const QString &idTransaction, const QString &connectionName)
{
    bool result;

    if ( bean->metadata()->relatedElementsContentToBeDelete().size() > 0 )
    {
        foreach (const AlephERP::RelatedElementsContentToBeDeleted &item, bean->metadata()->relatedElementsContentToBeDelete() )
        {
            RelatedElementPointerList itemsToBeDeleted;
            if ( item.category.isEmpty() || item.category == "*" )
            {
                itemsToBeDeleted = bean->getRelatedElements(item.type, item.cardinality);
            }
            else
            {
                itemsToBeDeleted = bean->getRelatedElementsByCategory(item.category, item.type, item.cardinality);
            }
            foreach (RelatedElementPointer element, itemsToBeDeleted)
            {
                if ( item.type == AlephERP::NoneAll || item.type == element->type() )
                {
                    if ( element->type() == AlephERP::Record )
                    {
                        if ( item.tableName == "*" || item.tableName == element->relatedTableName() )
                        {
                            if ( bean->actualContext().isEmpty() || !AERPTransactionContext::instance()->isOnTransaction(element->relatedBean()) )
                            {
                                if ( !BaseDAO::remove(element->relatedBean().data(), idTransaction, connectionName) )
                                {
                                    return false;
                                }
                            }
                        }
                    }
                    else if ( element->type() == AlephERP::Document )
                    {
#ifdef ALEPHERP_DOC_MANAGEMENT
                        QString error;
                        if ( !AERPDocumentDAOWrapper::instance()->deleteDocument(element->document()) )
                        {
                            QLogger::QLog_Error(AlephERP::stLogOther, QString("RelatedDAO::deleteRelations: [%1]").arg(AERPDocumentDAOWrapper::instance()->lastMessage()));
                            return false;
                        }
#endif
                    }
                    else if ( element->type() == AlephERP::Email )
                    {
#ifdef ALEPHERP_SMTP_SUPPORT
                        EmailObject obj = element->email();
                        if ( !EmailDAO::removeEmail(obj) )
                        {
                            QLogger::QLog_Error(AlephERP::stLogOther, QString("RelatedDAO:: deleteRelations: [%1]").arg(EmailDAO::lastErrorMessage()));
                            return false;
                        }
#endif
                    }
                }
            }
        }
    }

    QScopedPointer<QSqlQuery> qry(new QSqlQuery(Database::getQDatabase()));
    QString sql = QString("DELETE FROM %1_relations WHERE masteroid=:masteroid OR relatedoid=:relatedoid").
                  arg(alephERPSettings->systemTablePrefix());
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("RelatedDAO::deleteRelations: [%1]").arg(sql));
    if ( !qry->prepare(sql) )
    {
        writeDbMessages(qry.data());
        return false;
    }
    qry->bindValue(":masteroid", bean->dbOid());
    qry->bindValue(":relatedoid", bean->dbOid());
    result = qry->exec();
    if ( !result )
    {
        writeDbMessages(qry.data());
        return false;
    }


    return true;
}

