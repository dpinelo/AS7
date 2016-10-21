/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo                                    *
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
#include "relatedelementswidget.h"
#include "globales.h"
#include "ui_relatedelementswidget.h"
#include "models/relatedelementsmodel.h"
#include "models/relatedtreeitem.h"
#include "dao/beans/relatedelement.h"
#include "forms/dbrecorddlg.h"
#include "forms/dbformdlg.h"
#ifdef ALEPHERP_SMTP_SUPPORT
#include "business/smtp/emailobject.h"
#endif
#ifdef ALEPHERP_DOC_MANAGEMENT
#include "business/docmngmnt/aerpdocumentdaowrapper.h"
#include "dao/beans/aerpdocmngmntdocument.h"
#endif

class RelatedElementsWidgetPrivate
{
public:
    QPointer<BaseBean> m_bean;
    QPointer<RelatedElementsModel> m_model;
    QAction *m_openAttachment;
    QAction *m_saveAttachment;
    QAction *m_openRecord;
    QAction *m_openEmail;
    QAction *m_openDocument;
    QAction *m_saveDocument;
    RelatedTreeItem *m_treeMenuItemSelected;
    QPointer<DBFormDlg> m_dbFormDlg;

    RelatedElementsWidgetPrivate()
    {
        m_openAttachment = NULL;
        m_saveAttachment = NULL;
        m_openRecord = NULL;
        m_openEmail = NULL;
        m_treeMenuItemSelected = NULL;
        m_openDocument = NULL;
        m_saveDocument = NULL;
    }
};

