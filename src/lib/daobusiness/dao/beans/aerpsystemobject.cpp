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
#include "aerpsystemobject.h"
#include "business/aerploggeduser.h"
#include "dao/database.h"

class AERPSystemModulePrivate
{
public:
    QString m_id;
    QString m_name;
    QString m_description;
    QString m_showedName;
    QString m_icon;
    bool m_enabled;
    AlephERP::CreationTableSqlOptions m_tableCreationOptions;

    AERPSystemModulePrivate()
    {
        m_enabled = false;
        if ( QSqlDatabase::database(BASE_CONNECTION).driverName() == "QPSQL" )
        {
            m_tableCreationOptions = AlephERP::WithForeignKeys | AlephERP::CreateIndexOnRelationColumns;
        }
        else
        {
            m_tableCreationOptions = AlephERP::WithoutForeignKeys | AlephERP::WithSimulateOID;
        }
    }
};

AERPSystemModule::AERPSystemModule(QObject *parent) :
    QObject(parent),
    d(new AERPSystemModulePrivate)
{

}

AERPSystemModule::~AERPSystemModule()
{
    delete d;
}

QString AERPSystemModule::id() const
{
    return d->m_id;
}

void AERPSystemModule::setId(const QString &arg)
{
    d->m_id = arg;
    setObjectName(arg);
}

QString AERPSystemModule::name() const
{
    return d->m_name;
}

void AERPSystemModule::setName(const QString &arg)
{
    d->m_name = arg;
}

QString AERPSystemModule::description() const
{
    return d->m_description;
}

void AERPSystemModule::setDescription(const QString &arg)
{
    d->m_description = arg;
}

QString AERPSystemModule::showedName() const
{
    return d->m_showedName;
}

void AERPSystemModule::setShowedName(const QString &arg)
{
    d->m_showedName = arg;
}

QString AERPSystemModule::icon() const
{
    return d->m_icon;
}

void AERPSystemModule::setIcon(const QString &arg)
{
    d->m_icon = arg;
}

bool AERPSystemModule::enabled() const
{
    return d->m_enabled;
}

void AERPSystemModule::setEnabled(bool value)
{
    d->m_enabled = value;
}

AlephERP::CreationTableSqlOptions AERPSystemModule::tableCreationOptions() const
{
    return d->m_tableCreationOptions;
}

QString AERPSystemModule::stringTableCreationOptions() const
{
    QString options;
    if ( d->m_tableCreationOptions.testFlag(AlephERP::WithoutForeignKeys) )
    {
        options.append("WithoutForeignKeys").append(" | ");
    }
    if ( d->m_tableCreationOptions.testFlag(AlephERP::WithForeignKeys) )
    {
        options.append("WithForeignKeys").append(" | ");
    }
    if ( d->m_tableCreationOptions.testFlag(AlephERP::WithCommitColumnToLocalWork) )
    {
        options.append("WithCommitColumnToLocalWork").append(" | ");
    }
    if ( d->m_tableCreationOptions.testFlag(AlephERP::WithHashRowColumn) )
    {
        options.append("WithHashRowColumn").append(" | ");
    }
    if ( d->m_tableCreationOptions.testFlag(AlephERP::CreateIndexOnRelationColumns) )
    {
        options.append("CreateIndexOnRelationColumns").append(" | ");
    }
    if ( d->m_tableCreationOptions.testFlag(AlephERP::UseForeignKeyUniqueName) )
    {
        options.append("UseForeignKeyUniqueName").append(" | ");
    }
    if ( d->m_tableCreationOptions.testFlag(AlephERP::SimulateForeignKeys) )
    {
        options.append("SimulateForeignKeys").append(" | ");
    }
    if ( d->m_tableCreationOptions.testFlag(AlephERP::LogicalDeleteColumn) )
    {
        options.append("LogicalDeleteColumn").append(" | ");
    }
    if ( d->m_tableCreationOptions.testFlag(AlephERP::WithSimulateOID) )
    {
        options.append("WithSimulateOID").append(" | ");
    }
    if ( d->m_tableCreationOptions.testFlag(AlephERP::WithRemoteOID) )
    {
        options.append("WithRemoteOID").append(" | ");
    }
    return options;
}

void AERPSystemModule::setTableCreationOptions(AlephERP::CreationTableSqlOptions tableCreationOptions)
{
    d->m_tableCreationOptions = tableCreationOptions;
}

void AERPSystemModule::setTableCreationOptions(const QString &tableCreationOptions)
{
    AlephERP::CreationTableSqlOptions options;
    QStringList parts = tableCreationOptions.split("|");
    foreach (const QString &part, parts)
    {
        if ( part.contains("WithoutForeignKeys") )
        {
            options |= AlephERP::WithoutForeignKeys;
        }
        if ( part.contains("WithForeignKeys") )
        {
            options |= AlephERP::WithForeignKeys;
        }
        if ( part.contains("WithCommitColumnToLocalWork") )
        {
            options |= AlephERP::WithCommitColumnToLocalWork;
        }
        if ( part.contains("WithHashRowColumn") )
        {
            options |= AlephERP::WithHashRowColumn;
        }
        if ( part.contains("CreateIndexOnRelationColumns") )
        {
            options |= AlephERP::CreateIndexOnRelationColumns;
        }
        if ( part.contains("UseForeignKeyUniqueName") )
        {
            options |= AlephERP::UseForeignKeyUniqueName;
        }
        if ( part.contains("SimulateForeignKeys") )
        {
            options |= AlephERP::SimulateForeignKeys;
        }
        if ( part.contains("LogicalDeleteColumn") )
        {
            options |= AlephERP::LogicalDeleteColumn;
        }
        if ( part.contains("WithSimulateOID") )
        {
            options |= AlephERP::WithSimulateOID;
        }
        if ( part.contains("WithRemoteOID") )
        {
            options |= AlephERP::WithRemoteOID;
        }
    }
    d->m_tableCreationOptions = options;
}

