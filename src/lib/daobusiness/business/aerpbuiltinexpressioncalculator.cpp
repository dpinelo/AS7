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
#include "aerpbuiltinexpressioncalculator.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/basebean.h"
#include "qlogger.h"
#include <muParser.h>
#include "muParserBase.h"
#include "muParserDef.h"
#include "configuracion.h"

QThreadStorage< QList<AERPBuiltInExpressionCalculator *> > m_expressionCriticalZone;
double m_fooVar = 0;

class Variable
{
public:
    QString id;
    QByteArray name;
    double value;
    QString path;
    DBField *field;

    Variable()
    {
        value = 0;
        field = NULL;
        id = QString("%1").arg(alephERPSettings->uniqueId());
    }

    ~Variable()
    {
    }
};

class AERPBuiltInExpressionCalculatorPrivate
{
public:
    AERPBuiltInExpressionCalculator *q_ptr;
    /* Expression evaluation is done by calling the mupEval() function in the DLL version or the Eval() member
     * function of a parser object. When evaluating an expression for the first time the parser evaluates the
     * expression string directly and creates a bytecode during this first time evaluation. Every sucessive call to
     * Eval() will evaluate the bytecode directly unless you call a function that will silently reset the parser to
     * string parse mode. Some functions invalidate the bytecode due to possible changes in callback function
     * pointers or variable addresses. By doing so they effectively cause a recreation of the bytecode during the
     * next call to Eval(). */
    mu::Parser m_parser;
    QPointer<DBField> m_fld;
    QPointer<BaseBean> m_bean;
    BuiltInExpressionDef m_expression;
    /* Defining new Variables will reset the parser bytecode. Do not use this function just for changing the values
     * of variables! It would dramatically reduce the parser performance! Once the parser knows the address of the
     * variable there is no need to explicitely call a function for changing the value. Since the parser knows the
     * address it knows the value too so simply change the C++ variable in your code directly!
     * Por eso utilizamos esta expresión */
    QList<Variable *> m_variables;
    QVariant::Type m_type;
    QString m_id;

    explicit AERPBuiltInExpressionCalculatorPrivate(AERPBuiltInExpressionCalculator *qq) : q_ptr(qq)
    {
        m_id = QString("%1").arg(alephERPSettings->uniqueId());
        m_type = QVariant::Double;
    }

    void createVarsOnParser();
    void updateVarValues();
};

QString tokenWithoutFunctionsOrDisplay(const QString &token)
{
    QStringList list = token.split(".");
    list.removeAll("spellValue");
    list.removeAll("trimmed");
    list.removeAll("toUpper");
    list.removeAll("toLower");
    list.removeAll("value");
    list.removeAll("displayValue");
    return list.join(".");
}

void AERPBuiltInExpressionCalculatorPrivate::createVarsOnParser()
{
    if ( m_bean.isNull() || m_expression.expression().isEmpty() || !m_expression.valid() )
    {
        return;
    }
    std::string stdExp = m_expression.expression().toStdString();
    try
    {
        m_parser.SetExpr(stdExp);
    }
    catch (mu::Parser::exception_type &e)
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString::fromStdString(e.GetMsg()));
        return;
    }

    for ( int i = 0 ; i < m_expression.variables().size() ; i++ )
    {
        Variable *v = new Variable;
        v->path = m_expression.variables().at(i);
        v->value = 0;
        QString varName = m_expression.varNames().at(i);
        QString path = tokenWithoutFunctionsOrDisplay(v->path);
        DBObject *obj = m_bean->navigateThroughProperties(path);
        if ( obj != NULL )
        {
            DBField *fld = qobject_cast<DBField *>(obj);
            if (fld != NULL)
            {
                v->name = varName.toLocal8Bit();
                v->field = fld;
                QVariant variant;
                // Esta parte del código es CRÍTICA. Si var->field->value() empieza a escalar llamadas de values, por ser campos calculados
                // podría volver a invocarse el mismo, reescribiéndose, y haciendo crash la aplicación sin depuración posible.
                if ( !fld->isWorking() )
                {
                    variant = fld->value();
                }
                else
                {
                    variant = fld->rawValue();
                }
                bool ok;
                v->value = variant.toDouble(&ok);
                if ( ok )
                {
                    m_variables.append(v);
                    try
                    {
                        m_parser.DefineVar(v->name.constData(), &(v->value));
                    }
                    catch (mu::Parser::exception_type &e)
                    {
                        QLogger::QLog_Error(AlephERP::stLogOther, QString::fromStdString(e.GetMsg()));
                    }
                }
                else
                {
                    QLogger::QLog_Debug(AlephERP::stLogOther, QString("AERPBuiltInExpressionCalculatorPrivate::createVarsOnParser: El valor del field [%1] "
                             "no ha podido ser convertido a double. Valor: [%2]").arg(fld->metadata()->dbFieldName()).arg(variant.toString()));
                }
            }
        }
    }

    // Ahora buscamos las variables "internamente" definidas
    try
    {
        mu::varmap_type variables = m_parser.GetUsedVar();
        // Get the number of variables
        mu::varmap_type::const_iterator item = variables.begin();

        // Query the variables
        for (; item!=variables.end(); ++item)
        {
            QString varName = QString::fromStdString(item->first);
            QString path = tokenWithoutFunctionsOrDisplay(varName);
            DBObject *obj = m_bean->navigateThroughProperties(path);
            if ( obj != NULL )
            {
                DBField *fld = qobject_cast<DBField *>(obj);
                if ( fld != NULL )
                {
                    Variable *v = new Variable;
                    v->path = varName;
                    v->value = 0;
                    v->field = fld;
                    QVariant variant;
                    // Esta parte del código es CRÍTICA. Si var->field->value() empieza a escalar llamadas de values, por ser campos calculados
                    // podría volver a invocarse el mismo, reescribiéndose, y haciendo crash la aplicación sin depuración posible.
                    if ( !fld->isWorking() )
                    {
                        variant = fld->value();
                    }
                    else
                    {
                        variant = fld->rawValue();
                    }
                    bool ok;
                    v->value = variant.toDouble(&ok);
                    m_variables.append(v);
                }
            }
        }
    }
    catch (mu::Parser::exception_type &e)
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString::fromStdString(e.GetMsg()));
    }
}

