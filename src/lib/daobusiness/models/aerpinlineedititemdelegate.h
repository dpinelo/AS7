/***************************************************************************
 *   Copyright (C) 2012 by David Pinelo									*
 *   alepherp@alephsistemas.es													*
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
#ifndef AERPINLINEEDITITEMDELEGATE_H
#define AERPINLINEEDITITEMDELEGATE_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include "aerpitemdelegate.h"

class AERPInlineEditItemDelegatePrivate;
class DBField;
class BaseBean;

class AERPInlineEditItemDelegate : public AERPItemDelegate
{
    Q_OBJECT

private:
    AERPInlineEditItemDelegatePrivate *d;

public:
    explicit AERPInlineEditItemDelegate(const QString &type, QObject * parent = 0);
    ~AERPInlineEditItemDelegate();

    virtual QWidget *createEditor (QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index ) const;
    virtual void setEditorData (QWidget *editor, const QModelIndex &index ) const;
    virtual void setModelData (QWidget *editor, QAbstractItemModel *model, const QModelIndex &index ) const;
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    virtual bool editorEvent (QEvent * event, QAbstractItemModel * model, const QStyleOptionViewItem & option, const QModelIndex & index);

protected slots:
    void buttonClicked(const QModelIndex &index);
    void buttonUpdateFields(DBField *fld, BaseBean *selectedBean);

};

#endif // AERPINLINEEDITITEMDELEGATE_H
