/***************************************************************************
 *   Copyright (C) 2007 by David Pinelo                                    *
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
#ifndef BASEBEAN_H
#define BASEBEAN_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtCore>
#include <QtScript>
#include <aerpcommon.h>
#include "dao/dbobject.h"
#include "dao/beans/aerpuserrowaccess.h"
#ifdef ALEPHERP_SMTP_SUPPORT
#include "dao/emaildao.h"
#endif

class DBField;
class DBRelation;
class BaseBeanObserver;
class BaseBeanPrivate;
class BaseBeanMetadata;
class DBFieldMetadata;
class DBRelationMetadata;
class BaseDAO;
#ifdef ALEPHERP_LOCALMODE
class BatchDAO;
#endif
#ifdef ALEPHERP_DOC_MANAGEMENT
class AERPDocMngmntDocument;
#endif
class RelatedElement;
class BaseBean;

/** Definiciones muy útiles */
typedef QHash<QString, QString> HashString;
typedef QPointer<BaseBean> BaseBeanPointer;
typedef QList<BaseBeanPointer> BaseBeanPointerList;
typedef QSharedPointer<BaseBean> BaseBeanSharedPointer;
typedef QList<BaseBeanSharedPointer> BaseBeanSharedPointerList;
typedef QWeakPointer<BaseBean> BaseBeanWeakPointer;
typedef QList<BaseBeanWeakPointer> BaseBeanWeakPointerList;
typedef QPointer<RelatedElement> RelatedElementPointer;
typedef QList<RelatedElementPointer> RelatedElementPointerList;

inline BaseBeanSharedPointerList AERP_WEAKLIST_TO_STRONGLIST(const BaseBeanWeakPointerList &weakList)
{
    BaseBeanSharedPointerList strongList;
    for(int i = 0 ; i < weakList.size() ; i++)
    {
        strongList.append(weakList.at(i).toStrongRef());
    }
    return strongList;
}

inline BaseBeanWeakPointerList AERP_STRONGLIST_TO_WEAKLIST(const BaseBeanSharedPointerList &strongList)
{
    BaseBeanWeakPointerList weakList;
    for(int i = 0 ; i < strongList.size() ; i++)
    {
        weakList.append(strongList.at(i).toWeakRef());
    }
    return weakList;
}

inline BaseBeanPointerList AERP_STRONGLIST_TO_POINTERLIST(const BaseBeanSharedPointerList &strongList)
{
    BaseBeanPointerList list;
    for(int i = 0 ; i < strongList.size() ; i++)
    {
        list.append(strongList.at(i).data());
    }
    return list;
}

/**
	Clase base de todos los Beans de la aplicación. Es una clase abstracta,
	que implementa la interfaz necesaria para tratar con los Beans de forma genérica.
	Esta clase será usada cuando tratemos con un bean sin importarnos el tipo.
	@author David Pinelo <alepherp@alephsistemas.es>
 */
class ALEPHERP_DLL_EXPORT BaseBean : public DBObject
{
    Q_OBJECT
    Q_ENUMS(DbBeanStates)

