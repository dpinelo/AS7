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
#include "dbabstractfilterview.h"
#include "dbabstractfilterview_p.h"
#include "ui_dbabstractfilterview.h"
#include "configuracion.h"
#include "globales.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/backgrounddao.h"
#include "models/filterbasebeanmodel.h"
#include "models/basebeanmodel.h"
#include "models/treebasebeanmodel.h"
#include "widgets/dbtableviewcolumnorderform.h"
#include "widgets/dbabstractview.h"
#include "widgets/dbfullscheduleview.h"
#include "widgets/dbscheduleview.h"
#include "widgets/dbtreeview.h"
#include "scripts/perpscript.h"

DBAbstractFilterView::DBAbstractFilterView(QWidget *parent) :
    QWidget(parent), ui(new Ui::DBAbstractFilterView), d(new DBAbstractFilterViewPrivate(this))
{
    ui->setupUi(this);
    ui->cbFastFilterValue->setVisible(false);
    ui->deFastFilter1->setSpecialValueText(trUtf8("Seleccione fecha"));
    ui->deFastFilter2->setSpecialValueText(trUtf8("Seleccione fecha"));
    ui->deFastFilter1->setDisplayFormat(CommonsFunctions::dateFormat());
    ui->deFastFilter2->setDisplayFormat(CommonsFunctions::dateFormat());
    ui->deFastFilter1->setDate(alephERPSettings->minimumDate());
    ui->deFastFilter2->setDate(QDate::currentDate());
    // IMPORTANTE NO INICIAR AQUI. Se inicia cuando se necesite, ya que si no, nos metemos en bucles recursivos
    // d->m_engine = new AERPScript(this);

    connect (ui->leFastFilter, SIGNAL(textChanged(const QString &)), this, SLOT(fastFilterByText()));
    connect (ui->cbFastFilter, SIGNAL(currentIndexChanged(int)), this, SLOT(cbFastFilterIndexChanged(int)));
    connect (ui->cbFastFilterValue, SIGNAL(currentIndexChanged(int)), this, SLOT(fastFilterByCombo(int)));
    connect (ui->deFastFilter1, SIGNAL(dateChanged(QDate)), this, SLOT(fastFilterByDate()));
    connect (ui->deFastFilter2, SIGNAL(dateChanged(QDate)), this, SLOT(fastFilterByDate()));
    connect (ui->pbSort, SIGNAL(clicked()), this, SLOT(sortForm()));
    connect (ui->cbFastFilterOperators, SIGNAL(currentIndexChanged(int)), this, SLOT(prepareFilterControlsByOperator()));
    connect (ui->leFastFilter1, SIGNAL(textChanged(const QString &)), this, SLOT(fastFilterByNumbers()));
    connect (ui->leFastFilter2, SIGNAL(textChanged(const QString &)), this, SLOT(fastFilterByNumbers()));
    connect (BackgroundDAO::instance(), SIGNAL(queryExecuted(QString,bool)), this, SLOT(subTotalNewData(QString,bool)));
    connect (ui->leFastFilter, SIGNAL(returnPressed()), this, SIGNAL(fastFilterReturnPressed()));
    connect (ui->leFastFilter1, SIGNAL(returnPressed()), this, SIGNAL(fastFilterReturnPressed()));
    connect (ui->leFastFilter2, SIGNAL(returnPressed()), this, SIGNAL(fastFilterReturnPressed()));

    ui->leFastFilter->installEventFilter(this);
}

DBAbstractFilterView::~DBAbstractFilterView()
{
    saveStrongFilterWidgetStatus();
    delete d;
    delete ui;
}

QString DBAbstractFilterView::tableName() const
{
    return d->m_tableName;
}

BaseBeanMetadata *DBAbstractFilterView::metadata()
{
    return d->m_metadata;
}

void DBAbstractFilterView::setTableName (const QString &tableName)
{
    d->m_tableName = tableName;
    if ( d->m_itemViewIface != NULL )
    {
        d->m_itemViewIface->setTableName(tableName);
    }
    if ( !d->m_itemView.isNull() )
    {
        QString objName = QString("%1_dbFilterTableView_%2").
                          arg(d->m_itemView->metaObject()->className()).
                          arg(tableName);
        d->m_itemView->setObjectName(objName);
        d->m_metadata = BeansFactory::metadataBean(tableName);
        if ( d->m_metadata == NULL )
        {
            QLogger::QLog_Error(AlephERP::stLogOther, trUtf8("Su aplicación no está bien configurada. Existen tablas de sistemas no creadas. Consulte con Aleph Sistemas de Información."));
            return;
        }
        if ( !d->m_metadata->viewOnGrid().isEmpty() )
        {
            d->m_metadata = BeansFactory::metadataBean(d->m_metadata->viewOnGrid());
            if ( d->m_metadata == NULL )
            {
                QLogger::QLog_Error(AlephERP::stLogOther, trUtf8("Su aplicación no está bien configurada. Existen tablas de sistemas no creadas. Consulte con Aleph Sistemas de Información."));
                return;
            }
        }
    }
}

FilterBaseBeanModel *DBAbstractFilterView::filterModel()
{
    return d->m_modelFilter;
}

BaseBeanModel *DBAbstractFilterView::sourceModel()
{
    return d->m_model;
}

