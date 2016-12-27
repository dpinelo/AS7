/***************************************************************************
 *   Copyright (C) 2011 by David Pinelo   *
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
#include <QtCore>
#include "configuracion.h"
#include "observerfactory.h"
#include "dao/basebeanobserver.h"
#include "dao/dbfieldobserver.h"
#include "dao/dbrelationobserver.h"
#include "dao/dbmultiplerelationobserver.h"
#include "dao/dbobject.h"
#include "dao/beans/basebean.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/basebeanmetadata.h"
#include "widgets/dbbasewidget.h"
#include "forms/perpbasedialog.h"

QList<DBObject *> ObserverFactory::registeredObjects;

ObserverFactory::ObserverFactory(QObject *parent) :
    QObject(parent)
{
}

ObserverFactory::~ObserverFactory()
{
}

/**
 * @brief ObserverFactory::instance
 * Singleton. Pero no debe ser thread-safe ya que sirve para los widgets del GUI
 * @return
 */
ObserverFactory * ObserverFactory::instance()
{
    if ( qApp && QThread::currentThread() != qApp->thread() )
    {
        return NULL;
    }
    static ObserverFactory* singleton = 0;
    if ( singleton == 0 )
    {
        singleton = new ObserverFactory();
        // Esto garantiza que el objeto se borra al cerrar la aplicación
        singleton->setParent(qApp);
    }
    return singleton;
}

/*!
  Devuelve el observador adecuado para una entidad. Este observador le permitirá
  visualizar los datos en la UI si se ha definido así. Para devolver el observador, debe
  examinar las propiedades del entity y crearlo teniendo en cuenta esas propuedades.
  Un observador sólo tendrá un único entity y cero o varios widgets.
  */
AbstractObserver *ObserverFactory::newObserver(DBObject *entity)
{
    AbstractObserver *obj = NULL;
    QString className (entity->metaObject()->className());
    if ( className == QStringLiteral("BaseBean") )
    {
        obj = new BaseBeanObserver(entity);
    }
    else if ( className == QStringLiteral("DBField") )
    {
        obj = new DBFieldObserver(entity);
    }
    else if ( className == QStringLiteral("DBRelation") )
    {
        obj = new DBRelationObserver(entity);
    }
    return obj;
}

/*!
  Esta función será llamado por los widgets para saber qué observador le corresponde.
  Para ello se creará o buscará el observer adecuado dentro de bean
  */
AbstractObserver * ObserverFactory::registerBaseWidget(DBBaseWidget *baseWidget, BaseBean *bean)
{
    switch ( baseWidget->observerType(bean) )
    {
    case AlephERP::DbField:
        return installDBFieldObserver(baseWidget, bean);
    case AlephERP::DbRelation:
        return installDBRelationObserver(baseWidget, bean);
    case AlephERP::BaseBean:
        return installBaseBeanObserver(baseWidget, bean);
    case AlephERP::DbMultipleRelation:
        return installDBMultipleRelationObserver(baseWidget, bean);
    }
    return NULL;
}

AbstractObserver *ObserverFactory::installBaseBeanObserver(DBBaseWidget *baseWidget, BaseBean *bean)
{
    QWidget *widget = dynamic_cast<QWidget *>(baseWidget);
    if ( widget == 0 )
    {
        return NULL;
    }

    if ( bean != NULL && bean->dbState() != BaseBean::TO_BE_DELETED && bean->dbState() != BaseBean::DELETED )
    {
        AbstractObserver *observer = NULL;
        observer = bean->observer();
        observer->installWidget(widget);
        return observer;
    }
    return NULL;
}

/**
 * @brief ObserverFactory::installDBFieldObserver
 * Obtiene el DBField al que este observador relacionará widget y field.
 * @param baseWidget
 * @param bean
 * @return
 */