    /** Identificador único en base de datos. OID en PostgreSQL */
    Q_PROPERTY(qlonglong dbOid READ dbOid)
    Q_PROPERTY(QList<DBField *> fields READ fields)
    Q_PROPERTY(QList<DBRelation *> relations READ relations)
    Q_PROPERTY(RelatedElementPointerList relatedElements READ getRelatedElements)
    Q_PROPERTY(QVariantMap fieldsMap READ fieldsMap)
    Q_PROPERTY(QVariantMap relationsMap READ relationsMap)
    /** Flag que indica el estado en que se encuentra en base de datos */
    Q_PROPERTY(DbBeanStates dbState READ dbState WRITE setDbState)
    Q_PROPERTY(QString dbStateDisplayName READ dbStateDisplayName)
    /** Flag que indica si en este bean han modificado sus datos (los fields). Se entiende que se han modificado
    cuando cambia algo en el bean, después de crearse y después de establecerse los valores por defecto. */
    Q_PROPERTY(bool modified READ modified)
    /** Marca de tiempo, que indica cuándo fue la última vez que se leyó el registro de base de datos */
    Q_PROPERTY(QDateTime loadTime READ loadTime WRITE setLoadTime)
    /** Acceso a los metadatos */
    Q_PROPERTY(BaseBeanMetadata* metadata READ metadata)
    /** Contexto actual (para guardarse en BBDD en el que se encuentra el bean */
    Q_PROPERTY(QString actualContext READ actualContext)
    /** Email template con los datos de este bean */
    Q_PROPERTY(QString emailTemplate READ emailTemplate)
    /** Estos beans suelen presentarse en modelos de datos por filas. A veces, es interesante darles a esas filas un color
     * particular, que destaque ante el usuario. Eso puede determinarse a través de esta propiedad, que invocará a un script
     * definido en los metadatos */
    Q_PROPERTY(QColor rowColor READ rowColor)
    /** Devuelve para el presente bean la ruta del repositorio en el que se almacenarán los documentos asociados */
    Q_PROPERTY(QString repositoryPath READ repositoryPath)
    /** Keywords que tendrá un documento creado en con este bean */
    Q_PROPERTY(QStringList repositoryKeywords READ repositoryKeywords)
    /** Nos indicará si se han modificado los elementos relacionados, para su almacenamiento posterior */
    Q_PROPERTY(bool modifiedRelatedElements READ modifiedRelatedElements)
    Q_PROPERTY(QString string READ toString)
    Q_PROPERTY(bool readOnly READ readOnly WRITE setReadOnly)
    Q_PROPERTY(QString hash READ hash)
    Q_PROPERTY(QString rawHash READ hash)

    /** BaseDAO utiliza unas funciones específicas de DBField y BaseBean para así saber
      cuándo la lectura de un dato se ha producid por lectura de base de datos, de modo
      que no se modifique m_modified */
    friend class BaseDAO;
    friend class RelatedDAO;
#ifdef ALEPHERP_LOCALMODE
    friend class BatchDAO;
#endif
    friend class BeansFactory;
    friend class DBRelation;
    friend class AERPTransactionContext;
    friend class DBRecordDlg;
    friend class BaseBeanPrivate;
    friend class BaseBeanMetadata;
    friend class BaseBeanMetadataPrivate;
    friend class DBField;

    /** Para profiling */
    static int m_beansCount;
    static int m_maxBeansCount;

private:
    Q_DISABLE_COPY(BaseBean)
    BaseBeanPrivate *d;

    void init(BaseBeanMetadata *m, bool hastToSetDefaultValue, BaseBeanPointerList fatherBeans);
    void addRelatedElement(RelatedElement *element);
    /** Funciones factoria para crear los datos internos de este bean */
    DBRelation *newRelation(DBRelationMetadata *m);
    /** Funciones factoria para crear los datos internos de este bean */
    DBField *newField(DBFieldMetadata *m);
    void makeCalculatedFieldsConnections(const QString &fieldToCalc = "", const QStringList &fieldsOnCalc = QStringList());
    void setLoadTime(const QDateTime &time);
    RelatedElementPointer newRelatedElement(AlephERP::RelatedElementTypes type);

protected:
    void cleanToBeDeletedRelatedElements();

public:
    explicit BaseBean(QObject *parent = 0);
    virtual ~BaseBean();

    /** Valores con respecto a la base de datos en la que se encuentra el bean.
      Los valores de la enumeración se ponen de forma binaria para poder hacer combinaciones
      de estado.
      Estos valores reflejan los siguiente:
      -INSERT: El bean NO ha sido almacenado en base de datos nunca (por tanto, si se guarda en base de datos, deberá producirse un INSERT)
      -UPDATE: El bean existía en base de datos.
      -TO_BE_DELETED: El bean ha sido marcado para ser borrado en la próxima transacción (generalmente en el contexto).
    */
    enum DbBeanStates
    {
        INSERT = 1,
        UPDATE = 2,
        TO_BE_DELETED = 4,
        DELETED = 8
    };