QList<AERPSystemObject *> AERPSystemModule::systemObjects()
{
    return findChildren<AERPSystemObject *>();
}

QList<AERPSystemObject *> AERPSystemModule::userAllowedSystemObjects(const QString &type)
{
    QList<AERPSystemObject *> systemObjects = findChildren<AERPSystemObject *>();
    QList<AERPSystemObject *> result;

    foreach (AERPSystemObject *obj, systemObjects)
    {
        if ( obj->type() == type && AERPLoggedUser::instance()->checkMetadataAccess('r', obj->name()) )
        {
            result << obj;
        }
    }
    return result;
}

class AERPSystemObjectPrivate
{
public:
    int m_id;
    QString m_name;
    QString m_content;
    QString m_type;
    int m_version;
    bool m_debug;
    bool m_onInitDebug;
    bool m_fetched;
    int m_idOrigin;
    bool m_isPatch;
    QString m_patch;
    QStringList m_deviceType;
    QPointer<AERPSystemObject> m_ancestor;

    AERPSystemObjectPrivate()
    {
        m_id = -1;
        m_idOrigin = 0;
        m_version = 0;
        m_debug = false;
        m_onInitDebug = false;
        m_fetched = false;
        m_isPatch = false;
    }
};

AERPSystemObject::AERPSystemObject(AERPSystemModule *parent) :
    QObject(parent), d (new AERPSystemObjectPrivate)
{
}

AERPSystemObject::AERPSystemObject(const QString &objectName, const QString &type, const QString &content, bool debug, bool debugOnInit, int version, const QStringList &deviceTypes, AERPSystemModule *parent) :
    QObject(parent), d(new AERPSystemObjectPrivate)
{
    d->m_name = objectName;
    d->m_type = type;
    d->m_content = content;
    d->m_debug = debug;
    d->m_onInitDebug = debugOnInit;
    d->m_version = version;
    d->m_deviceType = deviceTypes;
}

AERPSystemObject::~AERPSystemObject()
{
    delete d;
}

int AERPSystemObject::id() const
{
    return d->m_id;
}

void AERPSystemObject::setId(int value)
{
    d->m_id = value;
}

QString AERPSystemObject::name() const
{
    return d->m_name;
}

void AERPSystemObject::setName(const QString &value)
{
    d->m_name = value;
}

QString AERPSystemObject::content() const
{
    return d->m_content;
}

void AERPSystemObject::setContent(const QString &value)
{
    d->m_content = value;
}

QString AERPSystemObject::type() const
{
    return d->m_type;
}

void AERPSystemObject::setType(const QString &value)
{
    d->m_type = value;
}

int AERPSystemObject::version() const
{
    return d->m_version;
}

void AERPSystemObject::setVersion(int value)
{
    d->m_version = value;
}

bool AERPSystemObject::debug() const
{
    return d->m_debug;
}

void AERPSystemObject::setDebug(bool value)
{
    d->m_debug = value;
}

bool AERPSystemObject::onInitDebug() const
{
    return d->m_onInitDebug;
}

void AERPSystemObject::setOnInitDebug(bool value)
{
    d->m_onInitDebug = value;
}

bool AERPSystemObject::fetched() const
{
    return d->m_fetched;
}

void AERPSystemObject::setFetched(bool value)
{
    d->m_fetched = value;
}

int AERPSystemObject::idOrigin()
{
    return d->m_idOrigin;
}

void AERPSystemObject::setIdOrigin(int value)
{
    d->m_idOrigin = value;
}

bool AERPSystemObject::isPatch()
{
    return d->m_isPatch;
}

void AERPSystemObject::setIsPatch(bool value)
{
    d->m_isPatch = value;
}

QString AERPSystemObject::patch() const
{
    return d->m_patch;
}

void AERPSystemObject::setPatch(const QString &value)
{
    d->m_patch = value;
}

QStringList AERPSystemObject::deviceTypes() const
{
    return d->m_deviceType;
}

void AERPSystemObject::setDeviceTypes(const QStringList &value)
{
    d->m_deviceType = value;
}

AERPSystemModule *AERPSystemObject::module() const
{
    return qobject_cast<AERPSystemModule *>(parent());
}

void AERPSystemObject::setModule(AERPSystemModule *module)
{
    setParent(module);
}

AERPSystemObject *AERPSystemObject::ancestor()
{
    return d->m_ancestor;
}

void AERPSystemObject::setAncestor(AERPSystemObject *value)
{
    d->m_ancestor = value;
}
