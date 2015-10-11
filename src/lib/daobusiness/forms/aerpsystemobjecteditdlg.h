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
#ifndef AERPSYSTEMOBJECTEDITDLG_H
#define AERPSYSTEMOBJECTEDITDLG_H

#include "dbrecorddlg.h"
#include "scripts/perpscriptcommon.h"
#include "dao/beans/basebean.h"


class AERPSystemObjectEditDlgPrivate;


/**
 * @brief The AERPSystemObjectEditDlg class
 * Formulario de edición de los objetos de sistema
 */
class AERPSystemObjectEditDlg : public DBRecordDlg
{
private:
    AERPSystemObjectEditDlgPrivate *d;

public:
    AERPSystemObjectEditDlg(BaseBeanPointer bean,
                            AlephERP::FormOpenType openType,
                            QWidget* parent = 0, Qt::WindowFlags fl = 0);
    AERPSystemObjectEditDlg(FilterBaseBeanModel *model, QItemSelectionModel *idx, const QHash<QString, QVariant> &fieldValueToSetOnNewBean,
                            AlephERP::FormOpenType openType,
                            QWidget* parent = 0, Qt::WindowFlags fl = 0);
    ~AERPSystemObjectEditDlg();

protected:
    virtual void setupMainWidget();
    virtual void execQs();

protected slots:
    virtual bool validate();
    virtual bool beforeSave();
    virtual void navigate(const QString &direction);

public slots:
    virtual bool save();
};

#endif // AERPSYSTEMOBJECTEDITDLG_H
