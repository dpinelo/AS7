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
#ifndef DBCHOOSERECORDBUTTONTASKMENU_H
#define DBCHOOSERECORDBUTTONTASKMENU_H

#include <QObject>
#include <QDesignerTaskMenuExtension>
#include <QExtensionFactory>
#include "dbchoosefieldnametaskmenu.h"

QT_BEGIN_NAMESPACE
class QExtensionManager;
QT_END_NAMESPACE
class DBChooseRecordButton;
class DBChooseRecordButtonTaskMenuPrivate;

/**
  Esta clase añadirá una entrada de menú en el Qt Designer que permitirá establecer al programador
  una relación entre los fields del bean seleccionado, y los fields del diálogo en el que está
  el DBChooseRecordButton para actualizar estos útimos con los campos de los primeros
*/
class DBChooseRecordButtonTaskMenu : public DBChooseFieldNameTaskMenu
{
    Q_OBJECT
    Q_INTERFACES(QDesignerTaskMenuExtension)

private:
    DBChooseRecordButtonTaskMenuPrivate *d;

public:
    explicit DBChooseRecordButtonTaskMenu(DBChooseRecordButton *button, QObject *parent = 0);
    ~DBChooseRecordButtonTaskMenu();

    QAction *preferredEditAction() const;
    QList<QAction *> taskActions() const;

private slots:
    void editFields();

public slots:

};

#endif // DBCHOOSERECORDBUTTONTASKMENU_H
