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
#include "qlogger.h"
#include <globales.h>
#include "dbchooserecordbutton.h"
#include "dao/basedao.h"
#include "dao/dbfieldobserver.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/beansfactory.h"
#include "models/relationbasebeanmodel.h"
#include "forms/dbsearchdlg.h"
#include "forms/dbrecorddlg.h"
#include "widgets/dbbasewidget.h"
#include "business/aerploggeduser.h"

class DBChooseRecordButtonPrivate
{
public:
    QString m_tableName;
    QString m_searchFilter;
    /** Contiene una copia del bean obtenido en la última búsqueda */
    BaseBeanPointer m_selectedBean;
    /** Este es el bean que se proporciona al motor QS */
    BaseBeanPointer m_beanForQs;
    /** Antiguas copias de los beans seleccionados ... */
    BaseBeanPointerList m_oldSelectedBeans;
    QString m_scriptExecuteAfterChoose;
    bool m_userInsertNewRecord;
    QStringList m_replaceFields;
    QVariantMap m_defaultValues;
    QString m_scriptBeforeInsert;
    QString m_scriptBeforeExecute;
    BaseBeanPointer m_masterBean;
    QString m_scriptAfterClear;
    QAction *m_actionClear;
    QAction *m_actionEdit;

    DBChooseRecordButton *q_ptr;

    DBChooseRecordButtonPrivate (DBChooseRecordButton *qq) : q_ptr(qq)
    {
        m_tableName = "";
        m_searchFilter = "";
        m_userInsertNewRecord = false;
        m_actionClear = new QAction(q_ptr);
        m_actionClear->setText("Deseleccionar el registro seleccionado");
        m_actionClear->setIcon(QIcon(":/mime/mimeicons/empty.png"));
        m_actionEdit = new QAction(q_ptr);
        m_actionEdit->setText(("Editar el registro seleccionado"));
        m_actionEdit->setIcon(QIcon(":/generales/images/edit_edit.png"));
        QObject::connect(m_actionClear, SIGNAL(triggered()), q_ptr, SLOT(clear()));
        QObject::connect(m_actionEdit, SIGNAL(triggered()), q_ptr, SLOT(editRecord()));
    }

    BaseBeanPointer selectedBean();
    bool selectedBeanFromRelation();
    void clearSelectedBean();
    void setRelationFather(BaseBeanPointer bean);
    AERPBaseDialog *aerpParentDialog();
    bool checkPreviousInserted();
    void setSelectedBean(const BaseBeanPointer &bean);
};

/**
 * @brief DBChooseRecordButtonPrivate::selectedBean
 * @return
 * Devuelve el bean asociado al registro escogido por el usuario. Puede ser una copia del registro seleccionado, o bien
 * un registro padre de una relación, o un registro hermano, si lo hay
 *
 */
BaseBeanPointer DBChooseRecordButtonPrivate::selectedBean()
{
    DBRelation *dlgBeanRelation = q_ptr->relation();
    if ( dlgBeanRelation != NULL )
    {
        if ( dlgBeanRelation->metadata()->type() == DBRelationMetadata::MANY_TO_ONE )
        {
            return dlgBeanRelation->father();
        }
        else if ( dlgBeanRelation->metadata()->type() == DBRelationMetadata::ONE_TO_ONE && !dlgBeanRelation->brother().isNull() )
        {
            return dlgBeanRelation->brother();
        }
    }
    return m_selectedBean;
}

bool DBChooseRecordButtonPrivate::selectedBeanFromRelation()
{
    DBRelation *dlgBeanRelation = q_ptr->relation();
    if ( dlgBeanRelation != NULL )
    {
        if ( dlgBeanRelation->metadata()->type() == DBRelationMetadata::MANY_TO_ONE )
        {
            return true;
        }
        else if ( dlgBeanRelation->metadata()->type() == DBRelationMetadata::ONE_TO_ONE && !dlgBeanRelation->brother().isNull() )
        {
            return true;
        }
    }
    return false;
}

void DBChooseRecordButtonPrivate::clearSelectedBean()
{
    if ( !selectedBeanFromRelation() )
    {
        if ( !m_selectedBean.isNull() )
        {
            m_oldSelectedBeans.append(m_selectedBean);
        }
        m_oldSelectedBeans.append(m_selectedBean);
        m_selectedBean.clear();
    }
}

