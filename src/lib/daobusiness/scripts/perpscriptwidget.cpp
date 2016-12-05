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
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtScript>
#include <QtUiTools>
#include <QtXml>
#include <QtXmlPatterns>
#ifdef ALEPHERP_DEVTOOLS
#include <QtScriptTools>
#endif
#include "configuracion.h"
#include "uiloader.h"
#include "perpscriptwidget.h"
#include "globales.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/basebean.h"
#include "dao/observerfactory.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbrelationmetadata.h"
#include "forms/dbrecorddlg.h"
#include "scripts/perpscript.h"

class AERPScriptWidgetPrivate
{
public:
    Q_DECLARE_PUBLIC(AERPScriptWidget)
    /** Nombre del widget para buscar en base de datos el .ui y el .qs que le darán funcionalidad.
      Si m_name está vacío se utilizará objectName() */
    QString m_name;
    /** Bean cuyo contenido mostrará o sobre el que trabajará este widget */
    BaseBeanSharedPointer m_bean;
    /** Motor para los scripts */
    AERPScriptQsObject m_engine;
    /** QWidget que contendrá el objeto creado por el UI */
    QWidget *m_widget;
    /** Indica si se ha iniciado la ejecución del script */
    bool m_init;
    /** Nombre de la clase en QS que dará la funcionalidad */
    QString m_qsClassName;
    AERPScriptWidget *q_ptr;
    QVBoxLayout *m_layout;

    explicit AERPScriptWidgetPrivate(AERPScriptWidget * qq) : q_ptr(qq)
    {
        m_widget = NULL;
        m_init = false;
        m_layout = NULL;
    }

    bool setupWidget();
};

AERPScriptWidget::AERPScriptWidget(QWidget *parent) :
    QWidget(parent), d(new AERPScriptWidgetPrivate(this))
{
    d->m_layout = new QVBoxLayout;
    d->m_layout->setContentsMargins(0, 0, 0, 0);
    d->m_layout->setSpacing(0);
    setLayout(d->m_layout);
}

AERPScriptWidget::~AERPScriptWidget()
{
    emit destroyed(this);
    delete d;
}

void AERPScriptWidget::emitSignal(const QString &signalName, const QVariant &value)
{
    emit signalEmitted(signalName, value);
}

void AERPScriptWidget::showEvent(QShowEvent *event)
{
    if ( d->m_init )
    {
        refresh();
    }
    // Cargamos las dimensiones guardadas de la ventana
    alephERPSettings->applyDimensionForm(this);
    QWidget::showEvent(event);
}

BaseBean * AERPScriptWidget::bean()
{
    BaseBean *bean = NULL;
    if ( m_observer == NULL )
    {
        return NULL;
    }
    BaseBeanPointer containerBean = beanFromContainer();
    if ( !containerBean.isNull() )
    {
        DBRelation *rel = containerBean.data()->relation(this->m_relationName);
        if ( rel != NULL )
        {
            if ( rel->metadata()->type() == DBRelationMetadata::MANY_TO_ONE )
            {
                bean = rel->father();
            }
            else
            {
                bean = rel->childByFilter(m_relationFilter).data();
            }
        }
        else
        {
            bean = containerBean.data();
        }
    }
    return bean;
}

/** Devuelve el bean del DBRecordDlg que contiene a este widget */
BaseBean *AERPScriptWidget::dialogBean()
{
    QDialog *dlg = CommonsFunctions::parentDialog(this);
    DBRecordDlg *dbDlg = qobject_cast<DBRecordDlg *>(dlg);
    if ( dbDlg != NULL )
    {
        return dbDlg->bean();
    }
    return NULL;
}

void AERPScriptWidget::setName(const QString &value)
{
    bool setNewWidget = d->m_name != value;
    d->m_name = value;
    if ( setNewWidget )
    {
        if ( d->m_widget != NULL )
        {
            delete d->m_widget;
        }
        d->setupWidget();
        if ( d->m_init )
        {
            refresh();
        }
    }
}

QString AERPScriptWidget::name()
{
    if ( d->m_name.isEmpty() )
    {
        return objectName();
    }
    return d->m_name;
}

QString AERPScriptWidget::qsClassName()
{
    return d->m_qsClassName;
}

void AERPScriptWidget::setQsClassName(const QString &value)
{
    d->m_qsClassName = value;
}

