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
#include <QtGui>
#include <QtDesigner>

#include <alepherpdaobusiness.h>
#include <alepherpdbcommons.h>

class DBChooseRecordButtonTaskMenuPrivate
{
public:
    QAction *m_editFieldsAction;
    QAction *m_replaceFieldsAction;
    DBChooseRecordButton *m_recordButton;

    DBChooseRecordButtonTaskMenuPrivate()
    {
        m_editFieldsAction = NULL;
        m_recordButton = NULL;
        m_replaceFieldsAction = NULL;
    }
};

DBChooseRecordButtonTaskMenu::DBChooseRecordButtonTaskMenu(DBChooseRecordButton *button, QObject *parent) :
    DBChooseFieldNameTaskMenu(button, parent), d(new DBChooseRecordButtonTaskMenuPrivate)
{
    d->m_recordButton = button;
    d->m_replaceFieldsAction = new QAction(tr("AlephERP: Edit Replace Fields when select a record..."), this);
    connect(d->m_replaceFieldsAction, SIGNAL(triggered()), this, SLOT(editFields()));
    d->m_editFieldsAction = new QAction(trUtf8("AlephERP: Assign DB Field..."), this);
    connect(d->m_editFieldsAction, SIGNAL(triggered()), this, SLOT(selectFields()));
}

DBChooseRecordButtonTaskMenu::~DBChooseRecordButtonTaskMenu()
{
    delete d;
}

void DBChooseRecordButtonTaskMenu::editFields()
{
    QPointer<DBChooseRecordButtonAssignFieldsDlg> dlg = new DBChooseRecordButtonAssignFieldsDlg(d->m_recordButton);
    if ( !dlg->openFailed() )
    {
        dlg->exec();
    }
    delete dlg;
}

QAction *DBChooseRecordButtonTaskMenu::preferredEditAction() const
{
    return d->m_editFieldsAction;
}

QList<QAction *> DBChooseRecordButtonTaskMenu::taskActions() const
{
    QList<QAction *> list;
    list.append(d->m_editFieldsAction);
    list.append(d->m_replaceFieldsAction);
    return list;
}


