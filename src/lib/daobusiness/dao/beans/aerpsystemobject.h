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
#ifndef AERPSYSTEMOBJECT_H
#define AERPSYSTEMOBJECT_H

#include <QtCore>
#include <QtScript>
#include <aerpcommon.h>

class AERPSystemObjectPrivate;
class AERPSystemModulePrivate;
class AERPSystemObject;

class AERPSystemModule : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString id READ id WRITE setId)
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(QString description READ description WRITE setDescription)
    Q_PROPERTY(QString showedName READ showedName WRITE setShowedName)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled)
    Q_PROPERTY(AlephERP::CreationTableSqlOptions tableCreationOptions READ tableCreationOptions WRITE setTableCreationOptions)

private:
    AERPSystemModulePrivate *d;

public:
    AERPSystemModule(QObject *parent = NULL);
    virtual ~AERPSystemModule();

    QString id() const;
    void setId(const QString &arg);
    QString name() const;
    void setName(const QString &arg);
    QString description() const;
    void setDescription(const QString &arg);
    QString showedName() const;
    void setShowedName(const QString &arg);
    QString icon() const;
    void setIcon(const QString &arg);
    bool enabled() const;
    void setEnabled(bool value);
    AlephERP::CreationTableSqlOptions tableCreationOptions() const;
    QString stringTableCreationOptions() const;
    void setTableCreationOptions(AlephERP::CreationTableSqlOptions tableCreationOptions);
    void setTableCreationOptions(const QString &tableCreationOptions);

    QList<AERPSystemObject *> systemObjects();
    QList<AERPSystemObject *> userAllowedSystemObjects(const QString &type);
};

/**
 * @brief The AERPSystemObject class
 * Representa un objeto de sistema, tal y como se almacena en base de datos. Es decir, contiene
 * las definiciones XML, o código QS y es especialmente indicado para su utilización desde el punto de vista
 * del desarrollo.
 */
class AERPSystemObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY (int id READ id WRITE setId)
    Q_PROPERTY (int idOrigin READ idOrigin WRITE setIdOrigin)
    Q_PROPERTY (QString name READ name WRITE setName)
    Q_PROPERTY (QString content READ content WRITE setContent)
    Q_PROPERTY (QString type READ type WRITE setType)
    Q_PROPERTY (int version READ version WRITE setVersion)
    Q_PROPERTY (bool debug READ debug WRITE setDebug)
    Q_PROPERTY (bool onInitDebug READ onInitDebug WRITE setOnInitDebug)
    Q_PROPERTY (bool fetched READ fetched WRITE setFetched)
    Q_PROPERTY (bool isPatch READ isPatch WRITE setIsPatch)
    Q_PROPERTY (QString patch READ patch WRITE setPatch)
    Q_PROPERTY (QStringList deviceTypes READ deviceTypes WRITE setDeviceTypes)
    Q_PROPERTY (AERPSystemModule *module READ module WRITE setModule)
    Q_PROPERTY (AERPSystemObject *ancestor READ ancestor WRITE setAncestor)

    friend class BeansFactory;

private:
    AERPSystemObjectPrivate *d;

    explicit AERPSystemObject(const QString &objectName, const QString &type, const QString &content, bool debug, bool debugOnInit,
                              int version, const QStringList &deviceTypes, AERPSystemModule *parent);

public:
    explicit AERPSystemObject(AERPSystemModule *parent = 0);
    virtual ~AERPSystemObject();

    int id() const;
    void setId(int value);
    QString name() const;
    void setName(const QString &value);
    QString content() const;
    void setContent(const QString &value);
    QString type() const;
    void setType(const QString &value);
    int version() const;
    void setVersion(int value);
    bool debug() const;
    void setDebug(bool value);
    bool onInitDebug() const;
    void setOnInitDebug(bool value);
    bool fetched() const;
    void setFetched(bool value);
    int idOrigin();
    void setIdOrigin(int value);
    bool isPatch();
    void setIsPatch(bool value);
    QString patch() const;
    void setPatch(const QString &value);
    QStringList deviceTypes() const;
    void setDeviceTypes(const QStringList &value);
    AERPSystemModule *module() const;
    void setModule(AERPSystemModule *module);
    AERPSystemObject *ancestor();
    void setAncestor(AERPSystemObject *value);

signals:

public slots:

};

Q_DECLARE_METATYPE(AERPSystemModule*)
Q_DECLARE_METATYPE(AERPSystemObject*)
Q_SCRIPT_DECLARE_QMETAOBJECT(AERPSystemModule, QObject*)
Q_SCRIPT_DECLARE_QMETAOBJECT(AERPSystemObject, AERPSystemModule*)

#endif // AERPSYSTEMOBJECT_H