AbstractObserver * AERPScriptWidget::observer()
{
    if ( m_observer == NULL )
    {
        BaseBean *containerBean = beanFromContainer();
        if ( containerBean == NULL )
        {
            m_observer = NULL;
        }
        else
        {
            DBRelation *rel = containerBean->relation(this->m_relationName);
            if ( rel == NULL )
            {
                m_observer = ObserverFactory::instance()->registerBaseWidget(this, containerBean);
            }
            else
            {
                BaseBeanPointer childBean;
                if ( rel->metadata()->type() == DBRelationMetadata::MANY_TO_ONE )
                {
                    childBean = rel->father();
                }
                else
                {
                    childBean = rel->childByFilter(m_relationFilter);
                }
                m_observer = ObserverFactory::instance()->registerBaseWidget(this, childBean.data());
            }
            if ( m_observer != NULL )
            {
                applyFieldProperties();
            }
        }
    }
    return m_observer;
}

/*!
  Esta función incia la funcionalidad de este widget
  */
bool AERPScriptWidget::initQs()
{
    QString qsName = QString ("%1.widget.qs").arg(name());
    QString mensaje = tr("Ha ocurrido un error al cargar el script asociado al widget "
                             "%1. Es posible que algunas funciones no estén disponibles.").arg(name());
    QDialog *dlg = CommonsFunctions::parentDialog(this);
    if ( dlg == NULL )
    {
        return false;
    }

    observer();
    applyFieldProperties();
    if ( !m_observer.isNull() )
    {
        m_observer->sync();
    }
    /* Ejecutamos el script asociado, si lo hubiera */
    if ( BeansFactory::systemScripts.contains(qsName) )
    {
        d->m_engine.setScript(BeansFactory::systemScripts.value(qsName), qsName);
        d->m_engine.setDebug(BeansFactory::systemScriptsDebug.value(qsName));
        d->m_engine.setOnInitDebug(BeansFactory::systemScriptsDebugOnInit.value(qsName));
        d->m_engine.setUi(d->m_widget);

        if ( d->m_qsClassName.isEmpty() )
        {
            d->m_engine.setScriptObjectName("Widget");
        }
        else
        {
            d->m_engine.setScriptObjectName(d->m_qsClassName);
        }
        d->m_engine.addToEnviroment("thisWidget", this);
        if ( !m_observer.isNull() )
        {
            BaseBean *bean = qobject_cast<BaseBean *>(m_observer->entity());
            if ( bean != NULL )
            {
                d->m_engine.addToEnviroment("bean", bean);
            }
            d->m_engine.addToEnviroment("containerBean", dialogBean());
        }
        if ( dlg != NULL )
        {
            d->m_engine.setThisFormObject(this);
        }

        CommonsFunctions::setOverrideCursor(QCursor(Qt::ArrowCursor));
        bool result = d->m_engine.createQsObject();
        CommonsFunctions::restoreOverrideCursor();
        if ( !result )
        {
            QMessageBox::warning(this,qApp->applicationName(), mensaje, QMessageBox::Ok);
#ifdef ALEPHERP_DEVTOOLS
            int ret = QMessageBox::information(this,qApp->applicationName(), tr("El script ejecutado contiene errores. ¿Desea editarlo?"),
                                               QMessageBox::Yes | QMessageBox::No);
            if ( ret == QMessageBox::Yes )
            {
                d->m_engine.editScript(this);
            }
#endif
            return false;
        }
    }
    d->m_init = true;
    emit ready();
    return true;
}

/*!
  Lee el widget de base de datos (el UI) y lo establece
  */
