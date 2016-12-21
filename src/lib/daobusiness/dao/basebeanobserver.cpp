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
#include "basebeanobserver.h"
#include <aerpcommon.h>
#include "dao/beans/basebean.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbrelation.h"
#include "dao/dbfieldobserver.h"
#include "dao/beans/basebeanvalidator.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/aerptransactioncontext.h"
#include "widgets/dbbasewidget.h"
#include "business/aerploggeduser.h"

class BaseBeanObserverPrivate
{
public:
    QPointer<BaseBeanValidator> m_validator;

    BaseBeanObserverPrivate()
    {
    }
};

BaseBeanObserver::BaseBeanObserver(DBObject *entity) :
    AbstractObserver(entity), d(new BaseBeanObserverPrivate)
{
    BaseBean *bean = qobject_cast<BaseBean *>(entity);
    if ( bean != 0 )
    {
        d->m_validator = new BaseBeanValidator(this);
        d->m_validator->setBean(bean);
        connect(bean, SIGNAL(beanModified(bool)), this, SLOT(setWindowModified(bool)));
        QList<DBRelation *> rels = bean->relations(AlephERP::OneToOne);
        for (DBRelation *rel : rels)
        {
            connect(rel, SIGNAL(brotherLoaded(BaseBean*)), this, SLOT(sync()));
        }
    }
}

BaseBeanObserver::~BaseBeanObserver()
{
    delete d;
}

void BaseBeanObserver::setEntity(QObject *obj)
{
    BaseBean *bean = qobject_cast<BaseBean *> (entity());
    if ( bean == NULL )
    {
        disconnect(bean, SIGNAL(beanModified(bool)), this, SLOT(setWindowModified(bool)));
    }
    AbstractObserver::setEntity(obj);
    if ( obj != 0 )
    {
        d->m_validator = new BaseBeanValidator(this);
        connect(obj, SIGNAL(beanModified(bool)), this, SLOT(setWindowModified(bool)));
    }
    d->m_validator->setBean(qobject_cast<BaseBean *>(obj));
    QList<DBRelation *> rels = bean->relations(AlephERP::OneToOne);
    for (DBRelation *rel : rels)
    {
        connect(rel, SIGNAL(brotherLoaded(BaseBean*)), this, SLOT(sync()));
    }
}

/*!
  Se informa a los widgets que muestran los datos de este bean que los datos han sido modificados
  */
void BaseBeanObserver::setWindowModified(bool value)
{
    QList<QPointer<QObject> > list = viewWidgets();
    for (QObject *widget : list)
    {
        // No se llama a setWindowModified, ya que esta función no es virtual, y no estaríamos
        // llamando al setWindowModified del hijo del widget (y no ejecutaría nuestro código propio en setWindowModified).
        // Llamamos a la propiedad y lo solucionamos
        if ( widget->property("windowModified").isValid() )
        {
            widget->setProperty("windowModified", value);
        }
    }
}

bool BaseBeanObserver::readOnly() const
{
    BaseBean *bean = qobject_cast<BaseBean *> (entity());
    if ( bean == NULL )
    {
        return true;
    }
    QString tableName = bean->metadata()->tableName();
    return !AERPLoggedUser::instance()->checkMetadataAccess('w', tableName);
}

/*!
  Realiza todas las conexiones necesarias entre el widget y el observador
  para permitir la sincronización de datos. Esta función depende de cada observador,
  por ello hay que implementarla en cada observador
  */
void BaseBeanObserver::installWidget(QObject *widget)
{
    appendToViewWidgets(widget);
}

/*!
  Desinstala un widget de este observador. Como este observador es el de un base bean
  se encargará de desinstalar a su vez los observadores hijos de este observador
  (los observadores de los DBField, DBRelation...)
  */
