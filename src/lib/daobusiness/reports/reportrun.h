/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo                                    *
 *   alepherp@alephsistemas.es                                             *
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
#ifndef REPORTRUN_H
#define REPORTRUN_H

#include <QtCore>
#include <QtScript>
#include <QtSql>
#include <aerpcommon.h>
#include "dao/beans/basebean.h"

class ReportRunPrivate;
class ReportMetadata;
class BaseBean;
class AERPReportsInterface;

/**
 * @brief The ReportRun class
 * Esta será la clase encargada de ejecutar y actuar con los informes.
 */
class ReportRun : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString reportName READ reportName WRITE setReportName)
    Q_PROPERTY(BaseBeanPointerList beans READ beans WRITE setBeans)
    Q_PROPERTY(ReportMetadata* metadata READ metadata)
    Q_PROPERTY(QString linkedTo READ linkedTo WRITE setLinkedTo)

private:
    ReportRunPrivate *d;

public:
    explicit ReportRun(QObject *parent = 0);
    ~ReportRun();

    ReportMetadata *metadata();
    void setReportName (const QString &reportName);
    QString reportName() const;
    BaseBeanPointerList beans();
    void setBeans(BaseBeanPointerList beans);
    void setParentWidget(QWidget *parent);
    void setParameters(const QVariantMap &parameters);
    QString linkedTo() const;
    void setLinkedTo(const QString &value);
    static QList<ReportMetadata *> availableReports(const QString &tableName);
    QList<ReportMetadata *> availableReports();
    QList<AlephERP::ReportParameterInfo> parametersRequired();
    AERPReportsInterface *iface();

    QString pathToGeneratedFile();
    QString message() const;

    bool canPreview();
    bool canCreatePDF();
    bool canExecute();
    bool canExportSpreadSheet();
    bool needsUserToEnterParameters();

    static QString lastErrorMessage();
    static AERPReportsInterface *loadPlugin(const QString &name);
    static bool editReport(const QString &reportName);

    static QScriptValue toScriptValue(QScriptEngine *engine, ReportRun * const &in);
    static void fromScriptValue(const QScriptValue &object, ReportRun * &out);

signals:
    void canExecuteReport(bool value);

public slots:
    bool print(int numCopies = 1);
    bool preview(int numCopies = 1);
    bool pdf(int numCopies = 1, bool open = true);
    bool showDialog();
    bool editReport();
    void setParameterValue(const QString &parameterName, const QVariant &value);
    bool exportToSpreadSheet(const QString &type, const QString &file);
    QSqlQuery query();
};

Q_DECLARE_METATYPE(ReportRun*)
Q_SCRIPT_DECLARE_QMETAOBJECT(ReportRun, QObject*)

#endif // REPORTRUN_H
