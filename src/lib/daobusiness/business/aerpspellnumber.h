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
#ifndef AERPSPELLNUMBER_H
#define AERPSPELLNUMBER_H

#include <QtCore>

typedef unsigned long Spellable;


/**
 * @brief Clase con metodos para nombrar o enumerar numeros
 */
class AERPSpellNumber
{
public:
    AERPSpellNumber();

    static QString spellNumber(Spellable n, const QString &language);
    static QString spellNumber(double n, int decimalPlaces, const QString &language);

};

#endif // AERPSPELLNUMBER_H