void DBChooseRecordButtonPrivate::setRelationFather(BaseBeanPointer bean)
{
    DBRelation *relation = q_ptr->relation();
    if ( relation != NULL &&
         bean != NULL &&
         relation->metadata()->type() == DBRelationMetadata::MANY_TO_ONE &&
         bean->dbState() == BaseBean::UPDATE )
    {
        QString childColumnName = relation->metadata()->childFieldName();
        if ( !childColumnName.isEmpty() )
        {
            QVariant v = bean->fieldValue(childColumnName);
            emit q_ptr->valueEdited(v);
            q_ptr->DBBaseWidget::askToRecalculateCounterField();
        }
    }
}

DBChooseRecordButton::DBChooseRecordButton(QWidget *parent) :
    QPushButton(parent), DBBaseWidget(), d(new DBChooseRecordButtonPrivate(this))
{
    connect(this, SIGNAL(clicked()), this, SLOT(buttonClicked()));
    setEnabled(false);
}

DBChooseRecordButton::~DBChooseRecordButton()
{
    delete d;
}

QString DBChooseRecordButton::tableName()
{
    return d->m_tableName;
}

void DBChooseRecordButton::setTableName(const QString &value)
{
    d->m_tableName = value;
    if ( !d->m_tableName.isEmpty() )
    {
        setEnabled(true);
    }
}

void DBChooseRecordButton::setFieldName(const QString &name)
{
    m_fieldName = name;
    if ( !m_fieldName.isEmpty() )
    {
        setEnabled(true);
    }
}

void DBChooseRecordButton::setSearchFilter(const QString &value)
{
    d->m_searchFilter = value;
    d->m_searchFilter = d->m_searchFilter.replace("${user}", AERPLoggedUser::instance()->userName());
}

QString DBChooseRecordButton::searchFilter()
{
    return d->m_searchFilter;
}

QString DBChooseRecordButton::scriptExecuteAfterChoose()
{
    return d->m_scriptExecuteAfterChoose;
}

void DBChooseRecordButton::setScriptExecuteAfterChoose(const QString &script)
{
    d->m_scriptExecuteAfterChoose = script;
}

QStringList DBChooseRecordButton::replaceFields()
{
    return d->m_replaceFields;
}

void DBChooseRecordButton::setReplaceFields(const QStringList &value)
{
    d->m_replaceFields = value;
    emit ( assignFieldsChanged() );
}

QString DBChooseRecordButton::scriptBeforeInsert() const
{
    return d->m_scriptBeforeInsert;
}

void DBChooseRecordButton::setScriptBeforeInsert(const QString &value)
{
    d->m_scriptBeforeInsert = value;
}

QString DBChooseRecordButton::scriptBeforeExecute() const
{
    return d->m_scriptBeforeExecute;
}

void DBChooseRecordButton::setScriptBeforeExecute(const QString &value)
{
    d->m_scriptBeforeExecute = value;
}

QVariantMap DBChooseRecordButton::defaultValues() const
{
    return d->m_defaultValues;
}

void DBChooseRecordButton::setDefaultValues(const QVariantMap &value)
{
    d->m_defaultValues = value;
}

BaseBean * DBChooseRecordButton::beanSearchList()
{
    return d->m_masterBean.data();
}

void DBChooseRecordButton::setBeanSearchList(BaseBean *bean)
{
    d->m_masterBean = bean;
}

QString DBChooseRecordButton::scriptAfterClear() const
{
    return d->m_scriptAfterClear;
}

void DBChooseRecordButton::setScriptAfterClear(const QString &value)
{
    d->m_scriptAfterClear = value;
}

void DBChooseRecordButton::showEvent(QShowEvent *event)
{
    if ( this->icon().isNull() )
    {
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/generales/images/edit_search.png"), QSize(), QIcon::Normal, QIcon::Off);
        this->setIcon(icon);
    }
    DBBaseWidget::showEvent(event);
    DBFieldObserver *obs= qobject_cast<DBFieldObserver *>(observer());
    if ( obs != NULL )
    {
        DBField *fld = qobject_cast<DBField *> (obs->entity());
        if ( fld != NULL )
        {
            QList<DBRelation *> relations = fld->relations(AlephERP::ManyToOne | AlephERP::OneToOne);
            if ( relations.size() > 0 )
            {
                setDBRelation(relations.at(0));
            }
        }
    }
}

