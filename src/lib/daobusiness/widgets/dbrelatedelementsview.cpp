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
#include "dbrelatedelementsview.h"
#include "ui_dbrelatedelementsview.h"
#include "models/relatedelementsrecordmodel.h"
#include "dao/observerfactory.h"
#include "dao/basebeanobserver.h"
#include "dao/beans/basebean.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/relatedelement.h"
#include "dao/aerptransactioncontext.h"
#include "dao/dbmultiplerelationobserver.h"
#include "forms/dbrecorddlg.h"
#include "forms/dbsearchdlg.h"
#include "dao/aerptransactioncontext.h"
#include "globales.h"

class DBRelatedElementsViewPrivate
{
public:
    AlephERP::RelatedElementCardinalities m_cardinality;
    DBRelatedElementsView *q_ptr;
    QStringList m_allowedMetadatas;
    QStringList m_categories;
    DBRelatedElementsView::CategoriesRule m_categoriesRule;
    DBRelatedElementsView::Buttons m_visibleButtons;
    QPointer<RelatedElementsRecordModel> m_model;
    QString m_visibleFields;
    bool m_inited;
    QStringList m_linkColumns;
    bool m_askForCategories;
    QString m_beforeAddScript;
    QString m_afterAddScript;
    QString m_beforeAddExistingScript;
    QString m_afterAddExistingScript;
    QString m_beforeEditScript;
    QString m_afterEditScript;
    QString m_beforeDeleteScript;
    QString m_afterDeleteScript;
    QString m_beforeRemoveExistingScript;
    QString m_afterRemoveExistingScript;
    bool m_useNewContext;

    DBRelatedElementsViewPrivate(DBRelatedElementsView *qq) : q_ptr(qq)
    {
        m_visibleButtons = DBRelatedElementsView::Buttons(DBRelatedElementsView::Insert | DBRelatedElementsView::Update | DBRelatedElementsView::Delete | DBRelatedElementsView::View);
        m_inited = false;
        m_cardinality = AlephERP::PointToChild;
        m_categoriesRule = DBRelatedElementsView::AllOfThem;
        m_askForCategories = false;
        m_useNewContext = true;
        foreach(BaseBeanMetadata *m, BeansFactory::instance()->allowedMetadatasToUser())
        {
            m_allowedMetadatas.append(m->tableName());
        }
    }

    void init();
};

DBRelatedElementsView::DBRelatedElementsView(QWidget *parent) :
    QWidget(parent), DBAbstractViewInterface(this, NULL), ui(new Ui::DBRelatedElementsView), d(new DBRelatedElementsViewPrivate(this))
{
    ui->setupUi(this);
    ui->pbAddExisting->setVisible(false);
    connect (ui->pbAdd, SIGNAL(clicked()), this, SLOT(addRelatedElement()));
    connect (ui->pbAddExisting, SIGNAL(clicked()), this, SLOT(addExisting()));
    connect (ui->pbDelete, SIGNAL(clicked()), this, SLOT(deleteRelatedElement()));
    connect (ui->pbEdit, SIGNAL(clicked()), this, SLOT(editRelatedElement()));
    connect (ui->pbView, SIGNAL(clicked()), this, SLOT(viewRelatedElement()));
    connect (ui->pbRemoveExisting, SIGNAL(clicked()), this, SLOT(removeExisting()));
    connect (ui->tableView, SIGNAL(clicked(QModelIndex)), this, SLOT(itemClicked(QModelIndex)));
}

DBRelatedElementsView::~DBRelatedElementsView()
{
    emit destroyed(this);
    delete ui;
    delete d;
}

QString DBRelatedElementsView::allowedMetadatas() const
{
    return d->m_allowedMetadatas.join(';');
}

void DBRelatedElementsView::setAllowedMetadatas(const QString &value)
{
    d->m_allowedMetadatas.clear();
    d->m_allowedMetadatas = value.split(QRegExp(QStringLiteral(";|,")));
}

