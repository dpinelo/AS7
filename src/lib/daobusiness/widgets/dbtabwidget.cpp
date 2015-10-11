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
#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include "dbtabwidget.h"
#include "dao/beans/basebean.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/basedao.h"
#include "forms/perpbasedialog.h"
#include "forms/dbrecorddlg.h"
#include "widgets/dbbasewidget.h"
#include "globales.h"

class DBTabWidgetPrivate
{
public:
    DBTabWidget *q_ptr;
    /** Tabla de cuyos registros se tomarán los datos */
    QString m_tableName;
    /** Campo que se visualizará en el tab text */
    QString m_fieldView;
    /** Filtro que se aplica para obtener los registros de la tabla */
    QString m_filter;
    /** Columna que marca el orden con el que se presentan los datos */
    QString m_order;
    /** Contiene los beans que soportan la visualización */
    BaseBeanSharedPointerList m_list;
    bool m_useAsChildFilter;

    DBTabWidgetPrivate(DBTabWidget *qq) : q_ptr(qq) {
        m_useAsChildFilter = false;
    }

    void filterChildren(int index);
};

DBTabWidget::DBTabWidget(QWidget * parent)
    : QTabWidget(parent), d(new DBTabWidgetPrivate(this))
{
    connect (this->tabBar(), SIGNAL(currentChanged(int)), this, SLOT(tabHasBeenChanged(int)));
}

DBTabWidget::DBTabWidget(const QString &dbField, QWidget * parent)
    : QTabWidget(parent), d(new DBTabWidgetPrivate(this))
{
    d->m_fieldView = dbField;
    connect (this->tabBar(), SIGNAL(currentChanged(int)), this, SLOT(tabHasBeenChanged(int)));
}


DBTabWidget::~DBTabWidget()
{
    delete d;
}

QString DBTabWidget::tableName()
{
    return d->m_tableName;
}

void DBTabWidget::setTableName (const QString &value)
{
    d->m_tableName = value;
    // Si la lista no estaba vacía es que se han cambiado datos. Reiniciamos
    if ( !d->m_list.isEmpty() )
    {
        clear();
        init();
    }
}

QString DBTabWidget::fieldView()
{
    return d->m_fieldView;
}

void DBTabWidget::setFieldView(const QString &field)
{
    d->m_fieldView = field;
    // Si la lista no estaba vacía es que se han cambiado datos. Reiniciamos
    if ( !d->m_list.isEmpty() )
    {
        clear();
        init();
    }
}

QString DBTabWidget::filter()
{
    return d->m_filter;
}

void DBTabWidget::setFilter(const QString &value)
{
    d->m_filter = value;
    // Si la lista no estaba vacía es que se han cambiado datos. Reiniciamos
    if ( !d->m_list.isEmpty() )
    {
        clear();
        init();
    }
}

QString DBTabWidget::order()
{
    return d->m_order;
}

void DBTabWidget::setOrder(const QString &value)
{
    d->m_order = value;
    // Si la lista no estaba vacía es que se han cambiado datos. Reiniciamos
    if ( !d->m_list.isEmpty() )
    {
        clear();
        init();
    }
}

bool DBTabWidget::useAsChildFilter() const
{
    return d->m_useAsChildFilter;
}

void DBTabWidget::setUseAsChildFilter(bool value)
{
    d->m_useAsChildFilter = value;
}

void DBTabWidget::init()
{
    if ( d->m_tableName.isEmpty() || d->m_fieldView.isEmpty() )
    {
        return;
    }
    if ( BaseDAO::select(d->m_list, d->m_tableName, d->m_filter, d->m_order ) )
    {
        QTabBar *barra = tabBar();
        if ( d->m_list.size () > 0 )
        {
            // La primera ficha está ya añadida.
            barra->setTabText(0, d->m_list.at(0)->fieldValue(d->m_fieldView).toString());
            for ( int i = 1 ; i < d->m_list.size() ; i++ )
            {
                barra->addTab(d->m_list.at(i)->fieldValue(d->m_fieldView).toString());
            }
        }
    }
}

void DBTabWidget::clear()
{
    QTabWidget::clear();
    d->m_list.clear();
}

void DBTabWidget::showEvent ( QShowEvent * event )
{
    Q_UNUSED(event)
    if ( d->m_list.isEmpty() )
    {
        init();
    }
    if ( d->m_useAsChildFilter )
    {
        d->filterChildren(currentIndex());
    }
}

