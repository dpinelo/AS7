/*
 * OpenRPT report writer and rendering engine
 * Copyright (C) 2001-2014 by OpenMFG, LLC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * Please contact info@openmfg.com with any questions on this license.
 */
#ifndef __ORUTILS_H__
#define __ORUTILS_H__

#include <QString>
#include <QStringList>
#include <QSqlDatabase>

#include <xsqlquery.h>
#include <parameter.h>

//
// These classes are used by the original orRender class and the new
// ORPreRenderer class as internal structures for processing. There is
// no need to have or use these classes otherwise.
//


//
// Private class definitions
// These classes are convienience classes just used here
// so there is no need to expose them to the outside
//
//  Query Class
class orQuery {
  private:
    QString      qstrName;

    QString      qstrQuery;
    XSqlQuery   *qryQuery;

    QSqlDatabase _database;

  public:
    orQuery();
    orQuery(const QString &, const QString &, ParameterList, bool doexec, QSqlDatabase pDb = QSqlDatabase());

    virtual ~orQuery();

    __inline bool queryExecuted() const { return (qryQuery != 0); }
    bool execute();

    __inline XSqlQuery *getQuery() { return qryQuery; }
    __inline const QString &getSql() const { return qstrQuery; }
    __inline const QString &getName() const { return qstrName; }

    QStringList     missingParamList;
};


// Data class
class orData {
  private:
    orQuery *qryThis;
    QString qstrField;
    QString qstrValue;
    bool    _valid;

  public:
    orData();

    void  setQuery(orQuery *qryPassed);
    void  setField(const QString &qstrPPassed);

    __inline bool  isValid() const { return _valid; }

    const QString &getValue();
	const QVariant getVariant() const;
};

#endif // __ORUTILS_H__

