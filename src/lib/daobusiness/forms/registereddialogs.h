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
#ifndef REGISTEREDDIALOGS_H
#define REGISTEREDDIALOGS_H

#include <QtCore>

class AERPBaseDialog;

/**
  Todos los objetos de tipo AERPBaseDialog se registrarán en esta clase.
  Esto permitirá construir funciones que devuelvan referencias a este tipo de objetos
  cuando estén abiertos y según el nombre que tienen en la tabla de sistema
  */
class RegisteredDialogs
{
public:
    RegisteredDialogs();

    static QList<AERPBaseDialog *> m_list;

    static void registerDialog(AERPBaseDialog *dialog);
    static void unRegisterDialog(AERPBaseDialog *dialog);
    static AERPBaseDialog * dialog(const QString &name);

};

#endif // REGISTEREDDIALOGS_H
