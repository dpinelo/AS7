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
#include <QTextDocument>
#include "sendemaildlg.h"
#include "ui_sendemail.h"
#include "configuracion.h"
#include "business/smtp/smtpobject.h"
#include "business/smtp/aerpsmtpinterface.h"
#include "business/aerploggeduser.h"
#include "globales.h"
#include "dao/database.h"
#include "dao/beans/basebean.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/userdao.h"
#include "models/envvars.h"
#include "models/filterbasebeanmodel.h"
#include "models/dbbasebeanmodel.h"
#include "models/perpquerymodel.h"
#include "scripts/perpscript.h"
#include "widgets/dbtableview.h"

#define QXT_MUST_QP(x) (x < char(32) || x > char(126) || x == '=' || x == '?')

class SendEmailDlgPrivate
{
public:
    SendEmailDlg *q_ptr;
    EmailObject m_email;
    BaseBeanPointer m_bean;
    AERPSMTPIface *m_iface;
    SMTPObject *m_smtp;
    QProgressDialog *m_dlg;
    bool m_wasSent;
    /** Motor de script para las funciones */
    AERPScriptQsObject m_engine;
    QString m_contactModel;
    QString m_contactModelDisplayField;
    QString m_contactModelEmailField;
    QString m_contactModelFilter;
    QList<QPair<QString, QString> > m_fileAttachments;
    QWidget *m_contactWidget;
    QSize m_contactWidgetSize;
    DBTableView *m_tableView;
    QPointer<DBBaseBeanModel> m_model;
    QPointer<FilterBaseBeanModel> m_filterModel;
    QPropertyAnimation *m_animation;
    QPointer<QCompleter> m_completerTo;
    QPointer<QCompleter> m_completerCopy;
    QPointer<AERPQueryModel> m_completerModel;
    QAction *m_addAttachment;
    QAction *m_openAttachment;
    QAction *m_saveAttachment;
    QAction *m_deleteAttachment;
    bool m_openSuccess;

    SendEmailDlgPrivate(SendEmailDlg *qq) : q_ptr(qq)
    {
        m_wasSent = false;
        m_contactWidget = NULL;
        m_tableView = NULL;
        m_animation = NULL;
        m_smtp = NULL;
        m_iface = NULL;
        m_openSuccess = false;
        m_dlg = NULL;
        m_addAttachment = NULL;
        m_openAttachment = NULL;
        m_saveAttachment = NULL;
        m_deleteAttachment = NULL;
    }

    void createContactListWidget();
    void createContactModels();
    void initCompleter();
};