void DBAbstractFilterView::setSourceModel(BaseBeanModel *mdl)
{
    if ( !d->m_model.isNull() )
    {
        delete d->m_model;
    }
    if ( !d->m_modelFilter.isNull() )
    {
        delete d->m_modelFilter;
    }
    d->m_model = mdl;
    d->m_modelFilter = new FilterBaseBeanModel(this);
    d->m_modelFilter->setAccessFilter('r');
    d->m_modelFilter->setSourceModel(d->m_model);
    d->m_modelFilter->setFilterCaseSensitivity(Qt::CaseInsensitive);
}

QItemSelectionModel * DBAbstractFilterView::selectionModel()
{
    if ( d->m_itemView->inherits("DBFullScheduleView") )
    {
        DBFullScheduleView *wid = qobject_cast<DBFullScheduleView *>(d->m_itemView);
        if ( wid != NULL )
        {
            DBScheduleView *scheduleView = wid->dbScheduleView();
            if ( scheduleView != NULL )
            {
                return scheduleView->selectionModel();
            }
        }
    }
    return d->m_itemView->selectionModel();
}

QAbstractItemView *DBAbstractFilterView::itemView()
{
    return d->m_itemView.data();
}

void DBAbstractFilterView::setItemView(QAbstractItemView *view)
{
    if ( !d->m_itemView.isNull() )
    {
        delete d->m_itemView;
    }
    d->m_itemView = view;
    d->m_itemViewIface = dynamic_cast<DBAbstractViewInterface *>(d->m_itemView.data());
    QTableView *tv = qobject_cast<QTableView *>(view);
    if ( tv != NULL )
    {
        tv->setSortingEnabled(true);
        connect(tv->horizontalHeader(), SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(saveModelOrder()));
        connect(tv->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SIGNAL(columnHeaderClicked(int)));
    }
    QTreeView *tree = qobject_cast<QTreeView *>(view);
    if ( tree != NULL )
    {
        // En los modelos en árbol no vamos a activar la ordenación todavía...
        // tree->setSortingEnabled(true);
        connect(tree->header(), SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(saveModelOrder()));
        connect(tree->header(), SIGNAL(sectionClicked(int)), this, SIGNAL(columnHeaderClicked(int)));
    }
}

void DBAbstractFilterView::setFilterLevel(int value)
{
    d->m_filterLevel = value;
}

void DBAbstractFilterView::buildFilterWhere(const QString &aditionalSql)
{
    d->m_whereFilter = d->buildFilterWhere(aditionalSql);
}

QString DBAbstractFilterView::initSortForModel()
{
    return d->initSortForModel();
}

/**
 * @brief DBAbstractFilterView::initOrderedColumn
 * Devuelve la columna de ordenación inicial que tiene el widget justo al abrirse.
 * @return
 */
QString DBAbstractFilterView::initOrderedColumn()
{
    return d->initOrderedColumn();
}

/**
 * @brief DBAbstractFilterView::initOrderedColumnSort
 * Devuelve el orden de la columna de ordenación inicial del widget justo al abrirse
 * @return
 */
QString DBAbstractFilterView::initOrderedColumnSort()
{
    return d->initOrderedColumnSort();
}

void DBAbstractFilterView::addFieldsCombo()
{
    d->addFieldsCombo();
}

DBAbstractViewInterface *DBAbstractFilterView::itemViewIface()
{
    if ( d->m_itemView.isNull() )
    {
        return NULL;
    }
    DBAbstractViewInterface *iface = dynamic_cast<DBAbstractViewInterface *>(d->m_itemView.data());
    return iface;
}

QString DBAbstractFilterView::whereFilter() const
{
    return d->m_whereFilter;
}

void DBAbstractFilterView::setWhereFilter(const QString &value)
{
    d->m_whereFilter = value;
}

bool DBAbstractFilterView::largeResultSets()
{
    return d->m_largeResultSets;
}

void DBAbstractFilterView::setLargeResultSets(bool value)
{
    d->m_largeResultSets = value;
}

/*!
  Destruye los combobox que hubiese de filtros fuertes
  */
void DBAbstractFilterView::destroyStrongFilter(const QString &dbFieldName)
{
    d->destroyStrongFilter(dbFieldName);
}

void DBAbstractFilterView::createStrongFilter()
{
    d->createStrongFilter();
}

void DBAbstractFilterView::createSubTotals()
{
    d->createSubTotals();
}

void DBAbstractFilterView::calculateSubTotals()
{
    d->calculateSubTotals();
}

/**
 * @brief DBAbstractFilterView::saveStrongFilterWidgetStatus
 * Almacena el estado de los filtros SQL para una mejor experiencia de usuario.
 */
void DBAbstractFilterView::saveStrongFilterWidgetStatus()
{
    QList<QComboBox *> list = findChildren<QComboBox *>(QRegExp("cbStrongFilter.+"));
    foreach ( QComboBox *cb, list )
    {
        if ( cb->currentIndex() != -1 )
        {
            QString key = QString("%1%2").arg(d->m_tableName).arg(cb->objectName());
            alephERPSettings->saveRegistryValue(key, cb->itemData(cb->currentIndex()));
        }
    }
}

void DBAbstractFilterView::saveState()
{
    if ( !d->m_onInit && d->m_itemViewIface )
    {
        d->m_itemViewIface->saveState();
    }
}

void DBAbstractFilterView::restoreState()
{
    if ( d->m_itemViewIface )
    {
        d->m_itemViewIface->restoreState();
    }
}

void DBAbstractFilterView::enableRestoreSaveState()
{
    d->m_restoreStateEnabled = true;
    d->m_itemViewIface->enableRestoreSaveState();
}

