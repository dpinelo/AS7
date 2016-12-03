/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo   *
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
#include "dbchooserelatedrecordbutton.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/aerptransactioncontext.h"
#include "business/aerploggeduser.h"
#include "forms/dbsearchdlg.h"
#include "forms/dbrecorddlg.h"
#include "globales.h"

class DBChooseRelatedRecordButtonPrivate
{
public:
    DBChooseRelatedRecordButton *q_ptr;
    BaseBeanPointer m_masterBean;
    BaseBeanPointerList m_oldMasterBeans;
    QPointer<RelatedElement> m_element;
    QStringList m_allowedMetadatas;
    QString m_category;
    QString m_scriptExecuteAfterChoose;
    QString m_scriptBeforeExecute;
    QString m_scriptAfterClear;
    QAction *m_actionClear;
    // Indica si el master bean se ha buscado por parentescos.
    bool m_masterBeanRetrievedFromParents;
    bool m_masterBeanChoosen;
    bool m_useNewContext;

    explicit DBChooseRelatedRecordButtonPrivate(DBChooseRelatedRecordButton *qq) : q_ptr(qq)
    {
        m_masterBeanRetrievedFromParents = false;
        m_masterBeanChoosen = false;
        m_actionClear = new QAction(q_ptr);
        m_actionClear->setText("Deseleccionar el registro seleccionado");
        m_actionClear->setIcon(QIcon(":/mime/mimeicons/empty.png"));
        m_useNewContext = true;
        QObject::connect(m_actionClear, SIGNAL(triggered()), q_ptr, SLOT(clear()));
    }
};

DBChooseRelatedRecordButton::DBChooseRelatedRecordButton(QWidget *parent) :
    QPushButton(parent), DBBaseWidget(), d(new DBChooseRelatedRecordButtonPrivate(this))
{
    connect(this, SIGNAL(clicked()), this, SLOT(chooseMasterBean()));
    setContextMenuPolicy(Qt::DefaultContextMenu);
}

DBChooseRelatedRecordButton::~DBChooseRelatedRecordButton()
{
    delete d;
}

BaseBeanPointer DBChooseRelatedRecordButton::masterBean()
{
    DBRecordDlg *dlgRecord = qobject_cast<DBRecordDlg *>(CommonsFunctions::aerpParentDialog(this));
    if ( d->m_masterBean.isNull() && dlgRecord != NULL && dlgRecord->bean() != NULL && !d->m_masterBeanRetrievedFromParents && !d->m_masterBeanChoosen )
    {
        if ( loadMasterBean() )
        {
            d->m_masterBeanRetrievedFromParents = true;
        }
    }
    return d->m_masterBean;
}

RelatedElement *DBChooseRelatedRecordButton::relatedElement() const
{
    return d->m_element;
}

QString DBChooseRelatedRecordButton::allowedMetadatas() const
{
    return d->m_allowedMetadatas.join(';');
}

void DBChooseRelatedRecordButton::setAllowedMetadatas(const QString &value)
{
    d->m_allowedMetadatas = value.split(QRegExp(QStringLiteral(";|,")));
}

QString DBChooseRelatedRecordButton::category() const
{
    return d->m_category;
}

void DBChooseRelatedRecordButton::setCategory(const QString &value)
{
    d->m_category = value;
}

QString DBChooseRelatedRecordButton::scriptExecuteAfterChoose() const
{
    return d->m_scriptExecuteAfterChoose;
}

void DBChooseRelatedRecordButton::setScriptExecuteAfterChoose(const QString &script)
{
    d->m_scriptExecuteAfterChoose = script;
}

QString DBChooseRelatedRecordButton::scriptBeforeExecute() const
{
    return d->m_scriptBeforeExecute;
}

void DBChooseRelatedRecordButton::setScriptBeforeExecute(const QString &value)
{
    d->m_scriptBeforeExecute = value;
}

QString DBChooseRelatedRecordButton::scriptExecuteAfterClear() const
{
    return d->m_scriptAfterClear;
}

void DBChooseRelatedRecordButton::setScriptExecuteAfterClear(const QString &script)
{
    d->m_scriptAfterClear = script;
}

bool DBChooseRelatedRecordButton::useNewContext() const
{
    return d->m_useNewContext;
}