    BaseBeanMetadata * metadata() const;
    qlonglong dbOid();
    void setDbOid(qlonglong value);

    void setDbState(DbBeanStates state);
    DbBeanStates dbState();
    QString dbStateDisplayName() const;

    bool modified();
    QString actualContext() const;
    QDateTime loadTime();

    bool readOnly();
    bool setReadOnly(bool value);

    QList<DBField *> fields();
    QVariantMap fieldsMap();
    Q_INVOKABLE DBField * field(const QString &dbFieldName);
    Q_INVOKABLE DBField * field(int index);
    Q_INVOKABLE QList<DBField *> pkFields ();
    int fieldIndex(const QString &dbFieldName);
    int fieldCount();
    bool modifiedRelatedElements();

    QList<DBRelation *> relations(AlephERP::RelationTypes type = AlephERP::All);
    QVariantMap relationsMap();
    Q_INVOKABLE DBRelation * relation(const QString &relationName);
    int relationIndex(const QString &relationName);
    Q_INVOKABLE BaseBeanPointerList relationChildren(const QString &relationName, const QString &order = "", bool includeToBeDeleted = false);
    Q_INVOKABLE BaseBeanPointer relationChildByField(const QString &relationName, const QString &fieldName, const QVariant &id, bool includeToBeDeleted = false);
    Q_INVOKABLE BaseBeanPointerList relationChildrenByFilter(const QString &relationName, const QString &filter, const QString &order = "", bool includeToBeDeleted = false);
    Q_INVOKABLE BaseBeanPointer father(const QString &relationName);
    Q_INVOKABLE QVariant fatherFieldValue(const QString &relationName, const QString &field);
    Q_INVOKABLE QString fatherDisplayFieldValue(const QString &relationName, const QString &field);
    Q_INVOKABLE int relationChildrenCount(const QString &relationName, bool includeToBeDeleted = true);
    Q_INVOKABLE void deleteAllChildren(const QString &relationName);
    Q_INVOKABLE BaseBeanSharedPointer newRelationChild(const QString &relationName);

    QList<DBObject *> navigateThrough(const QString &relationName, const QString &relationFilters);
    DBObject *navigateThroughProperties(const QString &path, bool returnIntermediate = false);

    Q_INVOKABLE QVariant fieldValue(int dbField);
    Q_INVOKABLE QVariant fieldValue(const QString &dbFieldName);
    Q_INVOKABLE virtual void setFieldValue(const QString &dbFieldName, const QVariant &value);
    Q_INVOKABLE virtual void setFieldValue(int index, const QVariant &value);
    Q_INVOKABLE virtual void setFieldValueFromSqlRawData(const QString &dbFieldName, const QString &data);
    Q_INVOKABLE virtual void setOverwriteFieldValue(const QString &dbFieldName, const QVariant &value);
    Q_INVOKABLE bool isFieldEmpty(const QString &dbFieldName);
    Q_INVOKABLE QPixmap pixmapFieldValue(int dbField);
    Q_INVOKABLE QPixmap pixmapFieldValue(const QString &dbFieldName);

    Q_INVOKABLE QVariant pkValue();
    Q_INVOKABLE QVariantList pkListValue();
    Q_INVOKABLE QString pkSerializedValue();
    Q_INVOKABLE bool pkEqual(const QVariant &value);
    Q_INVOKABLE void setPkValue(const QVariant &id);
    Q_INVOKABLE QVariant defaultFieldValue(const QString &dbFieldName);
    Q_INVOKABLE QString displayFieldValue(int iField);
    Q_INVOKABLE QString displayFieldValue(const QString & iField);
    Q_INVOKABLE QString sqlFieldValue(int iField);
    Q_INVOKABLE QString sqlFieldValue(const QString & iField);
    Q_INVOKABLE QString toolTipField(const QString &dbFieldName);
    Q_INVOKABLE QString toolTipField(int iField);
    Q_INVOKABLE QColor rowColor();
    Q_INVOKABLE QString toolTip();