SendEmailDlg::SendEmailDlg(BaseBeanPointer bean, QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl), ui(new Ui::SendEmailDlg), d(new SendEmailDlgPrivate(this))
{
    ui->setupUi(this);
    d->m_bean = bean;
    d->m_contactModel = bean->metadata()->emailContactModel();
    d->m_contactModelDisplayField = bean->metadata()->emailContactDisplayField();
    d->m_contactModelEmailField = bean->metadata()->emailContactAddressField();
    d->m_contactModelFilter = bean->metadata()->emailContactModelFilter();

    // Veamos primero si se puede cargar el interfaz de correos
    QString messageError;
    d->m_iface = CommonsFunctions::loadSMTPPlugin(EnvVars::instance()->var("smtpPlugin").toString(), messageError);
    if ( d->m_iface == NULL )
    {
        QMessageBox::warning(this, qApp->applicationName(),
                             trUtf8("No se ha podido cargar el plugin para el envío de correos. Error: %1").arg(messageError), QMessageBox::Ok);
        close();
        return;
    }
    d->m_smtp = d->m_iface->smtpObject(this);

    // Modelos de completado de la dirección de correos.
    if ( d->m_contactModel.isEmpty() )
    {
        ui->pbContacts->setVisible(false);
        ui->pbCopyContacts->setVisible(false);
    }
    else
    {
        d->createContactModels();
    }

    d->m_addAttachment = new QAction(ui->pbAddAttachment->icon(), ui->pbAddAttachment->text(), this);
    d->m_openAttachment = new QAction(QIcon(":/generales/images/edit_search.png"), trUtf8("Abrir adjunto"), this);
    d->m_saveAttachment = new QAction(QIcon(":/generaleS/images/save.png"), trUtf8("Guardar como..."), this);
    d->m_deleteAttachment = new QAction(ui->pbRemoveAttachment->icon(), ui->pbRemoveAttachment->text(), this);

    connect(ui->pbSend, SIGNAL(clicked()), this, SLOT(send()));
    connect(ui->pbClose, SIGNAL(clicked()), this, SLOT(askToClose()));
    connect(ui->pbContacts, SIGNAL(clicked()), this, SLOT(showOrHideContactFilter()));
    connect(ui->pbAddAttachment, SIGNAL(clicked()), this, SLOT(addAttachmentClick()));
    connect(ui->pbRemoveAttachment, SIGNAL(clicked()), this, SLOT(removeAttachmentClick()));
    connect(ui->tableWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
    connect(d->m_addAttachment, SIGNAL(triggered()), this, SLOT(addAttachmentClick()));
    connect(d->m_deleteAttachment, SIGNAL(triggered()), this, SLOT(removeAttachmentClick()));
    connect(d->m_openAttachment, SIGNAL(triggered()), this, SLOT(openAttachment()));
    connect(d->m_saveAttachment, SIGNAL(triggered()), this, SLOT(saveAttachment()));

    // Cargamos las dimensiones guardadas de la ventana
    alephERPSettings->applyDimensionForm(this);

    // Asunto del correo electrónico.
    ui->txtSubject->setText(bean->emailSubject());

    // Contenido del correo electrónico. La firma.
    QString content = bean->emailTemplate();
    if ( Qt::mightBeRichText(content) )
    {
        content += "<br/>" + EnvVars::instance()->var(AlephERP::stEmailUserSignature).toString();
    }
    else
    {
        if ( Qt::mightBeRichText(EnvVars::instance()->var(AlephERP::stEmailUserSignature).toString()) )
        {
            content += "<br/>" + EnvVars::instance()->var(AlephERP::stEmailUserSignature).toString();
        }
        else
        {
            content += "\n" + EnvVars::instance()->var(AlephERP::stEmailUserSignature).toString();
        }
    }
    if ( Qt::mightBeRichText(content) )
    {
        ui->htmlEditor->setHtml(content);
    }
    else
    {
        ui->htmlEditor->setPlainText(content);
    }

    QString from;
    if ( !AERPLoggedUser::instance()->userName().isEmpty() && !AERPLoggedUser::instance()->email().isEmpty() )
    {
        from = QString("%1 <%2>").arg(AERPLoggedUser::instance()->name()).arg(AERPLoggedUser::instance()->email());
    }
    else if ( !AERPLoggedUser::instance()->email().isEmpty() )
    {
        from = AERPLoggedUser::instance()->email();
    }
    if ( !from.isEmpty() )
    {
        ui->txtFrom->setText(from);
    }

    // Completado de las direcciones
    d->initCompleter();
    execQs();
    d->m_openSuccess = true;
}

SendEmailDlg::~SendEmailDlg()
{
    delete ui;
    delete d;
}

bool SendEmailDlg::openSuccess()
{
    return d->m_openSuccess;
}

bool SendEmailDlg::wasSent()
{
    return d->m_wasSent;
}

/**
 * @brief SendEmailDlg::sentEmail
 * Devuelve un XML formado con el contenido del mail enviado
 * @return
 */
EmailObject SendEmailDlg::sentEmail()
{
    return d->m_email;
}

void SendEmailDlg::closeEvent (QCloseEvent * event)
{
    alephERPSettings->saveDimensionForm(this);
    event->accept();
}

void SendEmailDlg::askToClose()
{
    if ( !d->m_wasSent )
    {
        int ret = QMessageBox::question(this, qApp->applicationName(), trUtf8("No ha enviado el correo electrónico. ¿Está seguro de querer salir?"), QMessageBox::Yes | QMessageBox::No);
        if ( ret == QMessageBox::Yes )
        {
            close();
        }
    }
    else
    {
        close();
    }
}

void SendEmailDlg::filterContactList(const QString &value)
{
    if ( d->m_filterModel != NULL )
    {
        d->m_filterModel->setFilterKeyColumn(d->m_contactModelDisplayField, value);
    }
}

void SendEmailDlg::showContextMenu(const QPoint &point)
{
    QTableWidgetItem *item = ui->tableWidget->itemAt(point);
    QMenu contextMenu;
    QPoint globalPos = ui->tableWidget->viewport()->mapToGlobal(point);
    if ( item == NULL )
    {
        // Para QAbstractScrollArea y clases derivadas debemos utilizar:
        QMenu contextMenu;
        contextMenu.addAction(d->m_addAttachment);
    }
    else
    {
        ui->tableWidget->setCurrentItem(item);
        contextMenu.addAction(d->m_openAttachment);
        contextMenu.addAction(d->m_saveAttachment);
        contextMenu.addSeparator();
        contextMenu.addAction(d->m_deleteAttachment);
    }
    contextMenu.exec(globalPos);
}

/*!
  Vamos a obtener y guardar cuándo el usuario ha modificado un control
  */
bool SendEmailDlg::eventFilter (QObject *target, QEvent *event)
{
    return QDialog::eventFilter(target, event);
}

void SendEmailDlg::send()
{
    d->m_dlg = new QProgressDialog(this);
    d->m_dlg->setAttribute(Qt::WA_DeleteOnClose);
    d->m_dlg->setLabelText(trUtf8("Envío de correo electrónico"));
    d->m_dlg->setWindowTitle(qApp->applicationName());

    connect(d->m_smtp, SIGNAL(init(int)), d->m_dlg, SLOT(setMaximum(int)));
    connect(d->m_smtp, SIGNAL(progress(int)), d->m_dlg, SLOT(setValue(int)));
    connect(d->m_smtp, SIGNAL(finished()), d->m_dlg, SLOT(close()));

    d->m_dlg->show();
    CommonsFunctions::processEvents();

    d->m_smtp->setFrom(ui->txtFrom->text());
    d->m_smtp->setCopy(ui->txtCopy->text().split(QRegExp("[;,]")));
    d->m_smtp->setTo(ui->txtTo->text().split(QRegExp("[;,]")));
    d->m_smtp->setSubject(ui->txtSubject->text());

    d->m_email.setFrom(ui->txtFrom->text());
    d->m_email.setCopy(ui->txtCopy->text().split(QRegExp("[;,]")));
    d->m_email.setTo(ui->txtTo->text().split(QRegExp("[;,]")));
    d->m_email.setSubject(ui->txtSubject->text());
    d->m_email.setBody(ui->htmlEditor->toHtml());

    // Para el tema de las tildes, lo mejor es convertir a Unicode...
    QString htmlBody = ui->htmlEditor->toHtml().toLatin1();
    d->m_smtp->setBody(htmlBody);
    d->m_smtp->setExtraHeaders("Content-Type", "text/html");

    d->m_smtp->setServerAddress(EnvVars::instance()->var(AlephERP::stEmailServerAddress).toString());
    d->m_smtp->setPort(EnvVars::instance()->var(AlephERP::stEmailServerPort).toInt());
    d->m_smtp->setSmtpUserName(EnvVars::instance()->var(AlephERP::stEmailUsername).toString());
    d->m_smtp->setSmtpPassword(EnvVars::instance()->var(AlephERP::stEmailPassword).toString());

    for (int i = 0 ; i < d->m_fileAttachments.size() ; i++)
    {
        QPair<QString, QString> pair = d->m_fileAttachments.at(i);
        d->m_smtp->addAttachment(pair.first, pair.second);
        QFileInfo fi(pair.first);
        EmailAttachment attachment;
        attachment.setEmailFileName(fi.fileName());
        attachment.setPath(pair.first);
        attachment.setType(pair.second);
        d->m_email.addAttachment(attachment);
    }
    d->m_email.setServer(EnvVars::instance()->var(AlephERP::stEmailServerAddress).toString());
    d->m_email.setPort(EnvVars::instance()->var(AlephERP::stEmailServerPort).toInt());
    d->m_email.setSmtpUsername(EnvVars::instance()->var(AlephERP::stEmailUsername).toString());

    if ( !d->m_smtp->send() )
    {
        QMessageBox::warning(this, qApp->applicationName(), trUtf8("Ha ocurrido un error enviando el correo electrónico. El error fue: %1").arg(d->m_smtp->errorMessage()), QMessageBox::Ok);
        d->m_wasSent = false;
    }
    else
    {
        d->m_wasSent = true;
        close();
    }
}

QString SendEmailDlg::contactModel() const
{
    return d->m_contactModel;
}

QString SendEmailDlg::contactModelDisplayField() const
{
    return d->m_contactModelDisplayField;
}

QString SendEmailDlg::contactModelEmailField() const
{
    return d->m_contactModelEmailField;
}

void SendEmailDlg::setContactModel(const QString &tableName)
{
    d->m_contactModel = tableName;
    if ( !d->m_contactModel.isEmpty() )
    {
        ui->pbContacts->setVisible(true);
        ui->pbContacts->setVisible(true);
        d->createContactModels();
    }
    else
    {
        ui->pbContacts->setVisible(false);
        ui->pbContacts->setVisible(false);
        if ( !d->m_model.isNull() )
        {
            delete d->m_model;
        }
        if ( !d->m_filterModel.isNull() )
        {
            delete d->m_filterModel;
        }
    }
}

void SendEmailDlg::setContactModelDisplayField(const QString &fieldName)
{
    d->m_contactModelDisplayField = fieldName;
}

void SendEmailDlg::setContactModelEmailField(const QString &fieldName)
{
    d->m_contactModelEmailField = fieldName;
}

void SendEmailDlg::setContactModelFilter(const QString &value)
{
    d->m_contactModelFilter = value;
    if ( !d->m_model.isNull() )
    {
        d->m_model->setWhere(d->m_contactModelFilter, "");
    }
}

void SendEmailDlg::addAttachment(const QString &absoluteFilePath, const QString &type)
{
    QPair<QString, QString> pair;
    pair.first = absoluteFilePath;
    pair.second = type;
    d->m_fileAttachments.append(pair);

    QFileInfo fi(absoluteFilePath);
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);
    QTableWidgetItem *item = new QTableWidgetItem(fi.fileName());
    ui->tableWidget->setItem(row, 0, item);
    item = new QTableWidgetItem(type);
    item->setIcon(CommonsFunctions::iconFromMimeType(type));
    ui->tableWidget->setItem(row, 1, item);
}

