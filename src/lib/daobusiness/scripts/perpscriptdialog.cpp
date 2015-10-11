/***************************************************************************
 *   Copyright (C) 2010 by David Pinelo   *
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
#ifdef ALEPHERP_DEVTOOLS
#include <QtScriptTools>
#endif

#include "perpscriptdialog.h"
#include "forms/dbsearchdlg.h"
#include "forms/dbrecorddlg.h"
#include "forms/scriptdlg.h"
#include "forms/dbreportrundlg.h"
#include "dao/beans/basebean.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/basedao.h"
#include "reports/reportrun.h"

class AERPScriptDialogPrivate
{
public:
    /** Tipo del diálogo */
    QString m_type;
    /** Tabla de la que editará los datos */
    QString m_tableName;
    /** Si abrimos un formulario de búsqueda, este será el field que podrá leerse a través
      de la propiedad fieldSearched tras la ejecución del diálogo */
    QString m_fieldToSearch;
    /** Dato buscado */
    QVariant m_fieldSearched;
    /** Filtro de búsqueda con el que se abrirá el formulario de búsqueda */
    QString m_searchFilter;
    /** Indica si el usuario, en el formulario abierto ejecutó la acción o no */
    bool m_userClickOk;
    /** Valores de la primary key del bean que se editará, si se utiliza el tipo "record" */
    QVariantMap m_pkValues;
    /** Widget que será el padre de los formularios */
    QWidget *m_parentWidget;
    /** Bean seleccionado por el usuario en una búsqueda, o bien editado o insertado */
    BaseBeanSharedPointer m_selectedBean;
    /** Código Qs que ejecutará un formulario de tipo Script */
    QString m_qsName;
    /** Código Ui que se creará para el formulario */
    QString m_uiName;
    /** El programador QS puede proporcionar un bean que quiere editar. Se guarda aquí */
    BaseBeanPointer m_beanToEdit;
    QString m_reportName;
    /** Datos que se pueden pasar a los diálogos */
    QHash<QString, QObject *> m_objects;
    QHash<QString, QVariant> m_data;
    QHash<QString, QVariant> m_defaultValues;
    /** Punteros seguros a los diálogos */
    QPointer<DBSearchDlg> m_dialogSearch;
    QPointer<DBRecordDlg> m_dialogRecord;
    QPointer<ScriptDlg> m_dialogScript;
    QPointer<DBReportRunDlg> m_dialogReport;
    bool m_staysOnTop;
    bool m_modal;

    AERPScriptDialogPrivate()
    {
        m_userClickOk = false;
        m_parentWidget = NULL;
        m_staysOnTop = false;
        m_modal = true;
    }
};


AERPScriptDialog::AERPScriptDialog(QObject *parent) :
    QObject(parent), d(new AERPScriptDialogPrivate)
{
}

AERPScriptDialog::~AERPScriptDialog()
{
    if ( d->m_dialogRecord )
    {
        d->m_dialogRecord->deleteLater();
    }
    if ( d->m_dialogScript )
    {
        d->m_dialogScript->deleteLater();
    }
    if ( d->m_dialogSearch )
    {
        d->m_dialogSearch->deleteLater();
    }
    delete d;
}

QScriptValue AERPScriptDialog::specialAERPScriptDialogConstructor(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(context)
    AERPScriptDialog *object = new AERPScriptDialog();
    return engine->newQObject(object, QScriptEngine::ScriptOwnership);
}

/*!
  Tenemos que decirle al motor de scripts, que AERPScriptDialog se convierte de esta forma a un valor script
  */
