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
#ifndef ENVVARS_H
#define ENVVARS_H

#include <QtCore>
#include <alepherpglobal.h>

/*!
  Existirán algunas variables de entorno que, por ejemplo marcarán la visualización
  de datos. El ejemplo típico es cuando se quieran mostrar datos según un centro de trabajo.
  Esta clase, contendrá esas variables. Lo hará en forma de pares:
  nombre_variable:valor
  Estas variables de entorno además, podrán guardarse entre sesión y sesión, y se asignan
  a un usuario.
  */
class ALEPHERP_DLL_EXPORT EnvVars : public QObject
{
    Q_OBJECT

private:
    QHash<QString, QVariant> m_vars;
    QMutex m_mutex;

    EnvVars(QObject *parent);

public:

    static EnvVars *instance();

    QVariant var(const QString &var);
    QVariant var(const QString &userName, const QString &var);
    void setVar(const QString &var, const QVariant &v);
    bool setDbVar(const QString &var, const QVariant &v);
    bool setDbVar(const QString &userName, const QString &var, const QVariant &v);
    QHash<QString, QVariant> vars();
    void clear();
    bool contains(const QString &var);

signals:
    void varChanged(QString var, QVariant value);
    void varDbChanged(QString var, QVariant value);

};

#endif // ENVVARS_H
