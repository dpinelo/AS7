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
#include <QtSql>

#include "dbbasewidget.h"
#include "globales.h"
#include "configuracion.h"
#include "forms/dbrecorddlg.h"
#include "forms/dbsearchdlg.h"
#include "forms/dbwizarddlg.h"
#include "dao/database.h"
#include "dao/basedao.h"
#include "dao/dbfieldobserver.h"
#include "dao/observerfactory.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/beansfactory.h"
#include "models/filterbasebeanmodel.h"
#include "models/basebeanmodel.h"
#include "models/dbbasebeanmodel.h"
#include "models/perpquerymodel.h"
#include "scripts/perpscriptwidget.h"
#include "widgets/dbtableview.h"
#include "widgets/dbbasewidgettimerworker.h"
#include "business/aerploggeduser.h"

DBBaseWidget::DBBaseWidget()
{
    m_relation = NULL;
    m_observer = NULL;
    m_dataEditable = false;
    m_userModified = false;
    m_dataFromParentDialog = true;
    m_sqlExecutionTimeout = MS_WORKER_TIMER;
    m_barCodeReaderAllowed = false;
    m_onBarCodeReadNextFocus = false;
    m_previousKeyPress = QDateTime::currentDateTime();
    m_sqlConnectedToWorker = false;
}

DBBaseWidget::~DBBaseWidget()
{
}

/*!
  Preguntaremos a la factoría de observadores cuál es mi observador, según
  la configuración que se le haya dado a este widget en el ui. Así sabremos siempre
  que estamos presentando la información adecuado
*/
AbstractObserver * DBBaseWidget::observer(bool sync)
{
    if ( m_observer == NULL )
    {
        QWidget *cont = alephERPContainer();
        if ( cont != NULL )
        {
            DBSearchDlg *dlgSearch = qobject_cast<DBSearchDlg *> (cont);
            DBRecordDlg *dlgRecord = qobject_cast<DBRecordDlg *> (cont);
            DBTableView *tableView = qobject_cast<DBTableView *> (cont);
            DBWizardDlg *dlgWizard = qobject_cast<DBWizardDlg *> (cont);
            if ( dlgSearch != 0 )
            {
                BaseBean *beanTemplate = dlgSearch->templateBean();
                if ( beanTemplate != NULL )
                {
                    m_observer = ObserverFactory::instance()->registerBaseWidget(this, beanTemplate);
                }
            }
            else if ( dlgRecord != 0 || tableView != 0 || dlgWizard != 0 )
            {
                m_observer = ObserverFactory::instance()->registerBaseWidget(this, beanFromContainer());
                if ( m_observer != NULL )
                {
                    applyFieldProperties();
                    // Hay un caso bastante raro: El widget muestra el campo de una relación M1, por ejemplo
                    // tarjetascredito.codtarjeta  Este campo es un campo con un valor por defecto. El control
                    // mostraría ese valor por defecto, engañando al usuario. Debemos filtrar ese caso que se
                    // produce cuando el valor del bean raíz está vacío.
                    if ( m_fieldName.contains(".") )
                    {
                        QStringList parts = m_fieldName.split(".");
                        BaseBean *bean = beanFromContainer();
                        if ( bean != NULL )
                        {
                            DBRelation *rel = bean->relation(parts.at(0));
                            if ( rel != NULL )
                            {
                                if ( bean->isFieldEmpty(rel->metadata()->rootFieldName()) &&
                                     rel->metadata()->type() == DBRelationMetadata::MANY_TO_ONE &&
                                     rel->father() &&
                                     !rel->father()->modified() )
                                {
                                    return m_observer;
                                }
                            }
                        }
                    }
                    if ( sync )
                    {
                        m_observer->sync();
                    }
                }
            }
        }
    }
    return m_observer;
}

void DBBaseWidget::setFieldName(const QString &name)
{
    m_fieldName = name;
    QWidget *obj = dynamic_cast<QWidget *>(this);
    QString objName = QString("db_%1").arg(m_fieldName);
    if ( objName.isEmpty() || objName.startsWith(obj->objectName()) )
    {
        // Vamos a asegurarnos de que no hay otro objeto con ese mismo nombre!!!
        QDialog *parentDlg = CommonsFunctions::parentDialog(obj);
        if ( parentDlg != NULL )
        {
            bool areOther = false;
            QList<QWidget *> others = parentDlg->findChildren<QWidget *>(objName);
            foreach (QWidget *other, others)
            {
                if ( other != obj )
                {
                    areOther = true;
                }
            }
            if ( areOther )
            {
                obj->setObjectName(objName);
            }
        }
    }
}