QString DBRelatedElementsView::categories() const
{
    return d->m_categories.join(QStringLiteral(","));
}

void DBRelatedElementsView::setCategories(const QString &value)
{
    d->m_categories.clear();
    d->m_categories = value.split(QRegExp(QStringLiteral(";|,")));
}

DBRelatedElementsView::CategoriesRule DBRelatedElementsView::categoriesRule() const
{
    return d->m_categoriesRule;
}

void DBRelatedElementsView::setCategoriesRule(DBRelatedElementsView::CategoriesRule rule)
{
    d->m_categoriesRule = rule;
}

void DBRelatedElementsView::setValue(const QVariant &value)
{
    Q_UNUSED(value)
}

QVariant DBRelatedElementsView::value()
{
    return QVariant();
}

void DBRelatedElementsView::applyFieldProperties()
{
    if ( !dataEditable() )
    {
        ui->pbAdd->setVisible(false);
        ui->pbAddExisting->setVisible(false);
        ui->pbDelete->setVisible(false);
        ui->pbEdit->setVisible(false);
        ui->pbRemoveExisting->setVisible(false);
        if ( qApp->focusWidget() == this )
        {
            focusNextChild();
        }
    }
}

void DBRelatedElementsView::refresh()
{
    if ( !d->m_model.isNull() )
    {
        d->m_model->refresh();
    }
}

QScriptValue DBRelatedElementsView::checkedBeans()
{
    return QScriptValue();
}

void DBRelatedElementsView::setCheckedBeans(BaseBeanSharedPointerList list, bool checked)
{
    Q_UNUSED(list)
    Q_UNUSED(checked)
}

void DBRelatedElementsView::setCheckedBeansByPk(QVariantList list, bool checked)
{
    Q_UNUSED(list)
    Q_UNUSED(checked)
}

void DBRelatedElementsView::orderColumns(const QStringList &order)
{
    ui->tableView->orderColumns(order);
}

void DBRelatedElementsView::sortByColumn(const QString &field, Qt::SortOrder order)
{
    ui->tableView->sortByColumn(field, order);
}

void DBRelatedElementsView::saveColumnsOrder(const QStringList &order, const QStringList &sort)
{
    ui->tableView->saveColumnsOrder(order, sort);
}

QString DBRelatedElementsView::askForMetadata(bool *cancel)
{
    QString metadata;
    *cancel = false;
    // Añadir un elemento relacionado, implica primero ver si se permiten varios metadatos, y si es así, determinar de cuál se va a añdir
    if ( d->m_allowedMetadatas.size() == 0 )
    {
        return metadata;
    }
    else if ( d->m_allowedMetadatas.size() > 1 )
    {
        QStringList items;
        foreach ( QString allowedMetadata, d->m_allowedMetadatas )
        {
            BaseBeanMetadata *m = BeansFactory::metadataBean(allowedMetadata);
            if ( m != NULL )
            {
                items.append(m->alias());
            }
        }
        bool ok;
        QString item = QInputDialog::getItem(this, qApp->applicationName(), trUtf8("Seleccione el tipo de elemento relacionado que va a agregar"), items, 0, false, &ok);
        if (ok && !item.isEmpty())
        {
            foreach ( QString allowedMetadata, d->m_allowedMetadatas )
            {
                BaseBeanMetadata *m = BeansFactory::metadataBean(allowedMetadata);
                if ( m != NULL )
                {
                    if ( m->alias() == item )
                    {
                        metadata = m->tableName();
                    }
                }
            }
        }
        else
        {
            *cancel = true;
        }
    }
    else if ( d->m_allowedMetadatas.size() == 1 )
    {
        metadata = d->m_allowedMetadatas.first();
    }
    return metadata;
}