/*!
Este slot interno emite un señal indicando qué tab ha cambiado el usuario y
lanzando el ID asociado a ese tab.
 */
void DBTabWidget::tabHasBeenChanged(int index)
{
    QVariant id;

    if ( index > -1 && !d->m_list.isEmpty() )
    {
        id = d->m_list.at(index)->pkValue();
    }
    emit tabChanged(id);
    emit tabChanged();

    if ( d->m_useAsChildFilter )
    {
        d->filterChildren(index);
    }
}

/*!
	Devuelve el id del tab seleccionado.
 */
QVariant DBTabWidget::idCurrentTab(void)
{
    QVariant id;
    if ( !d->m_list.isEmpty() && currentIndex() > -1 && currentIndex() < d->m_list.size() )
    {
        BaseBeanSharedPointer bean = d->m_list.at(currentIndex());
        id = bean->pkValue();
    }
    return id;
}

BaseBeanSharedPointer DBTabWidget::currentBean()
{
    if ( !d->m_list.isEmpty() && currentIndex() > -1 && currentIndex() < d->m_list.size() )
    {
        BaseBeanSharedPointer bean = d->m_list.at(currentIndex());
        return bean;
    }
    return BaseBeanSharedPointer();
}

BaseBeanSharedPointer DBTabWidget::bean(int index)
{
    if ( !d->m_list.isEmpty() && index > -1 && index < d->m_list.size() )
    {
        BaseBeanSharedPointer b = d->m_list.at(index);
        return b;
    }
    return BaseBeanSharedPointer();
}

/*!
  Permite añadir un tab que no está en la tabla. Por ejemplo, para añadir un
  tab "Todos"
  */
void DBTabWidget::addTab(const QVariant &id, const QString & nombre)
{
    BaseBeanSharedPointer temp = d->m_list.at(0);
    QTabBar *barra = tabBar();

    d->m_list.append(BeansFactory::instance()->newQBaseBean(temp->metadata()->tableName()));
    d->m_list.last()->setPkValue(id);
    d->m_list.last()->field(d->m_fieldView)->setValue(nombre);

    barra->addTab(nombre);
}

/*!
  Tenemos que decirle al motor de scripts, que DBFormDlg se convierte de esta forma a un valor script
  */
QScriptValue DBTabWidget::toScriptValue(QScriptEngine *engine, DBTabWidget * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBTabWidget::fromScriptValue(const QScriptValue &object, DBTabWidget * &out)
{
    out = qobject_cast<DBTabWidget *>(object.toQObject());
}


void DBTabWidgetPrivate::filterChildren(int index)
{
    DBRecordDlg *dlg = qobject_cast<DBRecordDlg *>(CommonsFunctions::aerpParentDialog(q_ptr));
    if ( dlg == NULL )
    {
        return;
    }
    BaseBeanSharedPointer selectedBean = q_ptr->bean(index);
    BaseBeanPointer bean = dlg->bean();
    if ( bean.isNull() )
    {
        return;
    }
    QList<QWidget *> widgets = q_ptr->findChildren<QWidget *>();
    foreach (QWidget *widget, widgets)
    {
        if ( widget->property(AlephERP::stAerpControl).toBool() )
        {
            DBBaseWidget *dbBase = dynamic_cast<DBBaseWidget *>(widget);
            BaseBeanMetadata *m = BeansFactory::metadataBean(dbBase->relationName());
            if ( m != NULL )
            {
                // El control tiene una relación por la que poder filtrar
                DBRelationMetadata *rel = m->relation(m_tableName);
                if ( rel != NULL )
                {
                    // Eliminamos el anterior filtro que se hubiese establecido
                    QRegExp filterRegExp (QString("%1 = [^\\s]").arg(rel->rootFieldName()));
                    QString relationFilter = dbBase->relationFilter();
                    relationFilter.replace(filterRegExp, "");
                    if ( !selectedBean.isNull() )
                    {
                        if ( !relationFilter.isEmpty() )
                        {
                            relationFilter.append(" AND ");
                        }
                        QVariantList list = selectedBean->pkListValue();
                        if ( list.size() > 0 )
                        {
                            relationFilter.append(QString("%1 = %2").
                                                  arg(rel->rootFieldName()).
                                                  arg(list.at(0).toString()));
                            dbBase->setRelationFilter(relationFilter);
                        }
                    }
                }
            }
        }
    }
}