void AERPBuiltInExpressionCalculatorPrivate::updateVarValues()
{
    static int anidado;
    if ( m_bean.isNull() )
    {
        return;
    }
    anidado++;
    foreach(Variable *v, m_variables)
    {
        if (v->field != NULL)
        {
            QVariant variant = v->field->rawValue();
            // Esta parte del código es CRÍTICA. Si var->field->value() empieza a escalar llamadas de values, por ser campos calculados
            // podría volver a invocarse el mismo, reescribiéndose, y haciendo crash la aplicación sin depuración posible.
            if ( !v->field->isWorking() )
            {
                variant = v->field->value();
            }
            bool ok;
            double dVal = variant.toDouble(&ok);
            if ( ok )
            {
                v->value = dVal;
            }
            else
            {
                QLogger::QLog_Debug(AlephERP::stLogOther, QString("AERPBuiltInExpressionCalculatorPrivate::setVarsValueOnParser: El valor del field [%1] "
                         "no ha podido ser convertido a double. Valor: [%2]").arg(v->field->metadata()->dbFieldName()).arg(variant.toString()));
            }
        }
    }
    anidado--;
}

AERPBuiltInExpressionCalculator::AERPBuiltInExpressionCalculator() :
    d(new AERPBuiltInExpressionCalculatorPrivate(this))
{
    d->m_parser.SetVarFactory(&(AERPBuiltInExpressionCalculator::searchForVariable), this);
}

AERPBuiltInExpressionCalculator::AERPBuiltInExpressionCalculator(const AERPBuiltInExpressionCalculator &other)
{
    this->d->m_bean = other.d->m_bean;
    this->d->m_expression = other.d->m_expression;
    this->d->m_fld = other.d->m_fld;
    this->d->m_type = other.d->m_type;
    foreach (Variable *var, other.d->m_variables)
    {
        Variable *thisVar = new Variable;
        thisVar->name = var->name;
        thisVar->path = var->path;
        thisVar->value = var->value;
        thisVar->field = var->field;
        this->d->m_variables.append(thisVar);
    }
}

AERPBuiltInExpressionCalculator::~AERPBuiltInExpressionCalculator()
{
    d->m_parser.ClearVar();
    qDeleteAll(d->m_variables);
    delete d;
}

BuiltInExpressionDef *AERPBuiltInExpressionCalculator::expression() const
{
    return &(d->m_expression);
}

void AERPBuiltInExpressionCalculator::setExpression(const BuiltInExpressionDef &expression)
{
    if ( m_expressionCriticalZone.localData().contains(this) )
    {
        return;
    }
    m_expressionCriticalZone.localData().append(this);
    d->m_expression = expression;
    m_expressionCriticalZone.localData().removeAll(this);
}

void AERPBuiltInExpressionCalculator::setField(DBField *fld)
{
    if ( m_expressionCriticalZone.localData().contains(this) || fld == d->m_fld.data() )
    {
        return;
    }
    m_expressionCriticalZone.localData().append(this);
    d->m_fld = fld;
    d->m_bean = fld->bean();
    d->m_expression = d->m_fld->metadata()->builtInExpression();

    d->m_parser.ClearVar();
    qDeleteAll(d->m_variables);
    d->m_variables.clear();
    d->createVarsOnParser();
    m_expressionCriticalZone.localData().removeAll(this);
}