QString DBBaseWidget::fieldName() const
{
    return m_fieldName;
}

void DBBaseWidget::setRelationName(const QString &name)
{
    m_relationName = name;
}

QString DBBaseWidget::relationName() const
{
    return m_relationName;
}

void DBBaseWidget::setRelationFilter(const QString &value)
{
    if ( m_relationFilter != value )
    {
        m_relationFilter = value;
        // Realizamos algunos reemplazos de algunas reglas
        m_relationFilter = DBBaseWidget::processSqlWhere(value);
        if ( m_observer != NULL )
        {
            m_observer->uninstallWidget(dynamic_cast<QObject *>(this));
            m_observer = NULL;
        }
        observer();
    }
}

QString DBBaseWidget::relationFilter() const
{
    return m_relationFilter;
}

bool DBBaseWidget::dataEditable()
{
    QWidget *wid = dynamic_cast<QWidget *>(this);
    DBRecordDlg *dlg = qobject_cast<DBRecordDlg *> (CommonsFunctions::aerpParentDialog(wid));
    if ( dlg != NULL )
    {
        if ( dlg->openType() == AlephERP::ReadOnly )
        {
            return false;
        }
    }

    if ( observer() == NULL )
    {
        return m_dataEditable;
    }
    DBSearchDlg *searchDlg = qobject_cast<DBSearchDlg *>(CommonsFunctions::aerpParentDialog(wid));
    if ( searchDlg != NULL )
    {
        return m_dataEditable;
    }
    if ( observer()->readOnly() )
    {
        return false;
    }
    return m_dataEditable;
}

void DBBaseWidget::setDataEditable(bool value)
{
    m_dataEditable = value;
    applyFieldProperties();
}

bool DBBaseWidget::userModified() const
{
    return m_userModified;
}

void DBBaseWidget::setUserModified(bool value)
{
    m_userModified = value;
}

bool DBBaseWidget::dataFromParentDialog() const
{
    return m_dataFromParentDialog;
}

void DBBaseWidget::setDataFromParentDialog(bool value)
{
    m_dataFromParentDialog = value;
}

DBRelation *DBBaseWidget::relation () const
{
    return m_relation;
}

void DBBaseWidget::setDBRelation(DBRelation *rel)
{
    m_relation = rel;
}

QString DBBaseWidget::reportParameterBinding()
{
    return m_reportParameterBinding;
}

void DBBaseWidget::setReportParameterBinding(const QString &value)
{
    m_reportParameterBinding = value;
}

QString DBBaseWidget::sqlData() const
{
    return m_sqlData;
}

void DBBaseWidget::setSqlData(const QString &value)
{
    m_sqlData = value;
    m_dataEditable = false;
    if ( !m_sqlData.isEmpty() )
    {
        connectToSqlWorker();
        if ( m_sqlWorkerUUID.isEmpty() )
        {
            QWidget *thisWidget = dynamic_cast<QWidget *>(this);
            m_sqlWorkerUUID = DBBaseWidgetTimerWorker::instance()->addData(this, m_sqlData, m_sqlExecutionTimeout);
            thisWidget->setProperty(AlephERP::stSqlWorkerUUID, m_sqlWorkerUUID);
        }
        else
        {
            DBBaseWidgetTimerWorker::instance()->setData(m_sqlWorkerUUID, value, m_sqlExecutionTimeout);
        }
        QWidget *wid = dynamic_cast<QWidget *>(this);
        wid->setFocusPolicy(Qt::NoFocus);
    }
}

int DBBaseWidget::sqlExecutionTimeout() const
{
    return m_sqlExecutionTimeout;
}

void DBBaseWidget::setSqlExecutionTimeout(int value)
{
    m_sqlExecutionTimeout = value;
    m_dataEditable = false;
    if ( !m_sqlData.isEmpty() )
    {
        if ( m_sqlWorkerUUID.isEmpty() )
        {
            m_sqlWorkerUUID = DBBaseWidgetTimerWorker::instance()->addData(this, m_sqlData, m_sqlExecutionTimeout);
        }
        else
        {
            DBBaseWidgetTimerWorker::instance()->setData(m_sqlWorkerUUID, m_sqlData, value);
        }
    }
}

BaseBeanPointer DBBaseWidget::workBean() const
{
    return m_workBean;
}

void DBBaseWidget::setWorkBean(BaseBeanPointer value)
{
    m_workBean = value;
}

