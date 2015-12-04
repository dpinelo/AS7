/***************************************************************************
 *   Copyright (C) 2007 by David Pinelo   *
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

#include "dbrelationmetadata.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/beansfactory.h"
#include "configuracion.h"

class DBRelationMetadataPrivate
{
//	Q_DECLARE_PUBLIC(DBRelationMetadata)
public:
    /** Nombre que se le da a la relacion */
    QString m_name;
    /** Tipo de relacion que se establece */
    DBRelationMetadata::DBRelationType m_type;
    /** Nombre de la tabla que contiene los hijos */
    QString m_tableName;
    /** Campo de relacion en la tabla raiz */
    QString m_rootFieldName;
    /** Campo de relacion en la tabla hija */
    QString m_childFieldName;
    /** Orden con el que se obtienen de base de datos los hijos */
    QString m_order;
    /** ¿Los hijos de esta relación son editables? */
    bool m_editable;
    /** ¿Se borran en cascada los datos, cuando se borra el bean padre? */
    bool m_deleteCascade;
    /** En otras relaciones, esta variable establece lo fuerte que es, de modo que el bean
      puede no ser borrado si está relacionado */
    bool m_avoidDeleteIfIsReferenced;
    /** Indica si al copiarse un bean (Desde DBForm, por ejemplo, y utilizando la función BaseDAO:copyBean)
      los children de esta relación se incluyen en la copia */
    bool m_includeOnCopy;
    /** Un field que tiene una relación M1 asociada puede presentarse en un asbtract view (un grid, o un tree view) como un enlace a
      la entidad padre, de modo que al hacer click sobre él puede abrirse un formulario en modo lectura (o edición) del mismo */
    bool m_linkToFather;
    /** Relacionada con la propiedad linkToFather, aquí se indica si el formulario se abre en modo lectura o edición */
    bool m_linkDialogReadOnly;
    /** Indica si esta relación es visible en los navegadores de relaciones */
    bool m_showOnRelatedModels;
    bool m_dbReferentialIntegrity;
    bool m_readOnly;
    bool m_allowedInsertChild;

    DBRelationMetadataPrivate ()
    {
        m_type = DBRelationMetadata::ONE_TO_ONE;
        m_editable = true;
        m_deleteCascade = false;
        m_avoidDeleteIfIsReferenced = false;
        m_includeOnCopy = false;
        m_linkToFather = false;
        m_linkDialogReadOnly = false;
        m_showOnRelatedModels = false;
        m_dbReferentialIntegrity = false;
        m_readOnly = false;
        m_allowedInsertChild = true;
    }
};

DBRelationMetadata::DBRelationMetadata(QObject *parent) : QObject(parent), d(new DBRelationMetadataPrivate)
{
}

DBRelationMetadata::~DBRelationMetadata()
{
    delete d;
}

QString DBRelationMetadata::name()
{
    if ( d->m_name.isEmpty() )
    {
        return d->m_tableName;
    }
    return d->m_name;
}

void DBRelationMetadata::setName(const QString &name)
{
    d->m_name = name;
}

QString DBRelationMetadata::rootFieldName()
{
    return d->m_rootFieldName;
}

void DBRelationMetadata::setRootFieldName(const QString &value)
{
    d->m_rootFieldName = value;
}

QString DBRelationMetadata::childFieldName()
{
    return d->m_childFieldName;
}

void DBRelationMetadata::setChildFieldName(const QString &value)
{
    d->m_childFieldName = value;
}

QString DBRelationMetadata::tableName()
{
    return d->m_tableName;
}

void DBRelationMetadata::setTableName(const QString &value)
{
    d->m_tableName = value;
}

QString DBRelationMetadata::sqlTableName(const QString &dialect)
{
    BaseBeanMetadata *metadata = BeansFactory::metadataBean(d->m_tableName);
    if ( metadata == NULL )
    {
        return QString("");
    }
    return metadata->sqlTableName(dialect);
}

DBRelationMetadata::DBRelationType DBRelationMetadata::type()
{
    return d->m_type;
}

void DBRelationMetadata::setType(DBRelationMetadata::DBRelationType type)
{
    d->m_type = type;
}