QStringList DBRelatedElementsView::askForCategory(bool *cancel)
{
    QStringList category;
    *cancel = false;
    if ( !d->m_askForCategories )
    {
        return d->m_categories;
    }
    if ( d->m_categories.size() > 1 )
    {
        bool ok;
        QString item = QInputDialog::getItem(this, qApp->applicationName(), trUtf8("Seleccione la categoría del elemento relacionado que va a agregar"), d->m_categories, 0, false, &ok);
        if (ok && !item.isEmpty())
        {
            category.append(item);
        }
        else
        {
            *cancel = true;
        }
    }
    else if ( d->m_categories.size() == 1 )
    {
        category.append(d->m_categories.first());
    }
    return category;
}

void DBRelatedElementsView::relationChildBeanInserted(BaseBean *bean, int index)
{
    Q_UNUSED(index)
    if ( !d->m_model.isNull() )
    {
        d->m_model->addRootBean(bean);
        d->m_model->refresh();
    }
}

void DBRelatedElementsView::relationChildBeanDeleted(BaseBean *bean, int index)
{
    Q_UNUSED(index)
    if ( !d->m_model.isNull() )
    {
        d->m_model->removeRootBean(bean);
        d->m_model->refresh();
    }
}

DBRelatedElementsView::Buttons DBRelatedElementsView::visibleButtons()
{
    return d->m_visibleButtons;
}

void DBRelatedElementsView::setVisibleButtons(DBRelatedElementsView::Buttons buttons)
{
    d->m_visibleButtons = buttons;
    ui->pbAdd->setVisible(buttons.testFlag(DBRelatedElementsView::Insert));
    ui->pbEdit->setVisible(buttons.testFlag(DBRelatedElementsView::Update));
    ui->pbDelete->setVisible(buttons.testFlag(DBRelatedElementsView::Delete));
    ui->pbView->setVisible(buttons.testFlag(DBRelatedElementsView::View));
    ui->pbAddExisting->setVisible(buttons.testFlag(DBRelatedElementsView::InsertExisting));
    ui->pbRemoveExisting->setVisible(buttons.testFlag(DBRelatedElementsView::RemoveExisting));
}

QAbstractItemView::EditTriggers DBRelatedElementsView::editTriggers () const
{
    return ui->tableView->editTriggers();
}

void DBRelatedElementsView::setEditTriggers (QAbstractItemView::EditTriggers triggers)
{
    ui->tableView->setEditTriggers(triggers);
}

QString DBRelatedElementsView::visibleColumns() const
{
    return d->m_visibleFields;
}

void DBRelatedElementsView::setVisibleColumns(const QString &value)
{
    d->m_visibleFields = value;
}

AlephERP::RelatedElementCardinalities DBRelatedElementsView::cardinality() const
{
    return d->m_cardinality;
}

void DBRelatedElementsView::setCardinality(AlephERP::RelatedElementCardinalities value)
{
    d->m_cardinality = value;
}

QString DBRelatedElementsView::linkColumns() const
{
    return d->m_linkColumns.join(';');
}

void DBRelatedElementsView::setLinkColumns(const QString &value)
{
    d->m_linkColumns = value.split(QRegExp(QStringLiteral(";|,")));
}

bool DBRelatedElementsView::askForCategories() const
{
    return d->m_askForCategories;
}

void DBRelatedElementsView::setAskForCategories(bool value)
{
    d->m_askForCategories = value;
}

bool DBRelatedElementsView::dataEditable()
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
    return true;
}

QString DBRelatedElementsView::beforeAddScript() const
{
    return d->m_beforeAddScript;
}

void DBRelatedElementsView::setBeforeAddScript(const QString &value)
{
    d->m_beforeAddScript = value;
}

QString DBRelatedElementsView::afterAddScript() const
{
    return d->m_afterAddScript;
}

void DBRelatedElementsView::setAfterAddScript(const QString &value)
{
    d->m_afterAddScript = value;
}

QString DBRelatedElementsView::beforeAddExistingScript() const
{
    return d->m_beforeAddExistingScript;
}

void DBRelatedElementsView::setBeforeAddExistingScript(const QString &value)
{
    d->m_beforeAddExistingScript = value;
}

