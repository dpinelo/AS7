/***************************************************************************
 *   Copyright (C) 2011 by David Pinelo   *
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
#ifndef HISTORYDAO_H
#define HISTORYDAO_H

#include <QtCore>
#include <aerpcommon.h>
#include "dao/beans/basebean.h"

/**
 * @brief The HistoryDAO class
 * Clase de acceso a datos para el almacenamiento de datos históricos
 */
class HistoryDAO : public QObject
{
    Q_OBJECT
private:

public:
    explicit HistoryDAO(QObject *parent = 0);

    static QString createData(BaseBean *bean);
    static bool insertEntry(BaseBean *bean, const QString &idTransaction);
    static bool updateEntry(BaseBean *bean, const QString &idTransaction);
    static bool removeEntry(BaseBean *bean, const QString &idTransaction);
    static QHash<QString, QVariant> lastEntry(BaseBean *bean, const QString &connection);

    static bool insertEntry(BaseBeanPointerList bean, const QString &idTransaction);

    static AlephERP::HistoryItemTransactionList historyEntries(BaseBeanPointer bean, const QString &connection = "");

signals:

public slots:

};

#endif // HISTORYDAO_H
