/***************************************************************************
 *   Copyright (C) 2010 by David Pinelo                                    *
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

#ifndef DBRELATION_H
#define DBRELATION_H

#include <QtCore>
#include <QtScript>
#include <alepherpglobal.h>
#include <aerpcommon.h>
#include "dao/dbobject.h"
#include "dao/beans/basebean.h"

class BaseBean;
class BeansFactory;
class DBField;
class DBRelationMetadata;
class DBRelationPrivate;
class AERPTransactionContext;

/**
	Modela una relación de base de datos. Este objeto será parte de beans que tengan
	como sucesores a otros beans o como ancestros
	*/
class ALEPHERP_DLL_EXPORT DBRelation : public DBObject
{
    Q_OBJECT

    friend class BaseBean;
    friend class BeansFactory;
    friend class BaseDAO;
    friend class DBRelationPrivate;
    friend class DBField;
    friend class AERPTransactionContext;

    /** En una relación M1, este debe ser el método utilizado para obtener el "padre" (por ejemplo EJERCICIOS pertenece a EMPRESAS. Empresas es el padre */
    Q_PROPERTY(BaseBeanPointer father READ father WRITE setFather)
    /** Cuando se obtienen los hijos de una relación, es posible agregar un filtro para devolverlos. Aquí se puede especificar */
    Q_PROPERTY(QString filter READ filter WRITE setFilter)
    /** Indica si los beans están cargados */
    Q_PROPERTY(bool isLoad READ childrenLoaded)
    /** Se pone a true cuando se ha modificado algún hijo, agregado o borrado alguno... */
    Q_PROPERTY(bool childrenModified READ childrenModified)
    /** Número de hijos de esta relación */
    Q_PROPERTY(int count READ childrenCount)
    /** Hijos de la relación */
    Q_PROPERTY(BaseBeanPointerList items READ children)
    Q_PROPERTY(DBRelationMetadata *metadata READ metadata)
    /** Atajo para las relaciones 1-1, para obtener el registro relacionado */
    Q_PROPERTY(BaseBeanPointer brother READ brother WRITE setBrother)
    /** Indica si el padre ha sido establecido */
    Q_PROPERTY(bool fatherSetted READ fatherSetted)

private:
    Q_DISABLE_COPY(DBRelation)
    DBRelationPrivate *d;
//    Q_DECLARE_PRIVATE(DBRelation)

    void connections(const BaseBeanPointer &child);
    void setMetadata(DBRelationMetadata *m);

    BaseBeanPointerList internalChildren();
    void addInternalOtherChildren(BaseBeanPointer bean);
    BaseBeanPointer internalBrother();

public:
    explicit DBRelation(QObject *parent = 0);
    ~DBRelation();

    DBRelationMetadata * metadata() const;

    Q_INVOKABLE BaseBeanSharedPointer newChild(int pos = -1);
    Q_INVOKABLE BaseBeanPointer childByOid(qlonglong oid, bool includeToBeDeleted = false);
    Q_INVOKABLE BaseBeanPointer childByField(const QString &dbField, const QVariant &value, bool includeToBeDeleted = false);
    Q_INVOKABLE BaseBeanPointer childByFilter(const QString &filter, bool includeToBeDeleted = false);
    Q_INVOKABLE BaseBeanPointerList childrenByFilter(const QString &filter, const QString &order = "", bool includeToBeDeleted = false);
    Q_INVOKABLE BaseBeanPointerList children(const QString &order = "", bool includeToBeDeleted = true);
    Q_INVOKABLE int childrenCountByState(BaseBean::DbBeanStates state);
    Q_INVOKABLE BaseBeanPointerList modifiedChildren();
    Q_INVOKABLE void addChildren(BaseBeanSharedPointerList list);
    Q_INVOKABLE void addChild(BaseBeanSharedPointer child);
    Q_INVOKABLE void deleteAllChildren();
    Q_INVOKABLE void deleteChildByObjectName(const QString &objectName);

    void removeChild(int row);
    void removeChild(BaseBeanWeakPointer child);
    void removeChild(QVariant pk);
    void removeAllChildren();
    void removeChildByObjectName(const QString &objectName);

    BaseBeanPointer father(bool retrieveOnDemand = true);
    void setFather(BaseBean *bean);
    bool fatherSetted();
    BaseBeanSharedPointerList sharedChildren(const QString &order = "", bool includeToBeDeleted = true);

    BaseBeanPointer brother();
    void setBrother(BaseBeanPointer bean);
    bool brotherSetted();

    QString filter() const;
    void setFilter(const QString &filter);
    bool childrenLoaded();
    bool childrenModified();
    /** Se proporcionar por semántica */
    bool isFatherLoaded()
    {
        return childrenLoaded();
    }

    QString fetchChildSqlWhere (const QString &aliasChild = QString (""));
    QString fetchFatherSqlWhere(const QString &aliasChild = QString (""));
    QString sqlChildTableAlias();

    Q_INVOKABLE DBField * masterField();
    BaseBeanPointer ownerBean() const;
    bool allSignalsBlocked() const;

    static QScriptValue toScriptValueSharedPointer(QScriptEngine *engine, const QSharedPointer<DBRelation> &in);
    static void fromScriptValueSharedPointer(const QScriptValue &object, QSharedPointer<DBRelation> &out);
    static QScriptValue toScriptValue(QScriptEngine *engine, DBRelation * const &in);
    static void fromScriptValue(const QScriptValue &object, DBRelation * &out);