    Q_INVOKABLE QString sqlWhere(const QString &fieldName, const QString &op);
    Q_INVOKABLE QString sqlWherePk();

    Q_INVOKABLE bool checkFilter(const QString &filterExpression, Qt::CaseSensitivity sensivity = Qt::CaseInsensitive);

    Q_INVOKABLE bool validate();
    Q_INVOKABLE QString validateMessages();
    Q_INVOKABLE QString validateHtmlMessages();
    Q_INVOKABLE QWidget *focusWidgetOnBadValidate();

    Q_INVOKABLE bool isAccessible();

    QString toString();

    Q_INVOKABLE QScriptValue newRelatedElement();

    // Funciones de creación de elementos relacionados.
    RelatedElementPointer newRelatedElement(const QString &tableName, const QStringList &category = QStringList());
    RelatedElementPointer newRelatedElement(BaseBeanPointer related, const QStringList &category = QStringList(), bool takeOwnerShip = false);
    RelatedElementPointer newRelatedElement(RelatedElementPointer otherElement);
#ifdef ALEPHERP_SMTP_SUPPORT
    RelatedElementPointer newRelatedElement(const EmailObject &emailContent, const QStringList &category = QStringList());
#endif
#ifdef ALEPHERP_DOC_MANAGEMENT
    RelatedElementPointer newRelatedElement(AERPDocMngmntDocument *doc, const QStringList &category = QStringList());
#endif

    Q_INVOKABLE RelatedElementPointerList getRelatedElements(AlephERP::RelatedElementTypes type = AlephERP::NoneAll, AlephERP::RelatedElementCardinalities cardinality = AlephERP::PointToChild, bool includeToBeDelete = false);
    Q_INVOKABLE RelatedElementPointerList getRelatedElementsByRelatedTableName(const QString &tableName, AlephERP::RelatedElementCardinalities cardinality = AlephERP::PointToChild, bool includeToBeDelete = false);
    Q_INVOKABLE RelatedElementPointerList getRelatedElementsByCategory(const QString &category, AlephERP::RelatedElementTypes type = AlephERP::NoneAll, AlephERP::RelatedElementCardinalities cardinality = AlephERP::PointToChild, bool includeToBeDelete = false);
    Q_INVOKABLE RelatedElementPointerList getRelatedElementsByCategoryAndRelatedTableName(const QString &tableName, const QString &category, AlephERP::RelatedElementCardinalities cardinality = AlephERP::PointToChild, bool includeToBeDelete = false);
    Q_INVOKABLE RelatedElementPointerList getRelatedElementsByCategoriesAndRelatedTableNames(const QStringList &tableName, const QStringList &category, AlephERP::RelatedElementCardinalities cardinality = AlephERP::PointToChild, bool includeToBeDelete = false);
#ifdef ALEPHERP_DOC_MANAGEMENT
    Q_INVOKABLE RelatedElementPointerList getRelatedElementsByDocumentId(int id);
#endif
    Q_INVOKABLE int countRelatedElementsByCategory(const QString &category, AlephERP::RelatedElementTypes type = AlephERP::NoneAll, AlephERP::RelatedElementCardinalities cardinality = AlephERP::PointToChild, bool includeToBeDelete = false);
    Q_INVOKABLE int countRelatedElementsByRelatedTableName(const QString &tableName, AlephERP::RelatedElementCardinalities cardinality = AlephERP::PointToChild, bool includeToBeDelete = false);
    Q_INVOKABLE int countRelatedElementsByCategoryAndRelatedTableName(const QString &tableName, const QString &category, AlephERP::RelatedElementCardinalities cardinality = AlephERP::PointToChild, bool includeToBeDelete = false);
    Q_INVOKABLE int countRelatedElementsByCategoriesAndRelatedTableNames(const QStringList &tableNames, const QStringList &categories, AlephERP::RelatedElementCardinalities cardinality = AlephERP::PointToChild, bool includeToBeDelete = false);
    Q_INVOKABLE int countRelatedElements(AlephERP::RelatedElementTypes type = AlephERP::NoneAll, AlephERP::RelatedElementCardinalities cardinality = AlephERP::PointToChild, bool includeToBeDelete = false);
    Q_INVOKABLE void deleteRelatedElementByCategory(const QString &category, AlephERP::RelatedElementTypes type = AlephERP::NoneAll, AlephERP::RelatedElementCardinalities cardinality = AlephERP::PointToChild);
    Q_INVOKABLE void deleteRelatedElement(RelatedElementPointer element);
    Q_INVOKABLE bool relatedElementsLoaded();

//	Q_INVOKABLE QVariant calculateAggregate(const QString &operation, const QString &relation,
//											const QString &dbFieldName, const QString &filter = "");