QString DBRelatedElementsView::afterAddExistingScript() const
{
    return d->m_afterAddExistingScript;
}

void DBRelatedElementsView::setAfterAddExistingScript(const QString &value)
{
    d->m_afterAddExistingScript = value;
}

QString DBRelatedElementsView::beforeEditScript() const
{
    return d->m_beforeEditScript;
}

void DBRelatedElementsView::setBeforeEditScript(const QString &value)
{
    d->m_beforeEditScript = value;
}

QString DBRelatedElementsView::afterEditScript() const
{
    return d->m_afterEditScript;
}

void DBRelatedElementsView::setAfterEditScript(const QString &value)
{
    d->m_afterEditScript = value;
}

QString DBRelatedElementsView::beforeDeleteScript() const
{
    return d->m_beforeDeleteScript;
}

void DBRelatedElementsView::setBeforeDeleteScript(const QString &value)
{
    d->m_beforeDeleteScript = value;
}

QString DBRelatedElementsView::afterDeleteScript() const
{
    return d->m_afterDeleteScript;
}

void DBRelatedElementsView::setAfterDeleteScript(const QString &value)
{
    d->m_afterDeleteScript = value;
}

QString DBRelatedElementsView::beforeRemoveExistingScript() const
{
    return d->m_beforeRemoveExistingScript;
}

void DBRelatedElementsView::setBeforeRemoveExistingScript(const QString &value)
{
    d->m_beforeRemoveExistingScript = value;
}

QString DBRelatedElementsView::afterRemoveExistingScript() const
{
    return d->m_afterRemoveExistingScript;
}

void DBRelatedElementsView::setAfterRemoveExistingScript(const QString &value)
{
    d->m_afterRemoveExistingScript = value;
}

bool DBRelatedElementsView::useNewContext() const
{
    return d->m_useNewContext;
}

void DBRelatedElementsView::setUseNewContext(bool value)
{
    d->m_useNewContext = value;
}

void DBRelatedElementsView::showEvent(QShowEvent *event)
{
    // Este orden es importante
    if ( !d->m_inited )
    {
        d->init();
    }
    DBBaseWidget::showEvent(event);
    if ( dataEditable() )
    {
        setVisibleButtons(d->m_visibleButtons);
    }
}

void DBRelatedElementsViewPrivate::init()
{
    AbstractObserver *obs = q_ptr->observer();
    if ( obs == NULL )
    {
        qDebug() << "DBRelatedElementsViewPrivate::init: No hay ningún observador asignado.";
        return;
    }
    DBMultipleRelationObserver *mObs = qobject_cast<DBMultipleRelationObserver *>(obs);
    if ( mObs == NULL )
    {
        qDebug() << "DBRelatedElementsViewPrivate::init: No hay ningún observador asignado.";
        return;
    }
    BaseBeanPointerList list;
    QList<DBObject *> items = mObs->entities();
    foreach (DBObject *obj, items)
    {
        DBRelation *rel = qobject_cast<DBRelation *>(obj);
        if ( rel != NULL )
        {
            QObject::connect(rel, SIGNAL(childInserted(BaseBean*,int)), q_ptr, SLOT(relationChildBeanInserted(BaseBean*,int)));
            QObject::connect(rel, SIGNAL(childDeleted(BaseBean*,int)), q_ptr, SLOT(relationChildBeanDeleted(BaseBean*,int)));
            list.append(rel->children());
        }
        else
        {
            BaseBean *b = qobject_cast<BaseBean *> (obj);
            if ( b != NULL )
            {
                list.append(b);
            }
        }
    }
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    int iCatRules = (int) m_categoriesRule;
    RelatedElementsRecordModel::CategoriesRule catRules = (RelatedElementsRecordModel::CategoriesRule) iCatRules;
    m_model = QPointer<RelatedElementsRecordModel>(new RelatedElementsRecordModel(list, m_cardinality, m_allowedMetadatas, m_categories, catRules, q_ptr));
    m_model->setVisibleFields(m_visibleFields.split(QRegExp(QStringLiteral(";|,"))));
    m_model->setLinkColumns(m_linkColumns);
    m_model->setCanShowCheckBoxes(false);
    q_ptr->ui->tableView->setModel(m_model.data());
    m_inited = true;
    CommonsFunctions::restoreOverrideCursor();
}