void DBChooseRelatedRecordButton::setUseNewContext(bool value)
{
    d->m_useNewContext = value;
}

void DBChooseRelatedRecordButton::setValue(const QVariant &value)
{
    Q_UNUSED(value)
}

QVariant DBChooseRelatedRecordButton::value()
{
    return QVariant();
}

void DBChooseRelatedRecordButton::clear()
{
    if ( !d->m_masterBean.isNull() )
    {
        d->m_oldMasterBeans.append(d->m_masterBean);
    }
    else
    {
        return;
    }
    d->m_masterBean->deleteRelatedElement(d->m_element);
    d->m_masterBean = NULL;
    d->m_element = NULL;
    d->m_masterBeanChoosen = false;
    d->m_masterBeanRetrievedFromParents = false;
    emit masterBeanCleared();
    DBRecordDlg *dlgRecord = qobject_cast<DBRecordDlg *>(CommonsFunctions::aerpParentDialog(this));
    if ( dlgRecord != NULL )
    {
        if ( !d->m_scriptAfterClear.isEmpty() )
        {
            dlgRecord->callQSMethod(d->m_scriptAfterClear);
        }
        else
        {
            QString scriptName = QString("%1AfterClear").arg(objectName());
            dlgRecord->callQSMethod(scriptName);
        }
    }
}

void DBChooseRelatedRecordButton::chooseMasterBean()
{
    QStringList prettyAllowedMetadatas;

    DBRecordDlg *dlgRecord = qobject_cast<DBRecordDlg *>(CommonsFunctions::aerpParentDialog(this));
    if ( dlgRecord == NULL )
    {
        qDebug() << "DBChooseRelatedRecordButton::chooseMasterBean: No se puede utilizar este botón en un diálogo que no sea DBRecordDlg";
        return;
    }
    if ( dlgRecord->bean() == NULL )
    {
        qDebug() << "DBChooseRelatedRecordButton::chooseMasterBean: El bean del DBRecordDlg es nulo";
        return;
    }

    foreach (const QString &allowedMetadata, d->m_allowedMetadatas)
    {
        if ( AERPLoggedUser::instance()->checkMetadataAccess('r', allowedMetadata) &&
                AERPLoggedUser::instance()->checkMetadataAccess('w', allowedMetadata) )
        {
            BaseBeanMetadata *m = BeansFactory::instance()->metadataBean(allowedMetadata);
            if ( m == NULL )
            {
                qDebug() << "DBChooseRelatedRecordButton::chooseMasterBean: No existe [" << allowedMetadata << "]";
            }
            else
            {
                prettyAllowedMetadatas.append(m->alias());
            }
        }
    }
    if ( prettyAllowedMetadatas.isEmpty() )
    {
        return;
    }

    if ( !d->m_masterBean.isNull() )
    {
        int ret = QMessageBox::question(this, qApp->applicationName(),
                                        tr("Este registro se ha asociado anteriormente a un registro de la tabla %1. "
                                               "Si continúa, esta relación se eliminará. ¿Está seguro de quere continuar?").arg(d->m_masterBean->metadata()->alias()),
                                        QMessageBox::Yes | QMessageBox::No);
        if ( ret == QMessageBox::No )
        {
            return;
        }
        d->m_masterBean->deleteRelatedElement(d->m_element);
        d->m_oldMasterBeans.append(d->m_masterBean);
        d->m_masterBean = NULL;
        d->m_element = NULL;
        d->m_masterBeanChoosen = false;
        d->m_masterBeanRetrievedFromParents = false;
    }

    bool ok;
    QString selectedMetadata = QInputDialog::getItem(this, qApp->applicationName(),
                               tr("Seleccione la tabla en la que se encuentra el registro que desea asociar"),
                               prettyAllowedMetadatas, 0, false, &ok);
    if ( !ok )
    {
        return;
    }

    if ( d->m_scriptBeforeExecute.isEmpty() )
    {
        QString scriptName = QString("%1BeforeChoose").arg(objectName());
        dlgRecord->callQSMethod(scriptName);
    }
    else
    {
        dlgRecord->callMethod(d->m_scriptBeforeExecute);
    }

    QString metadataName = d->m_allowedMetadatas.at(prettyAllowedMetadatas.indexOf(selectedMetadata));
    QScopedPointer<DBSearchDlg> dlg (new DBSearchDlg(metadataName, d->m_useNewContext, this));
    dlg->setModal(true);
    if ( dlg->openSuccess() && dlg->init() )
    {
        dlg->exec();
        if ( dlg->userClickOk() )
        {
            // Recogemos el campo buscado si lo hay
            BaseBeanSharedPointer choosenBean = dlg->selectedBean();
            if ( !choosenBean.isNull() )
            {
                if ( choosenBean->metadata()->dbObjectType() == AlephERP::View )
                {
                    d->m_masterBean = BeansFactory::instance()->originalBean(choosenBean.data());
                }
                else
                {
                    // Al clonar con this como parent, evitaremos los memory leaks.
                    d->m_masterBean = choosenBean->clone(this);
                }
                if ( d->m_masterBean )
                {
                    d->m_element = d->m_masterBean->newRelatedElement(dlgRecord->bean(), QStringList(d->m_category), false);
                    d->m_masterBeanChoosen = true;
                    AERPTransactionContext::instance()->addToContext(dlgRecord->bean()->actualContext(), d->m_masterBean);
                    emit masterBeanChoosen();
                    if ( !d->m_scriptExecuteAfterChoose.isEmpty() )
                    {
                        dlgRecord->callQSMethod(d->m_scriptExecuteAfterChoose);
                    }
                    else
                    {
                        QString scriptName = QString("%1AfterChoose").arg(objectName());
                        dlgRecord->callQSMethod(scriptName, d->m_masterBean);
                    }
                }
            }
        }
    }
}

