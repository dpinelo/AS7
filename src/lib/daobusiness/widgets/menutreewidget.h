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
#ifndef MENUTREEWIDGET_H
#define MENUTREEWIDGET_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtScript>
#include <alepherpglobal.h>

/*!
  Esta clase mostrará de forma jerárquica los menús de la aplicación en un QTreeWidget.
  También admitirá una configuración por usuario, que se almacenará en alepherp_envvars, mediante
  un fichero XML y tendrá la forma
  <menuTreeWidget>
	<item name="Agrupación de Actions">
	  <action name="action_name"/>
	  <action name="action_name2"/>
	</item>
	<action name="action_name3">
  </menuTreeWidget>

  @author David Pinelo alepherp@alephsistemas.es
  */
class ALEPHERP_DLL_EXPORT MenuTreeWidget : public QTreeWidget
{
    Q_OBJECT

private:
    bool m_init;
    QList<QTreeWidgetItem *> m_items;
    QMenuBar *menuBar();
    void deleteEmptyItems();
    void addMenuChildren(QAction *action, QTreeWidgetItem *parent);
    void addAction(QAction *action, QTreeWidgetItem *parent);

protected:
    virtual QMimeData *mimeData(const QList<QTreeWidgetItem *> items) const;
    void showEvent(QShowEvent *event);
    void init();

public:
    explicit MenuTreeWidget(QWidget *parent = 0);

signals:

private slots:
    void itemHasBeenClicked(QTreeWidgetItem *item, int column);

public slots:

};

Q_DECLARE_METATYPE(MenuTreeWidget*)

#endif // MENUTREEWIDGET_H