void DBRelatedElementsView::addRelatedElement()
{
    bool cancel;
    QString metadata = askForMetadata(&cancel);
    if ( cancel )
    {
        return;
    }
    QStringList category (askForCategory(&cancel));
    if ( cancel )
    {
        return;
    }
    if ( !metadata.isEmpty() )
    {
        DBRecordDlg *dlgRecord = qobject_cast<DBRecordDlg *>(CommonsFunctions::aerpParentDialog(this));
        if ( dlgRecord != NULL )
        {
            QScriptValue v = true;
            if ( !d->m_beforeAddScript.isEmpty() )
            {
                v = dlgRecord->callQSMethod(d->m_beforeAddScript);
            }
            if ( v.isValid() && !v.toBool() )
            {
                return;
            }
            QString scriptName = QString("%1BeforeAdd").arg(objectName());
            v = dlgRecord->callQSMethod(scriptName);
            if ( v.isValid() && !v.toBool() )
            {
                return;
            }
        }

        // Que newRelatedElement tiene un sharedPointer, nos garantiza que no se destruirá cuando finalice este bloque.
        RelatedElement *element = d->m_model->insertRow(metadata, category);
        if ( element != NULL )
        {
            QScopedPointer<DBRecordDlg> dlg(new DBRecordDlg(element->relatedBean().data(), AlephERP::Insert, d->m_useNewContext, this));
            dlg->setModal(true);
            if ( !dlg->openSuccess() || !dlg->init() )
            {
                QMessageBox::warning(this, qApp->applicationName(), trUtf8("Ha ocurrido un error abriendo el formulario de edición de %1").arg(element->relatedBean().data()->metadata()->alias()), QMessageBox::Ok);
                return;
            }
            dlg->exec();
            if (!dlg->userSaveData())
            {
                d->m_model->removeRelatedElement(element);
            }
            else if ( dlgRecord != NULL )
            {
                if ( !d->m_afterAddScript.isEmpty() )
                {
                    dlgRecord->callQSMethod(d->m_afterAddScript, element);
                }
                QString scriptName = QString("%1AfterAdd").arg(objectName());
                dlgRecord->callQSMethod(scriptName, element);
            }
        }
        else if ( !d->m_model->property(AlephERP::stLastErrorMessage).toString().isEmpty() )
        {
            QMessageBox::warning(this, qApp->applicationName(), trUtf8("Ha ocurrido un error al intentar agregar un registro. \nEl error es: %1").arg(d->m_model->property(AlephERP::stLastErrorMessage).toString()));
            d->m_model->setProperty(AlephERP::stLastErrorMessage, "");
        }
    }
}