void DBChooseRecordButton::buttonClicked()
{
    QString tableName, childColumnName, filter;

    if ( !d->checkPreviousInserted() )
    {
        return;
    }

    // Ejecutamos el método antes de la inserción.
    QObjectList args;
    args.append(this);
    AERPBaseDialog *thisForm = CommonsFunctions::aerpParentDialog(this);
    if ( thisForm != NULL )
    {
        if ( d->m_scriptBeforeExecute.isEmpty() )
        {
            QString scriptName;
            if ( !m_fieldName.isEmpty() )
            {
                scriptName = QString("%1BeforeChoose").arg(m_fieldName);
                thisForm->callQSMethod(scriptName, args);
            }
            scriptName = QString("%1BeforeChoose").arg(objectName());
            thisForm->callQSMethod(scriptName, args);
        }
        else
        {
            thisForm->callQSMethod(d->m_scriptBeforeExecute, args);
        }
    }

    DBRelation *dlgBeanRelation = relation();
    if ( dlgBeanRelation != NULL )
    {
        tableName = dlgBeanRelation->metadata()->tableName();
        childColumnName = dlgBeanRelation->metadata()->childFieldName();
        filter = m_relationFilter;
        if ( !d->m_searchFilter.isEmpty() )
        {
            if ( !filter.isEmpty() )
            {
                filter = QString("%1 AND").arg(filter);
            }
            filter = QString("%1 %2").arg(filter).arg(d->m_searchFilter);
        }
    }
    else
    {
        if ( !d->m_tableName.isEmpty() )
        {
            tableName = d->m_tableName;
            filter = d->m_searchFilter;
        }
        else
        {
            return;
        }
    }

    if ( !tableName.isEmpty() )
    {
        QPointer<DBSearchDlg> dlg (new DBSearchDlg(tableName, this));
        if ( !d->m_masterBean.isNull() )
        {
            dlg->setMasterBean(d->m_masterBean);
        }
        if ( dlgBeanRelation && dlgBeanRelation->metadata()->type() == DBRelationMetadata::MANY_TO_ONE )
        {
            dlg->setInsertFather(dlgBeanRelation->father());
        }
        QString qsBeforeInsertMethodName1 = QString("%1BeforeInsert").arg(objectName());
        QString qsBeforeInsertMethodName2 = QString("%1BeforeInsert").arg(m_fieldName);
        if ( thisForm->hasQSMethod(qsBeforeInsertMethodName1) || thisForm->hasQSMethod(qsBeforeInsertMethodName2) )
        {
            connect(dlg.data(), SIGNAL(beanAboutToBeInserted(BaseBeanPointer)), this, SLOT(beforeInsertBeanQs(BaseBeanPointer)));
        }

        if ( dlg->openSuccess() )
        {
            dlg->setModal(true);
            dlg->setFilterData(filter);
            if ( dlgBeanRelation != NULL )
            {
                DBSearchDlg::DBSearchButtons buttons (DBSearchDlg::Ok | DBSearchDlg::Close);
                if ( !dlgBeanRelation->metadata()->readOnly() )
                {
                    buttons |= DBSearchDlg::EditRecord;
                }
                if ( dlgBeanRelation->metadata()->allowedInsertChild() )
                {
                    buttons |= DBSearchDlg::NewRecord;
                }
                dlg->setVisibleButtons(buttons);
            }
            // Pasamos los valores por defecto de la búsqueda
            QMapIterator<QString, QVariant>it(d->m_defaultValues);
            while (it.hasNext())
            {
                it.next();
                dlg->addDefaultValue(it.key(), it.value());
            }
            if ( dlg->init() )
            {
                dlg->exec();
                if ( dlg->userClickOk() || dlg->userInsertNewRecord() )
                {
                    // Si se ha insertado un nuevo registro, directamente hemos editado el registro father de la relación
                    d->m_userInsertNewRecord = dlg->userInsertNewRecord();
                    d->setSelectedBean(dlg->selectedBean().data());
                    // Puede ocurrir que ciertos cambios en el padre, generen cambios en el registro que se está editando
                    // No está de más refrescar cambios.
                    BaseBeanPointer b = beanFromContainer();
                    if ( b && b->observer() )
                    {
                        b->observer()->sync();
                    }
                }
            }
        }
        delete dlg;
    }
}