    // Sí, soy un cateto... y cometí un error de novato en inglés. childs en lugar de children... ole mis ... Lo corrijo, y mientras
    // dejo esto por compatibilidad hacia atrás con otros clientes...
    // Yes. Big mistake with plural. Like I was 6 years old. "childs" instead of children... Now, I have to provide these functions for
    // backward compatibility
    Q_INVOKABLE BaseBeanPointerList childsByFilter(const QString &filter, const QString &order = "", bool includeToBeDeleted = false)
    {
        return childrenByFilter(filter, order, includeToBeDeleted);
    }
    Q_INVOKABLE BaseBeanPointerList childs(const QString &order = "", bool includeToBeDeleted = true)
    {
        return children(order, includeToBeDeleted);
    }
    Q_INVOKABLE int childsCount(bool includeToBeDeleted = true)
    {
        return childrenCount(includeToBeDeleted);
    }
    Q_INVOKABLE void removeAllChilds()
    {
        removeAllChildren();
    }
    Q_INVOKABLE BaseBeanPointerList modifiedChilds()
    {
        return modifiedChildren();
    }
    Q_INVOKABLE void deleteAllChilds()
    {
        deleteAllChildren();
    }

signals:
    /** Señal que se emite cuando algún hijo de la relación se modifica */
    void childModified(BaseBean *, bool);
    /** Señal que se emite al insertar algún hijo en la relación */
    void childInserted(BaseBean *, int pos);
    /** Se emite cuando YA se ha borrado un hijo de la relación */
    void childDeleted(BaseBean *, int pos);
    /** Se emite cuando se ha guardado un hijo mediante la transacción a la base de datos */
    void childSaved(BaseBean *);
    /** Se emite cuando se ha guardado un hijo mediante la transacción a la base de datos */
    void childComitted(BaseBean *);
    /** Se emite cuando se ha terminado de editar un hijo, en un DBRecord asociado. Estas señales
    sólo se emiten cuando el usuario pincha el botón guardar o salvar de esos formularios. */
    void childEndEdit();
    void childEndEdit(BaseBean *);
    /** Se emite cuando se modifica el estado de un child de esta relación */
    void childDbStateModified(BaseBean *, int state);
    /** Señal que indica que el field de un child se ha modificado. Es muy útil cuando
      haya que extender la modificación a los widgets asociados. La idea, estamos presentando los
      datos de un cliente en un presupuesto. Eso se hace utilizando la relación. Se modifica el codcliente,
      se actualiza la relación, se emite esta señal y el widget se entera de que se ha
      modificado el valor */
    void fieldChildModified(BaseBean *, const QString &field, const QVariant &value);
    void fieldChildDefaultValueCalculated(BaseBean *, const QString &field, const QVariant &value);
    /** Ocurre cuando se borran todos los hijos de una relación (porque se haya cambiado el campo maestro */
    void rootFieldChanged();
    /** Se ha cargado el bean padre de la relación */
    void fatherLoaded(BaseBean *father);
    /** Se ha descargado el bean padre... porque seguramente el root field ha cambiado */
    void fatherUnloaded();
    /** Se emite cuando se ha modificado el padre */
    void fatherModified(bool);
    /** Se ha cargado el hermano */
    void brotherLoaded(BaseBean *brother);
    /** Carga en background */
    void beanLoaded(BaseBeanSharedPointer bean);
    void beanLoaded(DBRelation *rel, int row, BaseBeanSharedPointer bean);
    void backgroundLoadFinished(DBRelation *rel, bool result);
    void beansLoaded(DBRelation *rel, BaseBeanSharedPointerList list);
    /** Los registros que cuelgan de esta relación, se han descargado de memoria */
    void childrenUnloaded();

private slots:
    void setChildrensModified(BaseBean *bean, bool value);
    void childFieldBeanModified(const QString &fieldName, const QVariant &value);
    void fatherHasBeenDeleted();
    void emitChildDbStateModified(BaseBean *bean, int state);
    void emitChildComitted(BaseBean *bean);
    void emitChildSaved(BaseBean *bean);
    void emitChildEndEdit();
    void emitChildEndEdit(BaseBean *bean);
    void setChildrenLoadedInternaly();
    void otherChildrenDestroyed(QObject *obj);
    void availableBean(QString id, int row, BaseBeanSharedPointer bean);
    void availableBeans(QString id, BaseBeanSharedPointerList list);
    void backgroundQueryExecuted(QString id, bool result);

protected slots:
    void updateChildrens();

public slots:
    void uncheckChildrensModified();
    bool blockAllSignals(bool value);
    bool loadChildrenOnBackground(const QString &order = "ASC");
    QString sqlRelationWhere();
    bool isLoadingBackground();
    int childrenCount(bool includeToBeDeleted = true);
    void setChildrenCount(int value);
    BaseBeanPointer child(int row);
    void restoreValues(bool blockSignals = false);
    void loadFather();
    bool unloadChildren(bool ignoreNotSavedBeans = true);
    bool unloadFather(bool ignoreNotSavedBeans = true);
    bool unload(bool ignoreNotSavedBeans = true);
};

Q_DECLARE_METATYPE(QList<DBRelation*>)
Q_DECLARE_METATYPE(DBRelation*)
Q_DECLARE_METATYPE(QSharedPointer<DBRelation>)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBRelation, QObject*)

#endif // DBRELATION_H
