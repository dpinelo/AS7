/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo   *
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
#ifndef BATCHDAO_H
#define BATCHDAO_H

#include <alepherpglobal.h>
#include <QtCore>
#include <QtSql>

class BaseBeanMetadata;
class DBFieldMetadata;
class BaseBean;
class BatchDAOPrivate;
class BeansFactory;

/**
 * @brief The BatchDAO class
 * Esta clase nos proporcionará todos los métodos necesarios para poder trabajar en local
 * y después trasladar los datos a producción.
 */
class ALEPHERP_DLL_EXPORT BatchDAO : public QObject
{
    Q_OBJECT

    friend class BatchDAOPrivate;

private:
    BatchDAOPrivate *d;

public:
    BatchDAO(BeansFactory *factory, QObject *parent = NULL);
    ~BatchDAO();

    void setDatabases(const QString &base, const QString &batch);
    QString lastError();
    bool prepareSystemToLocalWork(const QString &messageTemplate);
    bool uploadChanges(const QString &messageTemplate, QString &report);

public slots:
    void cancel();

private slots:
    void saveSerialsThatChange(BaseBean *bean);
    void updateSerialsThatChanged(BaseBean *bean);

signals:
    void initLoad(int itemsCount);
    void loadProgress(int value);
    void loadProgress(const QString &message);
    void finishLoad();

};

#endif // BATCHDAO_H
