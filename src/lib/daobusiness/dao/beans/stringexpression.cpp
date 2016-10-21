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
#include "stringexpression.h"
#include "dao/dbobject.h"
#include "dao/beans/basebean.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbfieldmetadata.h"
#include "business/aerpspellnumber.h"
#include "configuracion.h"

QString StringExpression::applyFunctions(const QString &token, const QString &displayValue)
{
    QString result = displayValue;
    bool trimmed = false, toUpper = false, toLower = false;
    QStringList list = token.split(".");

    if ( list.contains("trimmed") )
    {
        trimmed = true;
        list.removeAll("trimmed");
    }
    if ( list.contains("toUpper") )
    {
        toUpper = true;
        list.removeAll("toUpper");
    }
    if ( list.contains("toLower") )
    {
        toLower = true;
        list.removeAll("toLower");
    }

    if ( trimmed )
    {
        result = result.trimmed();
    }
    if ( toUpper )
    {
        result = result.toUpper();
    }
    if ( toLower )
    {
        result = result.toLower();
    }
    return result;
}

QString StringExpression::tokenWithoutFunctionsOrDisplay(const QString &token)
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

StringExpression::StringExpression()
{
    valid = false;
}

StringExpression::StringExpression(const StringExpression &other)
{
    this->expression = other.expression;
    this->paths = other.paths;
    this->valid = other.valid;
}

StringExpression &StringExpression::operator =(const StringExpression &other)
{
    this->expression = other.expression;
    this->paths = other.paths;
    this->valid = other.valid;
    return *this;
}

QString StringExpression::applyExpressionRules(DBObject *obj, const QHash<QString, QString> otherEntries, bool counterValue)
{
    QString result = this->expression;
    QStringList values;

    BaseBean *bean = qobject_cast<BaseBean *>(obj);
    if ( bean == NULL )
    {
        DBField *fld = qobject_cast<DBField *>(obj);
        if ( fld != NULL )
        {
            bean = fld->bean();
        }
        else
        {
            DBRelation *rel = qobject_cast<DBRelation *>(obj);
            if ( rel != NULL )
            {
                bean = rel->ownerBean();
            }
        }
    }
    if ( bean == NULL )
    {
        qDebug() << "StringExpression::applyExpressionRules: El objeto pasado como parámetro es nulo";
    }

    // Primero buscamos los objetos definidos como variables
    if ( bean != NULL )
    {
        foreach (const QString &path, this->paths)
        {
            if ( otherEntries.contains(path) )
            {
                values.append(otherEntries[path]);
            }
            else
            {
                DBObject *obj = bean->navigateThroughProperties(path);
                if ( obj != NULL )
                {
                    DBField *fldChild = qobject_cast<DBField *>(obj);
                    if ( fldChild != NULL )
                    {
                        if ( counterValue )
                        {
                            values.append(fldChild->valueOnCounter());
                        }
                        else
                        {
                            values.append(fldChild->displayValue());
                        }
                        if ( !dbObjectsOnCalc.contains(fldChild) )
                        {
                            dbObjectsOnCalc.append(fldChild);
                        }
                    }
                }
            }
        }
    }
    int index = 1;
    // Reemplazamos los valores de las variables
    foreach (const QString &val, values)
    {
        QString token = QString("{%1}").arg(index);
        result = result.replace(token, val);
        index++;
    }
    // Y ahora se reemplazan valores especiales
    QHashIterator<QString, QString> it(otherEntries);
    while (it.hasNext())
    {
        it.next();
        QString displayValue = applyFunctions(it.key(), it.value());
        QString token = QString("{%1}").arg(it.key());
        result = result.replace(token, displayValue);
    }
    if ( bean != NULL )
    {
        // También buscamos el resto de elementos entre llaves, que podamos hacer coincidir con objetos con propiedades
        QRegExp exp("\\{[^\\}]*\\}*");
        exp.setCaseSensitivity(Qt::CaseInsensitive);
        int idx = exp.indexIn(result);
        while (idx != -1)
        {
            QStringList list = exp.capturedTexts();
            if ( !list.isEmpty() )
            {
                foreach (const QString &item, list)
                {
                    if ( !item.isEmpty() )
                    {
                        // Eliminamos los corchetes { } de inicio y fin
                        QString token = item.mid(1, item.size()-2);
                        QString pathToFind = tokenWithoutFunctionsOrDisplay(token);
                        DBObject *obj = bean->navigateThroughProperties(pathToFind, true);
                        if ( obj != NULL )
                        {
                            DBField *fldChild = qobject_cast<DBField *>(obj);
                            if ( fldChild != NULL )
                            {
                                QString displayValue = !counterValue ?
                                            fldChild->displayValue() :
                                            fldChild->valueOnCounter();
                                if ( token.contains("spellValue") )
                                {
                                    QString language;
                                    if ( alephERPSettings->locale()->language() == QLocale::Spanish )
                                    {
                                        language = "es";
                                    }
                                    else if ( alephERPSettings->locale()->language() == QLocale::English )
                                    {
                                        language = "en";
                                    }
                                    displayValue = AERPSpellNumber::spellNumber(fldChild->value().toDouble(), fldChild->metadata()->partD(), language);
                                }
                                displayValue = applyFunctions(token, displayValue);
                                result = result.replace(item, displayValue);
                                if ( !dbObjectsOnCalc.contains(fldChild) )
                                {
                                    dbObjectsOnCalc.append(fldChild);
                                }
                            }
                            else
                            {
                                result = result.replace(item, "");
                            }
                        }
                        else
                        {
                            result = result.replace(item, "");
                        }
                    }
                }
            }
            idx = exp.indexIn(result);
        }
    }
    return result;
}

/**
 * @brief StringExpression::fieldsInvolvedOnCalc
 * Devuelve los fields de este bean a partir del cual surge el cálculo: Los fields que tiene relaciones cuyos elementos
 * se mostrarán.
 * @param bean
 * @return
 */
QList<DBField *> StringExpression::fieldsInvolvedOnCalc(BaseBean *bean)
{
    QList<DBField *> fldList;

    // También buscamos el resto de elementos entre llaves, que podamos hacer coincidir con objetos con propiedades
    QRegExp exp("\\{[^\\}]*\\}*");
    QString result = this->expression;

    exp.setCaseSensitivity(Qt::CaseInsensitive);
    int idx = exp.indexIn(result);
    while (idx != -1)
    {
        QStringList list = exp.capturedTexts();
        if ( !list.isEmpty() )
        {
            foreach (const QString &item, list)
            {
                if ( !item.isEmpty() )
                {
                    // Eliminamos los corchetes { } de inicio y fin
                    QString token = item.mid(1, item.size()-2);
                    QString pathToFind = tokenWithoutFunctionsOrDisplay(token);
                    QString firstItemOnPath = pathToFind.split(".").first();
                    DBObject *obj = bean->navigateThroughProperties(firstItemOnPath, true);
                    if ( obj != NULL )
                    {
                        DBField *fldChild = qobject_cast<DBField *>(obj);
                        if ( fldChild != NULL && !fldList.contains(fldChild) )
                        {
                            fldList.append(fldChild);
                        }
                        else
                        {
                            DBRelation *rel = qobject_cast<DBRelation *>(obj);
                            if ( rel != NULL )
                            {
                                foreach (DBField *fld, bean->fields())
                                {
                                    if ( fld->relations().contains(rel) && !fldList.contains(fld) )
                                    {
                                        fldList.append(fld);
                                    }
                                }
                            }
                        }
                    }
                    result = result.replace(item, "");
                }
            }
        }
        idx = exp.indexIn(result);
    }
    return fldList;
}