    /** Función que sólo podrán ser llamadas desde BaseDAO. Establece el valor
      sin modificar m_modified */
    virtual void setInternalFieldValue(const QString &dbFieldName, const QVariant &value, bool overwriteOnReadOnly = false);
    /** Función que sólo podrán ser llamadas desde BaseDAO. Establece el valor
      sin modificar m_modified */
    virtual void setInternalFieldValue(int index, const QVariant &value, bool overwriteOnReadOnly = false);
    virtual void setOldValue(const QString &dbFieldName, const QVariant &oldValue);
    virtual void setOldValue(int index, const QVariant &oldValue);

    virtual void deleteObserver();

    Q_INVOKABLE QScriptValue duplicate();

    BaseBeanSharedPointer clone();
    BaseBeanPointer clone(QObject *parent);

    void resetToDefaultValues();
    void setViewLinkedBean(BaseBean *bean);

    void copyValues(BaseBeanPointer otherBean, bool saveValues = false);
    void copyValues(BaseBeanPointer otherBean, const QStringList &fields);
    void copyRelationChildren(BaseBeanPointer otherBean, QStringList &relationNames);
    void deepCopyValues(BaseBeanPointer otherBean, const QStringList &relationNames);

    QString hash(bool useRawValue = false);
    QString rawHash();
    QString emailTemplate() const;
    QString emailSubject() const;
    QString repositoryPath() const;
    QStringList repositoryKeywords() const;

    QList<DBField *> counterFields(const QString &fld);

    AERPUserRowAccessList access();
    void setAccess(AERPUserRowAccessList list);
    void appendAccess(const AERPUserRowAccess &item);
    bool checkAccess(QChar access);

    // Sí, soy un cateto... y cometí un error de novato en inglés. childs en lugar de children... ole mis ... Lo corrijo, y mientras
    // dejo esto por compatibilidad hacia atrás con otros clientes...
    // Yes. Big mistake with plural. Like I was 6 years old. "childs" instead of children... Now, I have to provide these functions for
    // backward compatibility
    Q_INVOKABLE BaseBeanPointerList relationChilds(const QString &relationName, const QString &order = "", bool includeToBeDeleted = false)
    {
        return relationChildren(relationName, order, includeToBeDeleted);
    }
    Q_INVOKABLE BaseBeanPointerList relationChildsByFilter(const QString &relationName, const QString &filter, const QString &order = "", bool includeToBeDeleted = false)
    {
        return relationChildrenByFilter(relationName, filter, order, includeToBeDeleted);
    }
    Q_INVOKABLE int relationChildsCount(const QString &relationName, bool includeToBeDeleted = true)
    {
        return relationChildrenCount(relationName, includeToBeDeleted);
    }
    Q_INVOKABLE void deleteAllChilds(const QString &relationName)
    {
        deleteAllChildren(relationName);
    }