/*!
  Este formulario puede contener cierto código script a ejecutar en su inicio. Esta función lo lanza
  inmediatamente. El código script está en presupuestos_system, con el nombre de la tabla principal
  acabado en dbform.qs
  */
void SendEmailDlg::execQs()
{
    QString qsName = QString ("sendmail.form.qs");

    /** Ejecutamos el script asociado. La filosofía fundamental de ese script es proporcionar
      algo de código básico que justifique este formulario de edición de registros */
    if ( !BeansFactory::systemScripts.contains(qsName) )
    {
        return;
    }
    d->m_engine.setScript(BeansFactory::systemScripts.value(qsName), qsName);
    d->m_engine.setDebug(BeansFactory::systemScriptsDebug.value(qsName));
    d->m_engine.setOnInitDebug(BeansFactory::systemScriptsDebugOnInit.value(qsName));
    d->m_engine.setScriptObjectName("SendEmailDlg");
    d->m_engine.setUi(this);
    d->m_engine.setThisFormObject(this);
    if ( !d->m_bean.isNull() )
    {
        d->m_engine.addToEnviroment("bean", d->m_bean.data());
    }
    bool executeOk = false;
    while ( !executeOk )
    {
        CommonsFunctions::setOverrideCursor(QCursor(Qt::ArrowCursor));
        executeOk = d->m_engine.createQsObject();
        CommonsFunctions::restoreOverrideCursor();
        if ( !executeOk )
        {
            CommonsFunctions::setOverrideCursor(QCursor(Qt::ArrowCursor));
            QMessageBox::warning(this,qApp->applicationName(), trUtf8("Ha ocurrido un error al cargar el script asociado a este "
                                 "formulario. Es posible que algunas funciones no estén disponibles."),
                                 QMessageBox::Ok);
#ifdef ALEPHERP_DEVTOOLS
            int ret = QMessageBox::information(this,qApp->applicationName(), trUtf8("El script ejecutado contiene errores. ¿Desea editarlo?"),
                                               QMessageBox::Yes | QMessageBox::No);
            if ( ret == QMessageBox::Yes )
            {
                if ( !d->m_engine.editScript(this) )
                {
                    executeOk = true;
                }
            }
            else
            {
                executeOk = true;
            }
#else
            executeOk = true;
#endif
            CommonsFunctions::restoreOverrideCursor();
        }
    }
}