bool DBBaseWidget::barCodeReaderAllowed() const
{
    return m_barCodeReaderAllowed;
}

void DBBaseWidget::setBarCodeReaderAllowed(bool value)
{
    m_barCodeReaderAllowed = value;
}

bool DBBaseWidget::onBarCodeReadNextFocus() const
{
    return m_onBarCodeReadNextFocus;
}

void DBBaseWidget::setOnBarCodeReadNextFocus(bool value)
{
    m_onBarCodeReadNextFocus = value;
}

QString DBBaseWidget::scriptAfterBarCodeRead() const
{
    return m_scriptAfterCodeBarRead;
}

void DBBaseWidget::setScriptAfterBarCodeRead(const QString &value)
{
    m_scriptAfterCodeBarRead = value;
}

QString DBBaseWidget::barCodeEndString() const
{
    return m_barCodeEndString;
}

void DBBaseWidget::setBarCodeEndString(const QString &value)
{
    m_barCodeEndString = value;
}

void DBBaseWidget::observerUnregistered()
{
    m_observer = NULL;
}

/**
 * @brief DBBaseWidget::wasCounterFieldValid
 * El campo actual, puede estar involucrado en el cálculo de un campo contador. Éste campo contador
 * puede aún tener valores nulos, y por tanto, no ser necesario realizar la pregunta relacionada
 * con el cálculo del contador
 * @return
 */
bool DBBaseWidget::wasCounterFieldValid()
{
    bool result = false;

    if ( observer() == NULL )
    {
        return result;
    }
    DBField *fld = qobject_cast<DBField *>(observer()->entity());
    if ( fld == NULL )
    {
        return false;
    }
    BaseBean *bean = fld->bean();
    QList<DBField *> counters = bean->counterFields(fld->metadata()->dbFieldName());
    foreach (DBField *fldCounter, counters)
    {
        if ( !fldCounter->value().toString().isEmpty() )
        {
            result = true;
        }
    }
    return result;
}

/**
 * Si este widget presenta un campo relacionado en el cálculo de un contador, al cambiar, puede ser interesante preguntar
 * al usuario si desea recalcularlo
 */
void DBBaseWidget::askToRecalculateCounterField()
{
    if ( observer() == NULL )
    {
        return;
    }
    QDialog *dlg = CommonsFunctions::parentDialog(dynamic_cast<QWidget *>(this));
    if ( dlg == NULL || (QString(dlg->metaObject()->className()) != "DBRecordDlg" && QString(dlg->metaObject()->className()) != "DBWizardDlg") )
    {
        return;
    }
    DBField *fld = qobject_cast<DBField *>(observer()->entity());
    if ( fld == NULL )
    {
        return;
    }
    BaseBean *bean = fld->bean();
    DBRecordDlg *recordDlg = qobject_cast<DBRecordDlg *>(dlg);
    if ( bean->readOnly() || (recordDlg != NULL && recordDlg->openType() == AlephERP::ReadOnly) )
    {
        return;
    }
    QList<DBField *> counters = bean->counterFields(fld->metadata()->dbFieldName());
    QStringList counterFields;
    foreach (DBField *fldCounter, counters)
    {
        if ( !fldCounter->value().toString().isEmpty() )
        {
            counterFields.append(fldCounter->metadata()->fieldName());
        }
    }

    if ( counters.size() > 0 )
    {
        int ret = QMessageBox::question(dynamic_cast<QWidget *>(this), qApp->applicationName(),
                                        QObject::trUtf8("Este campo que acaba de modificar está involucrado en la generación del valor de los campos: %1. "
                                                "¿Desea usted recalcular estos campos?").arg(counterFields.join(", ")), QMessageBox::Yes | QMessageBox::No);
        if ( ret == QMessageBox::Yes )
        {
            foreach (DBField *fldCounter, counters)
            {
                fldCounter->setValue(fldCounter->calculateCounter());
            }
        }
    }
}

void DBBaseWidget::workerDataAvailable(QVariant &value)
{
    setValue(value);
}

/*!
  Cuando el objeto pasa de visible a no visible, pregunta cuál es su observador, según
  sus datos de UI para mostrar los datos. Este es el origen por el que se empieza a producir
  la sincronía entre los datos de la capa de negocio y la de visualización
  */
void DBBaseWidget::showEvent(QShowEvent * event)
{
    Q_UNUSED (event)
    observer();
    showtMandatoryWildcardForLabel();
}

void DBBaseWidget::hideEvent(QHideEvent * event)
{
    Q_UNUSED (event)
}

