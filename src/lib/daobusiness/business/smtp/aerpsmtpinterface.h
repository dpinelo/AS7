/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo                                    *
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
#ifndef AERPSMTPINTERFACE_H
#define AERPSMTPINTERFACE_H

#include <QtPlugin>
#include <QtCore>
#include <aerpcommon.h>
#include <alepherpglobal.h>

class SMTPObject;

/**
 * Interfaz para los plugins que permitan enviar correos por SMTP.
 */
class AERPSMTPIface
{
public:
    /** Devuelve un objeto de tipo SMTPObject con el código para el envío del correo */
    virtual SMTPObject *smtpObject(QObject *parent) = 0;
};

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(AERPSMTPIface, "es.alephsistemas.alepherp.AERPSMTPIface/1.0")
QT_END_NAMESPACE

#endif // AERPSMTPINTERFACE_H