void DBChooseRecordButton::beforeInsertBeanQs(BaseBeanPointer bean)
{
    AERPBaseDialog *dlg = qobject_cast<AERPBaseDialog *>(CommonsFunctions::aerpParentDialog(this));
    if ( dlg && bean )
    {
        QString scriptName = QString("%1BeforeInsert").arg(objectName());
        dlg->callQSMethod(scriptName, bean.data());
        scriptName = QString("%1BeforeInsert").arg(m_fieldName);
        dlg->callQSMethod(scriptName, bean.data());
    }
}

/*!
  Tenemos que decirle al motor de scripts, que DBFormDlg se convierte de esta forma a un valor script
  */
QScriptValue DBChooseRecordButton::toScriptValue(QScriptEngine *engine, DBChooseRecordButton * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBChooseRecordButton::fromScriptValue(const QScriptValue &object, DBChooseRecordButton * &out)
{
    out = qobject_cast<DBChooseRecordButton *>(object.toQObject());
}

QScriptValue DBChooseRecordButton::selectedBean()
{
    if ( engine() != NULL && d->selectedBean() )
    {
        if ( !d->selectedBeanFromRelation() )
        {
            if ( d->m_beanForQs.isNull() )
            {
                d->m_beanForQs = d->selectedBean()->clone(this);
                d->m_beanForQs->setParent(NULL);
            }
            QScriptValue result = engine()->newQObject(d->m_beanForQs, QScriptEngine::ScriptOwnership, QScriptEngine::PreferExistingWrapperObject);
            return result;
        }
        else
        {
            QScriptValue result = engine()->newQObject(d->selectedBean(), QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
            return result;
        }
    }
    return QScriptValue(QScriptValue::NullValue);
}

void DBChooseRecordButton::setSelectedBean(QScriptValue &val)
{
    d->m_beanForQs = qobject_cast<BaseBean *>(val.toQObject());
    d->setSelectedBean(d->m_beanForQs);
}

void DBChooseRecordButtonPrivate::setSelectedBean(const BaseBeanPointer &bean)
{
    if ( selectedBeanFromRelation() )
    {
        setRelationFather(bean);
    }
    else if ( selectedBean().isNull() || selectedBean()->objectName() != bean->objectName() )
    {
        m_selectedBean = bean->clone(q_ptr);
        m_beanForQs = NULL;
    }

    q_ptr->updateFields();

    AERPBaseDialog *thisForm = CommonsFunctions::aerpParentDialog(q_ptr);
    if ( thisForm != NULL )
    {
        QObjectList args;
        args.append(selectedBean().data());
        // Invocamos al método tras escoger o insertar un nuevo registro.
        if ( !m_scriptExecuteAfterChoose.isEmpty() )
        {
            thisForm->callQSMethod(m_scriptExecuteAfterChoose, args);
        }
        else
        {
            QString scriptName;
            if ( !q_ptr->fieldName().isEmpty() )
            {
                scriptName = QString("%1AfterChoose").arg(q_ptr->fieldName());
                thisForm->callQSMethod(scriptName, args);
            }
            scriptName = QString("%1AfterChoose").arg(q_ptr->objectName());
            thisForm->callQSMethod(scriptName, args);
        }
    }
}

/*!
  Ajusta los parámetros de visualización de este Widget en función de lo definido en DBField d->m_field
  */
void DBChooseRecordButton::applyFieldProperties()
{
    setEnabled(dataEditable());
    if ( !isEnabled() && qApp->focusWidget() == this )
    {
        focusNextChild();
    }
    else
    {
        setContextMenuPolicy(Qt::DefaultContextMenu);
    }
}

QVariant DBChooseRecordButton::value()
{
    QVariant v;
    if ( d->selectedBean().isNull() )
    {
        return v;
    }
    DBFieldObserver *obs= qobject_cast<DBFieldObserver *>(observer());
    if ( obs != NULL )
    {
        DBField *fld = qobject_cast<DBField *> (obs->entity());
        if ( fld != NULL )
        {
            DBRelation *rel = NULL;
            QList<DBRelation *> relations = fld->relations(AlephERP::ManyToOne);
            foreach ( DBRelation *tmp, relations )
            {
                rel = tmp;
            }
            if ( rel != NULL )
            {
                v = d->selectedBean()->fieldValue(rel->metadata()->childFieldName());
            }
        }
    }
    return v;
}

/**
 * @brief DBChooseRecordButton::setValue
 * Aplicar un valor nulo, implica que se limpia el bean seleccionado.
 * @param v
 */
void DBChooseRecordButton::setValue(const QVariant &v)
{
    if ( !m_fieldName.isEmpty() )
    {
        if ( v != value() && !d->m_userInsertNewRecord )
        {
            if ( v.isNull() || v.toInt() == 0 || v.toString().isEmpty() )
            {
                d->clearSelectedBean();
                d->m_beanForQs = NULL;
                return;
            }
        }
    }
    else
    {
        if ( v.isNull() )
        {
            d->clearSelectedBean();
            d->m_beanForQs = NULL;
        }
    }
}

/*!
  Pide un nuevo observador si es necesario
  */
void DBChooseRecordButton::refresh()
{
    observer();
    if ( m_observer != NULL )
    {
        m_observer->sync();
    }
}

void DBChooseRecordButton::observerUnregistered()
{
    DBBaseWidget::observerUnregistered();
    bool blockState = blockSignals(true);
    d->clearSelectedBean();
    blockSignals(blockState);
}

/**
  Si hay un bean seleccionado, reemplaza los valores de los fields del bean que contiene a este DBChooseRecordButton con los valores
  del bean seleccionado, según las reglas definicas en replaceFields
  */
void DBChooseRecordButton::updateFields()
{
    BaseBeanPointer dialogBean = beanFromContainer();

    foreach ( QString item, d->m_replaceFields )
    {
        QStringList items = item.split(QRegExp(QStringLiteral(";|,")));
        if ( items.size() == 2 )
        {
            QString dialogField = items.at(1);
            QString selectedField = items.at(0);
            if ( d->selectedBean().isNull() )
            {
                dialogBean.data()->setFieldValue(dialogField, QVariant());
            }
            else
            {
                BaseBeanPointer b = d->selectedBean();
                if ( b->field(selectedField) != NULL )
                {
                    if ( dialogBean.data()->field(dialogField) )
                    {
                        dialogBean.data()->setFieldValue(dialogField, b->fieldValue(selectedField));
                    }
                    else
                    {
                        QLogger::QLog_Debug(AlephERP::stLogOther, QString::fromUtf8("DBChooseRecordButton::updateFields: Field no existe en bean editado en el formulario: [%1]").arg(items.at(1)));
                    }
                }
                else
                {
                    QLogger::QLog_Debug(AlephERP::stLogOther, QString::fromUtf8("DBChooseRecordButton::updateFields: Field no existe en bean seleccionado: [%1]").arg(items.at(0)));
                }
            }
        }
        else
        {
            QLogger::QLog_Debug(AlephERP::stLogOther, QString::fromUtf8("DBChooseRecordButton::updateFields: Formato de replaceFields incorrecto: Primero el field que sustituir, después el del field del registro seleccionado separados por ;"));
        }
    }
}

void DBChooseRecordButton::clear()
{
    DBRelation *rel = relation();
    if ( rel != NULL )
    {
        rel->father()->clean(true, true);
    }
    d->clearSelectedBean();
    d->m_userInsertNewRecord = false;
    updateFields();
    emit beanCleared();
    emit valueEdited(QVariant());
    if ( rel != NULL )
    {
        rel->father()->uncheckModifiedFields();
        BaseBeanPointer b = beanFromContainer();
        if ( b && b->observer() )
        {
            b->observer()->sync();
        }
    }
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
            scriptName = QString("%1AfterClear").arg(m_fieldName);
            dlgRecord->callQSMethod(scriptName);
        }
    }
}

