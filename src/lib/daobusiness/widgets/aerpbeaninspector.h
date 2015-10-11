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
#ifndef AERPBEANINSPECTOR_H
#define AERPBEANINSPECTOR_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif

class BaseBean;
class AERPBeanInspectorPrivate;
/**
 * @brief The AERPBeanInspector class
 * Clase interesante para la depuración. Presenta los valores de un bean
 */
class AERPBeanInspector : public QTreeView
{
    Q_OBJECT
private:
    AERPBeanInspectorPrivate *d;

protected:
    virtual void showEvent(QShowEvent *);
    virtual void closeEvent(QCloseEvent *);

public:
    explicit AERPBeanInspector(QWidget *parent = 0);
    virtual ~AERPBeanInspector();

signals:

public slots:
    void inspect(BaseBean *bean);
    void inspect(const QString &contextName);

};

#endif // AERPBEANINSPECTOR_H
