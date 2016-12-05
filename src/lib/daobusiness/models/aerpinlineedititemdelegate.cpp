/***************************************************************************
 *   Copyright (C) 2012 by David Pinelo									*
 *   alepherp@alephsistemas.es													*
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
#include <QDebug>

#include <aerpcommon.h>
#include <globales.h>
#include "aerpinlineedititemdelegate.h"
#include "widgets/dbchooserecordbutton.h"
#include "widgets/dbcombobox.h"
#include "widgets/dblineedit.h"
#include "dao/beans/basebean.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/basebean.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbrelationmetadata.h"
#include "models/filterbasebeanmodel.h"
#include "models/basebeanmodel.h"
#include "forms/dbrecorddlg.h"
#include "forms/dbsearchdlg.h"
#include "scripts/perpscript.h"

class AERPInlineEditItemDelegatePrivate
{
public:
    QString m_type;

    AERPInlineEditItemDelegatePrivate()
    {
    }
};

AERPInlineEditItemDelegate::AERPInlineEditItemDelegate(const QString &type, QObject *parent) :
    AERPItemDelegate(parent),
    d(new AERPInlineEditItemDelegatePrivate)
{
    d->m_type = type;
}

AERPInlineEditItemDelegate::~AERPInlineEditItemDelegate()
{
    delete d;
}

QWidget *AERPInlineEditItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QVariant pointer = index.data(AlephERP::DBFieldRole);
    DBField *fld = static_cast<DBField *>(pointer.value<void *>());
    if ( fld == NULL )
    {
        return QStyledItemDelegate::createEditor(parent, option, index);
    }
    if ( d->m_type == QStringLiteral("DBChooseRecordButton") )
    {
        QPushButton *button = new QPushButton(parent);
        setEditor(button);
        return button;
    }
    else if ( d->m_type == QStringLiteral("DBLineEdit") )
    {
        DBLineEdit *le = new DBLineEdit(parent);
        setEditor(le);
        le->setFieldName(fld->metadata()->dbFieldName());
        le->setDataFromParentDialog(false);

        QHashIterator<QString, QVariant> it(fld->metadata()->behaviourOnInlineEdit());
        while (it.hasNext())
        {
            it.next();
            QByteArray key = it.key().toLatin1();
            if ( le->property(key.constData()).isValid() )
            {
                QVariant v;
                if ( it.value().toString() == QStringLiteral("true") || it.value().toString() == QStringLiteral("false") )
                {
                    v = it.value().toString() == QStringLiteral("true");
                }
                else
                {
                    v = it.value();
                }
                le->setProperty(key.constData(), v);
            }
        }

        if ( fld->metadata()->behaviourOnInlineEdit().contains("autoComplete") )
        {
            AlephERP::AutoCompleteTypes flags;
            QString autoComplete = fld->metadata()->behaviourOnInlineEdit().value("autoComplete").toString();
            if ( autoComplete.contains("NoCompletition") )
            {
                flags = AlephERP::NoCompletition;
            }
            if ( autoComplete.indexOf("ValuesFromThisField", 0, Qt::CaseInsensitive) != -1 )
            {
                flags = AlephERP::ValuesFromThisField;
            }
            if ( autoComplete.indexOf("ValuesFromRelation", 0, Qt::CaseInsensitive) != -1 )
            {
                flags = AlephERP::ValuesFromRelation;
            }
            if ( autoComplete.indexOf("ValuesFromTableWithNoRelation", 0, Qt::CaseInsensitive) != -1 )
            {
                flags = AlephERP::ValuesFromTableWithNoRelation;
            }
            if ( autoComplete.indexOf("RestrictValueToItemFromList", 0, Qt::CaseInsensitive) != -1 )
            {
                flags |= AlephERP::RestrictValueToItemFromList;
            }
            if ( autoComplete.indexOf("UpdateOwnerFieldBean", 0, Qt::CaseInsensitive) != -1 )
            {
                flags |= AlephERP::UpdateOwnerFieldBean;
            }
            le->setAutoComplete(flags);
            le->setAutoCompleteColumn(
                        fld->metadata()->behaviourOnInlineEdit().value("autoCompleteColumn").toString().isEmpty() ?
                        fld->metadata()->behaviourOnInlineEdit().value("viewOnRead").toString() :
                        fld->metadata()->behaviourOnInlineEdit().value("autoCompleteColumn").toString());
            QString size = fld->metadata()->behaviourOnInlineEdit().value("autoCompletePopupSize").toString();
            QStringList parts = size.split("x");
            if ( parts.size() > 1 )
            {
                QSize sz;
                sz.setWidth(parts.at(0).toInt());
                sz.setHeight(parts.at(1).toInt());
                le->setAutoCompletePopupSize(sz);
            }
        }
        le->setWorkBean(fld->bean());
        le->setValue(fld->value());
        le->setDataEditable(true);
        return le;
    }
    else if ( d->m_type == QStringLiteral("DBComboBox") )
    {
        DBComboBox *combo = new DBComboBox(parent);
        setEditor(combo);
        combo->setFieldName(fld->metadata()->dbFieldName());
        if ( fld->relations().size() > 0 )
        {
            DBRelation *rel = NULL;
            foreach ( DBRelation *r, fld->relations(AlephERP::ManyToOne) )
            {
                rel = r;
            }
            if ( rel != NULL && fld->metadata()->behaviourOnInlineEdit().contains("viewOnRead") )
            {
                QStringList temp = fld->metadata()->behaviourOnInlineEdit().value("viewOnRead").toString().split(".");
                combo->setListTableModel(rel->metadata()->tableName());
                combo->setListColumnToSave(rel->metadata()->childFieldName());
                if ( temp.size() > 0 )
                {
                    combo->setListColumnName(temp.at(temp.size()-1));
                }
            }
        }
        combo->setWorkBean(fld->bean());
        combo->setValue(fld->value());
        combo->setDataEditable(true);
        return combo;
    }
    return QStyledItemDelegate::createEditor(parent, option, index);
}

void AERPInlineEditItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if ( !index.isValid() )
    {
        return;
    }
    QVariant pointer = index.data(AlephERP::DBFieldRole);
    DBField *fld = static_cast<DBField *>(pointer.value<void *>());
    if ( fld == NULL )
    {
        return;
    }
    if ( d->m_type == QStringLiteral("DBChooseRecordButton") )
    {
/*
        QPushButton *button = qobject_cast<QPushButton *> (editor);
        if ( button != NULL )
        {
            button->setValue(fld->value());
        }
*/
    }
    else if ( d->m_type == QStringLiteral("DBComboBox") )
    {
        DBComboBox *combo = qobject_cast<DBComboBox *> (editor);
        if ( combo != NULL )
        {
            combo->setWorkBean(fld->bean());
            combo->setValue(fld->value());
        }
    }
    else if ( d->m_type == QStringLiteral("DBLineEdit") )
    {
        DBLineEdit *le = qobject_cast<DBLineEdit *> (editor);
        if ( le != NULL )
        {
            // Esta operación puede llamar a observerUnregistered, y borrar el contenido del texto... antes de establecerlo. Lo guardamos
            le->setWorkBean(fld->bean());
            le->setValue(fld->value());
        }
    }
}

void AERPInlineEditItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    if ( d->m_type == QStringLiteral("DBChooseRecordButton") )
    {
        /*        DBChooseRecordButton *button = qobject_cast<DBChooseRecordButton *>(editor);
                if ( button != NULL ) {
                    model->setData(index, button->value());
                }*/
    }
    else if ( d->m_type == QStringLiteral("DBComboBox") )
    {
        DBComboBox *combo = qobject_cast<DBComboBox *>(editor);
        if ( combo != NULL )
        {
            model->setData(index, combo->value());
        }
    }
    else if ( d->m_type == QStringLiteral("DBLineEdit") )
    {
        DBLineEdit *le = qobject_cast<DBLineEdit *> (editor);
        if ( le != NULL )
        {
            BaseBeanSharedPointer b;
            FilterBaseBeanModel *filterModel = qobject_cast<FilterBaseBeanModel *>(model);
            BaseBeanModel *beanModel = qobject_cast<BaseBeanModel *>(model);
            if ( filterModel != NULL )
            {
                b = filterModel->bean(index);
            }
            else if ( beanModel != NULL )
            {
                b = beanModel->bean(index);
            }
            if ( !b.isNull() )
            {
                le->setWorkBean(b.data());
            }
            if ( le != NULL )
            {
                model->setData(index, le->finalValue());
            }
        }
    }
}

void AERPInlineEditItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if ( !index.isValid() )
    {
        return;
    }
    QVariant pointer = index.data(AlephERP::DBFieldRole);
    DBField *fld = static_cast<DBField *>(pointer.value<void *>());
    if ( fld == NULL )
    {
        return;
    }
    Qt::Alignment align = fld->metadata()->alignment();
    QString text = fld->displayValue();
    if ( fld->metadata()->behaviourOnInlineEdit().contains("viewOnRead") )
    {
        DBField *field = NULL;
        QList<DBObject *> objectList = fld->bean()->navigateThrough(fld->metadata()->behaviourOnInlineEdit().value("viewOnRead").toString(), "");
        if ( objectList.size() > 0 )
        {
            field = qobject_cast<DBField *>(objectList.at(0));
        }
        else
        {
            DBObject *obj = fld->bean()->navigateThroughProperties(fld->metadata()->behaviourOnInlineEdit().value("viewOnRead").toString(), true);
            if (obj != NULL)
            {
                field = qobject_cast<DBField *>(obj);
            }
        }
        if ( field != NULL )
        {
            text = field->displayValue();
            align = field->metadata()->alignment();
        }
    }
    painter->save();
    if ( d->m_type == QStringLiteral("DBChooseRecordButton") )
    {
        QStyleOptionButton buttonOption;
        buttonOption.state = QStyle::State_Enabled | QStyle::State_AutoRaise | QStyle::State_Active;
        if ( fld->isEmpty() )
        {
            buttonOption.rect = option.rect;
            buttonOption.text = tr("Seleccione %1").arg(fld->metadata()->fieldName());
            QApplication::style()->drawControl(QStyle::CE_PushButton, &buttonOption, painter);
        }
        else
        {
            QRect rectIcon(option.rect.x(), option.rect.y(), option.rect.height(), option.rect.height());
            buttonOption.rect = rectIcon;
            buttonOption.icon.addFile(QString::fromUtf8(":/generales/images/edit_search.png"), QSize(), QIcon::Normal, QIcon::Off);
            buttonOption.iconSize = QSize(buttonOption.rect.height()-4, buttonOption.rect.height()-4);
            QApplication::style()->drawControl(QStyle::CE_PushButton, &buttonOption, painter);

            QStyleOptionViewItemV4 optionText = option;
            initStyleOption(&optionText, index);
            /* Call this to get the focus rect and selection background. */
            optionText.text = text;
            optionText.displayAlignment = align;
            optionText.rect = QRect(rectIcon.x() + rectIcon.width(), rectIcon.y(), optionText.rect.width() - option.rect.height(), optionText.rect.height());
            optionText.widget->style()->drawControl(QStyle::CE_ItemViewItem, &optionText, painter, optionText.widget);
        }
    }
    else if ( d->m_type == QStringLiteral("DBComboBox") )
    {
        QStyleOptionViewItemV4 options = option;
        initStyleOption(&options, index);
        /* Call this to get the focus rect and selection background. */
        options.text = text;
        options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter, options.widget);
    }
    else if ( d->m_type == QStringLiteral("DBLineEdit") )
    {
        QStyleOptionViewItemV4 options = option;
        initStyleOption(&options, index);
        /* Call this to get the focus rect and selection background. */
        options.text = text;
        options.displayAlignment = align;
        options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter, options.widget);
    }
    painter->restore();
}

bool AERPInlineEditItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if ( event->type() == QEvent::MouseButtonPress && d->m_type == QStringLiteral("DBChooseRecordButton") )
    {
        buttonClicked(index);
        return true;
    }
    else
    {
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }
}

/**
  Aquí es donde se da funcionalidad al botón desde el propio AbstractView.
  */