void DBAbstractFilterView::disableRestoreSaveState()
{
    d->m_restoreStateEnabled = false;
    d->m_itemViewIface->disableRestoreSaveState();
}

bool DBAbstractFilterView::isRestoreSaveStateEnabled()
{
    return d->m_restoreStateEnabled;
}

void DBAbstractFilterView::reSort()
{
    if ( filterModel() )
    {
        filterModel()->invalidate();
        int sortColumn = 0;
        Qt::SortOrder sort = Qt::AscendingOrder;
        QTableView *tv = qobject_cast<QTableView *>(d->m_itemView);
        if ( tv != NULL )
        {
            sortColumn = tv->horizontalHeader()->sortIndicatorSection();
            sort = tv->horizontalHeader()->sortIndicatorOrder();
        }
        QTreeView *tree = qobject_cast<QTreeView *>(d->m_itemView);
        if ( tree != NULL )
        {
            sortColumn = tree->header()->sortIndicatorSection();
            sort = tree->header()->sortIndicatorOrder();
        }
        filterModel()->sort(sortColumn, sort);
    }
}

/*!
    Slot que realiza un filtrado rápido de los registros presentados, en función de lo que el
    usuario haya introducido en los camposera. Este filtrado se realiza a partir del objeto Filter definido.
    Pero si el número de registros del modelo es muy grande, se realiza un filtrado fuerte
*/
void DBAbstractFilterView::fastFilterByText()
{
    QVariantMap filter = d->m_modelFilter->filter().value(-1);
    QString textFilter = ui->leFastFilter->text();

    /* Si no hay filtro establecido no se hace nada */
    if ( filter.isEmpty() && textFilter.isEmpty() && !ui->leFastFilter->property(AlephERP::stFilterHistory).isValid() )
    {
        return;
    }
    if ( ui->cbFastFilter->currentIndex() == -1 )
    {
        QMessageBox::warning(this,qApp->applicationName(),
                             trUtf8(MSG_NO_COLUMN_SELECCIONADA), QMessageBox::Ok);
    }
    else
    {
        DBTreeView *tv = qobject_cast<DBTreeView *>(d->m_itemView);
        if ( !d->historyContainsFilter(ui->leFastFilter, textFilter) && !d->isEvolutionFilter(ui->leFastFilter, textFilter) )
        {
            d->m_model->setWhere("", d->m_order);
        }
        int rowCount = d->m_model->totalRecordCount();
        bool strongFilter = d->filterTypeToApply(ui->leFastFilter, textFilter, rowCount, filter.count());
        QString dbFieldName = ui->cbFastFilter->itemData(ui->cbFastFilter->currentIndex()).toString();
        if ( tv != NULL )
        {
            tv->setViewIntermediateNodesWithoutChildren(ui->leFastFilter->text().isEmpty());
        }
        // Si el filtro es fuerte, o si la búsqueda es totalmente nueva...
        if ( strongFilter )
        {
            filterWithSql();
        }
        d->m_modelFilter->removeFilterKeyColumn(dbFieldName, d->m_filterLevel, false);
        d->m_modelFilter->setFilterKeyColumn(dbFieldName, ui->leFastFilter->text(), "=", d->m_filterLevel);
        d->calculateSubTotals();
        if ( tv != NULL )
        {
            tv->expandFirstBranchWithData();
        }
    }
}

/*!
    Slot que realiza un filtrado rápido de los registros presentados, en función de lo que el
    usuario haya introducido en los campos. Este filtrado se realiza a partir del objeto Filter definido.
    Pero si el número de registros del modelo es muy grande, se realiza un filtrado fuerte
*/
void DBAbstractFilterView::fastFilterByNumbers()
{
    QVariantMap filter = d->m_modelFilter->filter().value(-1);
    QString textFilter = ui->leFastFilter1->text();
    QString textFilter2 = ui->leFastFilter2->text();
    QLineEdit *le = qobject_cast<QLineEdit *>(QObject::sender());
    if ( le == NULL )
    {
        return;
    }
    QString modifiedText = le->text();

    /* Si no hay filtro establecido no se hace nada */
    if ( filter.isEmpty() && textFilter.isEmpty() && !le->property(AlephERP::stFilterHistory).isValid() )
    {
        return;
    }
    if ( ui->cbFastFilterOperators->currentIndex() == CB_OPERATOR_BETWEEN && (textFilter.isEmpty() || textFilter2.isEmpty()) )
    {
        return;
    }
    if ( ui->cbFastFilter->currentIndex() == -1 )
    {
        QMessageBox::warning(this,qApp->applicationName(),
                             trUtf8(MSG_NO_COLUMN_SELECCIONADA), QMessageBox::Ok);
    }
    else
    {
        // Con números no tiene sentido el histórico, por lo que directamente se elimina el anterior filtro fuerte.
        d->m_model->setWhere("", d->m_order);
        int rowCount = d->m_model->rowCount();
        bool strongFilter = d->filterTypeToApply(QObject::sender(), modifiedText, rowCount, filter.count());
        QString dbFieldName = ui->cbFastFilter->itemData(ui->cbFastFilter->currentIndex()).toString();
        if ( strongFilter || ui->cbFastFilterOperators->currentIndex() == CB_OPERATOR_BETWEEN )
        {
            d->m_modelFilter->removeFilterKeyColumn(dbFieldName, d->m_filterLevel, false);
            filterWithSql();
        }
        else
        {
            double v1 = alephERPSettings->locale()->toDouble(ui->leFastFilter1->text());
            double v2 = alephERPSettings->locale()->toDouble(ui->leFastFilter2->text());
            if ( ui->cbFastFilterOperators->currentIndex() != CB_OPERATOR_BETWEEN && !textFilter.isEmpty() && !textFilter2.isEmpty() )
            {
                d->m_modelFilter->setFilterKeyColumnBetween(dbFieldName, v1, v2, d->m_filterLevel);
            }
            else
            {
                QString op;
                if ( ui->cbFastFilterOperators->currentIndex() == CB_OPERATOR_EQUAL )
                {
                    op = "=";
                }
                else if ( ui->cbFastFilterOperators->currentIndex() == CB_OPERATOR_LESS )
                {
                    op = "<";
                }
                else if ( ui->cbFastFilterOperators->currentIndex() == CB_OPERATOR_MORE )
                {
                    op = ">";
                }
                else if ( ui->cbFastFilterOperators->currentIndex() == CB_OPERATOR_DISTINCT )
                {
                    op = "!=";
                }
                d->m_modelFilter->setFilterKeyColumn(dbFieldName, v1, op, d->m_filterLevel);
            }
        }
        d->calculateSubTotals();
        QTreeView *tv = qobject_cast<QTreeView *>(d->m_itemView);
        if ( tv != NULL )
        {
            tv->expandAll();
        }
    }
}

