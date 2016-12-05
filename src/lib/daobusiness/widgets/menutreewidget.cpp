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
#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include <aerpcommon.h>
#include "menutreewidget.h"
#include "business/aerploggeduser.h"

MenuTreeWidget::MenuTreeWidget(QWidget *parent) :
    QTreeWidget(parent)
{
    m_init = false;
}

void MenuTreeWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    if ( !m_init )
    {
        init();
    }
}

void MenuTreeWidget::init()
{
    QFont font;
    font.setBold(true);
    // Buscamos la barra de menú si existiera
    QMenuBar *menu = menuBar();
    if ( menu == NULL )
    {
        return;
    }
    QList<QAction *> list = menu->actions();
    setColumnCount(1);
    setAnimated(true);
    setDragEnabled(true);
    QTreeWidget::clear();
    foreach (QAction *m, list)
    {
        // Ahora vamos recorriendo uno a uno los menus hijos
        addMenuChildren(m, NULL);
    }
    deleteEmptyItems();
    connect(this, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(itemHasBeenClicked(QTreeWidgetItem*,int)));
    m_init = true;
}

QMenuBar * MenuTreeWidget::menuBar()
{
    QObject *temp = this;
    while ( temp != 0 )
    {
        QMainWindow *dlg = qobject_cast<QMainWindow *>(temp);
        if ( dlg != 0 )
        {
            return dlg->menuBar();
        }
        temp = temp->parent();
    }
    return NULL;
}

void MenuTreeWidget::addMenuChildren(QAction *action, QTreeWidgetItem *parent)
{
    if ( action == NULL )
    {
        return;
    }
    if ( action->menu() == NULL )
    {
        if ( !action->isSeparator() )
        {
            addAction(action, parent);
        }
    }
    else
    {
        QFont font;
        font.setBold(true);
        QTreeWidgetItem *item = new QTreeWidgetItem(parent, QStringList(action->text()));
        item->setBackgroundColor(0, Qt::lightGray);
        item->setTextAlignment(0, Qt::AlignLeft);
        item->setFont(0, font);
        item->setData(0, Qt::UserRole, QVariant(0));
        if (parent == NULL)
        {
            addTopLevelItem(item);
        }
        m_items.append(item);

        QList<QAction *> menuActions = action->menu()->actions();
        foreach (QAction *childAction, menuActions)
        {
            addMenuChildren(childAction, item);
        }
    }
}

void MenuTreeWidget::addAction(QAction *action, QTreeWidgetItem *parent)
{
    bool allowed = false;
    if ( action == NULL )
    {
        return;
    }
    if ( action->objectName().contains("table") )
    {
        QString tableName = action->objectName();
        tableName.replace("table_", "");
        if ( AERPLoggedUser::instance()->checkMetadataAccess('r', tableName) )
        {
            allowed = true;
        }
    }
    if ( allowed && !action->isSeparator() && action->isVisible() )
    {
        QVariant actionPtr = (qulonglong) action;
        QTreeWidgetItem  *itemAction = new QTreeWidgetItem(parent, QStringList(action->text()));
        itemAction->setIcon(0, action->icon());
        itemAction->setDisabled(!action->isEnabled());
        itemAction->setData(0, Qt::UserRole, actionPtr);
        m_items.append(itemAction);
    }
}

QMimeData *MenuTreeWidget::mimeData(const QList<QTreeWidgetItem *> items) const
{
    QMimeData *mimeData = new QMimeData;
    QString text;
    foreach (QTreeWidgetItem *item, items)
    {
        QAction *act = (QAction *) (item->data(0, Qt::UserRole).toULongLong());
        if ( text.isEmpty() )
        {
            text = act->objectName();
        }
        else
        {
            text = QString("%1;%2").arg(mimeData->text(), act->objectName());
        }
    }
    mimeData->setData(AlephERP::stMimeDataAction, text.toUtf8());
    return mimeData;
}

void MenuTreeWidget::deleteEmptyItems()
{
    for ( int i = 0 ; i < m_items.size() ; i++ )
    {
        QTreeWidgetItem *item = m_items.at(i);
        QVariant data = item->data(0, Qt::UserRole);
        if ( data.toInt() == 0 && item->childCount() == 0 )
        {
            this->removeItemWidget(item, 0);
            delete item;
            m_items.removeAt(i);
            i = 0;
        }
    }
}

void MenuTreeWidget::itemHasBeenClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column)
    if ( item != NULL )
    {
        QAction *action = (QAction *) (item->data(0, Qt::UserRole).toULongLong());
        if ( action != NULL )
        {
            action->trigger();
        }
    }
}