void AERPBuiltInExpressionCalculator::setBean(BaseBean *bean)
{
    if ( m_expressionCriticalZone.localData().contains(this) || bean == d->m_bean.data() )
    {
        return;
    }
    m_expressionCriticalZone.localData().append(this);
    d->m_bean = bean;

    d->m_parser.ClearVar();
    qDeleteAll(d->m_variables);
    d->m_variables.clear();
    d->createVarsOnParser();
    m_expressionCriticalZone.localData().removeAll(this);
}

void AERPBuiltInExpressionCalculator::setFieldType(QVariant::Type type)
{
    d->m_type = type;
}

bool AERPBuiltInExpressionCalculator::isWorking()
{
    return m_expressionCriticalZone.localData().contains(this);
}

QString AERPBuiltInExpressionCalculator::id() const
{
    return d->m_id;
}

/**
 * @brief AERPBuiltInExpressionCalculator::searchForVariable
 * Esta función es una factoría para variables definidas en el código, sin predefinición.
 * @param aVarName
 * @param pData
 * @return
 */
double * AERPBuiltInExpressionCalculator::searchForVariable(const char *aVarName, void *pData)
{
    static int anidado;
    anidado++;
    AERPBuiltInExpressionCalculator *calc = static_cast<AERPBuiltInExpressionCalculator *>(pData);
    Variable *var = NULL;
    QString path(aVarName);
    if ( calc->d->m_bean.isNull() )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("AERPBuiltInExpressionCalculatorPrivate::searchForVariable: El bean ha sido borrado"));
        throw mu::ParserError("El bean ha sido borrado.");
    }
    else
    {
        foreach (Variable *v, calc->d->m_variables)
        {
            if ( v->path == path )
            {
                var = v;
            }
        }
        if ( var == NULL )
        {
            QString pathWithoutToken = tokenWithoutFunctionsOrDisplay(path);
            DBObject *obj = calc->d->m_bean->navigateThroughProperties(pathWithoutToken);
            if ( obj != NULL )
            {
                DBField *fld = qobject_cast<DBField *>(obj);
                if (fld != NULL)
                {
                    var = new Variable;
                    var->name = path.toLocal8Bit();
                    var->path = path;
                    var->field = fld;
                    calc->d->m_variables.append(var);
                }
                else
                {
                    QLogger::QLog_Error(AlephERP::stLogOther, QString("AERPBuiltInExpressionCalculatorPrivate::searchForVariable: No hay definida ninguna variable: [%1]").arg(path));
                    anidado--;
                    return &(m_fooVar);
                }
            }
        }
        if ( var != NULL && var->field != NULL )
        {
            // Esta parte del código es CRÍTICA. Si var->field->value() empieza a escalar llamadas de values, por ser campos calculados
            // podría volver a invocarse el mismo, reescribiéndose, y haciendo crash la aplicación sin depuración posible.
            if ( var->field->isWorking() )
            {
                var->value = var->field->rawValue().toDouble();
            }
            else
            {
                var->value = var->field->value().toDouble();
            }
            anidado--;
            return &(var->value);
        }
    }
    anidado--;
    return &(m_fooVar);
}

QVariant AERPBuiltInExpressionCalculator::value()
{
    QVariant data;
    double dVal = 0;
    if ( m_expressionCriticalZone.localData().contains(this) )
    {
        return 0;
    }
    if ( d->m_expression.varNames().size() == 1 )
    {
        if ( d->m_expression.expression() == d->m_expression.varNames().at(0) )
        {
            if ( !d->m_bean.isNull() )
            {
                QString path = tokenWithoutFunctionsOrDisplay(d->m_expression.variables().at(0));
                DBObject *obj = d->m_bean->navigateThroughProperties(path);
                if ( obj != NULL )
                {
                    if ( obj != NULL )
                    {
                        DBField *fld = qobject_cast<DBField *>(obj);
                        if (fld != NULL)
                        {
                            data = fld->value();
                        }
                    }
                }
            }
            return data;
        }
    }
    m_expressionCriticalZone.localData().append(this);
    d->updateVarValues();
    try
    {
        dVal = d->m_parser.Eval();
    }
    catch (mu::Parser::exception_type &e)
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString::fromStdString(e.GetMsg()));
    }
    m_expressionCriticalZone.localData().removeAll(this);
    if ( d->m_type == QVariant::Int )
    {
        data = (int) dVal;
    }
    else if ( d->m_type == QVariant::LongLong )
    {
        data = (qlonglong) dVal;
    }
    else
    {
        data = dVal;
    }
    return data;
}