QString SendEmailDlg::contactModelFilter() const
{
    return d->m_contactModelFilter;
}

/**
  Crea el widget que permitirá el filtrado a niveles del árbol
  */
void SendEmailDlgPrivate::createContactListWidget()
{
    if ( m_contactWidget == NULL )
    {
        QFrame *frame = new QFrame(q_ptr);
        frame->setAutoFillBackground(true);
        frame->setFrameStyle(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Raised);
        frame->setLineWidth(3);
        // Le añadimos una sombra molona.
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(q_ptr);
        shadow->setBlurRadius(2);
        frame->setGraphicsEffect(shadow);
        m_contactWidget = frame;
        QGridLayout *layout = new QGridLayout();

        QLabel *filterLabel = new QLabel(QObject::trUtf8("Nombre del contacto"), frame);
        QLineEdit *filterEdit = new QLineEdit(frame);
        filterEdit->resize(200, filterEdit->height());
        filterEdit->setObjectName(QString("filterContactName"));
        QPushButton *pbOk = new QPushButton(QIcon(":/aplicacion/images/ok.png"), QObject::trUtf8("Ok"), frame);

        m_tableView = new DBTableView(frame);

        layout->addWidget(filterLabel, 0, 0);
        layout->addWidget(filterEdit, 0, 1);
        layout->addWidget(m_tableView, 1, 0, 1, 2);
        layout->addWidget(pbOk, 2, 1, 1, 2);

        QObject::connect(filterEdit, SIGNAL(textChanged(QString)), q_ptr, SLOT(filterContactList(QString)));
        frame->setLayout(layout);
        m_contactWidget->show();
        m_contactWidgetSize = QSize(q_ptr->ui->lblTo->frameGeometry().width() + q_ptr->ui->txtTo->frameGeometry().width()
                                    + q_ptr->ui->pbContacts->frameGeometry().width(), q_ptr->height() * 0.6);
        m_contactWidget->resize(QSize(0, 0));
    }
}