QString DBRelationMetadata::order()
{
    if ( d->m_order.isEmpty() )
    {
        // Vamos a extraer el orden del campo primario
        return QString ("%1 ASC").arg(d->m_childFieldName);
    }
    return d->m_order;
}

void DBRelationMetadata::setOrder(const QString &value)
{
    d->m_order = value;
}

bool DBRelationMetadata::editable()
{
    return d->m_editable;
}

void DBRelationMetadata::setEditable(bool value)
{
    d->m_editable = value;
}

BaseBeanMetadata * DBRelationMetadata::rootMetadata()
{
    return qobject_cast<BaseBeanMetadata *>(parent());
}

DBFieldMetadata *DBRelationMetadata::childFieldMetadata()
{
    DBFieldMetadata *fld = NULL;
    BaseBeanMetadata *m = BeansFactory::metadataBean(d->m_tableName);
    if ( m != NULL )
    {
        fld = m->field(d->m_childFieldName);
    }
    return fld;
}

BaseBeanMetadata *DBRelationMetadata::father()
{
    return BeansFactory::metadataBean(d->m_tableName);
}

bool DBRelationMetadata::deleteCascade()
{
    return d->m_deleteCascade;
}

void DBRelationMetadata::setDeleteCascade(bool value)
{
    d->m_deleteCascade = value;
}

void DBRelationMetadata::setAvoidDeleteIfIsReferenced (bool value)
{
    d->m_avoidDeleteIfIsReferenced = value;
}

bool DBRelationMetadata::avoidDeleteIfIsReferenced ()
{
    return d->m_avoidDeleteIfIsReferenced;
}

bool DBRelationMetadata::includeOnCopy()
{
    return d->m_includeOnCopy;
}

void DBRelationMetadata::setIncludeOnCopy(bool value)
{
    d->m_includeOnCopy = value;
}

bool DBRelationMetadata::linkToFather()
{
    return d->m_linkToFather;
}

void DBRelationMetadata::setLinkToFather(bool value)
{
    d->m_linkToFather = value;
}

bool DBRelationMetadata::linkDialogReadOnly()
{
    return d->m_linkDialogReadOnly;
}

void DBRelationMetadata::setLinkDialogReadOnly(bool value)
{
    d->m_linkDialogReadOnly = value;
}

bool DBRelationMetadata::showOnRelatedModels()
{
    return d->m_showOnRelatedModels;
}

void DBRelationMetadata::setShowOnRelatedModels(bool value)
{
    d->m_showOnRelatedModels = value;
}

bool DBRelationMetadata::dbReferentialIntegrity() const
{
    return d->m_dbReferentialIntegrity;
}

void DBRelationMetadata::setDbReferentialIntegrity(bool value)
{
    d->m_dbReferentialIntegrity = value;
}

bool DBRelationMetadata::readOnly() const
{
    return d->m_readOnly;
}

void DBRelationMetadata::setReadOnly(bool v)
{
    d->m_readOnly = v;
}

bool DBRelationMetadata::allowedInsertChild() const
{
    return d->m_allowedInsertChild;
}

void DBRelationMetadata::setAllowedInsertChild(bool b)
{
    d->m_allowedInsertChild = b;
}

QString DBRelationMetadata::sqlForeignKeyName(AlephERP::CreationTableSqlOptions options, const QString &dialect)
{
    QString foreignKeyName;
    if ( options.testFlag(AlephERP::UseForeignKeyUniqueName) )
    {
        foreignKeyName = QString("fk_%1").arg(alephERPSettings->uniqueId());
    }
    else
    {
        foreignKeyName = QString("fk_%1_%2").arg(rootMetadata()->tableName()).arg(sqlTableName(dialect));
        if ( dialect == QLatin1String("QIBASE") && foreignKeyName.size() > MAX_LENGTH_OBJECT_NAME_FIREBIRD )
        {
            foreignKeyName = QString("fk_%1").arg(alephERPSettings->uniqueId());
        }
        if ( dialect == QLatin1String("QPSQL") && foreignKeyName.size() > MAX_LENGTH_OBJECT_NAME_PSQL )
        {
            foreignKeyName = QString("fk_%1").arg(alephERPSettings->uniqueId());
        }
    }
    return foreignKeyName;
}