/*!
  Se filtra por el valor del combo
  */
void DBAbstractFilterView::fastFilterByCombo(int index)
{
    QVariant data = ui->cbFastFilterValue->itemData(index);
    if ( data.isValid() && ui->cbFastFilter->currentIndex() != -1 )
    {
        QString dbFieldName = ui->cbFastFilter->itemData(ui->cbFastFilter->currentIndex()).toString();
        if ( d->m_largeResultSets )
        {
            filterWithSql();
        }
        d->m_modelFilter->removeFilterKeyColumn(dbFieldName, d->m_filterLevel, false);
        DBFieldMetadata *fld = d->dbFieldSelectedOnCombo();
        if ( fld->type() == QVariant::Bool )
        {
            QString val = data.toString();
            if ( !val.isEmpty() )
            {
                bool v = val == "true" ? true : false;
                d->m_modelFilter->setFilterKeyColumn(dbFieldName, v, "=", d->m_filterLevel);
            }
        }
        else
        {
            d->m_modelFilter->setFilterKeyColumn(dbFieldName, data, "=", d->m_filterLevel);
        }
        d->calculateSubTotals();
    }
}

/*!
    Slot que realiza un filtrado rápido de los registros presentados, en función de lo que el
    usuario haya introducido en los campos de fecha. Este filtrado se realiza a partir del objeto Filter definido.
*/
void DBAbstractFilterView::fastFilterByDate()
{
    if ( ui->deFastFilter1->date().isNull() || !ui->deFastFilter1->date().isValid() )
    {
        return;
    }
    else
    {
        QString dbFieldName = ui->cbFastFilter->itemData(ui->cbFastFilter->currentIndex()).toString();
        if ( d->m_largeResultSets )
        {
            filterWithSql();
        }
        QDate date1 = ui->deFastFilter1->date();
        QDate date2 = ui->deFastFilter2->date();
        if ( date2.isNull() || !date2.isValid() )
        {
            date2 = date1;
        }
        d->m_modelFilter->removeFilterKeyColumn(dbFieldName, d->m_filterLevel, false);
        d->m_modelFilter->setFilterKeyColumnBetween(dbFieldName, date1, date2, d->m_filterLevel);
        d->calculateSubTotals();
    }
}

void DBAbstractFilterView::clearFilter()
{
    QString dbFieldName = ui->cbFastFilter->itemData(ui->cbFastFilter->currentIndex()).toString();
    d->m_modelFilter->removeFilterKeyColumn(dbFieldName, d->m_filterLevel);
    ui->leFastFilter1->clear();
    ui->leFastFilter1->setProperty(AlephERP::stFilterHistory, QVariant());
    ui->leFastFilter2->clear();
    ui->leFastFilter2->setProperty(AlephERP::stFilterHistory, QVariant());
    ui->leFastFilter->clear();
    ui->leFastFilter->setProperty(AlephERP::stFilterHistory, QVariant());
    d->m_order.clear();
    d->calculateSubTotals();
}

/*!
  Realiza un filtro fuerte sobre el modelo teniendo en cuenta, no sólo los filtros fuertes
  que hubiese, si no además la información que el usuario ha metido ya que se supone
  que la búsqueda sería muy pesada.
  Esta función se invoca cuando se cambia el item en el cbFastFilter
  */