void SendEmailDlgPrivate::createContactModels()
{
    if ( !m_model.isNull() )
    {
        delete m_model;
    }
    if ( !m_filterModel.isNull() )
    {
        delete m_filterModel;
    }
    m_model = QPointer<DBBaseBeanModel> (new DBBaseBeanModel(m_contactModel, m_contactModelFilter, m_contactModelDisplayField + " ASC", true, true, false, q_ptr));
    m_filterModel = QPointer<FilterBaseBeanModel> (new FilterBaseBeanModel(q_ptr));
    m_filterModel->setAccessFilter('s');
    m_filterModel->setSourceModel(m_model);
}

void SendEmailDlg::showOrHideContactFilter()
{
    if ( d->m_contactWidget == NULL )
    {
        d->createContactListWidget();
    }
    if ( d->m_animation == NULL )
    {
        d->m_animation = new QPropertyAnimation(d->m_contactWidget, "geometry", this);
    }
    QPushButton *pb = qobject_cast<QPushButton *>(sender());
    QWidget *wid = findChild<QWidget *>("filterContactName");
    if ( pb == NULL || wid == NULL )
    {
        return;
    }
    QPoint pos = ui->lblTo->geometry().topLeft() + QPoint(5, pb->frameGeometry().height()+10);
    QRect startValue(pos, QSize(0, 0));
    QRect stopValue(pos, d->m_contactWidgetSize);
    if ( pb->isChecked() )
    {
        d->m_animation->setStartValue(startValue);
        d->m_animation->setEndValue(stopValue);
        wid->setFocus();
    }
    else
    {
        d->m_animation->setStartValue(d->m_contactWidget->geometry());
        d->m_animation->setEndValue(startValue);
    }
    d->m_animation->setDuration(125);
    d->m_animation->start(QAbstractAnimation::KeepWhenStopped);
}

void SendEmailDlg::addAttachmentClick()
{
    QStringList files = QFileDialog::getOpenFileNames(this, trUtf8("Seleccione los ficheros adjuntos"));
    if ( files.isEmpty() )
    {
        return;
    }
    foreach (const QString & file, files)
    {
        QFileInfo fi (file);
        QString type;
        if ( fi.exists() )
        {
            type = CommonsFunctions::mimeType(fi.absoluteFilePath());
        }
        addAttachment(fi.absoluteFilePath(), type);
    }
}

void SendEmailDlg::removeAttachmentClick()
{
    QList<QTableWidgetItem *> selected = ui->tableWidget->selectedItems();
    foreach (QTableWidgetItem *item, selected)
    {
        if ( AERP_CHECK_INDEX_OK(item->row(), d->m_fileAttachments) )
        {
            d->m_fileAttachments.removeAt(item->row());
        }
    }
    QList<int> rowsToDelete;
    foreach (QTableWidgetItem *item, selected)
    {
        if ( !rowsToDelete.contains(item->row()) )
        {
            rowsToDelete.append(item->row());
        }
    }
    foreach (int row, rowsToDelete)
    {
        ui->tableWidget->removeRow(row);
    }
}