/*!
  Devuelve el widget que contiene a este: este widget podrá ser o un DBRecordDlg o un DBSearchDlg o un
  AERPScriptWidget
  */
QWidget *DBBaseWidget::alephERPContainer()
{
    QObject *temp = (dynamic_cast<QObject *>(this))->parent();
    if ( temp == NULL )
    {
        return NULL;
    }
    if ( m_dataFromParentDialog )
    {
        do
        {
            DBRecordDlg *dlg = qobject_cast<DBRecordDlg *>(temp);
            if ( dlg != 0 )
            {
                return dlg;
            }
            DBSearchDlg *dlgSearch = qobject_cast<DBSearchDlg *>(temp);
            if ( dlgSearch != 0 )
            {
                return dlgSearch;
            }
            DBTableView *tableView = qobject_cast<DBTableView *>(temp);
            if ( tableView != 0 )
            {
                return tableView;
            }
            DBWizardDlg *dlgWizard = qobject_cast<DBWizardDlg *>(temp);
            if ( dlgWizard != 0 )
            {
                return dlgWizard;
            }
            temp = temp->parent();
        }
        while ( temp != 0 );
    }
    else
    {
        do
        {
            AERPScriptWidget *wid = qobject_cast<AERPScriptWidget *>(temp);
            if ( wid != 0 )
            {
                return wid;
            }
            temp = temp->parent();
        }
        while ( temp != 0 );
        temp = (dynamic_cast<QObject *>(this))->parent();
        do
        {
            if ( temp->property(AlephERP::stAerpControlRelation).toBool() )
            {
                return qobject_cast<QWidget *>(temp);
            }
            DBRecordDlg *dlg = qobject_cast<DBRecordDlg *>(temp);
            if ( dlg != 0 )
            {
                return dlg;
            }
            DBSearchDlg *dlgSearch = qobject_cast<DBSearchDlg *>(temp);
            if ( dlgSearch != 0 )
            {
                return dlgSearch;
            }
            DBTableView *tableView = qobject_cast<DBTableView *>(temp);
            if ( tableView != 0 )
            {
                return tableView;
            }
            DBWizardDlg *dlgWizard = qobject_cast<DBWizardDlg *>(temp);
            if ( dlgWizard != 0 )
            {
                return dlgWizard;
            }
            temp = temp->parent();
        }
        while ( temp != 0 );
    }
    return NULL;
}

/*!
  Los widgets que muestran datos pueden estar en tres tipos de contenedores lógicos: Uno, el DBRecord,
  otro, un AERPScriptWidget, otro un DBTableView. Esta función busca el contenedor más cercano en la jerarquía
  para de ahí obtener el bean que controla
  */
BaseBeanPointer DBBaseWidget::beanFromContainer()
{
    if (!m_workBean.isNull())
    {
        return m_workBean;
    }
    QWidget *temp = alephERPContainer();
    if ( temp == NULL )
    {
        return NULL;
    }
    DBRecordDlg *dlg = qobject_cast<DBRecordDlg *>(temp);
    if ( dlg != 0 )
    {
        BaseBean *b;
        if ( dlg->bean() == NULL )
        {
            b = NULL;
        }
        else
        {
            b = dlg->bean();
        }
        return b;
    }
    DBWizardDlg *dlgWizard = qobject_cast<DBWizardDlg *>(temp);
    if ( dlgWizard != 0 )
    {
        BaseBean *b;
        if ( dlgWizard->bean() == NULL )
        {
            b = NULL;
        }
        else
        {
            b = dlgWizard->bean();
        }
        return b;
    }
    AERPScriptWidget *wid = qobject_cast<AERPScriptWidget *>(temp);
    if ( wid != 0 )
    {
        BaseBean *b;
        if ( wid->bean() == NULL )
        {
            b = NULL;
        }
        else
        {
            b = wid->bean();
        }
        return b;
    }
    return NULL;
}

QString DBBaseWidget::processSqlWhere(const QString &where)
{
    // Realizamos algunos reemplazos de algunas reglas
    QString result = where;
    result = result.replace("${user}", AERPLoggedUser::instance()->userName());
    if ( Database::getQDatabase(BASE_CONNECTION).driverName() == QStringLiteral("QSQLITE") )
    {
        result = result.replace("true", "1");
        result = result.replace("false", "0");
    }
    return result;
}

