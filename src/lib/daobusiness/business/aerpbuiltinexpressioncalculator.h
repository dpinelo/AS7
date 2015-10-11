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
#ifndef AERPBUILTINEXPRESSIONCALCULATOR_H
#define AERPBUILTINEXPRESSIONCALCULATOR_H

#include <QtCore>
#include "dao/beans/dbfieldmetadata.h"

class DBField;
class AERPBuiltInExpressionCalculatorPrivate;

/**
 * @brief The AERPBuiltInExpressionCalculator class
 * Objeto encargado de realizar el cálculo rápido con las expresiones internas
 */
class AERPBuiltInExpressionCalculator
{
private:
    AERPBuiltInExpressionCalculatorPrivate *d;

public:
    explicit AERPBuiltInExpressionCalculator();
    AERPBuiltInExpressionCalculator(const AERPBuiltInExpressionCalculator &other);
    virtual ~AERPBuiltInExpressionCalculator();

    BuiltInExpressionDef * expression() const;
    void setExpression(const BuiltInExpressionDef &expression);
    void setField(DBField *fld);
    void setBean(BaseBean *bean);
    void setFieldType(QVariant::Type type);
    bool isWorking();
    QString id() const;

    static double * searchForVariable(const char *aVarName, void *pData);

    virtual QVariant value();

};

#endif // AERPBUILTINEXPRESSIONCALCULATOR_H