void DBChooseRelatedRecordButton::applyFieldProperties()
{
    setEnabled(m_dataEditable);
    if ( !isEnabled() && qApp->focusWidget() == this )
    {
        focusNextChild();
    }
}

void DBChooseRelatedRecordButton::refresh()
{

}

void DBChooseRelatedRecordButton::showEvent(QShowEvent *)
{
    if ( this->icon().isNull() )
    {
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/generales/images/edit_search.png"), QSize(), QIcon::Normal, QIcon::Off);
        this->setIcon(icon);
    }
}

void DBChooseRelatedRecordButton::contextMenuEvent(QContextMenuEvent *event)
{
    // Para QAbstractScrollArea y clases derivadas debemos utilizar:
    QPoint globalPos = event->globalPos();
    QMenu contextMenu;
    contextMenu.addAction(d->m_actionClear);
    contextMenu.exec(globalPos);
}

bool DBChooseRelatedRecordButton::loadMasterBean()
{
    DBRecordDlg *dlgRecord = qobject_cast<DBRecordDlg *>(CommonsFunctions::aerpParentDialog(this));
    if ( dlgRecord == NULL )
    {
        qDebug() << "DBChooseRelatedRecordButton::loadMasterBean: No se puede utilizar este botón en un diálogo que no sea DBRecordDlg";
        return false;
    }
    if ( dlgRecord->bean() == NULL )
    {
        qDebug() << "DBChooseRelatedRecordButton::loadMasterBean: El bean del DBRecordDlg es nulo";
        return false;
    }

    if ( !d->m_masterBean.isNull() )
    {
        d->m_masterBean->deleteRelatedElement(d->m_element);
        d->m_oldMasterBeans.append(d->m_masterBean);
        d->m_masterBean = NULL;
        d->m_element = NULL;
        d->m_masterBeanChoosen = false;
        d->m_masterBeanRetrievedFromParents = false;
    }

    BaseBean *recordBean = dlgRecord->bean();
    RelatedElementPointerList list = recordBean->getRelatedElementsByCategory(d->m_category, AlephERP::Record, AlephERP::PointToMaster, false);
    if ( list.size() > 0 )
    {
        d->m_masterBean = list.at(0)->relatedBean();
        if ( !d->m_masterBean.isNull() )
        {
            RelatedElementPointerList list = d->m_masterBean->getRelatedElementsByCategoryAndRelatedTableName(recordBean->metadata()->tableName(), d->m_category);
            foreach (RelatedElementPointer elem, list)
            {
                if ( !elem->relatedBean().isNull() && elem->relatedDbOid() == recordBean->dbOid() )
                {
                    d->m_element = elem;
                    return true;
                }
            }
            d->m_masterBean = NULL;
            d->m_element = NULL;
        }
    }
    return false;
}