void DBAbstractFilterView::filterWithSql()
{
    QString aditionalSql;
    DBFieldMetadata *fld = d->dbFieldSelectedOnCombo();
    if ( fld != NULL )
    {
        if ( !fld->optionsList().isEmpty() )
        {
            QString val = ui->cbFastFilterValue->itemData(ui->cbFastFilterValue->currentIndex()).toString();
            if ( !val.isEmpty() )
            {
                aditionalSql = fld->sqlWhere("=", val);
            }
        }
        else
        {
            if ( fld->type() == QVariant::Bool )
            {
                QString val = ui->cbFastFilterValue->itemData(ui->cbFastFilterValue->currentIndex()).toString();
                if ( !val.isEmpty() )
                {
                    bool v = val == "true" ? true : false;
                    aditionalSql = fld->sqlWhere("=", v);
                }
            }
            else if ( fld->type() == QVariant::String )
            {
                if ( !ui->leFastFilter->text().isEmpty() )
                {
                    aditionalSql = QString("UPPER(%1) LIKE UPPER('%%%2%%')").arg(fld->dbFieldName()).arg(fld->sqlValue(ui->leFastFilter->text(), false));
                }
            }
            else if ( fld->type() == QVariant::Date || fld->type() == QVariant::DateTime )
            {
                aditionalSql = QString("%1 BETWEEN %2 AND %3").arg(fld->dbFieldName()).
                               arg(fld->sqlValue(ui->deFastFilter1->date())).arg(fld->sqlValue(ui->deFastFilter2->date()));
            }
            else if ( fld->type() == QVariant::Int || fld->type() == QVariant::Double )
            {
                if ( !ui->leFastFilter1->text().isEmpty() || !ui->leFastFilter2->text().isEmpty() )
                {
                    double v1 = alephERPSettings->locale()->toDouble(ui->leFastFilter1->text());
                    double v2 = alephERPSettings->locale()->toDouble(ui->leFastFilter2->text());
                    if ( !(v1 == 0 && v2 == 0) )
                    {
                        if ( ui->cbFastFilterOperators->currentIndex() == CB_OPERATOR_EQUAL )
                        {
                            aditionalSql = fld->sqlWhere("=", v1);
                        }
                        else if ( ui->cbFastFilterOperators->currentIndex() == CB_OPERATOR_BETWEEN )
                        {
                            aditionalSql = QString("%1 BETWEEN %2 AND %3").arg(fld->dbFieldName()).
                                           arg(fld->sqlValue(v1)).arg(fld->sqlValue(v2));
                        }
                        else if ( ui->cbFastFilterOperators->currentIndex() == CB_OPERATOR_LESS )
                        {
                            aditionalSql = fld->sqlWhere("<", v1);
                        }
                        else if ( ui->cbFastFilterOperators->currentIndex() == CB_OPERATOR_MORE )
                        {
                            aditionalSql = fld->sqlWhere(">", v1);
                        }
                        else if ( ui->cbFastFilterOperators->currentIndex() == CB_OPERATOR_DISTINCT )
                        {
                            aditionalSql = fld->sqlWhere("<>", v1);
                        }
                    }
                }
            }
        }
    }
    d->m_whereFilter = d->buildFilterWhere(aditionalSql);
    CommonsFunctions::setOverrideCursor(QCursor(Qt::WaitCursor));
    d->m_model->setWhere(d->m_whereFilter, d->m_order);
    // No es necesario el invalidate, ya que los modelos harán un beginResetModel y endResetModel que provocará el invalidate.
    // d->m_modelFilter->invalidate();
    CommonsFunctions::restoreOverrideCursor();
}

/*!
    Cuando el combo de filtro rápido cambia, se actualizan los controles que se presenta en el toolbar, se aplican
    las máscaras adecuados y se reordena el table view poniendo como primera columna la seleccionada
    por el usuario y ordenando por esa columna...
 */
void DBAbstractFilterView::cbFastFilterIndexChanged(int index)
{
    if ( index == -1 )
    {
        return;
    }
    int modelIndex = -1;
    QList<DBFieldMetadata *> flds = d->m_modelFilter->visibleFields();
    foreach (DBFieldMetadata *fld, flds)
    {
        modelIndex++;
        if ( fld->dbFieldName() == ui->cbFastFilter->itemData(index).toString() )
        {
            break;
        }
    }
    DBFieldMetadata *fldOnCombo = flds.at(modelIndex);
    if ( d->m_itemView->inherits("QTableView") )
    {
        QTableView *tv = qobject_cast<QTableView *>(d->m_itemView);
        if ( tv != NULL )
        {
            int visibleIndex = tv->horizontalHeader()->visualIndex(modelIndex);
            tv->horizontalHeader()->moveSection(visibleIndex, 0);
        }
    }
    else if ( d->m_itemView->inherits("QTreeView") )
    {
        /*
         * No vamos a permitir que se muevan columnas en los TreeView. La razón es que el span de las filas
         * se ve raro (se ven los elemantos span en la columna de en medio
        QTreeView *tv = qobject_cast<QTreeView *>(d->m_itemView);
        int visibleIndex = tv->header()->visualIndex(modelIndex);
        tv->header()->moveSection(visibleIndex, 0);
        */
    }
    clearFilter();
    d->m_modelFilter->resetFilter();
    if ( fldOnCombo->isOnDb() )
    {
        d->m_order = fldOnCombo->dbFieldName() + " ASC";
    }
    else
    {
        QMessageBox::warning(this,
                             qApp->applicationName(),
                             tr("Este es un campo calculado. Las operaciones de ordenación y búsqueda por él pueden ser muy lentas. Por defecto, no se podrá ordenar por él."));
    }
    prepareFilterControlsAndFilter();
    if ( d->m_itemView->inherits("QTableView") )
    {
        QTableView *tv = qobject_cast<QTableView *>(d->m_itemView);
        if ( tv != NULL )
        {
            tv->sortByColumn(modelIndex, Qt::AscendingOrder);
            tv->horizontalHeader()->setSortIndicator(modelIndex, Qt::AscendingOrder);
        }
    }
    else if ( d->m_itemView->inherits("QTreeView") )
    {
        QTreeView *tv = qobject_cast<QTreeView *>(d->m_itemView);
        if ( tv != NULL )
        {
            tv->sortByColumn(modelIndex, Qt::AscendingOrder);
            tv->header()->setSortIndicator(modelIndex, Qt::AscendingOrder);
        }
    }
    d->calculateSubTotals();
    ui->leFastFilter->setFocus();
}