void DBRelatedElementsView::addExisting()
{
    bool cancel;
    QString metadata = askForMetadata(&cancel);
    if ( cancel )
    {
        return;
    }
    QStringList category(askForCategory(&cancel));
    if ( cancel )
    {
        return;
    }
    DBRecordDlg *dlgRecord = qobject_cast<DBRecordDlg *>(CommonsFunctions::aerpParentDialog(this));
    if ( dlgRecord != NULL )
    {
        QScriptValue v = true;
        if ( !d->m_beforeAddExistingScript.isEmpty() )
        {
            v = dlgRecord->callQSMethod(d->m_beforeAddExistingScript);
        }
        if ( v.isValid() && !v.toBool() )
        {
            return;
        }
        QString scriptName = QString("%1BeforeAddExisting").arg(objectName());
        v = dlgRecord->callQSMethod(scriptName);
        if ( v.isValid() && !v.toBool() )
        {
            return;
        }
    }

    QString contextName = QUuid::createUuid().toString();
    QScopedPointer<DBSearchDlg> dlg (new DBSearchDlg(metadata, d->m_useNewContext, this));
    if ( dlg->openSuccess() )
    {
        QString filter;
        if ( !m_relationFilter.isEmpty() )
        {
            if ( !filter.isEmpty() )
            {
                filter = QString("%1 AND ").arg(filter);
            }
            filter = QString("%1%2").arg(filter).arg(m_relationFilter);
        }
        if ( !filter.isEmpty() )
        {
            dlg->setFilterData(filter);
        }
        dlg->setModal(true);
        dlg->setCanSelectSeveral(true);
        if ( !dlg->init() )
        {
            return;
        }
        dlg->exec();
        BaseBeanSharedPointerList list = dlg->checkedBeans();
        RelatedElementPointerList relatedList;
        foreach (BaseBeanSharedPointer relBean, list)
        {
            BaseBeanPointer clonedBean = relBean->clone(NULL);
            RelatedElement *rel = d->m_model->insertRow(metadata, category, clonedBean, true);
            if ( rel == NULL )
            {
                if ( !d->m_model->property(AlephERP::stLastErrorMessage).toString().isEmpty() )
                {
                    QMessageBox::warning(this, qApp->applicationName(), trUtf8("Ha ocurrido un error al intentar agregar un elemento. \nEl error es: %1").arg(d->m_model->property(AlephERP::stLastErrorMessage).toString()));
                    d->m_model->setProperty(AlephERP::stLastErrorMessage, "");
                }
            }
            else
            {
                relatedList.append(rel);
            }
        }
        if ( dlgRecord != NULL && list.size() > 0 )
        {
            if ( !d->m_afterAddExistingScript.isEmpty() )
            {
                dlgRecord->callQSMethod(d->m_afterAddExistingScript, relatedList);
            }
            QString scriptName = QString("%1AfterAddExisting").arg(objectName());
            dlgRecord->callQSMethod(scriptName, relatedList);
        }
    }
}

void DBRelatedElementsView::deleteRelatedElement()
{
    BaseBeanPointer bean = beanFromContainer();

    if ( bean.isNull() )
    {
        return;
    }
    if ( d->m_model.isNull() )
    {
        return;
    }
    QModelIndexList idx = ui->tableView->selectionModel()->selectedIndexes();
    if ( idx.size() == 0 )
    {
        QMessageBox::information(this, qApp->applicationName(),
                                 trUtf8("Debe seleccionar al menos un elemento para eliminar."), QMessageBox::Ok);
        return;
    }
    int ret = QMessageBox::question(this, qApp->applicationName(), trUtf8("El registro relacionado será eliminado. Si lo único que desea es romper "
                                    "la relación, escoga la opción 'Desasociar'. ¿Está seguro de querer borrar el registro "
                                    "relacionado?"), QMessageBox::Yes | QMessageBox::No);
    if ( ret == QMessageBox::No )
    {
        return;
    }
    QList<int> rows;
    for (int i = 0 ; i < idx.size() ; i++)
    {
        if ( !rows.contains(idx.at(i).row() ) )
        {
            rows.append(idx.at(i).row());
        }
    }

    DBRecordDlg *dlgRecord = qobject_cast<DBRecordDlg *>(CommonsFunctions::aerpParentDialog(this));

    foreach (int row, rows)
    {
        RelatedElement *element = d->m_model->element(row);
        if ( element->found() && !element->relatedBean().isNull() && element->relatedBean()->dbState() == BaseBean::UPDATE )
        {
            bool del = true;
            if ( dlgRecord != NULL )
            {
                QScriptValue v = true;
                if ( !d->m_beforeDeleteScript.isEmpty() )
                {
                    v = dlgRecord->callQSMethod(d->m_beforeDeleteScript, element);
                }
                if ( v.isValid() && !v.toBool() )
                {
                    del = false;
                }
                QString scriptName = QString("%1BeforeDelete").arg(objectName());
                v = dlgRecord->callQSMethod(scriptName, element);
                if ( v.isValid() && !v.toBool() )
                {
                    del = false;
                }
            }
            if ( del )
            {
                element->relatedBean()->setDbState(BaseBean::TO_BE_DELETED);
                AERPTransactionContext::instance()->addToContext(bean->actualContext(), element->relatedBean());
            }
        }
        if ( d->m_model->removeRow(row) )
        {
            if ( dlgRecord != NULL )
            {
                if ( !d->m_afterDeleteScript.isEmpty() )
                {
                    dlgRecord->callQSMethod(d->m_afterDeleteScript, element);
                }
                QString scriptName = QString("%1AfterDelete").arg(objectName());
                dlgRecord->callQSMethod(scriptName, element);
            }
        }
    }
}