RelatedElementsWidget::RelatedElementsWidget(BaseBean *bean, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RelatedElementsWidget), d(new RelatedElementsWidgetPrivate)
{
    ui->setupUi(this);
    d->m_bean = bean;
    d->m_dbFormDlg = qobject_cast<DBFormDlg *>(parent);
    if ( d->m_bean != NULL )
    {
        d->m_model = QPointer<RelatedElementsModel>(new RelatedElementsModel(d->m_bean.data(), this));

        ui->treeView->setModel(d->m_model.data());
    }
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    d->m_openAttachment = new QAction(QIcon(":/generales/images/scripts.png"), trUtf8("Abrir adjunto"), this);
    d->m_saveAttachment = new QAction(QIcon(":/generales/images/save.png"), trUtf8("Guardar como..."), this);
    d->m_openRecord = new QAction(QIcon(":/generales/images/edit_edit.png"), trUtf8("Abrir registro"), this);
    d->m_openEmail = new QAction(QIcon(":/generales/images/email.png"), trUtf8("Abrir correo"), this);
    d->m_openDocument = new QAction(QIcon(":/generales/images/edit_search.png"), trUtf8("Abrir documento"), this);
    d->m_saveDocument = new QAction(QIcon(":/generales/images/save.png"), trUtf8("Guardar como..."), this);

    connect(ui->pbOk, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(ui->treeView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

    connect(d->m_openAttachment, SIGNAL(triggered()), this, SLOT(openAttachment()));
    connect(d->m_saveAttachment, SIGNAL(triggered()), this, SLOT(saveAttachment()));
    connect(d->m_openRecord, SIGNAL(triggered()), this, SLOT(openRecord()));
    connect(d->m_openEmail, SIGNAL(triggered()), this, SLOT(openEmail()));
    connect(d->m_openDocument, SIGNAL(triggered()), this, SLOT(openDocument()));
    connect(d->m_saveDocument, SIGNAL(triggered()), this, SLOT(saveDocument()));
}

RelatedElementsWidget::~RelatedElementsWidget()
{
    delete ui;
    delete d;
}

void RelatedElementsWidget::clear()
{
    if ( d->m_model )
    {
        delete d->m_model;
    }
    ui->treeView->setModel(NULL);
    if ( d->m_bean )
    {
        disconnect (d->m_bean.data(), SIGNAL(destroyed()), this, SLOT(refresh()));
    }
    d->m_bean = NULL;
}

void RelatedElementsWidget::setBean(BaseBean *bean)
{
    if ( d->m_bean == bean )
    {
        return;
    }
    if ( !d->m_model.isNull() )
    {
        delete d->m_model;
    }
    if ( bean == NULL )
    {
        ui->treeView->setModel(NULL);
    }
    else
    {
        ui->treeView->setModel(NULL);
        d->m_bean = bean;
        d->m_model = QPointer<RelatedElementsModel>(new RelatedElementsModel(bean, this));
        ui->treeView->setModel(d->m_model.data());
        connect (d->m_bean.data(), SIGNAL(destroyed()), this, SLOT(clear()));
    }
}

void RelatedElementsWidget::setOkButtonVisible(bool visible)
{
    ui->pbOk->setVisible(visible);
}

void RelatedElementsWidget::refresh()
{
    setBean(d->m_bean);
}

void RelatedElementsWidget::okClicked()
{
    emit ok();
}

void RelatedElementsWidget::showContextMenu(const QPoint &point)
{
    QModelIndex idx = ui->treeView->indexAt(point);
    if ( idx.isValid() )
    {
        d->m_treeMenuItemSelected = static_cast<RelatedTreeItem*>(idx.internalPointer());
        if ( d->m_treeMenuItemSelected == NULL )
        {
            return;
        }
        // Para QAbstractScrollArea y clases derivadas debemos utilizar:
        QPoint globalPos = ui->treeView->viewport()->mapToGlobal(point);
        QMenu contextMenu;
        if ( d->m_treeMenuItemSelected->type() == RelatedTreeItem::Record )
        {
            contextMenu.addAction(d->m_openRecord);
        }
        else if ( d->m_treeMenuItemSelected->type() == RelatedTreeItem::Email )
        {
            contextMenu.addAction(d->m_openEmail);
        }
        else if ( d->m_treeMenuItemSelected->type() == RelatedTreeItem::EmailAttachment )
        {
            contextMenu.addAction(d->m_openAttachment);
            contextMenu.addAction(d->m_saveAttachment);
        }
        else if ( d->m_treeMenuItemSelected->type() == RelatedTreeItem::Document )
        {
            contextMenu.addAction(d->m_openDocument);
            contextMenu.addAction(d->m_saveDocument);
        }
        else if ( d->m_treeMenuItemSelected->type() == RelatedTreeItem::DBRelationChildRecord )
        {
            contextMenu.addAction(d->m_openRecord);
        }
        contextMenu.exec(globalPos);
    }
}

void RelatedElementsWidget::openAttachment()
{
#ifdef ALEPHERP_SMTP_SUPPORT
    if ( d->m_treeMenuItemSelected == NULL )
    {
        return;
    }
    if ( d->m_treeMenuItemSelected->relatedElement() != NULL )
    {
        return;
    }
    int attachmentCount = d->m_treeMenuItemSelected->attachmentRow();
    if ( attachmentCount > -1 && attachmentCount < d->m_treeMenuItemSelected->relatedElement()->email().attachments().size() )
    {
        EmailAttachment attach = d->m_treeMenuItemSelected->relatedElement()->email().attachments().at(attachmentCount);
        if ( EmailDAO::loadAttachmentData(&attach) )
        {
            QUrl url = QUrl::fromLocalFile(attach.path());
            QDesktopServices::openUrl(url);
        }
    }
#endif
}

void RelatedElementsWidget::saveAttachment()
{
    QMessageBox::information(this, qApp->applicationName(), trUtf8("Probando"));
}

void RelatedElementsWidget::openRecord()
{
    BaseBeanPointer bean;
    if ( d->m_treeMenuItemSelected->type() == RelatedTreeItem::Record )
    {
        if ( d->m_treeMenuItemSelected == NULL && d->m_treeMenuItemSelected->relatedElement() != NULL )
        {
            return;
        }
        RelatedElement *element = d->m_treeMenuItemSelected->relatedElement();
        if ( element == NULL )
        {
            return;
        }
        bean = element->relatedBean();
        if ( !element->found() )
        {
            QMessageBox::warning(this, qApp->applicationName(), trUtf8("El registro fue borrado."), QMessageBox::Ok);
            return;
        }
    }
    else if ( d->m_treeMenuItemSelected->type() == RelatedTreeItem::DBRelationChildRecord )
    {
        bean = d->m_treeMenuItemSelected->bean();
    }
    if ( !bean.isNull() )
    {
        QScopedPointer<DBRecordDlg> dlg (new DBRecordDlg(bean.data(), AlephERP::ReadOnly, false, this));
        if ( dlg->openSuccess() && dlg->init() )
        {
            // Guardar los datos de los hijos agregados, será responsabilidad del bean padre
            // que se está editando
            dlg->setModal(true);
            dlg->exec();
        }
    }
}

void RelatedElementsWidget::openEmail()
{
    QMessageBox::information(this, qApp->applicationName(), trUtf8("Probando"));
}

void RelatedElementsWidget::openDocument()
{
#ifdef ALEPHERP_DOC_MANAGEMENT
    if ( d->m_treeMenuItemSelected == NULL )
    {
        return;
    }
    if ( d->m_treeMenuItemSelected->relatedElement() != NULL )
    {
        return;
    }
    AERPDocMngmntDocument *doc = d->m_treeMenuItemSelected->relatedElement()->document();
    if ( !d->m_treeMenuItemSelected->relatedElement()->found() )
    {
        QMessageBox::warning(this, qApp->applicationName(), trUtf8("El documento asociado no ha podido ser encontrado. Es probable que haya sido borrado."), QMessageBox::Ok);
        return;
    }
    if ( doc == NULL )
    {
        QMessageBox::warning(this, qApp->applicationName(), trUtf8("El documento asociado no ha podido ser cargado."), QMessageBox::Ok);
        return;
    }
    AERPDocumentVersion *version = doc->version(doc->lastVersion());
    if ( version != NULL )
    {
        QString tempPath = QDir::fromNativeSeparators(QDir::tempPath()) + "/" + doc->name();
        if ( !AERPDocumentDAOWrapper::instance()->fetchVersion(version, tempPath) )
        {
            QMessageBox::warning(this, qApp->applicationName(), trUtf8("No se ha podido obtener el documento. ERROR: %1").arg(AERPDocumentDAOWrapper::instance()->lastMessage()));
            return;
        }
        else
        {
            QUrl url = QUrl::fromLocalFile(tempPath);
            if ( !QDesktopServices::openUrl(url) )
            {
                QMessageBox::warning(this, qApp->applicationName(), trUtf8("No se ha podido abrir el archivo. Se encuentra en: %1").arg(tempPath));
            }
        }
    }
#endif
}

void RelatedElementsWidget::saveDocument()
{
#ifdef ALEPHERP_DOC_MANAGEMENT
    if ( d->m_treeMenuItemSelected == NULL )
    {
        return;
    }
    if ( d->m_treeMenuItemSelected->relatedElement() != NULL )
    {
        return;
    }
    AERPDocMngmntDocument *doc = d->m_treeMenuItemSelected->relatedElement()->document();
    if ( doc == NULL )
    {
        QMessageBox::warning(this, qApp->applicationName(), trUtf8("El documento asociado no ha podido ser cargado."), QMessageBox::Ok);
        return;
    }
    AERPDocumentVersion *version = doc->version(doc->lastVersion());
    if ( version != NULL )
    {
        QString dirPath = QFileDialog::getExistingDirectory(this, trUtf8("Seleccione ruta..."), QDir::homePath());
        if ( dirPath.isEmpty() )
        {
            return;
        }
        QString tempPath = QDir::fromNativeSeparators(dirPath) + "/" + doc->name();
        if ( !AERPDocumentDAOWrapper::instance()->fetchVersion(version, tempPath) )
        {
            QMessageBox::warning(this, qApp->applicationName(), trUtf8("No se ha podido obtener el documento. ERROR: %1").arg(AERPDocumentDAOWrapper::instance()->lastMessage()));
            return;
        }
        else
        {
            int ret = QMessageBox::question(this, qApp->applicationName(), trUtf8("Se ha obtenido el fichero adecuadamente. ¿Desea abrir el directorio que lo contiene?"), QMessageBox::Yes | QMessageBox::No);
            if ( ret == QMessageBox::Yes )
            {
                QUrl url = QUrl::fromLocalFile(dirPath);
                QDesktopServices::openUrl(url);
            }
        }
    }
#endif
}
