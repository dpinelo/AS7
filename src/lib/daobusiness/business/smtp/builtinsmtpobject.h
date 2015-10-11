/***************************************************************************
 *   Copyright (C) 2014 by David Pinelo                                    *
 *   alepherp@alephsistemas.es                                             *
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
#ifndef BUILTINSMTPOBJECT_H
#define BUILTINSMTPOBJECT_H

#include <QtCore>
#include <alepherpglobal.h>
#include "business/smtp/smtpobject.h"
#include "business/smtp/aerpsmtpinterface.h"

/**
 * @brief The AERPBuiltInSMTPObject class
 * Clase para envío de correos electrónicos a partir de código interno
 */
class ALEPHERP_DLL_EXPORT AERPBuiltInSMTPObject : public SMTPObject
{
    Q_OBJECT

public:
    explicit AERPBuiltInSMTPObject(QObject *parent);
    virtual ~AERPBuiltInSMTPObject();

signals:

public slots:
    virtual bool send();

};

class ALEPHERP_DLL_EXPORT AERPBuiltInSMTPIface : public AERPSMTPIface
{
public:
    explicit AERPBuiltInSMTPIface();

    /** Devuelve un objeto de tipo SMTPObject con el código para el envío del correo */
    virtual SMTPObject *smtpObject(QObject *parent);
};

#endif // BUILTINSMTPOBJECT_H