void AERPInlineEditItemDelegate::buttonClicked(const QModelIndex &index)
{
    if ( !index.isValid() )
    {
        return;
    }
    QVariant pointer = index.data(AlephERP::DBFieldRole);
    DBField *fld = static_cast<DBField *>(pointer.value<void *>());
    if ( fld == NULL )
    {
        return;
    }
    QString tableName, childColumnName;
    BaseBeanPointer workedBean = fld->bean();
    if ( !workedBean.isNull() )
    {
        QList<DBRelation *> relations = fld->relations(AlephERP::ManyToOne);
        DBRelation *rel = NULL;
        foreach ( DBRelation *tmp, relations )
        {
            rel = tmp;
        }
        if ( rel != NULL )
        {
            tableName = rel->metadata()->tableName();
            childColumnName = rel->metadata()->childFieldName();
            // Comprobemos si previamente el usuario ha creado un nuevo field padre
            BaseBeanPointer father = rel->father();
            if ( !father.isNull() )
            {
                if ( father->modified() )
                {
                    if ( father->dbState() == BaseBean::INSERT )
                    {
                        int ret = QMessageBox::question(0,qApp->applicationName(),
                                                        tr("Previamente creó un nuevo registro de tipo <strong>%1</strong>."
                                                               "Este registro aún no ha sido guardado en la base de datos. ¿Desea editarlo? <br/>"
                                                               "Si responde no, perderá este registro que insertó y se abrirá la ventana de búsqueda.").arg(father->metadata()->alias()),
                                                        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel );
                        if ( ret == QMessageBox::Yes )
                        {
                            QPointer<DBRecordDlg> dlg = new DBRecordDlg(father, AlephERP::Update, true, 0);
                            if ( dlg->openSuccess() && dlg->init() )
                            {
                                dlg->setModal(true);
                                dlg->setAttribute(Qt::WA_DeleteOnClose);
                                dlg->exec();
                                return;
                            }
                        }
                        else if ( ret == QMessageBox::Cancel )
                        {
                            return;
                        }
                    }
                }
            }
        }
        if ( !tableName.isEmpty() )
        {
            QPointer<DBSearchDlg> dlg = new DBSearchDlg(tableName, 0);
            if ( dlg->openSuccess() )
            {
                dlg->setModal(true);
                if ( dlg->init() )
                {
                    dlg->exec();
                    if ( dlg->userClickOk() )
                    {
                        // Recogemos el campo buscado si lo hay
                        BaseBeanSharedPointer choosedBean = dlg->selectedBean();
                        if ( !choosedBean.isNull() && !childColumnName.isEmpty() )
                        {
                            QVariant v = choosedBean->fieldValue(childColumnName);
                            fld->setValue(v);
                            buttonUpdateFields(fld, choosedBean.data());
                        }
                    }
                    if ( (dlg->userClickOk() || dlg->userInsertNewRecord()) && fld->metadata()->behaviourOnInlineEdit().contains("executeScriptAfterChoose") )
                    {
                        QWidget *par = qobject_cast<QWidget *>(this->parent());
                        AERPBaseDialog *thisForm = CommonsFunctions::aerpParentDialog(par);
                        if ( thisForm != NULL )
                        {
                            AERPScript engine;
                            CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
                            QString script = fld->metadata()->behaviourOnInlineEdit().value("executeScriptAfterChoose").toString();
                            engine.addFunctionArgument("bean", fld->bean());
                            engine.addFunctionArgument("dbField", fld);
                            engine.addFunctionArgument(AlephERP::stThisForm, thisForm);
                            engine.setScript(script, QString("%1.afterChoose").arg(fld->dbFieldName()));
                            engine.callQsFunction(QString("afterChooseScript"));
                            CommonsFunctions::restoreOverrideCursor();
                        }
                    }
                }
            }
            delete dlg;
        }
    }
}

/**
  Si hay un bean seleccionado, reemplaza los valores de los fields del bean que contiene a este DBChooseRecordButton con los valores
  del bean seleccionado, según las reglas definicas en replaceFields
  */
void AERPInlineEditItemDelegate::buttonUpdateFields(DBField *fld, BaseBean *selectedBean)
{
    if ( fld == NULL || !fld->metadata()->behaviourOnInlineEdit().contains("replaceFields") )
    {
        return;
    }
    BaseBeanPointer editedBean = fld->bean();
    QHashIterator<QString, QVariant> iterator (fld->metadata()->behaviourOnInlineEdit());
    while ( iterator.hasNext() )
    {
        iterator.next();
        if ( iterator.key() == QStringLiteral("replaceFields") )
        {
            QStringList items = iterator.value().toString().split(';');
            if ( items.size() == 2 && selectedBean->field(items.at(0)) != NULL )
            {
                QString editedField = items.at(1);
                QString selectedField = items.at(0);
                if ( editedBean->field(editedField) )
                {
                    editedBean->setFieldValue(editedField, selectedBean->fieldValue(selectedField));
                }
            }
        }
    }
}