void DBRelatedElementsView::editRelatedElement()
{
    QModelIndexList idx = ui->tableView->selectionModel()->selectedIndexes();
    if ( idx.size() == 0 )
    {
        QMessageBox::information(this, qApp->applicationName(),
                                 trUtf8("Debe seleccionar al menos un elemento para editar."), QMessageBox::Ok);
        return;
    }
    RelatedElement *element = d->m_model->element(idx.at(0));
    BaseBeanPointer bean = element->relatedBean();
    if ( !bean.isNull() )
    {
        DBRecordDlg *dlgRecord = qobject_cast<DBRecordDlg *>(CommonsFunctions::aerpParentDialog(this));
        if ( dlgRecord != NULL )
        {
            QScriptValue v = true;
            if ( !d->m_beforeEditScript.isEmpty() )
            {
                v = dlgRecord->callQSMethod(d->m_beforeEditScript, element);
            }
            if ( v.isValid() && !v.toBool() )
            {
                return;
            }
            QString scriptName = QString("%1BeforeEdit").arg(objectName());
            v = dlgRecord->callQSMethod(scriptName, element);
            if ( v.isValid() && !v.toBool() )
            {
                return;
            }
        }
        QScopedPointer<DBRecordDlg> dlg (new DBRecordDlg(bean.data(), AlephERP::Update, d->m_useNewContext, this));
        if ( dlg->openSuccess() && dlg->init() )
        {
            // Guardar los datos de los hijos agregados, será responsabilidad del bean padre
            // que se está editando
            dlg->setModal(true);
            dlg->exec();
            if ( dlgRecord != NULL )
            {
                if ( !d->m_afterEditScript.isEmpty() )
                {
                    dlgRecord->callQSMethod(d->m_afterEditScript, element);
                }
                QString scriptName = QString("%1AfterEdit").arg(objectName());
                dlgRecord->callQSMethod(scriptName, element);
            }
        }
    }
}

