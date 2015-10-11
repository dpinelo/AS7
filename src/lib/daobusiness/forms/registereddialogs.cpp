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

#include "forms/registereddialogs.h"
#include "forms/perpbasedialog.h"

QList<AERPBaseDialog *> RegisteredDialogs::m_list;

RegisteredDialogs::RegisteredDialogs()
{
}

void RegisteredDialogs::registerDialog(AERPBaseDialog *dialog)
{
    RegisteredDialogs::m_list << dialog;
}

void RegisteredDialogs::unRegisterDialog(AERPBaseDialog *dialog)
{
    int index = RegisteredDialogs::m_list.indexOf(dialog);
    RegisteredDialogs::m_list.removeAt(index);
}

AERPBaseDialog * RegisteredDialogs::dialog(const QString &name)
{
    foreach ( AERPBaseDialog * dialog, RegisteredDialogs::m_list )
    {
        if ( dialog->objectName() == name )
        {
            return dialog;
        }
    }
    return NULL;
}
