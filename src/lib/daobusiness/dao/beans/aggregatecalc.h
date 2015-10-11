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
#ifndef AGGREGATECALC_H
#define AGGREGATECALC_H

#include <QtCore>
#include "dao/beans/builtinexpressiondef.h"

class AggregateCalc
{
public:
    QString operation;
    QStringList relation;
    QStringList field;
    QStringList filter;
    QStringList script;
    QList<BuiltInExpressionDef> expression;

    AggregateCalc();
    AggregateCalc &operator = (const AggregateCalc &other);
    bool useExpression();
    bool useScript();
};

Q_DECLARE_METATYPE(AggregateCalc)

#endif // AGGREGATECALC_H