void DBRelatedElementsView::viewRelatedElement()
{
    QModelIndexList idx = ui->tableView->selectionModel()->selectedIndexes();
    if ( idx.size() == 0 )
    {
        QMessageBox::information(this, qApp->applicationName(),
                                 trUtf8("Debe seleccionar al menos un elemento para visualizar."), QMessageBox::Ok);
        return;
    }
    RelatedElement *element = d->m_model->element(idx.at(0));
    if ( element == NULL )
    {
        QMessageBox::information(this, qApp->applicationName(),
                                 trUtf8("Ha ocurrido un error, y no se puede visualizar el registro"), QMessageBox::Ok);
        return;
    }
    BaseBeanPointer bean = element->relatedBean();
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

void DBRelatedElementsView::removeExisting()
{
    QModelIndexList idx = ui->tableView->selectionModel()->selectedIndexes();
    if ( idx.size() == 0 )
    {
        QMessageBox::information(this, qApp->applicationName(),
                                 trUtf8("Debe seleccionar al menos un elemento para visualizar."), QMessageBox::Ok);
        return;
    }
    QList<int> rows;
    for (int i = 0 ; i < idx.size() ; i++)
    {
        if ( !rows.contains(idx.at(i).row() ) )
        {
            rows.append(idx.at(i).row());
        }
    }
    DBRecordDlg *dlgRecord = qobject_cast<DBRecordDlg *>(CommonsFunctions::aerpParentDialog(this));
    foreach (int row, rows)
    {
        bool del = true;
        RelatedElement *element = d->m_model->element(row);
        if ( dlgRecord != NULL )
        {
            QScriptValue v = true;
            if ( !d->m_beforeRemoveExistingScript.isEmpty() )
            {
                v = dlgRecord->callQSMethod(d->m_beforeRemoveExistingScript, element);
            }
            if ( v.isValid() && !v.toBool() )
            {
                del = false;
            }
            QString scriptName = QString("%1BeforeRemoveExisting").arg(objectName());
            v = dlgRecord->callQSMethod(scriptName, element);
            if ( v.isValid() && !v.toBool() )
            {
                del = false;
            }
        }
        if ( del )
        {
            if ( d->m_model->removeRow(row) )
            {
                if ( dlgRecord != NULL )
                {
                    if ( !d->m_afterRemoveExistingScript.isEmpty() )
                    {
                        dlgRecord->callQSMethod(d->m_afterRemoveExistingScript, element);
                    }
                    QString scriptName = QString("%1AfterRemoveExisting").arg(objectName());
                    dlgRecord->callQSMethod(scriptName, element);
                }
            }
        }
    }
}

void DBRelatedElementsView::itemClicked(const QModelIndex &idx)
{
    if ( d->m_model.isNull() || !idx.isValid() || !d->m_model->isLinkColumn(idx.column()) )
    {
        return;
    }

    AlephERP::FormOpenType openType;
    // Si es un check el contenido de la celda, no abrimos nada
    Qt::ItemFlags flags = d->m_model->flags(idx);
    if ( flags.testFlag(Qt::ItemIsUserCheckable) )
    {
        qDebug() << "DBRelatedElementsView::itemClicked: La columna esta marcado como enlace, pero a la misma vez contiene un check. Deshabilitamos el enlace.";
        return;
    }
    if ( d->m_visibleButtons.testFlag(DBRelatedElementsView::Update) )
    {
        openType = AlephERP::Update;
    }
    else
    {
        openType = AlephERP::ReadOnly;
    }
    // Y ahora creamos el formulario que presentará los datos de este bean
    RelatedElement *element = d->m_model->element(idx);
    if ( element == NULL || element->relatedBean().isNull() )
    {
        return;
    }
    // OJO: Lo mismo este registro apunta al propio registro que abrió un DBRecordDlg que contiene a este widget
    DBRecordDlg *parentDlg = qobject_cast<DBRecordDlg *>(CommonsFunctions::aerpParentDialog(this));
    if ( parentDlg != NULL )
    {
        DBRecordDlg *grandfatherDlg = qobject_cast<DBRecordDlg *>(CommonsFunctions::aerpParentDialog(qobject_cast<QWidget *>(parentDlg->parent())));
        if ( grandfatherDlg != NULL )
        {
            if ( grandfatherDlg->bean()->objectName() == element->relatedBean()->objectName() )
            {
                return;
            }
        }
    }
    DBRecordDlg *recordDlg = qobject_cast<DBRecordDlg *>(CommonsFunctions::aerpParentDialog(this));
    // Vemos si el formulario principal está en modo navegación especial
    if ( recordDlg != NULL && recordDlg->advancedNavigation() )
    {
        recordDlg->navigateBean(element->relatedBean().data(), openType);
    }
    else
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        QString contextName = QUuid::createUuid().toString();
        QPointer<DBRecordDlg> dlg = new DBRecordDlg(element->relatedBean().data(), openType, d->m_useNewContext, this);
        QApplication::restoreOverrideCursor();
        if ( dlg->openSuccess() && dlg->init() )
        {
            // Guardar los datos de los hijos agregados, será responsabilidad del bean padre
            // que se está editando
            dlg->setModal(true);
            dlg->exec();
        }
        delete dlg;
    }
}
