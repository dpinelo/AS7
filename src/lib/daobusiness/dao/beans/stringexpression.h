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
#ifndef STRINGEXPRESSION_H
#define STRINGEXPRESSION_H

#include <QtCore>

class DBObject;
class DBField;
class BaseBean;

/**
 * @brief The StringExpression class
 * Esta clase permitirá el uso de expresiones de cadena para el cálculo de valores.
 * Por ejemplo
 *       <displayValueExpression>
 *           <expression>{displayValue} {divisas.father.simbolo}</expression>
 *       </displayValueExpression>
 * Esta expresión se utiliza para presentar en "bonito" un campo. displayValue es el valor precalculado del DBField en el que
 * está contenida esta expresión. divisas es una relación de este bean al que pertenece este field, y por propiedades
 * se llega al DBField simbolo, cuyo value es el que se escoge.
 * Se admiten otras expresiones como por ejemplo
 * {facturasprov.father.importe.spellNumber}
 * donde spellNumber es una función que devolverá, según el locale de la aplicación el número "nombrado".
 * Si se quiere directamente el numero nombrado del campo, se aplica {spellNumber}
 */
class StringExpression
{
private:
    QString applyFunctions(const QString &function, const QString &displayValue);
    static QString tokenWithoutFunctionsOrDisplay(const QString &token);

public:
    QString expression;
    QStringList paths;
    bool valid;
    /** Esta estructura contendrá los DBFields que estarán involucrados en el cálculo de esta expresión. Tendrá datos
     * tras llamar a applyExpressionRules */
    QList<DBObject *> dbObjectsOnCalc;

    StringExpression();
    StringExpression(const StringExpression &other);
    StringExpression &operator = (const StringExpression &other);

    QString applyExpressionRules(DBObject *obj, const QHash<QString, QString> otherEntries = QHash<QString, QString>(), bool counterValue = false);
    QList<DBField *> fieldsInvolvedOnCalc(BaseBean *bean);

};

Q_DECLARE_METATYPE(StringExpression)

#endif // STRINGEXPRESSION_H
