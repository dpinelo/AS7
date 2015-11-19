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
#ifndef DBRELATIONMETADATA_H
#define DBRELATIONMETADATA_H

#include <QObject>
#include <QString>
#include <QScriptValue>
#include <QScriptEngine>
#include <alepherpglobal.h>
#include <aerpcommon.h>

class DBRelationMetadataPrivate;
class BaseBeanMetadata;
class DBFieldMetadata;

class ALEPHERP_DLL_EXPORT DBRelationMetadata : public QObject
{
    Q_OBJECT
    Q_ENUMS(DBRelationType)
    /** Tipo de relacion que se establece */
    Q_PROPERTY (DBRelationType type READ type WRITE setType)
    /** Nombre que se le da a la relacion */
    Q_PROPERTY (QString name READ name WRITE setName)
    /** Campo de relacion en la tabla raiz */
    Q_PROPERTY (QString rootFieldName READ rootFieldName WRITE setRootFieldName)
    /** Campo de relacion en la tabla hija */
    Q_PROPERTY (QString childFieldName READ childFieldName WRITE setChildFieldName)
    /** Nombre de la tabla que contiene los hijos */
    Q_PROPERTY (QString tableName READ tableName WRITE setTableName)
    /** Nombre a utilizar en las claúsulas SQL */
    Q_PROPERTY (QString sqlTableName READ sqlTableName)
    /** Orden con el que se obtienen de base de datos los hijos */
    Q_PROPERTY (QString order READ order WRITE setOrder)
    /** ¿Los hijos de esta relación son editables? */
    Q_PROPERTY (bool editable READ editable WRITE setEditable)
    /** ¿Se borran en cascada los datos, cuando se borra el bean padre? */
    Q_PROPERTY (bool deleteCascade READ deleteCascade WRITE setDeleteCascade)
    /** ¿Existe reglas de integridad referencia en base de datos implementadas? */
    Q_PROPERTY (bool dbReferentialIntegrity READ dbReferentialIntegrity WRITE setDbReferentialIntegrity)
    /** En otras relaciones, esta variable establece lo fuerte que es, de modo que el bean
      puede no ser borrado si está relacionado */
    Q_PROPERTY (bool avoidDeleteIfIsReferenced READ avoidDeleteIfIsReferenced WRITE setAvoidDeleteIfIsReferenced)
    /** Indica si al copiarse un bean (Desde DBForm, por ejemplo, y utilizando la función BaseDAO:copyBean)
      los children de esta relación se incluyen en la copia */
    Q_PROPERTY (bool includeOnCopy READ includeOnCopy WRITE setIncludeOnCopy)
    /** Un field que tiene una relación M1 asociada puede presentarse en un asbtract view (un grid, o un tree view) como un enlace a
      la entidad padre, de modo que al hacer click sobre él puede abrirse un formulario en modo lectura (o edición) del mismo */
    Q_PROPERTY (bool linkToFather READ linkToFather WRITE setLinkToFather)
    /** Relacionada con la propiedad linkToFather, aquí se indica si el formulario se abre en modo lectura o edición */
    Q_PROPERTY (bool linkDialogReadOnly READ linkDialogReadOnly WRITE setLinkDialogReadOnly)
    /** Existen una serie de modelos de representación de datos, que permiten navegar por los mismos, obteniendo información
     * relacionada. En algunos casos, es interesante mostrar esos datos al usuario (en otros no. Por ejemplo, mostrando una relación
     * que apunte al ejercicio fiscal, lo que podría ser redundante). Se permite al programador escoger qué si esta relación
     * será navegable o no */
    Q_PROPERTY (bool showOnRelatedModels READ showOnRelatedModels WRITE setShowOnRelatedModels)
    /** Apunta al metadato del registro padre */
    Q_PROPERTY (BaseBeanMetadata *father READ father)
    /** Si esta propiedad está marcada como readOnly, no se permitirá modificar los beans hijos */
    Q_PROPERTY (bool readOnly READ readOnly WRITE setReadOnly)
    /** Indica si está permitido insertar hijos */
    Q_PROPERTY (bool allowedInsertChild READ allowedInsertChild WRITE setAllowedInsertChild)

private:
    Q_DISABLE_COPY(DBRelationMetadata)
    DBRelationMetadataPrivate *d;
    Q_DECLARE_PRIVATE(DBRelationMetadata)

public:
    DBRelationMetadata(QObject *parent = NULL);
    ~DBRelationMetadata();

    enum DBRelationType {ONE_TO_ONE, ONE_TO_MANY, MANY_TO_ONE};

    DBRelationType type();
    void setType(DBRelationType type);
    QString name();
    void setName(const QString &name);
    QString rootFieldName();
    void setRootFieldName(const QString &value);
    QString childFieldName();
    void setChildFieldName(const QString &value);
    QString tableName();
    void setTableName(const QString &value);
    QString sqlTableName(const QString &dialect = "");
    QString order();
    void setOrder(const QString &value);
    bool editable();
    void setEditable(bool value);
    bool deleteCascade();
    void setDeleteCascade(bool value);
    void setAvoidDeleteIfIsReferenced (bool value);
    bool avoidDeleteIfIsReferenced ();
    bool includeOnCopy();
    void setIncludeOnCopy(bool value);
    bool linkToFather();
    void setLinkToFather(bool value);
    bool linkDialogReadOnly();
    void setLinkDialogReadOnly(bool value);
    bool showOnRelatedModels();
    void setShowOnRelatedModels(bool value);
    bool dbReferentialIntegrity() const;
    void setDbReferentialIntegrity(bool value);
    bool readOnly() const;
    void setReadOnly(bool v);
    bool allowedInsertChild() const;
    void setAllowedInsertChild(bool b);

    QString sqlForeignKeyName(AlephERP::CreationTableSqlOptions options, const QString &dialect);
    QString sqlForeignKey(AlephERP::CreationTableSqlOptions options, const QString &dialect);
    QString sqlDropForeignKey(AlephERP::CreationTableSqlOptions options, const QString &dialect);

    BaseBeanMetadata *rootMetadata();
    DBFieldMetadata *childFieldMetadata();
    BaseBeanMetadata *father();

    static QScriptValue toScriptValue(QScriptEngine *engine, DBRelationMetadata * const &in);
    static void fromScriptValue(const QScriptValue &object, DBRelationMetadata * &out);

};

Q_DECLARE_METATYPE(DBRelationMetadata*)
Q_DECLARE_METATYPE(QList<DBRelationMetadata *>)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBRelationMetadata, QObject*)

#endif // DBRELATIONMETADATA_H
