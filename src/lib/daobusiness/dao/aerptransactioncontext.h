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
#ifndef AERPTRANSACTIONCONTEXT_H
#define AERPTRANSACTIONCONTEXT_H

#include <QtCore>
#include "dao/beans/basebean.h"

class BaseBean;
class BeansReferenceList;
class AERPTransactionContextPrivate;

/**
 * @brief The AERPTransactionContext class
 * Se definirán contexto transaccionales en los que se realizarán las acciones de almacenamiento en
 * basde de datos. La idea básica es que cuando se invoca el método save de un bean, éste llamará
 * a este contexto, y se introducirá en él, de modo que cuando se realice el commit del mismo,
 * se almacenen todos los datos en base de datos.
 */
class AERPTransactionContext : public QObject
{
    Q_OBJECT

private:
    AERPTransactionContextPrivate *d;

protected:
    explicit AERPTransactionContext(QObject *parent = 0);

public:
    ~AERPTransactionContext();

    static AERPTransactionContext * instance();

    bool addPreviousSqlToContext(const QString &contextName, const QString &sql);
    bool addFinalSqlToContext(const QString &contextName, const QString &sql);
    bool addToContext(const QString &contextName, const BaseBeanPointer &bean);
    bool discardFromContext(const BaseBeanPointer &bean, bool includeFatherBeansOnDiscard = true, QStack<BaseBean *> *discardStak = NULL);
    bool discardContext(const QString &contextName);
    bool commit(const QString &contextName, bool discardContextOnSuccess = true);
    bool saveOneToOneIds(BaseBeanPointer bean, const QString &contextName);
    void waitCommitToEnd(const QString &contextName = "");
    bool rollback(const QString &contextName);
    bool isContextEmpty(const QString &contextName);
    int objectsContextSize(const QString &contextName);
    QString lastErrorMessage() const;
    void setDatabase(const QString &name);
    QString database() const;
    BaseBeanPointerList beansOnContext(const QString &contextName);
    BaseBeanPointerList beansOrderedToPersist(const QString &contextName);
    bool isDirty(const QString &contextName);
    bool isOnTransaction(BaseBeanPointer bean);
    QList<QVariant> registeredCounterValues(DBField *fld);
    void registerCounterValue(DBField *fld, const QVariant &counterValue);
    bool doingCommit(const QString &contextName = "");

signals:
    void beforeSaveBean(BaseBean *bean);
    void beforeDeleteBean(BaseBean *bean);
    void beforeAction(BaseBean *bean);
    void beanSaved(BaseBean *bean);
    void beanDeleted(BaseBean *bean);
    void workingWithBean(BaseBean *bean);
    void beanModified(BaseBean *bean, bool modified);
    void beanAddedToContext(QString contextName, BaseBean *bean);
    void transactionInited(QString contextName, int items);
    void transactionCommited(QString contextName);
    void transactionAborted(QString contextName);
    void preparationInited(QString contextName, int items);
    void preparationFinished(QString contextName);
    void orderingInited(QString contextName, int items);
    void orderingFinished(QString contextName);
    void validationInited(QString contextName, int items);
    void validationFinished(QString contextName);

public slots:
    void cancel(const QString &contextName);
};

#endif // AERPTRANSACTIONCONTEXT_H