/**
 * @brief DBRelationMetadata::sqlForeignKey
 * Genera la DDL para la generación de las foreign key
 * @param options
 * @param dialect
 * @return
 */
QString DBRelationMetadata::sqlForeignKey(AlephERP::CreationTableSqlOptions options, const QString &dialect)
{
    QString sql;
    // Tabla relacionada
    BaseBeanMetadata *f = BeansFactory::metadataBean(tableName());
    // Tabla "padre" u origen
    BaseBeanMetadata *root = rootMetadata();
    if ( f != NULL )
    {
        if ( options.testFlag(AlephERP::WithForeignKeys) )
        {
            if ( type() == DBRelationMetadata::MANY_TO_ONE )
            {
                QString delCascade = deleteCascade() ? " ON DELETE CASCADE" : "";
                QString foreignKeyName = sqlForeignKeyName(options, dialect);
                sql = QString("ALTER TABLE %1 ADD CONSTRAINT %2 FOREIGN KEY (%3) REFERENCES %4(%5) ON UPDATE CASCADE %6;").
                              arg(root->sqlTableName(dialect)).
                              arg(foreignKeyName).
                              arg(rootFieldName()).
                              arg(sqlTableName(dialect)).
                              arg(childFieldName()).
                              arg(delCascade);
            }
        }
        else if ( options.testFlag(AlephERP::SimulateForeignKeys) )
        {
            if ( type() == DBRelationMetadata::ONE_TO_MANY || type() == DBRelationMetadata::ONE_TO_ONE )
            {
                if ( dialect == QLatin1String("QSQLITE") )
                {
                    QString foreignKeyName = sqlForeignKeyName(options, dialect);
                    sql = QString("CREATE TRIGGER %1 UPDATE ON %2 BEGIN UPDATE %3 SET %4 = new.%5 WHERE %6 = old.%7;").
                                  arg(foreignKeyName).
                                  arg(sqlTableName(dialect)).
                                  arg(sqlTableName(dialect)).
                                  arg(childFieldName()).
                                  arg(rootFieldName()).
                                  arg(childFieldName()).
                                  arg(rootFieldName());
                }
                else if ( dialect == QLatin1String("QIBASE") )
                {
                    QString foreignKeyName = sqlForeignKeyName(options, dialect);
                    sql = QString("CREATE TRIGGER %1 FOR %2 AFTER UPDATE AS BEGIN UPDATE %3 SET %4 = new.%5 WHERE %6 = old.%7; END;").
                                  arg(foreignKeyName).
                                  arg(sqlTableName(dialect)).
                                  arg(sqlTableName(dialect)).
                                  arg(childFieldName()).
                                  arg(rootFieldName()).
                                  arg(childFieldName()).
                                  arg(rootFieldName());
                }
            }
        }
    }
    return sql;
}

QString DBRelationMetadata::sqlDropForeignKey(AlephERP::CreationTableSqlOptions options, const QString &dialect)
{
    QString sql;
    if ( dialect == "QPSQL" )
    {
        sql = QString("ALTER TABLE %1 DROP CONSTRAINT IF EXISTS %2").
                arg(sqlTableName(dialect)).
                arg(sqlForeignKeyName(options, dialect));
    }
    else if ( dialect == "QIBASE" )
    {
        sql = QString("DROP TRIGGER %1").arg(sqlForeignKeyName(options, dialect));
    }
    else if ( dialect == "QSQLITE" )
    {
        sql = QString("DROP TRIGGER %1").arg(sqlForeignKeyName(options, dialect));
    }
    return sql;
}

/*!
  Tenemos que decirle al motor de scripts, que BaseBean se convierte de esta forma a un valor script
  */
QScriptValue DBRelationMetadata::toScriptValue(QScriptEngine *engine, DBRelationMetadata * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBRelationMetadata::fromScriptValue(const QScriptValue &object, DBRelationMetadata * &out)
{
    out = qobject_cast<DBRelationMetadata *>(object.toQObject());
}