/*!
  Prepara los controles que se utilizarán en el filtro
  */
void DBAbstractFilterView::prepareFilterControlsAndFilter()
{
    if ( d->m_model == NULL )
    {
        return;
    }
    DBFieldMetadata *fld = d->dbFieldSelectedOnCombo();

    if ( fld != NULL )
    {
        if ( !fld->optionsList().isEmpty() || fld->type() == QVariant::Bool )
        {
            d->addOptionsCombo(fld);
            ui->cbFastFilterValue->setVisible(true);
            ui->deFastFilter1->setVisible(false);
            ui->deFastFilter2->setVisible(false);
            ui->lblDate1->setVisible(false);
            ui->lblDate2->setVisible(false);
            ui->leFastFilter->setVisible(false);
            ui->lblBetween->setVisible(false);
            ui->lblAnd->setVisible(false);
            ui->cbFastFilterOperators->setVisible(false);
            ui->leFastFilter1->setVisible(false);
            ui->leFastFilter2->setVisible(false);
            fastFilterByCombo(ui->cbFastFilterValue->currentIndex());
            setFocusProxy(ui->cbFastFilterValue);
        }
        else
        {
            ui->cbFastFilterValue->setVisible(false);
            if ( fld->type() == QVariant::Date || fld->type() == QVariant::DateTime )
            {
                ui->deFastFilter1->setVisible(true);
                ui->deFastFilter2->setVisible(true);
                ui->lblDate1->setVisible(true);
                ui->lblDate2->setVisible(true);
                ui->leFastFilter->setVisible(false);
                ui->lblBetween->setVisible(false);
                ui->lblAnd->setVisible(false);
                ui->cbFastFilterOperators->setVisible(false);
                ui->leFastFilter1->setVisible(false);
                ui->leFastFilter2->setVisible(false);
                fastFilterByDate();
                setFocusProxy(ui->deFastFilter1);
            }
            else
            {
                ui->leFastFilter->setInputMask("");
                ui->deFastFilter1->setVisible(false);
                ui->deFastFilter2->setVisible(false);
                ui->lblDate1->setVisible(false);
                ui->lblDate2->setVisible(false);
                if ( fld->type() == QVariant::String )
                {
                    ui->leFastFilter->setVisible(true);
                    ui->lblBetween->setVisible(false);
                    ui->lblAnd->setVisible(false);
                    ui->cbFastFilterOperators->setVisible(false);
                    ui->leFastFilter1->setVisible(false);
                    ui->leFastFilter2->setVisible(false);
                }
                else
                {
                    ui->leFastFilter->setVisible(false);
                    ui->cbFastFilterOperators->setVisible(true);
                    ui->cbFastFilterOperators->setCurrentIndex(0);
                    prepareFilterControlsByOperator();
                }
                fastFilterByText();
                setFocusProxy(ui->leFastFilter);
            }
        }
    }
}

/*!
  Según la opción seleccionada en el combo de operadores (entre, mayor, menor, igual...)
  se visualizarán algunos controles
  */
void DBAbstractFilterView::prepareFilterControlsByOperator()
{
    clearFilter();
    if ( ui->cbFastFilterOperators->currentText() == trUtf8("Entre") )
    {
        ui->lblBetween->setVisible(true);
        ui->lblAnd->setVisible(true);
        ui->leFastFilter2->setVisible(true);
    }
    else
    {
        ui->lblBetween->setVisible(false);
        ui->lblAnd->setVisible(false);
        ui->leFastFilter2->setVisible(false);
    }
    ui->leFastFilter1->setVisible(true);
    filterWithSql();
}

/**
 * @brief DBAbstractFilterView::saveModelOrder
 * Necesitamos saber qué ordenación se está aplicando. El usuario puede pinchar en las cabeceras para
 * establecer ordenaciones, y cuando posteriormente se aplica un filtro, esa ordenación debe conocerla el modelo.
 * Eso ya ocurre porque los DBTableView o DBTreeView informan a sus modelos de los cambios de ordenación internas
 * pero este widget DBAbstractFilterView también tiene que conocerlo al aplicar las sentencias setWhere
 */
void DBAbstractFilterView::saveModelOrder()
{
    if ( !d->m_model.isNull() )
    {
        d->m_order = d->m_model->internalOrderClausule();
    }
}

void DBAbstractFilterView::subTotalNewData(const QString &id, bool result)
{
    d->subTotalNewData(result, id);
}

void DBAbstractFilterView::allowUserRefresh()
{
    d->m_modelFilter->invalidate();
    ui->pbRefresh->setEnabled(true);
}

void DBAbstractFilterView::disableUserRefresh()
{
    ui->pbRefresh->setEnabled(false);
}

void DBAbstractFilterView::forceRefresh()
{
    d->m_modelFilter->refresh(true);
}