void BaseBeanObserver::uninstallWidget(QObject *widget)
{
    BaseBean *bean = qobject_cast<BaseBean *> (entity());
    if ( bean == NULL )
    {
        return;
    }
    AbstractObserver::uninstallWidget(widget);
    // Al desinstalar el formulario, debemos desinstalar todos los controles de ese form que pudieran estar asociados
    // TODO También habría que hacerlo para todos los hijos de las relaciones
    QList<QWidget *> children = widget->findChildren<QWidget *>();
    for ( QWidget *childWidget : children )
    {
        if ( childWidget->property(AlephERP::stAerpControl).toBool() )
        {
            DBBaseWidget *baseWidget = dynamic_cast<DBBaseWidget *> (childWidget);
            QList<DBObject *> lstObj = bean->navigateThrough(baseWidget->fieldName(), baseWidget->relationFilter());
            if ( lstObj.size() > 0 )
            {
                for (DBObject *obj : lstObj)
                {
                    obj->observer()->uninstallWidget(childWidget);
                }
            }
            else
            {
                DBObject *obj = bean->navigateThroughProperties(baseWidget->fieldName());
                if ( obj != NULL )
                {
                    obj->observer()->uninstallWidget(childWidget);
                }
            }
            // TODO: Ver, es muy probable que el código de arriba sobra... pero tampoco pasa nada por dejarlo.
            if ( baseWidget->observer(false) != NULL )
            {
                baseWidget->observer()->uninstallWidget(childWidget);
            }
        }
        if ( childWidget->property(AlephERP::stAerpControlRelation).toBool() )
        {
            DBBaseWidget *baseWidget = dynamic_cast<DBBaseWidget *> (childWidget);
            QList<DBObject *> lstObj = bean->navigateThrough(baseWidget->relationName(), baseWidget->relationFilter());
            if ( lstObj.size() > 0 )
            {
                for (DBObject *obj : lstObj)
                {
                    obj->observer()->uninstallWidget(childWidget);
                }
            }
            else
            {
                DBObject *obj = bean->navigateThroughProperties(baseWidget->relationName());
                if ( obj != NULL )
                {
                    obj->observer()->uninstallWidget(childWidget);
                }
            }
        }
    }
}

/*!
  Realiza la sincronización entre los datos del objeto de negocio y el de visualización
  */
void BaseBeanObserver::sync()
{
    BaseBean *bean = qobject_cast<BaseBean *>(m_entity);
    if ( bean == NULL )
    {
        return;
    }
    setWindowModified(bean->modified() | AERPTransactionContext::instance()->isDirty(bean->actualContext()));
    QList<QPointer<QObject> > list = viewWidgets();
    for (QObject *form : list)
    {
        QList<QWidget *> widgets = form->findChildren<QWidget *>();
        if ( form->property(AlephERP::stAerpControl).toBool() )
        {
            DBBaseWidget *baseWidget = dynamic_cast<DBBaseWidget *>(form);
            baseWidget->refresh();
        }
        for ( QWidget *w : widgets )
        {
            if ( w->property(AlephERP::stAerpControl).toBool() )
            {
                DBBaseWidget *baseWidget = dynamic_cast<DBBaseWidget *>(w);
                baseWidget->refresh();
            }
        }
    }
}

/*!
  Valida los datos contenidos en este bean, antes de almacenar en BBDD
  */
bool BaseBeanObserver::validate()
{
    if ( d->m_validator.isNull() )
    {
        return false;
    }
    return d->m_validator->validate();
}

/*!
  Esta función se llama después de llamar a validate, y devuelve una cadena de texto
  con el resultado de la validación
  */
QString BaseBeanObserver::validateMessages()
{
    if ( d->m_validator.isNull() )
    {
        return QString();
    }
    return d->m_validator->validateMessages();
}

/*!
  Esta función se llama después de llamar a validate, y devuelve una cadena de texto
  con el resultado de la validación, en html
  */
QString BaseBeanObserver::validateHtmlMessages()
{
    if ( d->m_validator.isNull() )
    {
        return QString();
    }
    return d->m_validator->validateHtmlMessages();
}

/*!
  Caso de una validación incorrecta, devuelve el primer widget que generó un problema
  */
QWidget *BaseBeanObserver::focusWidgetOnBadValidate()
{
    if ( d->m_validator.isNull() )
    {
        return NULL;
    }
    return d->m_validator->widgetOnBadValidate();
}