    static QScriptValue toScriptValue(QScriptEngine *engine, BaseBean * const &in);
    static void fromScriptValue(const QScriptValue &object, BaseBean * &out);
    static QScriptValue toScriptValuePointer(QScriptEngine *engine, BaseBeanPointer const &in);
    static void fromScriptValuePointer(const QScriptValue &object, BaseBeanPointer &out);
    static QScriptValue toScriptValueSharedPointer(QScriptEngine *engine, const BaseBeanSharedPointer &in);
    static void fromScriptValueSharedPointer(const QScriptValue &object, BaseBeanSharedPointer &out);
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
    static QScriptValue toScriptValueWeakPointer(QScriptEngine *engine, const BaseBeanWeakPointer &in);
    static void fromScriptValueWeakPointer(const QScriptValue &object, BaseBeanWeakPointer &out);
#endif

    static int countBeans();
    static int maxCountBeans();

    bool blockAllSignals(bool value);

    DBField *fieldForRole(int role);
    QString scheduleTitle();

    void setDefaultValues(BaseBeanPointerList fathers = BaseBeanPointerList());

public slots:
    void uncheckModifiedFields();
    void uncheckModifiedRelatedElements();
    bool save(const QString &idTransaction = "", bool recalculateFieldsBefore = true);
    bool saveRelatedElements(const QString &idTransaction = "");
    void backupValues();
    void restoreValues(bool blockSignals = false);
    void setSerialUniqueId();
    void clean(bool onlyFields = true, bool children = false, bool fathers = false);
    bool reloadFromDb();
    bool reloadFromLinkedBean();
    QScriptValue copyValues();
    QScriptValue copyRelationChildren();
    QScriptValue deepCopyValues();
    void touch();
    void registerRecalculateFields();
    void setValuesFromFilter(const QString &filter);
    void setValuesFromFilter(const QVariantMap &filter);

private slots:
    void setModified();
    void setModified(BaseBean *child, bool value);
    void recalculateCalculatedFields();
    void emitBeforeInsert();
    void emitBeforeUpdate();
    void emitBeforeSave();
    void emitBeforeDelete();
    void emitBeanSaved();
    void emitBeanCommitted();
    void emitEndEdit();
    void emitFieldModified(const QString &dbFieldName, const QVariant &value);
    void emitDefaultValueFieldCalculated(const QString &dbFieldName, const QVariant &value);
    void emitRelatedElementModified(RelatedElement *element);
    void setPkValueFromInternal(const QVariant &id);
    void setActualContext(const QString &value);
    void setRelatedElementsModified();
    void adjustOldValues();
    void prepareToDeleteRelatedElements();
    bool removeConfiguredRelatedElements(const QString &idTransaction);

signals:
    /** El bean se ha modificado, por alguna de las siguientes razones: Ha cambiado el value de algún field,
      se ha agregado, modificado o borrado algún child, o se ha modificado algún field de algún child. Es
      una señal muy genérica, ideal para utilizar en caso de comprobar si hay que guardar en base de datos, o
      para actualizar la marca setWindowModified de un widget */
    void beanModified(bool value);
    void beanModified(BaseBean *, bool value);
    /** Cuando se modifica un field de este bean, se emite esta señal */
    void fieldModified(const QString &dbFieldName, const QVariant &value);
    void fieldModified(BaseBean *, const QString &dbFieldName, const QVariant &value);
    void defaultValueCalculated(const QString &dbFieldName, const QVariant &value);
    void defaultValueCalculated(BaseBean *, const QString &dbFieldName, const QVariant &value);
    /** Cuando se modifica el estado del bean, se emite esta señal */
    void dbStateModified(int);
    void dbStateModified(BaseBean *bean, int);
    /** Esta señal se emite justo antes de intentar guardar el registro en base de datos. Se emite tanto se se inserta, como si se actualiza */
    void beforeSave();
    void beforeSave(BaseBean *);
    /** Esta señal se emite justo antes de crear un nuevo registro en base de datos */
    void beforeInsert();
    void beforeInsert(BaseBean *);
    /** Esta señal se emite justo antes de actualizar un registro */
    void beforeUpdate();
    void beforeUpdate(BaseBean *);
    /** Se emite justo antes de borrar el bean */
    void beforeDelete();
    void beforeDelete(BaseBean *);
    /** Se emite cuando se ha producido la operación en base de datos: es decir, cuando se ha hecho INSERT, UPDATE, o DELETE.
    Se emite ANTES de que se haya completado la transacción. Es decir, AÚN NO hay certeza de que el bean está almacenado en base de datos.
    La transacción aún podría fallar. */
    void beanSaved();
    /** Se emite cuando se ha producido la operación en base de datos: es decir, cuando se ha hecho INSERT, UPDATE, o DELETE.
    Se emite ANTES de que se haya completado la transacción. Es decir, AÚN NO hay certeza de que el bean está almacenado en base de datos.
    La transacción aún podría fallar. */
    void beanSaved(BaseBean *);
    /** Se emite cuando el bean está definitivamente almacenado en base de datos, es decir, se ha hecho el commit */
    void beanCommitted();
    void beanCommitted(BaseBean *);
    /** Esta señal se emite por parte del bean, cuando el formulario DBRecordDlg, cierra la edición del bean. Es decir, el usuario
     * ha pinchado en guardar y se cierra el formulario */
    void endEdit();
    void endEdit(BaseBean *);
    /** Se emite al agregarse un nuevo elemento relacionado */
    void relatedElementAdded(RelatedElement *);
    /** Se emite al borrarse un elemento relacionado */
    void relatedElementDeleted(RelatedElement *);
    /** Se emite al modificarse un elemento relacionado */
    void relatedElementModified(RelatedElement *);
};

