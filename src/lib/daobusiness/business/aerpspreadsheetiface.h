/***************************************************************************
 *   Copyright (C) 2014 by David Pinelo   *
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
#ifndef AERPSPREADSHEETIFACE_H
#define AERPSPREADSHEETIFACE_H

#include <QtPlugin>
#include <QtCore>
#include <aerpcommon.h>
#include <alepherpglobal.h>

class AERPSpreadSheet;

class AERPSpreadSheetIface
{
public:
    /** Si se produce un error en alguna función, aquí se obtiene el mensaje de error en formato
     * Human Readable */
    virtual QString lastMessage() const = 0;

    virtual bool wasInited() = 0;
    virtual bool init() = 0;
    virtual bool release() = 0;
    virtual QString type() = 0;
    virtual QString displayType() = 0;
    virtual AERPSpreadSheet *openFile(const QString &file) = 0;
    virtual bool writeFile(AERPSpreadSheet *info, const QString &file) = 0;
    virtual bool canWriteFiles() = 0;
    virtual void setProperties(const QVariantMap &properties) = 0;

};

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(AERPSpreadSheetIface, "es.alephsistemas.alepherp.AERPSpreadSheetIface/1.0")
QT_END_NAMESPACE

#endif // AERPSPREADSHEETIFACE_H

