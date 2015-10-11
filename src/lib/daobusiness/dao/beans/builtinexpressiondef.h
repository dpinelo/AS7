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
#ifndef BUILTINEXPRESSIONDEF_H
#define BUILTINEXPRESSIONDEF_H

#include <QtCore>

class BuiltInExpressionDef
{
private:
    QStringList m_variables;
    QStringList m_varNames;
    QString m_expression;
    bool m_valid;

public:
    BuiltInExpressionDef();
    BuiltInExpressionDef(const BuiltInExpressionDef &other);
    BuiltInExpressionDef &operator = (const BuiltInExpressionDef &other);

    QStringList variables() const;
    void setVariables(const QStringList &val);
    void addVariable(const QString &value);

    QStringList varNames() const;
    void setVarNames(const QStringList &val);
    void addVarName(const QString &value);

    QString expression() const;
    void setExpression(const QString &val);

    bool valid() const;
    void setValid(bool value);

    void processExpression();
};

Q_DECLARE_METATYPE(BuiltInExpressionDef)

#endif // BUILTINEXPRESSIONDEF_H
