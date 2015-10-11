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
#include "widgets/dbbasewidget.h"
#include "widgets/dbdetailview.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/basebean.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/dbobject.h"
#include "dao/dbfieldobserver.h"
#include "business/aerploggeduser.h"

DBFieldObserver::DBFieldObserver(DBObject *entity) :
    AbstractObserver(entity)
{
    if ( m_entity != NULL )
    {
        connect(m_entity, SIGNAL(valueModified(QVariant)), this, SIGNAL(entityValueModified(QVariant)));
        connect(m_entity, SIGNAL(dataAvailable(QVariant)), this, SIGNAL(dataAvailable(QVariant)));
        connect(m_entity, SIGNAL(errorBackgroundLoad(QString)), this, SIGNAL(errorBackgroundLoad(QString)));
        // Si este es el field de un bean de una relación con padre, nos conectamos también a la señal
        // cuando se carguen los datos del padre
        DBRelation *rel = qobject_cast<DBRelation *>(entity->owner());
        if ( rel != NULL )
        {
            connect(rel, SIGNAL(fatherLoaded(BaseBean*)), this, SLOT(syncWidgetField(BaseBean*)));
            connect(rel, SIGNAL(brotherLoaded(BaseBean*)), this, SLOT(syncWidgetField(BaseBean*)));
            //connect(rel, SIGNAL(fatherModified(bool)), this, SLOT(sync()));
        }
    }
}

DBFieldObserver::~DBFieldObserver()
{
    if ( m_entity != NULL )
    {
        DBField *fld = qobject_cast<DBField *>(m_entity);
        if ( fld != NULL )
        {
            disconnect(fld, SIGNAL(valueModified(const QVariant &)), this, SIGNAL(entityValueModified(const QVariant &)));
        }
    }
}

bool DBFieldObserver::readOnly()
{
    DBField *fld = qobject_cast<DBField *>(entity());
    if ( fld == NULL )
    {
        return true;
    }
    QString tableName = fld->bean()->metadata()->tableName();
    if ( !AERPLoggedUser::instance()->checkMetadataAccess('w', tableName) )
    {
        return true;
    }
    if ( fld != NULL )
    {
        return fld->metadata()->readOnly();
    }
    return false;
}

/*!
  Sincroniza el modelo y la vista
  */
void DBFieldObserver::sync()
{
    DBField *fld = qobject_cast<DBField *>(entity());
    if ( viewWidgets().size() > 0 )
    {
        if ( fld != NULL )
        {
            // Obtener un valor para ser representado, no puede implicar ningún cambio en el registro.
            // Por eso, bloqueamos las señales del registro, y del campo
            bool beanBlockSignals = fld->bean()->blockAllSignals(true);
            if ( fld->metadata()->loadOnBackground() )
            {
                if ( fld->memoLoaded() )
                {
                    bool blockSignalsState = fld->blockSignals(true);
                    emit entityValueModified(fld->value());
                    fld->blockSignals(blockSignalsState);
                }
                else
                {
                    if ( !fld->isLoadingOnBackground() )
                    {
                        emit initBackgroundLoad();
                        fld->loadValueOnBackground();
                    }
                }
            }
            else
            {
                bool blockSignalsState = fld->blockSignals(true);
                emit entityValueModified(fld->value());
                fld->blockSignals(blockSignalsState);
            }
            fld->bean()->blockAllSignals(beanBlockSignals);
        }
        else
        {
            emit entityValueModified(QVariant());
        }
    }
}

/*!
  Realiza todas las conexiones necesarias entre el widget y el observador
  para permitir la sincronización de datos. Esta función depende de cada observador,
  por ello hay que implementarla en cada observador
  */
void DBFieldObserver::installWidget(QObject *widget)
{
    appendToViewWidgets(widget);
    connect(this, SIGNAL(entityValueModified(QVariant)), widget, SLOT(setValue(QVariant)));
    if ( widget->property(AlephERP::stAerpControl).toBool() )
    {
        connect(widget, SIGNAL(valueEdited(QVariant)), this, SLOT(widgetEdited(QVariant)));
    }
}

void DBFieldObserver::uninstallWidget(QObject *widget)
{
    AbstractObserver::uninstallWidget(widget);
    if ( widget->property(AlephERP::stAerpControl).toBool() )
    {
        DBBaseWidget *bw = dynamic_cast<DBBaseWidget *>(widget);
        bw->observerUnregistered();
        disconnect(widget, SIGNAL(valueEdited(QVariant)), this, SLOT(widgetEdited(QVariant)));
    }
    disconnect(this, SIGNAL(entityValueModified(QVariant)), widget, SLOT(setValue(QVariant)));
}

void DBFieldObserver::widgetEdited(const QVariant &value)
{
    DBField *fld = qobject_cast<DBField *>(entity());
    if ( fld != NULL )
    {
        fld->setValue(value);
    }
}

void DBFieldObserver::syncWidgetField(BaseBean *bean)
{
    Q_UNUSED(bean)
    DBField *fld = qobject_cast<DBField *>(entity());
    if ( fld != NULL )
    {
        emit entityValueModified(fld->value());
    }
}

/*!
  Longitud máxima de edición del campo de texto
  */
int DBFieldObserver::maxLength()
{
    DBField *fld = qobject_cast<DBField *>(entity());
    if ( fld == NULL )
    {
        return 0;
    }
    if ( fld->metadata()->type() == QVariant::String && fld->length() == 0 )
    {
        qDebug() << "DBFieldObserver::maxLength: OJO: Máxima longitud del field [" << fld->metadata()->dbFieldName() << "] es 0. No se imprimirá nada en él";
    }
    return fld->length();
}

/*!
  Número de decimales
  */
int DBFieldObserver::partD()
{
    DBField *fld = qobject_cast<DBField *>(entity());
    if ( fld == NULL )
    {
        return 0;
    }
    return fld->metadata()->partD();
}
