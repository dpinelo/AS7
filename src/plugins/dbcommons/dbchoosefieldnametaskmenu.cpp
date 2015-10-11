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

class DBChooseFieldNameTaskMenuPrivate
{
public:
    QWidget *m_widget;
    QAction *m_editFieldsAction;

    DBChooseFieldNameTaskMenuPrivate()
    {
        m_widget = NULL;
        m_editFieldsAction = NULL;
    }
};

DBChooseFieldNameTaskMenu::DBChooseFieldNameTaskMenu(QWidget *widget, QObject *parent) :
    QObject(parent), d(new DBChooseFieldNameTaskMenuPrivate())
{
    d->m_widget = widget;
    d->m_editFieldsAction = new QAction(trUtf8("AlephERP: Assign DB Field..."), this);
    connect(d->m_editFieldsAction, SIGNAL(triggered()), this, SLOT(selectFields()));
}

DBChooseFieldNameTaskMenu::~DBChooseFieldNameTaskMenu()
{
    delete d;
}

void DBChooseFieldNameTaskMenu::selectFields()
{
    if ( DBBaseWidget::aerpBaseDialogTableName(d->m_widget).isEmpty() )
    {
        QMessageBox::warning(0,qApp->applicationName(), trUtf8("Debe indicar una tabla para el formulario. Para ello, rellene la propiedad tableName del diálogo principal. Éste debe ser de tipo AERPBaseDialog."), QMessageBox::Ok);
        return;
    }
    QPointer<DBChooseFieldNameDlg> dlg = new DBChooseFieldNameDlg(d->m_widget);
    if ( !dlg->openFailed() )
    {
        dlg->exec();
        if ( !dlg->field().isEmpty() )
        {
            if ( d->m_widget )
            {
                QDesignerFormWindowInterface *form;
                form = QDesignerFormWindowInterface::findFormWindow(d->m_widget);
                if ( form )
                {
                    form->cursor()->setWidgetProperty(d->m_widget, "fieldName", dlg->field());
                }
            }

        }
    }
    delete dlg;
}

QAction *DBChooseFieldNameTaskMenu::preferredEditAction() const
{
    return d->m_editFieldsAction;
}

QList<QAction *> DBChooseFieldNameTaskMenu::taskActions() const
{
    QList<QAction *> list;
    list.append(d->m_editFieldsAction);
    return list;
}