AbstractObserver *ObserverFactory::installDBFieldObserver(DBBaseWidget *baseWidget, BaseBean *bean)
{
    DBField *fld;
    QWidget *widget = dynamic_cast<QWidget *>(baseWidget);
    if ( widget == 0 )
    {
        return NULL;
    }

    if ( bean != NULL && bean->dbState() != BaseBean::TO_BE_DELETED && bean->dbState() != BaseBean::DELETED )
    {
        fld = NULL;
        QStringList path = baseWidget->fieldName().split(".");
        if ( path.size() == 1 )
        {
            fld = bean->field(baseWidget->fieldName());
        }
        else
        {
            const QList<DBObject *> lstObj = bean->navigateThrough(baseWidget->fieldName(), baseWidget->relationFilter());
            if ( lstObj.size() > 0 )
            {
                fld = qobject_cast<DBField *>(lstObj.at(0));
            }
            // A lo mejor la expresión del fieldName es utilizando propiedades...
            if ( lstObj.size() == 0 )
            {
                DBObject *obj = bean->navigateThroughProperties(baseWidget->fieldName());
                if ( obj != NULL )
                {
                    fld = qobject_cast<DBField *>(obj);
                    if ( baseWidget->fieldName().contains("father") )
                    {
                        // Para los controles, los padres no modificados e insertados no existen.
                        if ( fld != NULL && !fld->bean()->modified() && fld->bean()->dbState() == BaseBean::INSERT )
                        {
                            fld = NULL;
                        }
                    }
                }
            }
        }
        // Quizás el campo es NULO porque es el field de un father de una relación, y este father aún no se ha cargado.
        if ( fld == NULL )
        {
            DBRelation *rel = bean->relation(path.first());
            if ( rel != NULL)
            {
                connect(rel, SIGNAL(fatherLoaded(BaseBean*)), widget, SLOT(refresh()));
                connect(rel, SIGNAL(brotherLoaded(BaseBean*)), widget, SLOT(refresh()));
            }
        }
        else
        {
            AbstractObserver *observer = NULL;
            observer = fld->observer();
            if ( observer != NULL )
            {
                observer->installWidget(widget);
                // El field es el fruto de un "padre" o un objeto derivado... hay que efectuar esta conexión
                if ( bean->objectName() != fld->bean()->objectName() )
                {
                    DBRelation *rel = qobject_cast<DBRelation *>(fld->bean()->owner());
                    if ( rel != NULL)
                    {
                        connect(rel, SIGNAL(fatherLoaded(BaseBean*)), observer, SLOT(syncWidgetField(BaseBean*)));
                        connect(rel, SIGNAL(brotherLoaded(BaseBean*)), observer, SLOT(syncWidgetField(BaseBean*)));
                    }
                }
                return observer;
            }
        }
    }
    return NULL;
}

/*!
  Determina la relación que se devuelve según la configuración de un base widget.
  Se mira para determinarlo lo que aparezca en relationName y relationFilter. Así es posible
  tener:
  relationName = presupuestos_cabecera.presupuestos_ejemplares.presupuestos_actividades
  relationFilter = id_presupuesto = primaryKeyIdPresupuesto;id=pkNumEjemplares;id_categoria=2
  Se debe por tanto pasar las primary key de las primeras tablas.
  */
AbstractObserver *ObserverFactory::installDBRelationObserver(DBBaseWidget *baseWidget, BaseBean *bean)
{
    AbstractObserver *observer = NULL;
    DBRelation *rel = NULL;
    QWidget *widget = dynamic_cast<QWidget *>(baseWidget);
    if ( widget == 0 )
    {
        return NULL;
    }

    if ( bean != NULL && bean->dbState() != BaseBean::TO_BE_DELETED && bean->dbState() != BaseBean::DELETED )
    {
        // El relationName puede venir en la forma: presupuestos_ejemplares.presupuestos_actividades ...
        // En ese caso, debemos iterar hasta dar con la relación adecuada. Esa iteración se realiza en
        // combinación con el filtro establecido. El filtro en ese caso debe mostrar primero
        // la primary key de presupuestos_ejemplares, y después el filtro que determina presupuestos_actividades
        QStringList relations = baseWidget->relationName().split(".");
        if ( relations.size() == 1 )
        {
            rel = bean->relation(baseWidget->relationName());
        }
        else
        {
            const QList<DBObject *> relations = bean->navigateThrough(baseWidget->relationName(), baseWidget->relationFilter());
            if ( relations.size() == 0 )
            {
                DBObject *obj = bean->navigateThroughProperties(baseWidget->relationName());
                if ( obj != NULL )
                {
                    rel = qobject_cast<DBRelation *>(obj);
                }
            }
            else
            {
                rel = qobject_cast<DBRelation *> (relations.at(0));
            }
        }
        if ( rel != NULL )
        {
            observer = rel->observer();
        }
    }
    if ( observer != NULL )
    {
        observer->installWidget(widget);
    }
    return observer;
}

