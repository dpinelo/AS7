/***************************************************************************
 *   Copyright (C) 2012 by David Pinelo   *
 *   info@alephsistemas.es   *
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
#ifndef MODULESDAO_H
#define MODULESDAO_H

#include <QtCore>
#include <alepherpglobal.h>

class BaseBean;
class BaseBeanMetadata;
class AERPSystemObject;
class AERPSystemModule;
class ModulesDAOPrivate;

class ALEPHERP_DLL_EXPORT ModulesDAO : public QObject
{
    Q_OBJECT

    friend class ModulesDAOPrivate;

private:
    ModulesDAOPrivate *d;

    ModulesDAO();

public:
    virtual ~ModulesDAO();
    static ModulesDAO *instance();

    bool importModules(const QString &xmlOrigin, const QString &moduleId = "", bool askForObjects = true);
    bool importModulesData(const QString &xmlOrigin, const QString &moduleId = "", bool showProgressDialog = true);
    bool exportModules(const QDir &directory, const QString &moduleId = "");
    bool exportModulesList(const QDir &directory);
    bool importRecord(BaseBean *bean, const QHash<QString, QString> &data);
    void updateModuleMetadata(const QString &moduleId, const QString &path);

    bool exportData(const QString &module, const QDir &directory);
    bool exportData(const QList<BaseBeanMetadata *> metadatasToExport, const QDir &directory);
    bool importData(QFile &file, const QString tableName = "");
    QString lastErrorMessage() const;

public slots:
    void cancelProcess();

signals:
    void initImportData(int count);
    void importDataProgress(int progress);
    void endImportData();
    void initExportData();
    void initExportDataTable(QString, int);
    void initExportDataTable(int);
    void exportDataTableProgress(QString, int);
    void exportDataTableProgress(int);
    void endExportDataTable(QString);
    void endExportData();

};

#endif // MODULESDAO_H