void DBChooseRecordButton::editRecord()
{
    BaseBeanPointer editedBean = d->selectedBean();
    // Llamada al procedimiento Qs para que el código pueda dar algunos valores.
    DBRecordDlg *dlgRecord = qobject_cast<DBRecordDlg *>(CommonsFunctions::aerpParentDialog(this));
    if ( dlgRecord != NULL )
    {
        if ( !d->m_scriptBeforeInsert.isEmpty() )
        {
            dlgRecord->callQSMethod(d->m_scriptBeforeInsert);
        }
        else
        {
            beforeInsertBeanQs(editedBean.data());
        }
    }
    QScopedPointer<DBRecordDlg> dlg (new DBRecordDlg(editedBean.data(), AlephERP::Update, this));
    if ( dlg->openSuccess() && dlg->init() )
    {
        dlg->setModal(true);
        dlg->exec();
    }
    // No está de más refrescar cambios.
    BaseBeanPointer b = beanFromContainer();
    if ( b && b->observer() )
    {
        b->observer()->sync();
    }
}

void DBChooseRecordButton::addDefaultValue(const QString &dbFieldName, const QVariant &value)
{
    d->m_defaultValues[dbFieldName] = value;
}

void DBChooseRecordButton::contextMenuEvent(QContextMenuEvent *event)
{
    // Para QAbstractScrollArea y clases derivadas debemos utilizar:
    QPoint globalPos = event->globalPos();
    QMenu contextMenu;

    DBRelation *rel = relation();
    if ( rel != NULL )
    {
        if ( rel->father()->dbState() == BaseBean::INSERT )
        {
            if ( !rel->father()->modified() && rel->metadata()->allowedInsertChild() )
            {
                d->m_actionEdit->setText(trUtf8("Insertar un nuevo registro"));
                contextMenu.addAction(d->m_actionEdit);
            }
            else
            {
                if ( !rel->metadata()->readOnly() )
                {
                    d->m_actionEdit->setText(trUtf8("Editar registro seleccionado"));
                    d->m_actionClear->setText(trUtf8("Limpiar los datos antes editados"));
                    contextMenu.addAction(d->m_actionEdit);
                    contextMenu.addAction(d->m_actionClear);
                }
            }
        }
        else
        {
            if ( !rel->metadata()->readOnly() )
            {
                d->m_actionEdit->setText(trUtf8("Editar registro seleccionado"));
                d->m_actionClear->setText(trUtf8("Deseleccionar el registro antes seleccionado"));
                contextMenu.addAction(d->m_actionEdit);
                contextMenu.addAction(d->m_actionClear);
            }
        }
    }
    else
    {
        d->m_actionEdit->setText(trUtf8("Editar registro seleccionado"));
        d->m_actionClear->setText(trUtf8("Deseleccionar el registro antes seleccionado"));
        contextMenu.addAction(d->m_actionEdit);
        contextMenu.addAction(d->m_actionClear);
    }
    contextMenu.exec(globalPos);
}

