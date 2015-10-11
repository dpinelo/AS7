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
#include "aggregatecalc.h"

AggregateCalc::AggregateCalc()
{
}

AggregateCalc &AggregateCalc::operator =(const AggregateCalc &other)
{
    this->operation = other.operation;
    this->expression = other.expression;
    this->field = other.field;
    this->relation = other.relation;
    this->script = other.script;
    this->filter = other.filter;
    return *this;
}

bool AggregateCalc::useExpression()
{
    foreach (const BuiltInExpressionDef &exp, expression)
    {
        if ( exp.valid() )
        {
            return true;
        }
    }
    return false;
}

bool AggregateCalc::useScript()
{
    foreach (const QString &script, script)
    {
        if ( !script.isEmpty() )
        {
            return true;
        }
    }
    return false;
}