bool AERPScriptWidgetPrivate::setupWidget()
{
    QString fileName = QString("%1.widget.ui").arg(q_ptr->name());
    QString mensaje = QObject::tr("No se ha podido cargar la interfaz de usuario del objeto %1. "
                                      "Existe un problema en la definición de las tablas de sistema de su programa.").
                      arg(q_ptr->name());
    bool result = true;

    if ( BeansFactory::systemUi.contains(fileName) )
    {
        QBuffer buffer(&BeansFactory::systemUi[fileName]);
        m_widget = AERPUiLoader::instance()->load(&buffer, q_ptr);
        if ( m_widget != NULL )
        {
            // Ahora instalamos los filtros que el diálogo principal necesita
            // para controlar a estos widgets (y la modificación por parte del usuario)
            QDialog *dlg = CommonsFunctions::parentDialog(q_ptr);
            if ( dlg != NULL )
            {
                QList<QWidget *> children = m_widget->findChildren<QWidget *>();
                foreach ( QWidget *child, children )
                {
                    if ( child->property(AlephERP::stAerpControl).toBool() )
                    {
                        child->installEventFilter(dlg);
                    }
                }
            }
            m_layout->addWidget(m_widget);
        }
        else
        {
            QLabel *lbl = new QLabel(q_ptr);
            lbl->setText(mensaje);
            m_layout->addWidget(lbl);
            result = false;
        }
    }
    else
    {
        QLabel *lbl = new QLabel(q_ptr);
        lbl->setText(mensaje);
        m_layout->addWidget(lbl);
        result = false;
    }
    return result;
}

/*!
  Establece el valor a mostrar en el control
*/
void AERPScriptWidget::setValue(const QVariant &value)
{
    Q_UNUSED(value)
}

/*!
 Devuelve el valor mostrado o introducido en el control
*/
QVariant AERPScriptWidget::value()
{
    return QVariant();
}

/*!
 Ajusta el control y sus propiedades a lo definido en el field
*/
void AERPScriptWidget::applyFieldProperties()
{
    QWidgetList list = findChildren<QWidget *>();
    foreach (QWidget *widget, list)
    {
        if ( widget->property(AlephERP::stAerpControl).toBool() )
        {
            widget->setProperty(AlephERP::stDataEditable, dataEditable());
        }
    }
}

/*!
	Para refrescar los controles: Piden nuevo observador si es necesario
*/
void AERPScriptWidget::refresh()
{
    if ( d->m_init )
    {
        observer();
        applyFieldProperties();
        if ( !m_observer.isNull() )
        {
            m_observer->sync();
            BaseBean *bean = qobject_cast<BaseBean *>(m_observer->entity());
            if ( bean != NULL )
            {
                d->m_engine.replaceEnviromentObject("bean", bean);
            }
            else
            {
                d->m_engine.replaceEnviromentObject("bean", NULL);
            }
        }
        callQSMethod("refresh");
    }
}

/*!
  Permite llamar a un método de la clase que controla al widget. Muy interesante
  para obtener valores determinados del widget.
  */
QScriptValue AERPScriptWidget::callQSMethod(const QString &method)
{
    QScriptValue result;
    if ( !d->m_init )
    {
        initQs();
    }
    d->m_engine.callQsObjectFunction(result, method);
    return result;
}

void AERPScriptWidget::closeEvent ( QCloseEvent * event )
{
    // Guardamos las dimensiones del usuario
    alephERPSettings->saveDimensionForm(this);
    event->accept();
}

void AERPScriptWidget::observerUnregistered()
{
    DBBaseWidget::observerUnregistered();
    bool blockState = blockSignals(true);
    d->m_engine.replaceEnviromentObject("bean", NULL);
    blockSignals(blockState);
}

void AERPScriptWidget::setRelationName(const QString &name)
{
    if ( m_relationName != name )
    {
        DBBaseWidget::setRelationName(name);
        refresh();
    }
}

void AERPScriptWidget::setRelationFilter(const QString &name)
{
    if ( m_relationFilter != name )
    {
        DBBaseWidget::setRelationFilter(name);
        refresh();
    }
}

/**
	Cuando el texto recibe el foco, se le eliminan todos los caracteres menos números y punto.
	Es decir, mostramos en el texto, el numerito almacenado internamente, pero sin locale.
 */
void AERPScriptWidget::focusInEvent(QFocusEvent * event)
{
    if ( event->reason() == Qt::TabFocusReason || event->reason() == Qt::ShortcutFocusReason )
    {
        if ( d->m_widget != NULL )
        {
            QObjectList list = d->m_widget->children();
            foreach (QObject *obj, list)
            {
                QWidget *wid = qobject_cast<QWidget *>(obj);
                if ( wid != NULL && wid->focusPolicy() != Qt::NoFocus )
                {
                    qDebug() << obj->objectName();
                    qDebug() << obj->metaObject()->className();
                    wid->setFocus();
                    return;
                }
            }
        }
    }
}