void SendEmailDlg::openAttachment()
{
    QList<QTableWidgetItem *> selected = ui->tableWidget->selectedItems();
    QList<int> openedRows;
    foreach (QTableWidgetItem *item, selected)
    {
        int row = item->row();
        if ( !openedRows.contains(row) && AERP_CHECK_INDEX_OK(row, d->m_fileAttachments) )
        {
            QPair<QString, QString> pair = d->m_fileAttachments.at(row);
            QString path = pair.first;
            QUrl url = QUrl::fromLocalFile(path);
            if ( !QDesktopServices::openUrl(url) )
            {
                QMessageBox::warning(this, qApp->applicationName(), trUtf8("No se pudo abrir el archivo adjunto. Puede usted ir al archivo en la ruta: %1").arg(path));
            }
            openedRows.append(item->row());
        }
    }
}

void SendEmailDlg::saveAttachment()
{
    QList<QTableWidgetItem *> selected = ui->tableWidget->selectedItems();
    QList<int> openedRows;
    if ( selected.size() == 0 )
    {
        return;
    }
    QString newPath = QFileDialog::getExistingDirectory(this, trUtf8("Seleccione la ruta y el nombre con el que almacenará el adjunto"),
                      QDir::homePath());
    if ( newPath.isEmpty() )
    {
        return;
    }
    foreach (QTableWidgetItem *item, selected)
    {
        int row = item->row();
        if ( !openedRows.contains(row) && AERP_CHECK_INDEX_OK(row, d->m_fileAttachments) )
        {
            QPair<QString, QString> pair = d->m_fileAttachments.at(row);
            QString path = pair.first;
            QFileInfo fi (path);
            QString copyTo = newPath + "/" + fi.fileName();
            if ( !QFile::copy (path, copyTo) )
            {
                QMessageBox::warning(this, qApp->applicationName(), trUtf8("No se ha podido guardar el archivo en la ruta indicada. Puede usted ir al archivo en la ruta: %1").arg(path));
            }
            openedRows.append(item->row());
        }
    }
}

void SendEmailDlgPrivate::initCompleter ()
{
    BaseBeanMetadata *m = BeansFactory::metadataBean(m_contactModel);
    if ( m != NULL && !m_contactModelDisplayField.isEmpty() && !m_contactModelEmailField.isEmpty() )
    {
        m_completerTo = QPointer<QCompleter> (new QCompleter(q_ptr));
        m_completerCopy = QPointer<QCompleter> (new QCompleter(q_ptr));
        m_completerModel = new AERPQueryModel(true, q_ptr);
        m_completerModel->freezeModel();
        // Esta consulta es tan rara para evitar que haya elementos en blanco, que hacen que el Completer no funcione.
        // En el momento en el que el primer elemento de la lista ordenada sera por ejemplo, "ALM" y el último sea ""
        // el completer no es capaz de detectar la ordenación y no genera completado.
        QString sql = QString("SELECT * FROM (SELECT DISTINCT %1 || ' <' || %2 || '>' as column1 FROM %3 WHERE %2 <> '') AS FOO WHERE column1 <> '' ORDER BY column1 ASC").
                      arg(m_contactModelDisplayField).
                      arg(m_contactModelEmailField).
                      arg(m->sqlTableName());
        qDebug() << "SendEmailDlgPrivate::initCompleter: " << sql;
        m_completerModel->setQuery(sql);

        m_completerTo->setModel(m_completerModel.data());
        m_completerTo->setCaseSensitivity(Qt::CaseInsensitive);
        m_completerTo->setWrapAround(true);
        m_completerTo->setCompletionMode(QCompleter::PopupCompletion);
        m_completerTo->setModelSorting(QCompleter::CaseInsensitivelySortedModel);

        m_completerCopy->setModel(m_completerModel.data());
        m_completerCopy->setCaseSensitivity(Qt::CaseInsensitive);
        m_completerCopy->setWrapAround(true);
        m_completerCopy->setCompletionMode(QCompleter::PopupCompletion);
        m_completerCopy->setModelSorting(QCompleter::CaseInsensitivelySortedModel);

        q_ptr->ui->txtTo->setCompleter(m_completerTo.data());
        q_ptr->ui->txtCopy->setCompleter(m_completerCopy.data());
        q_ptr->ui->txtTo->installEventFilter(q_ptr);
        q_ptr->ui->txtCopy->installEventFilter(q_ptr);
    }
}
