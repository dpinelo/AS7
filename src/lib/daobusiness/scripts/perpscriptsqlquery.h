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
#ifndef AERPSCRIPTSQLQUERY_H
#define AERPSCRIPTSQLQUERY_H

#include <QtCore>
#include <QtScript>

class AERPScriptSqlQuery;
class AERPScriptSqlQueryPrivate;

/*!
  Permite lanzar una consulta a base de datos desde el motor Javascript. La forma de utilizarlo
  es como sigue:

  var qry = new AERPSqlQuery;
  var sql = "SELECT max(codcliente) FROM clientes WHERE codgrupo='" + bean.defaultFieldValue("codgrupo") + "'";
  qry.prepare(sql);
  var result = qry.exec();
  if ( result == true && qry.first() ) {
      var ultimo = qry.value(0);
  }
  while (qry.next()) {
    do something...
  }
*/
class AERPScriptSqlQuery : public QObject, public QScriptable
{
    Q_OBJECT
private:
    AERPScriptSqlQueryPrivate *d;

public:
    explicit AERPScriptSqlQuery(QObject *parent = 0);
    virtual ~AERPScriptSqlQuery();

    Q_INVOKABLE void bindValue (const QString & placeholder, const QVariant & val);
    Q_INVOKABLE void bindValue (int pos, const QVariant & val);
    Q_INVOKABLE bool exec (const QString & sql);
    Q_INVOKABLE bool exec ();
    Q_INVOKABLE bool prepare (const QString & query);
    Q_INVOKABLE QVariant value (int index);
    Q_INVOKABLE bool first ();
    Q_INVOKABLE bool next ();
    Q_INVOKABLE int size ();
    static QScriptValue specialAERPScriptSqlQueryConstructor(QScriptContext *context, QScriptEngine *engine);

signals:

public slots:

};

Q_DECLARE_METATYPE(AERPScriptSqlQuery*)
/** Esto nos va a permitir crear objetos QSqlQuery desde QtScript */
Q_SCRIPT_DECLARE_QMETAOBJECT(AERPScriptSqlQuery, QObject*)

#endif // AERPSCRIPTSQLQUERY_H