AbstractObserver *ObserverFactory::installDBMultipleRelationObserver(DBBaseWidget *baseWidget, BaseBean *bean)
{
    AbstractObserver *observer = NULL;
    QWidget *widget = dynamic_cast<QWidget *>(baseWidget);
    if ( widget == 0 )
    {
        return NULL;
    }

    if ( bean != NULL && bean->dbState() != BaseBean::TO_BE_DELETED && bean->dbState() != BaseBean::DELETED )
    {
        // TODO: Esto hay que verlo... puede generar una degradación del sistema, ya que se van generando observadores
        // que se borran sólo al cerrar la aplicación, cuando se destruye el singleton
        observer = new DBMultipleRelationObserver(bean, baseWidget->relationName());
        observer->setParent(this);
    }
    if ( observer != NULL )
    {
        observer->installWidget(widget);
    }
    return observer;
}

AbstractObserver::AbstractObserver(DBObject *entity) : QObject(entity)
{
    m_entity = entity;
    setObjectName(QString("%1").arg(alephERPSettings->uniqueId()));
}

AbstractObserver::~AbstractObserver()
{
    observerToBeDestroyed();
}

QObject * AbstractObserver::entity() const
{
    if ( m_entity.isNull() )
    {
        return NULL;
    }
    return m_entity.data();
}

void AbstractObserver::setEntity(QObject *obj)
{
    m_entity = obj;
    connect(m_entity.data(), SIGNAL(destroyed()), this, SLOT(observerToBeDestroyed()));
}

const QList<QPointer<QObject> > AbstractObserver::viewWidgets() const
{
    return m_viewWidgets;
}

/*!
  Este slot se conecta automáticamente a la destrucción de un widget. Elimina el widget
  de la lista de objetos observados por el observador, y procede al unregister del mismo (esto
  es, las desconexiones)
  */
void AbstractObserver::widgetToBeDestroyed(QObject *widget)
{
    uninstallWidget(widget);
}

/*!
  Este slot se conecta automáticamente a la destrucción de una entidad. Forzara por tanto
  a los widgets hijos a solicitar una nueva entidad a la que conectarse
  */
void AbstractObserver::observerToBeDestroyed()
{
    if ( m_entity )
    {
        qDebug() << m_entity->metaObject()->className();
        qDebug() << m_entity->property(AlephERP::stDbFieldName);
    }
    foreach ( QPointer<QObject> obj, m_viewWidgets )
    {
        if ( !obj.isNull() && obj->property(AlephERP::stAerpControl).toBool() )
        {
            DBBaseWidget *baseWidget = dynamic_cast<DBBaseWidget *>(obj.data());
            baseWidget->observerUnregistered();
        }
    }
    m_viewWidgets.clear();
}

void AbstractObserver::appendToViewWidgets(QObject *widget)
{
    m_viewWidgets.append(widget);
    connect(widget, SIGNAL(destroyed(QObject *)), this, SLOT(widgetToBeDestroyed(QObject*)));
}

void AbstractObserver::uninstallWidget(QObject *widget)
{
    int index = -1;
    for (int i = 0 ; i < m_viewWidgets.size() ; i++ )
    {
        if ( m_viewWidgets.at(i).data() == widget )
        {
            index = i;
            break;
        }
    }
    if ( index != -1 )
    {
        m_viewWidgets.removeAt(index);
        disconnect(widget, SIGNAL(destroyed(QObject *)), this, SLOT(widgetToBeDestroyed(QObject*)));
    }
}
