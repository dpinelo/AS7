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

#ifndef RELATEDDAO_H
#define RELATEDDAO_H

#include <QtCore>
#include <QtSql>
#include <aerpcommon.h>
#include "dao/beans/basebean.h"

class BaseBean;
class RelatedElement;
typedef QPointer<RelatedElement> RelatedElementPointer;

/**
 * @brief The RelatedDAO class
 * Clase de acceso a los elementos relacionados.
 */
class RelatedDAO : public QObject
{
    Q_OBJECT
public:
    explicit RelatedDAO(QObject *parent = 0);

    static void writeDbMessages(QSqlQuery *qry);
    static QString lastErrorMessage();

    static bool saveRelatedElements (BaseBean *bean, const QString &idTransaction = "");
    static bool saveRelatedElement (RelatedElementPointer element, const QString &idTransaction = "");
    static bool loadRelatedElements (BaseBean *bean);
    static QHash<AlephERP::RelatedElementTypes, int> countRelatedElements (BaseBean *bean, AlephERP::RelatedElementCardinalities cardinality);
    static int countRelatedElements (AlephERP::RelatedElementTypes type, BaseBean *bean, AlephERP::RelatedElementCardinalities cardinality);
    static bool deleteRelations(BaseBean *bean, const QString &idTransaction = "", const QString &connectionName = "");

signals:

public slots:

};

#endif // RELATEDDAO_H
