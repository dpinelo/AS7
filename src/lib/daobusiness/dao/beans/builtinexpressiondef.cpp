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
#include "builtinexpressiondef.h"
#include "qlogger.h"
#include <aerpcommon.h>

BuiltInExpressionDef::BuiltInExpressionDef()
{
    m_valid = false;
}

BuiltInExpressionDef::BuiltInExpressionDef(const BuiltInExpressionDef &other)
{
    this->m_expression = other.m_expression;
    this->m_variables = other.m_variables;
    this->m_valid = other.m_valid;
    this->m_varNames = other.m_varNames;
}

BuiltInExpressionDef &BuiltInExpressionDef::operator =(const BuiltInExpressionDef &other)
{
    this->m_expression = other.m_expression;
    this->m_variables = other.m_variables;
    this->m_valid = other.m_valid;
    this->m_varNames = other.m_varNames;
    return *this;
}

QStringList BuiltInExpressionDef::variables() const
{
    return m_variables;
}

void BuiltInExpressionDef::setVariables(const QStringList &val)
{
    m_variables = val;
}

void BuiltInExpressionDef::addVariable(const QString &value)
{
    m_variables.append(value);
}

QStringList BuiltInExpressionDef::varNames() const
{
    return m_varNames;
}

void BuiltInExpressionDef::setVarNames(const QStringList &val)
{
    m_varNames = val;
}

void BuiltInExpressionDef::addVarName(const QString &value)
{
    m_varNames.append(value);
}

QString BuiltInExpressionDef::expression() const
{
    return m_expression;
}

void BuiltInExpressionDef::setExpression(const QString &val)
{
    m_expression = val;
    processExpression();
}

bool BuiltInExpressionDef::valid() const
{
    return m_valid;
}

void BuiltInExpressionDef::setValid(bool value)
{
    m_valid = value;
}

void BuiltInExpressionDef::processExpression()
{
    // Buscamos datos entre llaves, que sustituiremos por nombres adecuados, para quedarnos
    // con la expresión que nos lleva a un campo...
    QString result = m_expression;
    QLogger::QLog_Debug(AlephERP::stLogOther, QString("BuiltInExpressionDef::processExpression: Expresión inicial: [%1]").arg(m_expression));
    QRegExp exp("\\{[^\\}]*\\}*");
    exp.setCaseSensitivity(Qt::CaseInsensitive);

    int varCount = 1;
    int idx = exp.indexIn(result);
    while (idx != -1)
    {
        const QStringList list = exp.capturedTexts();
        if ( !list.isEmpty() )
        {
            for (const QString &item : list)
            {
                if ( !item.isEmpty() )
                {
                    // Eliminamos los corchetes { } de inicio y fin
                    QString token = item.mid(1, item.size()-2);
                    QString variable = token;
                    QString varName = QString("aerpVar%1").arg(varCount);
                    result = result.replace(item, varName);
                    addVariable(variable);
                    addVarName(varName);
                }
            }
        }
        idx = exp.indexIn(result);
    }
    m_expression = result;
    QLogger::QLog_Debug(AlephERP::stLogOther, QString("BuiltInExpressionDef::processExpression: Expresión final: [%1]").arg(result));
}


