/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo                                    *
 *   alepherp@alephsistemas.es                                                    *
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
#ifndef AERPWAITDB_H
#define AERPWAITDB_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QLabel>
#include <alepherpglobal.h>

class AERPWaitDbPrivate;
class QSqlDriver;

/**
  Este widget mostrará el estado de las transacciones con la base de datos. Sólo operará
  con el driver desarrollado para el funcionamiento con la nube. En otros drivers,
  ya que la implantación por defecto de Qt no puede ofrecer estado de los trabajos, no será usado
  */
class ALEPHERP_DLL_EXPORT AERPWaitDB : public QWidget
{
    Q_OBJECT

private:
    AERPWaitDbPrivate *d;

    static AERPWaitDB *m_waitDbSingleton;
    static QLabel *m_lblMessage;

    static void createWaitWidget(const QString &message);

public:
    explicit AERPWaitDB(QWidget *parent = 0);
    ~AERPWaitDB();

    static void showWaitWidget(const QSqlDriver *driver, const QString &message);
    static void closeWaitWidget();

    static AERPWaitDB *instance();

    void setSqlDriver(const QSqlDriver *driver);
    QSqlDriver *sqlDriver();

signals:

private slots:
    void updateProgressBar(qint64 bytesUpdate, qint64 bytesTotal);
    void closeWaitDb();
};

#endif // AERPWAITDB_H