void DBBaseWidget::connectToSqlWorker()
{
    if ( m_sqlConnectedToWorker )
    {
        return;
    }
    m_sqlConnectedToWorker = true;
    QWidget *thisWidget = dynamic_cast<QWidget *>(this);
    QObject::connect(DBBaseWidgetTimerWorker::instance(),
                     &DBBaseWidgetTimerWorker::newDataAvailable,
                     thisWidget,
                     [thisWidget](const QString &uuid, const QVariant &value)
    {
        if ( thisWidget->property(AlephERP::stSqlWorkerUUID).isValid() )
        {
            QString sqlWorkerUUID = thisWidget->property(AlephERP::stSqlWorkerUUID).toString();
            if ( uuid == sqlWorkerUUID )
            {
                thisWidget->setProperty(AlephERP::stValue, value);
            }
        }
    });
}

/**
  Devuelve la propiedad tableName del diálogo AERPBaseDialog que contiene al widget pasado
  */
QString DBBaseWidget::aerpBaseDialogTableName(QWidget *widget)
{
    if ( widget == NULL )
    {
        return QString();
    }
    if ( widget->property(AlephERP::stAerpControl).isNull() || !widget->property(AlephERP::stAerpControl).toBool() )
    {
        return QString();
    }
    DBBaseWidget *baseWidget = dynamic_cast<DBBaseWidget *>(widget);
    QObject *temp = qobject_cast<QObject *>(widget->parent());
    if ( temp == NULL )
    {
        return QString();
    }
    if ( baseWidget->dataFromParentDialog() )
    {
        do
        {
            QDialog *qDlg = qobject_cast<QDialog *>(temp);
            if ( qDlg != 0 && qDlg->property(AlephERP::stTableName).isValid() )
            {
                return qDlg->property(AlephERP::stTableName).toString();
            }
            AERPBaseDialog *dlg = qobject_cast<AERPBaseDialog *>(temp);
            if ( dlg != 0 )
            {
                return dlg->tableName();
            }
            temp = temp->parent();
        }
        while ( temp != 0 );
    }
    else
    {
        do
        {
            AERPScriptWidget *wid = qobject_cast<AERPScriptWidget *>(temp);
            if ( wid != 0 )
            {
                return wid->relationName();
            }
            temp = temp->parent();
        }
        while ( temp != 0 );
        temp = qobject_cast<QObject *>(widget->parent());
        do
        {
            QDialog *qDlg = qobject_cast<QDialog *>(temp);
            if ( qDlg != 0 && qDlg->property(AlephERP::stTableName).isValid() )
            {
                return qDlg->property(AlephERP::stTableName).toString();
            }
            AERPBaseDialog *dlg = qobject_cast<AERPBaseDialog *>(temp);
            if ( dlg != 0 )
            {
                return dlg->tableName();
            }
            temp = temp->parent();
        }
        while ( temp != 0 );
    }
    return QString();
}

/**
 * @brief DBBaseWidget::setWildcardForLabel
 * Si el campo no puede ser nulo, añade a la etiqueta relacionada (a través de buddy) un *, indicando al usuario
 * que debe rellenarla obligatoriamente
 */
void DBBaseWidget::showtMandatoryWildcardForLabel()
{
    QDialog *dlg = CommonsFunctions::parentDialog(dynamic_cast<QWidget *>(this));
    if ( dlg != NULL )
    {
        if ( observer() == NULL )
        {
            return;
        }
        DBField *fld = qobject_cast<DBField *>(observer()->entity());
        if ( fld == NULL )
        {
            return;
        }
        if ( fld->metadata()->canBeNull() )
        {
            return;
        }

        QList<QLabel *> labels = dlg->findChildren<QLabel *>();
        foreach (QLabel *label, labels)
        {
            QWidget *wid = dynamic_cast<QWidget *>(this);
            if ( label->buddy() == wid )
            {
                if ( !label->text().endsWith("*") )
                {
                    label->setText(label->text() + " *");
                }
            }
        }
    }
}

void DBBaseWidget::setFontAndColor()
{
    if ( observer() == NULL )
    {
        return;
    }
    DBField *fld = qobject_cast<DBField *>(observer()->entity());
    if ( fld == NULL )
    {
        return;
    }
    // Si el bean original está marcado como borrado, damos un aspecto diferente a los datos
    if ( fld->bean()->dbState() == BaseBean::TO_BE_DELETED || fld->bean()->dbState() == BaseBean::DELETED )
    {
        QWidget *widget = dynamic_cast<QWidget *>(this);
        QFont font = widget->font();
        font.setStrikeOut(true);
        widget->setFont(font);
        widget->setStyleSheet(QStringLiteral("color: #cccccc"));
    }
}