void DBAbstractFilterView::init(bool initStrongFilter)
{
    if ( d->m_model.isNull() || d->m_modelFilter.isNull() )
    {
        return;
    }
    d->m_onInit = true;
    /** Este tipo de controles que tiran directamente de base de datos, visualizan inicialmente
      todos los registros para una carga rápida */
    d->m_modelFilter->setDbStates(BaseBean::INSERT | BaseBean::UPDATE | BaseBean::TO_BE_DELETED);
    if ( d->m_model->rowCount() > alephERPSettings->strongFilterRowCountLimit() || initStrongFilter )
    {
        setLargeResultSets(true);
    }
    else
    {
        setLargeResultSets(false);
    }

    d->m_itemView->setModel(d->m_modelFilter);
    d->m_itemView->setDragDropMode(QAbstractItemView::DragOnly);
    d->m_itemView->setDragEnabled(true);

    d->addFieldsCombo();

    // Debe hacerse en este orden para que no se produzcan retrasos de ordenación.
    // Es importante indicar la columna por la que se ordene y que está ordenado
    // antes de introducir el modelo. El modelo aseguramos que viene en ese orden.
    // Lo que se garantiza con esto es que de entrada, FilterBaseBeanModel no ordene
    // y la ordenación la haga la base de datos.
    QList<DBFieldMetadata *> list = d->m_modelFilter->visibleFields();
    QString initOrderColumn = d->initOrderedColumn();
    QString initOrderColumnSort = d->initOrderedColumnSort();
    QTableView *tv = qobject_cast<QTableView *>(itemView());
    QStringList columnsOrder;

    if ( tv != NULL )
    {
        columnsOrder = alephERPSettings->viewColumnsOrder<QTableView>(tv);
        for ( int i = 0 ; i < list.size() ; i++ )
        {
            if ( list.at(i)->dbFieldName() == initOrderColumn )
            {
                bool blockState = tv->horizontalHeader()->blockSignals(true);
                tv->horizontalHeader()->setSortIndicator(i, initOrderColumnSort.toUpper() == "DESC" ? Qt::DescendingOrder : Qt::AscendingOrder);
                tv->horizontalHeader()->setSortIndicatorShown(true);
                tv->horizontalHeader()->blockSignals(blockState);
                break;
            }
        }
    }
    // En el caso de los modelos en árboles, la ordenación se hace aquí
    DBTreeView *tree = qobject_cast<DBTreeView *>(itemView());
    if ( tree != NULL )
    {
        columnsOrder = alephERPSettings->viewColumnsOrder<QTreeView>(tree);
        for ( int i = 0 ; i < list.size() ; i++ )
        {
            if ( list.at(i)->dbFieldName() == initOrderColumn )
            {
                // Hacemos esto porque debemos evitar que se salve el estado.
                tree->setIsOnInit(true);
                tree->header()->setSortIndicator(i, initOrderColumnSort.toUpper() == "DESC" ? Qt::DescendingOrder : Qt::AscendingOrder);
                tree->header()->setSortIndicatorShown(true);
                tree->setIsOnInit(false);
                break;
            }
        }
    }
    // Establecemos en el combobox de filtrado, como valor seleccionado, el de la columna con la marca de ordenación.
    int index = ui->cbFastFilter->currentIndex();
    if ( !columnsOrder.isEmpty() )
    {
        index = ui->cbFastFilter->findData(columnsOrder.at(0));
        if ( index == -1 )
        {
            index = ui->cbFastFilter->currentIndex();
        }
    }
    bool blockState = ui->cbFastFilter->blockSignals(true);
    ui->cbFastFilter->setCurrentIndex(index);
    prepareFilterControlsAndFilter();
    ui->cbFastFilter->blockSignals(blockState);
    d->calculateSubTotals();

    d->m_onInit = false;
    connect(d->m_modelFilter.data(), SIGNAL(initRefresh()), this, SLOT(disableUserRefresh()));
    connect(d->m_modelFilter.data(), SIGNAL(endRefresh()), this, SLOT(allowUserRefresh()));
    connect(ui->pbRefresh, SIGNAL(clicked()), this, SLOT(forceRefresh()));
}

/*!
  Esta función devuelve el bean actualmente seleccionado en el modelo.
  */
BaseBeanPointer DBAbstractFilterView::selectedBean()
{
    BaseBeanPointer bean;
    QModelIndex currentIndex = d->m_itemView->currentIndex();
    if ( currentIndex.isValid() && filterModel() )
    {
        bean = filterModel()->bean(currentIndex).data();
    }
    return bean;
}

void DBAbstractFilterView::setSelectedBean(const BaseBeanSharedPointer &bean)
{
    if ( bean.isNull() || d->m_model == NULL || d->m_modelFilter == NULL )
    {
        return;
    }
    QModelIndex filterIdx = d->m_modelFilter->indexByPk(bean->pkValue());
    QModelIndex firstIdx = d->m_modelFilter->index(filterIdx.row(), 0);
    QModelIndex lastIdx = d->m_modelFilter->index(filterIdx.row(), bean->fieldCount() - 1);
    QItemSelection selection(firstIdx, lastIdx);
    d->m_itemView->selectionModel()->select(selection, QItemSelectionModel::Select);
    d->m_itemView->setCurrentIndex(filterIdx);
    d->m_itemView->scrollTo(filterIdx, QAbstractItemView::EnsureVisible);
}

void DBAbstractFilterView::refresh()
{
    if ( d->m_model != NULL )
    {
        d->m_itemViewIface->saveState();
        d->m_model->refresh();
        if ( ui->leFastFilter->isVisible() )
        {
            fastFilterByText();
        }
        else if ( ui->cbFastFilterValue->isVisible() )
        {
            fastFilterByCombo(ui->cbFastFilterValue->currentIndex());
        }
        else if ( ui->deFastFilter1->isVisible() )
        {
            fastFilterByDate();
        }
        else if ( ui->leFastFilter1->isVisible() )
        {
            fastFilterByNumbers();
        }
        d->m_modelFilter->invalidate();
        d->m_itemViewIface->restoreState();
    }
}