Q_DECLARE_METATYPE(BaseBean*)
Q_DECLARE_METATYPE(QList<BaseBean *>)
Q_DECLARE_METATYPE(BaseBeanPointer)
Q_DECLARE_METATYPE(BaseBeanPointerList)
Q_DECLARE_METATYPE(BaseBeanSharedPointer)
Q_DECLARE_METATYPE(BaseBeanSharedPointerList)
Q_DECLARE_METATYPE(BaseBeanWeakPointer)
Q_DECLARE_METATYPE(BaseBeanWeakPointerList)
Q_DECLARE_METATYPE(BaseBean::DbBeanStates)
Q_SCRIPT_DECLARE_QMETAOBJECT(BaseBean, QObject*)


template<typename LISTTYPE, typename BEANTYPE>
inline int AERPListContainsBean(LISTTYPE list, BEANTYPE bean)
{
    if ( list.isEmpty() )
    {
        return -1;
    }
    for (int i = 0 ; i < list.size() ; i++)
    {
        if ( !list.at(i).isNull() && !bean.isNull() &&
                ( list.at(i).data()->objectName() == bean.data()->objectName() ||
                  (list.at(i).data()->dbOid() == bean.data()->dbOid() && list.at(i).data()->dbOid() != 0 && bean.data()->dbOid() != 0 ) ))
        {
            return i;
        }
    }
    return -1;
}

template<typename LISTTYPE, typename BEANTYPE>
inline BEANTYPE AERPBeanByOid(LISTTYPE list, qlonglong oid)
{
    if ( list.isEmpty() )
    {
        return BEANTYPE();
    }
    for (int i = 0 ; i < list.size() ; i++)
    {
        if ( !list.at(i).isNull() && list.at(i).data()->dbOid() == oid )
        {
            return list.at(i);
        }
    }
    return BEANTYPE();
}

#endif