QScriptValue AERPScriptDialog::toScriptValue(QScriptEngine *engine, AERPScriptDialog * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void AERPScriptDialog::fromScriptValue(const QScriptValue &object, AERPScriptDialog * &out)
{
    out = qobject_cast<AERPScriptDialog *>(object.toQObject());
}


/*!
  Tenemos que decirle al motor de scripts, que AERPScriptDialog se convierte de esta forma a un valor script
  */
QScriptValue AERPScriptDialog::toScriptValueSharedPointer(QScriptEngine *engine, const QSharedPointer<AERPScriptDialog> &in)
{
    return engine->newQObject(in.data(), QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void AERPScriptDialog::fromScriptValueSharedPointer(const QScriptValue &object, QSharedPointer<AERPScriptDialog> &out)
{
    out = AERPScriptDialogPointer(qobject_cast<AERPScriptDialog *>(object.toQObject()));
}

/*!
  type indicará qué tipo de diálogo se crea:
  "search" para un diálogo de búsqueda
  "record" para la edición de datos
  */
void AERPScriptDialog::setType(const QString &type)
{
    d->m_type = type;
}

QString AERPScriptDialog::type()
{
    return d->m_type;
}

void AERPScriptDialog::setTableName(const QString &tableName)
{
    d->m_tableName = tableName;
}

QString AERPScriptDialog::tableName()
{
    return d->m_tableName;
}

void AERPScriptDialog::setQsName(const QString &name)
{
    d->m_qsName = name;
}

QString AERPScriptDialog::qsName()
{
    return d->m_qsName;
}

void AERPScriptDialog::setUiName(const QString &name)
{
    d->m_uiName = name;
}

QString AERPScriptDialog::uiName()
{
    return d->m_uiName;
}

void AERPScriptDialog::setReportName(const QString &name)
{
    d->m_reportName = name;
}

QString AERPScriptDialog::reportName() const
{
    return d->m_reportName;
}

QString AERPScriptDialog::fieldToSearch()
{
    return d->m_fieldToSearch;
}

void AERPScriptDialog::setFieldToSearch(const QString &value)
{
    d->m_fieldToSearch = value;
}

QScriptValue AERPScriptDialog::fieldSearched()
{
    if ( !d->m_fieldSearched.isValid() )
    {
        return QScriptValue(QScriptValue::UndefinedValue);
    }
    return engine()->newVariant(d->m_fieldSearched);
}

QString AERPScriptDialog::searchFilter()
{
    return d->m_searchFilter;
}

void AERPScriptDialog::setSearchFilter(const QString &value)
{
    d->m_searchFilter = value;
}

BaseBeanPointer AERPScriptDialog::beanToEdit()
{
    return d->m_beanToEdit;
}

void AERPScriptDialog::setBeanToEdit(BaseBeanPointer bean)
{
    d->m_beanToEdit = bean;
}


bool AERPScriptDialog::userClickOk()
{
    return d->m_userClickOk;
}

bool AERPScriptDialog::staysOnTop() const
{
    return d->m_staysOnTop;
}

void AERPScriptDialog::setStaysOnTop(bool value)
{
    d->m_staysOnTop = value;
}

bool AERPScriptDialog::modal() const
{
    return d->m_modal;
}

void AERPScriptDialog::setModal(bool value)
{
    d->m_modal = value;
}

/**
    Puede editarse un registro de un bean, indicando la primary key. Por ejemplo:
  var dlg = new DBDialog;
  dlg.addPkValue("codempresa", 1);
  dlg.addPkValue("codtercero", "EP00001");
  dlg.tableName = "terceros";
  dlg.type = "record";
  dlg.show();
  OJO: Si se ha establecido un bean a editar (dlg.beanToEdit) ésta opción será prioritaria
  sobre la primary key
  */
void AERPScriptDialog::addPkValue(const QString &field, const QVariant &data)
{
    d->m_pkValues[field] = data;
}

void AERPScriptDialog::setParentWidget(QWidget *parent)
{
    d->m_parentWidget = parent;
}

void AERPScriptDialog::show()
{
    if ( d->m_type == "search" )
    {
        newSearchDlg();
    }
    else if ( d->m_type == "record" )
    {
        newRecordDlg();
    }
    else if ( d->m_type == "script" )
    {
        newScriptDlg();
    }
    else if ( d->m_type == "report" )
    {
        newReportDlg();
    }
}

void AERPScriptDialog::newSearchDlg()
{
    if ( d->m_dialogSearch )
    {
        delete d->m_dialogSearch;
    }
    d->m_dialogSearch = new DBSearchDlg(d->m_tableName, d->m_parentWidget);
    QHashIterator<QString, QObject *> itObjects(d->m_objects);
    QHashIterator<QString, QVariant> itData(d->m_data);

    if ( d->m_staysOnTop )
    {
        d->m_dialogSearch->setWindowFlags(d->m_dialogSearch->windowFlags() | Qt::WindowStaysOnTopHint);
    }
    // No vamos a establecer el flag de borrado automático
    if ( d->m_dialogSearch->openSuccess() )
    {
        d->m_dialogSearch->setDefaultValues(d->m_defaultValues);
        d->m_dialogSearch->setModal(d->m_modal);
        d->m_dialogSearch->setFilterData(d->m_searchFilter);
        while ( itObjects.hasNext() )
        {
            itObjects.next();
            d->m_dialogSearch->addPropertyToThisForm(itObjects.key(), itObjects.value());
        }
        while ( itData.hasNext() )
        {
            itData.next();
            d->m_dialogSearch->addPropertyToThisForm(itData.key(), itData.value());
        }
        d->m_dialogSearch->init();
        if ( d->m_modal )
        {
            d->m_dialogSearch->exec();
        }
        else
        {
            d->m_dialogSearch->show();
        }
        d->m_userClickOk = d->m_dialogSearch->userClickOk();
        // Recogemos el campo buscado si lo hay
        BaseBeanSharedPointer bean = d->m_dialogSearch->selectedBean();
        if ( !bean.isNull() )
        {
            d->m_selectedBean = bean;
            d->m_fieldSearched = bean->fieldValue(d->m_fieldToSearch);
        }
    }
}

void AERPScriptDialog::newRecordDlg()
{
    AlephERP::FormOpenType openType = AlephERP::Insert;
    BaseBeanPointer beanToEdit;

    if ( !d->m_beanToEdit.isNull() )
    {
        beanToEdit = d->m_beanToEdit;
        if ( beanToEdit->dbState() == BaseBean::INSERT )
        {
            openType = AlephERP::Insert;
        }
        else if ( beanToEdit.data()->dbState() == BaseBean::UPDATE )
        {
            openType = AlephERP::Update;
        }
        else
        {
            qDebug() << "AERPScriptDialog::newRecordDlg: El bean no está en estado INSERT o UPDATE";
        }
    }
    else
    {
        if ( d->m_pkValues.isEmpty() )
        {
            d->m_selectedBean = BeansFactory::instance()->newQBaseBean(d->m_tableName);
            openType = AlephERP::Insert;
        }
        else
        {
            d->m_selectedBean = BaseDAO::selectByPk(d->m_pkValues, d->m_tableName);
            openType = AlephERP::Update;
        }
        beanToEdit = d->m_selectedBean.data();
        if ( d->m_selectedBean.isNull() )
        {
            QMessageBox::warning(d->m_parentWidget, qApp->applicationName(),
                                 trUtf8("Ha ocurrido un error leyendo todos los datos a modificar. No es posible la modificac\303\263n del registro."),
                                 QMessageBox::Ok);
            return;
        }
    }
    if ( d->m_dialogRecord )
    {
        delete d->m_dialogRecord;
    }
    d->m_dialogRecord = new DBRecordDlg(beanToEdit, openType, d->m_parentWidget);
    QHashIterator<QString, QObject *> itObjects(d->m_objects);
    QHashIterator<QString, QVariant> itData(d->m_data);
    if ( d->m_staysOnTop )
    {
        d->m_dialogRecord->setWindowFlags(d->m_dialogRecord->windowFlags() | Qt::WindowStaysOnTopHint);
    }
    if ( d->m_dialogRecord->openSuccess() && d->m_dialogRecord->init() )
    {
        d->m_dialogRecord->setModal(d->m_modal);
        while ( itObjects.hasNext() )
        {
            itObjects.next();
            d->m_dialogRecord->addPropertyToThisForm(itObjects.key(), itObjects.value());
        }
        while ( itData.hasNext() )
        {
            itData.next();
            d->m_dialogRecord->addPropertyToThisForm(itData.key(), itData.value());
        }
        if ( d->m_modal )
        {
            d->m_dialogRecord->exec();
        }
        else
        {
            d->m_dialogRecord->show();
        }
        d->m_userClickOk = d->m_dialogRecord->userSaveData();
    }
}

void AERPScriptDialog::newScriptDlg()
{
    QHashIterator<QString, QObject *> itObjects(d->m_objects);
    QHashIterator<QString, QVariant> itData(d->m_data);
    if ( d->m_dialogScript )
    {
        delete d->m_dialogScript;
    }
    d->m_dialogScript = new ScriptDlg(d->m_uiName, d->m_qsName, d->m_parentWidget);
    if ( d->m_staysOnTop )
    {
        d->m_dialogScript->setWindowFlags(d->m_dialogScript->windowFlags() | Qt::WindowStaysOnTopHint);
    }
    if ( d->m_dialogScript->openSuccess() )
    {
        d->m_dialogScript->setModal(d->m_modal);
        while ( itObjects.hasNext() )
        {
            itObjects.next();
            d->m_dialogScript->addPropertyToThisForm(itObjects.key(), itObjects.value());
        }
        while ( itData.hasNext() )
        {
            itData.next();
            d->m_dialogScript->addPropertyToThisForm(itData.key(), itData.value());
        }
        if ( d->m_modal )
        {
            d->m_dialogScript->exec();
        }
        else
        {
            d->m_dialogScript->show();
        }
    }
}

void AERPScriptDialog::newReportDlg()
{
    QHashIterator<QString, QObject *> itObjects(d->m_objects);
    QHashIterator<QString, QVariant> itData(d->m_data);

    ReportRun run;
    run.setReportName(d->m_reportName);
    run.showDialog();
    /*
     *TODO
        ReportMetadata *report = BeansFactory::metadataReport(d->m_reportName);
        d->m_dialogReport = new DBReportRunDlg(report, d->m_parentWidget);
        if ( d->m_dialogReport->openSuccess() ) {
            d->m_dialogReport->setModal(d->m_modal);
            while ( itObjects.hasNext() ) {
                itObjects.next();
                d->m_dialogReport->addPropertyToThisForm(itObjects.key(), itObjects.value());
            }
            while ( itData.hasNext() ) {
                itData.next();
                d->m_dialogReport->addPropertyToThisForm(itData.key(), itData.value());
            }
            d->m_dialogReport->exec();
        }
        */
}

QScriptValue AERPScriptDialog::selectedBean()
{
    QScriptValue result;
    if ( !d->m_selectedBean.isNull() && engine() != NULL )
    {
        BaseBean *bean = BeansFactory::instance()->newBaseBean(d->m_selectedBean->metadata()->tableName());
        // Debemos dar un bean nuevo por si existiese la posibilidad de que el bean que se entrega al Script tuviera
        // una vida más larga que el asociado a este control, lo que pudiera desembocar en un null pointer en el script.
        if ( BaseDAO::selectByPk(d->m_selectedBean->pkValue(), bean) )
        {
            result = engine()->newQObject(bean, QScriptEngine::ScriptOwnership);
        }
    }
    return result;
}

/**
  Hace visible al Diálogo que se va a crear los siguientes el objeto obj, accediendo a él mediante thisForm.name
  */
void AERPScriptDialog::addPropertyObjectToThisForm(const QString &name, QObject *obj)
{
    d->m_objects[name] = obj;
}

void AERPScriptDialog::addPropertyValueToThisForm(const QString &name, QVariant data)
{
    d->m_data[name] = data;
}

/**
  Devuelve la propiedad \a name, del objeto thisForm presente en el QS asociado al diálogo creado
  */
QScriptValue AERPScriptDialog::readPropertyFromThisForm(const QString &name)
{
    if ( d->m_type == "search" && d->m_dialogSearch )
    {
        return d->m_dialogSearch->readPropertyFromThisForm(name);
    }
    else if ( d->m_type == "record" && d->m_dialogRecord )
    {
        return d->m_dialogRecord->readPropertyFromThisForm(name);
    }
    else if ( d->m_type == "script" && d->m_dialogScript )
    {
        return d->m_dialogScript->readPropertyFromThisForm(name);
    }
    return QScriptValue(QScriptValue::UndefinedValue);
}

/** Cuando se abre un formulario de búsqueda puede ser interesante abrirlo especificando ya algunos valores previos
 * a los controles de búsqueda que ve el usuario (no hablamos del filtro SQL, que es fijo y el usuario no puede cambiar).
 * Estos valores serán visibles y editables por el usuario */
void AERPScriptDialog::addFieldValue(const QString &dbFieldName, const QVariant &value)
{
    d->m_defaultValues[dbFieldName] = value;
}

void AERPScriptDialog::openScriptDialog(const QString &ui, const QString &qs)
{
    QHashIterator<QString, QObject *> itObjects(d->m_objects);
    QHashIterator<QString, QVariant> itData(d->m_data);
    if ( d->m_dialogScript )
    {
        delete d->m_dialogScript;
    }
    d->m_uiName = ui;
    d->m_qsName = qs;
    d->m_type = "script";
    d->m_dialogScript = new ScriptDlg(d->m_uiName, d->m_qsName, d->m_parentWidget);
    if ( d->m_staysOnTop )
    {
        d->m_dialogScript->setWindowFlags(d->m_dialogScript->windowFlags() | Qt::WindowStaysOnTopHint);
    }
    if ( d->m_dialogScript->openSuccess() )
    {
        d->m_dialogScript->setModal(d->m_modal);
        while ( itObjects.hasNext() )
        {
            itObjects.next();
            d->m_dialogScript->addPropertyToThisForm(itObjects.key(), itObjects.value());
        }
        while ( itData.hasNext() )
        {
            itData.next();
            d->m_dialogScript->addPropertyToThisForm(itData.key(), itData.value());
        }
        if ( d->m_modal )
        {
            d->m_dialogScript->exec();
        }
        else
        {
            d->m_dialogScript->show();
        }
    }
}