void DBAbstractFilterView::sortForm()
{
    QStringList initialOrder, initialSort;
    QList<QPair<QString, QString> > result;
    QHeaderView *header;
    if ( d->m_itemView->inherits("QTableView") )
    {
        header = (qobject_cast<QTableView *>(d->m_itemView))->horizontalHeader();
        initialOrder = alephERPSettings->viewColumnsOrder<QTableView>(qobject_cast<QTableView *>(d->m_itemView));
        initialSort = alephERPSettings->viewColumnsSort<QTableView>(qobject_cast<QTableView *>(d->m_itemView));
    }
    else if ( d->m_itemView->inherits("QTreeView") )
    {
        header = (qobject_cast<QTreeView *>(d->m_itemView))->header();
        initialOrder = alephERPSettings->viewColumnsOrder<QTreeView>(qobject_cast<QTreeView *>(d->m_itemView));
        initialSort = alephERPSettings->viewColumnsSort<QTreeView>(qobject_cast<QTreeView *>(d->m_itemView));
    }
    else
    {
        return;
    }

    // Vamos a pasarle al formulario la ordenación actual.
    for (int i = 0; i < initialOrder.size() ; i++)
    {
        QPair<QString, QString> pair;
        if ( AERP_CHECK_INDEX_OK(i, initialSort) )
        {
            pair.first = initialOrder.at(i);
            pair.second = initialSort.at(i);
        }
        else
        {
            pair.first = initialOrder.at(i);
            pair.second = "ASC";
        }
        result.append(pair);
    }
    // Utilizando QPointer por http://blogs.kde.org/node/3919
    QPointer<DBTableViewColumnOrderForm> dlg = new DBTableViewColumnOrderForm(d->m_modelFilter,
                                                                              header,
                                                                              &result,
                                                                              this);
    dlg->setModal(true);
    dlg->exec();
    if ( result.size() > 0 )
    {
        QStringList order;
        QStringList orderSort;
        for ( int i = 0 ; i < result.size() ; i++ )
        {
            order << result[i].first;
            orderSort << result[i].second;
        }
        d->m_itemViewIface->orderColumns(order);
        d->m_itemViewIface->saveColumnsOrder(order, orderSort);
        delete d->m_modelFilter;
        delete d->m_model;

        init(false);
    }
    delete dlg;
}

void DBAbstractFilterView::freezeModel()
{
    if ( d->m_model != NULL )
    {
        d->m_model->freezeModel();
    }
}

/**
 * @brief DBAbstractFilterView::defrostModel
 * Descongela el modelo y además realiza un refresh.
 */
void DBAbstractFilterView::defrostModel()
{
    if ( d->m_model != NULL )
    {
        if ( d->m_model->isFrozenModel() )
        {
            d->m_model->defrostModel();
        }
        d->m_model->refresh();
    }
    /*
     * Este código tiene efectos fatales en la estabilidad del programa con modelos en árbol.
     * Aparte, no tendría que se necesario: En caso de un refresco, el modelo emite dataChanged, y
     * el filtro debería invalidarse solo
     * */
    /*
    if ( d->m_modelFilter && alephERPSettings->modelsRefresh() )
    {
        d->m_modelFilter->invalidate();
    }
    */
}

void DBAbstractFilterView::resizeRowsToContents()
{
}

/**
  Permite obtener de los filtros fuertes que se puede establecer en el formulario, el valor actualmente
  seleccionado o de filtro. Caso de estar seleccionada la opción "todos", devuelve un QVariant vacío.
  Como parámetro debe recibir el nombre de la columna de base de datos.
  */
QVariant DBAbstractFilterView::filterValue(const QString &dbFieldName)
{
    QVariant v;
    QList<QComboBox*> list = this->findChildren<QComboBox*>(QRegExp("cbStrongFilter.+"));
    if ( d->m_metadata == NULL )
    {
        return v;
    }
    DBFieldMetadata *fld = d->m_metadata->field(dbFieldName);
    foreach ( QComboBox *cb, list )
    {
        QString cbName = QString("cbStrongFilter%1").arg(dbFieldName);
        if ( cbName == cb->objectName() && cb->currentIndex() != -1 )
        {
            QString filter = cb->itemData(cb->currentIndex()).toString();
            QStringList filterParts = filter.split("=");
            if ( filterParts.size() > 1 )
            {
                v = fld->parseValue(filterParts.at(1));
                if ( fld->type() == QVariant::String )
                {
                    QString tmp = v.toString().trimmed();
                    if ( tmp.at(0) == '\'' && tmp.at(tmp.size()-1) == '\'' )
                    {
                        v = tmp.mid(1, tmp.size() - 2);
                    }
                }
            }
        }
    }
    return v;
}

bool DBAbstractFilterView::isFrozenModel() const
{
    if ( d->m_model != NULL )
    {
        return d->m_model->isFrozenModel();
    }
    return false;
}

bool DBAbstractFilterView::eventFilter(QObject *sender, QEvent *event)
{
    if ( sender == ui->leFastFilter && event->type() == QEvent::KeyPress )
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if ( keyEvent->key() == Qt::Key_Up ||
             keyEvent->key() == Qt::Key_Down ||
             keyEvent->key() == Qt::Key_PageUp ||
             keyEvent->key() == Qt::Key_PageDown )
        {
            emit fastFilterKeyPress(keyEvent->key());
        }
    }
    return QWidget::eventFilter(sender, event);
}