/**
 * @brief DBChooseRecordButtonPrivate::checkPreviousInserted
 * Comprueba y pregunta al usuario, si previamente había insertado un registro en el formulario de búsqueda
 * @return false si el usuario no quiere continuar
 */
bool DBChooseRecordButtonPrivate::checkPreviousInserted()
{
    DBRelation *dlgBeanRelation = q_ptr->relation();
    if ( dlgBeanRelation != NULL && !dlgBeanRelation->metadata()->readOnly() )
    {
        // Comprobemos si previamente el usuario ha creado un nuevo field padre
        BaseBeanPointer father = dlgBeanRelation->father(false);
        if ( !father.isNull() && father->modified() && father->dbState() == BaseBean::INSERT )
        {
            int ret = QMessageBox::question(q_ptr, qApp->applicationName(),
                                            QObject::trUtf8("Previamente creó un nuevo registro de tipo <strong>%1</strong>. "
                                                    "Este registro aún no ha sido guardado en la base de datos. ¿Desea editarlo? <br/>"
                                                    "Si responde no, perderá este registro que insertó y se abrirá la ventana de búsqueda.").arg(father->metadata()->alias()),
                                            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel );
            if ( ret == QMessageBox::Yes )
            {
                QPointer<DBRecordDlg> dlg = new DBRecordDlg(father, AlephERP::Update, q_ptr);
                if ( dlg->openSuccess() && dlg->init() )
                {
                    dlg->setModal(true);
                    dlg->setAttribute(Qt::WA_DeleteOnClose);
                    dlg->exec();
                    return false;
                }
            }
            else if ( ret == QMessageBox::Cancel )
            {
                return false;
            }
        }
    }
    return true;
}
