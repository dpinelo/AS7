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
#include "configuracion.h"
#include "dao/beans/basebean.h"
#include "dbtableview.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/observerfactory.h"
#include "models/aerphtmldelegate.h"
#include "models/dbbasebeanmodel.h"
#include "models/relationbasebeanmodel.h"
#include "models/filterbasebeanmodel.h"
#include "models/basebeanmodel.h"
#include "models/aerpimageitemdelegate.h"
#include "models/aerpinlineedititemdelegate.h"
#include "models/aerpmoviedelegate.h"
#include "models/aerpitemdelegate.h"
#include "globales.h"
#include "business/aerpspreadsheet.h"
#include "dbnumberedit.h"

class DBTableViewPrivate
{
    Q_DECLARE_PUBLIC(DBTableView)
public:
    DBTableView *q_ptr;
    QString m_itemCheckBox;
    /** Se proporciona para garantizar la persistencia de los beans checkeados */
    BaseBeanSharedPointerList m_checkedBeans;
    bool m_canMoveRows;
    QPointer<AERPMovieDelegate> m_movieDelegate;
    // Evitar recursividad emitiendo algunas señales
    bool m_emittingShowContextMenu;

    DBTableViewPrivate(DBTableView *qq);
};

DBTableViewPrivate::DBTableViewPrivate (DBTableView *qq) : q_ptr(qq)
{
    m_canMoveRows = false;
    m_movieDelegate = new AERPMovieDelegate(qq, qq);
    m_emittingShowContextMenu = false;
}

DBTableView::DBTableView (QWidget * parent) :
    QTableView(parent),
    DBAbstractViewInterface(this, horizontalHeader()),
    AERPBackgroundAnimation(viewport()),
    d(new DBTableViewPrivate(this))
{
    setAutomaticName(true);

    setMouseTracking(true);

    connect(this, SIGNAL(enterPressedOnValidIndex(QModelIndex)), this, SLOT(nextCellOnEnter(QModelIndex)), Qt::QueuedConnection);
    connect(this, SIGNAL(clicked(QModelIndex)), this, SLOT(itemClicked(QModelIndex)));
    connect(m_hideColumn, SIGNAL(triggered()), this, SLOT(fromMenuHideColumn()));

#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
    m_header->setMovable(true);
#else
    m_header->setSectionsMovable(true);
#endif

    setContextMenuPolicy(Qt::CustomContextMenu);
    m_header->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_header, SIGNAL(sectionResized(int, int, int)), this, SLOT(saveHeaderSize(int, int, int)));
    connect(m_header, SIGNAL(sectionMoved(int,int,int)), this, SLOT(saveColumnsOrder()));
    connect(m_header, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(headerContextMenu(QPoint)));
    connect(m_header, SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(saveColumnsOrder()));
    connect(verticalHeader(), SIGNAL(clicked(QModelIndex)), this, SLOT(applyRowSpan()));
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

    m_header->installEventFilter(m_eventForwarder);
    connect(m_eventForwarder, SIGNAL(entered()), this, SLOT(resetCursor()));
    verticalHeader()->installEventFilter(m_eventForwarder);
}

DBTableView::~DBTableView()
{
    emit destroyed(this);
    delete d;
}

AlephERP::ObserverType DBTableView::observerType(BaseBean *bean)
{
    if ( bean == NULL )
    {
        return AlephERP::DbRelation;
    }
    DBObject *obj = bean->navigateThroughProperties(m_relationName);
    if ( obj != NULL )
    {
        return AlephERP::DbRelation;
    }
    return AlephERP::DbMultipleRelation;
}

void DBTableView::setRelationName(const QString &name)
{
    m_relationName = name;
    if ( automaticName() )
    {
        setObjectName (configurationName());
    }
}

void DBTableView::setTableName(const QString &name)
{
    m_tableName = name;
    if ( automaticName() )
    {
        setObjectName (configurationName());
    }
}

/*!
  Al mostrarse el control es cuando se crean los modelos y demás.
  */
void DBTableView::showEvent(QShowEvent *event)
{
    if ( !this->m_externalModel )
    {
        DBBaseWidget::showEvent(event);
        if ( !property(AlephERP::stInited).toBool() )
        {
            init(true);
            setProperty(AlephERP::stInited, true);
        }
        if ( filterModel() )
        {
            filterModel()->setReadOnlyColumns(m_readOnlyColumns);
            filterModel()->setVisibleFields(m_visibleColumns);
            filterModel()->setLinkColumns(m_linkColumns);
            if (!m_visibleColumns.isEmpty())
            {
                prepareColumns();
            }
        }
        setFieldCheckBox(d->m_itemCheckBox);
        if ( observer() != NULL )
        {
            if ( !observer()->readOnly() && dataEditable() )
            {
                setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked |
                                QAbstractItemView::AnyKeyPressed | QAbstractItemView::EditKeyPressed);
            }
        }
    }
}

bool DBTableView::init(bool onShowEvent)
{
    if ( !m_dataEditable )
    {
        m_readOnlyModel = true;
    }
    else
    {
        if ( !editTriggers().testFlag(QAbstractItemView::NoEditTriggers) )
        {
            m_readOnlyModel = false;
        }
        else
        {
            m_readOnlyModel = true;
        }
    }
    return DBAbstractViewInterface::init(onShowEvent);
}

void DBTableView::keyPressEvent(QKeyEvent * event)
{
    QModelIndex index;

    if ( event->matches(QKeySequence::Copy) )
    {
        copy();
    }
    else if ( event->matches(QKeySequence::Paste) )
    {
        paste();
    }
    else
    {
        if ( event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Tab )
        {
            index = currentIndex();
            if ( index.isValid() )
            {
                emit enterPressedOnValidIndex(index);
                event->accept();
                return;
            }
        }
        QTableView::keyPressEvent(event);
    }
}

void DBTableView::mouseDoubleClickEvent(QMouseEvent * event)
{
    QModelIndex index;

    if ( event->button() == Qt::LeftButton )
    {
        index = currentIndex();
        if ( index.isValid() )
        {
            emit doubleClickOnValidIndex( index );
            event->accept();
            return;
        }
    }
    QTableView::mouseDoubleClickEvent(event);
}

void DBTableView::mouseMoveEvent(QMouseEvent *event)
{
    DBAbstractViewInterface::mouseMoveEvent(event);
    QTableView::mouseMoveEvent(event);
}

/**
 * @brief DBTableView::viewportEvent
 * Vamos a presentar un tooltip cuando no se ve el contenido entero de la celda
 * @param event
 * @return
 */
bool DBTableView::viewportEvent(QEvent *event)
{
    if ( event->type() == QEvent::ToolTip )
    {
        QHelpEvent *helpEvent = static_cast<QHelpEvent*>(event);
        QModelIndex index = indexAt(helpEvent->pos());
        if ( index.isValid() && filterModel() != NULL )
        {
            BaseBeanSharedPointer bean = filterModel()->bean(index);
            if ( !bean.isNull() )
            {
                DBField *fld = bean->field(index.data(AlephERP::DBFieldNameRole).toString());
                if ( fld == NULL || !fld->hasToolTip() )
                {
                    QSize sizeHint = itemDelegate(index)->sizeHint(viewOptions(), index);
                    QRect rItem(0, 0, sizeHint.width(), sizeHint.height());
                    QRect rVisual = visualRect(index);
                    if (rItem.width() <= rVisual.width())
                    {
                        QToolTip::hideText();
                        return true;
                    }
                }
            }
        }
    }
    return QTableView::viewportEvent(event);
}

void DBTableView::paintEvent(QPaintEvent *event)
{
    paintAnimation(event);
    QTableView::paintEvent(event);
}

void DBTableView::closeEvent (QCloseEvent * event)
{
    saveColumnsOrder();
    QTableView::closeEvent(event);
}

bool DBTableView::setupInternalModel()
{
    bool r = DBAbstractViewInterface::setupInternalModel();
    if ( !m_metadata.isNull() && filterModel() )
    {
        prepareColumns();
    }
    if ( filterModel() )
    {
        filterModel()->setDbStates(visibleRecords());
        filterModel()->invalidate();
    }
    setCanMoveRows(d->m_canMoveRows);
    return r;
}

bool DBTableView::setupExternalModel()
{
    bool r = DBAbstractViewInterface::setupExternalModel();
    if ( !m_metadata.isNull() && filterModel() )
    {
        prepareColumns();
    }
    setCanMoveRows(d->m_canMoveRows);
    return r;
}

void DBTableView::setModel(QAbstractItemModel *mdl)
{
    // Importante hacer esto aquí para que la animación funcione correctamente!!
    hideAnimation();
    if ( mdl != NULL && mdl->property(AlephERP::stBaseBeanModel).toBool() )
    {
        if ( model() != NULL )
        {
            disconnect(model(), SIGNAL(initLoadingData()));
            disconnect(model(), SIGNAL(endLoadingData()));
        }
        // Animación en espera del widget
        setAnimation(":/generales/images/animatedWait.gif");
        connect(mdl, SIGNAL(endLoadingData()), this, SLOT(hideAnimation()));
        connect(mdl, SIGNAL(initLoadingData()), this, SLOT(showAnimation()));
        connect(mdl, SIGNAL(endLoadingData()), this, SLOT(applyRowSpan()));
    }
    if ( mdl == NULL )
    {
        return;
    }
    m_externalModel = true;
    setSourceModel(mdl);
    QTableView::setModel(filterModel());
    if ( filterModel() != NULL )
    {
        QAbstractItemModel *tmp = filterModel()->sourceModel();
        if ( tmp != NULL && tmp->property(AlephERP::stBaseBeanModel).toBool() )
        {
            BaseBeanModel *metadataModel = qobject_cast<BaseBeanModel *>(filterModel()->sourceModel());
            if ( metadataModel != NULL )
            {
                if ( QString(metadataModel->metaObject()->className()) != "RelationBaseBeanModel" )
                {
                    // Animación en espera de carga de los items... Pero ojo: Sólo si no hay un itemDelegate
                    for (int i = 0 ; i < metadataModel->columnCount() ; i++)
                    {
                        if ( itemDelegateForColumn(i) == NULL )
                        {
                            setItemDelegate(d->m_movieDelegate);
                        }
                    }
                }
                m_metadata = filterModel()->metadata();
                prepareColumns();
            }
        }
    }
    setCanMoveRows(d->m_canMoveRows);
    QScrollBar *sc = verticalScrollBar();
    if ( sc != NULL )
    {
        connect(sc, SIGNAL(actionTriggered(int)), this, SLOT(applyRowSpan()));
    }
}

void DBTableView::hideColumn(const QString &dbFieldName)
{
    FilterBaseBeanModel *mdl = qobject_cast<FilterBaseBeanModel *> (model());

    if ( m_metadata.isNull() || mdl == NULL )
    {
        return;
    }
    QList<DBFieldMetadata *> list = mdl->visibleFields();
    int i = 0;
    foreach (DBFieldMetadata *fld, list)
    {
        if ( fld->dbFieldName() == dbFieldName )
        {
            QTableView::hideColumn(i);
        }
        i++;
    }
}

void DBTableView::newRowOrder(int logicalIndex, int oldVisualIndex, int newVisualIndex)
{
    Q_UNUSED(oldVisualIndex)
    Q_UNUSED(logicalIndex)
    Q_UNUSED(newVisualIndex)

    if ( filterModel() == NULL )
    {
        return;
    }
    RelationBaseBeanModel *mdl = qobject_cast<RelationBaseBeanModel *> (filterModel()->sourceModel());
    if ( mdl != NULL )
    {
        for (int logicalIndex = 0 ; logicalIndex < mdl->rowCount() ; logicalIndex++)
        {
            int visualIndex = verticalHeader()->visualIndex(logicalIndex);
            mdl->setOrderRow(logicalIndex, visualIndex);
        }
    }
}

/*!
  Nos almacenará los anchos que el usuario haya seleccionado
  */
void DBTableView::saveHeaderSize(int logicalSection, int oldSize, int newSize)
{
    Q_UNUSED (logicalSection)
    if ( oldSize != newSize )
    {
        alephERPSettings->saveViewState<QTableView>(this);
    }
}

void DBTableView::nextCellOnEnter(const QModelIndex &actualCell)
{
    if ( state() == QAbstractItemView::EditingState )
    {
        AERPItemDelegate *delegate = qobject_cast<AERPItemDelegate *> (itemDelegateForColumn(actualCell.column()));
        if ( delegate != NULL && delegate->editor() != NULL )
        {
            commitData(delegate->editor());
        }
    }
    DBAbstractViewInterface::nextCellOnEnter(actualCell, moveCursor(QAbstractItemView::MoveNext, Qt::NoModifier));
}

/*!
Establece el valor a mostrar en el control
*/
void DBTableView::setValue(const QVariant &value)
{
    Q_UNUSED(value)
}

/*!
Devuelve el valor mostrado o introducido en el control
*/
QVariant DBTableView::value()
{
    return QVariant();
}

/*!
 Ajusta el control y sus propiedades a lo definido en el field
*/
void DBTableView::applyFieldProperties()
{
    prepareColumns();
    if ( !dataEditable() )
    {
        setFocusPolicy(Qt::NoFocus);
        if ( qApp->focusWidget() == this )
        {
            focusNextChild();
        }
    }
    else
    {
        setFocusPolicy(Qt::StrongFocus);
    }
}

/*!
  Para refrescar los controles: Piden nuevo observador si es necesario
*/
void DBTableView::refresh()
{
    if ( m_observer.isNull() )
    {
        observer();
        if ( !m_observer.isNull() )
        {
            init(true);
        }
    }
    if ( filterModel() )
    {
        filterModel()->invalidate();
    }
}

/*!
  Añade un bean al modelo de esta vista. Para ello, se debe estar en modelo datos internos
  y el bean debe ser del mismo tipo que los metadatos que presenta este widget
  */
BaseBeanSharedPointer DBTableView::addBean()
{
    BaseBeanSharedPointer bean;
    if ( m_relationName.isEmpty() )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, tr("DBTableView::deleteSelectedsBean: LLamada a esta función sin estar configurada para usar datos internos, es decir, sin haber indicado un relationName."));
    }
    else
    {
        if ( filterModel() == NULL )
        {
            return bean;
        }
        int row = filterModel()->rowCount();
        if ( filterModel()->insertRow(row) )
        {
            bean = filterModel()->bean(row);
        }
        else
        {
            if ( !filterModel()->property(AlephERP::stLastErrorMessage).toString().isEmpty() )
            {
                QMessageBox::warning(this, qApp->applicationName(), trUtf8("Ha ocurrido un error al intentar agregar un registro. \nEl error es: %1").arg(filterModel()->property(AlephERP::stLastErrorMessage).toString()));
                filterModel()->setProperty(AlephERP::stLastErrorMessage, "");
            }
        }
        filterModel()->invalidate();
    }
    return bean;
}

/*!
  Marca los beans seleccionados para ser borrados. El widget debe estar en modo datos internos
  */
void DBTableView::deleteSelectedsBean()
{
    QModelIndexList list = selectionModel()->selectedIndexes();
    QList<int> deleteRows;

    if ( m_relationName.isEmpty() )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, tr("DBTableView::deleteSelectedsBean: LLamada a esta función sin estar configurada para usar datos internos, es decir, sin haber indicado un relationName."));
        return;
    }
    if ( filterModel() )
    {
        foreach ( QModelIndex index, list )
        {
            if ( !deleteRows.contains(index.row()) )
            {
                filterModel()->removeRow(index.row());
                deleteRows << index.row();
            }
        }
        filterModel()->invalidate();
    }
}

/*!
  Ordena las columnas de visualización según el hash indicado. El primer elemento
  es el índice del field en los metadatos de BaseBean. El segundo int es el lugar
  en el que debe aparecer
  */
void DBTableView::orderColumns(const QStringList &order)
{
    if ( m_metadata.isNull() || filterModel() == NULL )
    {
        return;
    }

    QHeaderView *header = this->horizontalHeader();
    QList<DBFieldMetadata *> list = filterModel()->visibleFields();
    for ( int i = 0 ; i < order.size() ; i++ )
    {
        int logicalIndex = -1;
        for ( int iField = 0 ; iField < list.size() ; iField ++ )
        {
            if ( list.at(iField)->dbFieldName() == order.at(i) )
            {
                logicalIndex = iField;
            }
        }
        int visualIndex = header->visualIndex(logicalIndex);
        header->moveSection(visualIndex, i);
    }
}

/*!
  Almacena en registro el orden y tipo de ordenación de las columnas del DBTableView
  */
void DBTableView::saveColumnsOrder(const QStringList &order, const QStringList &sort)
{
    alephERPSettings->saveViewColumnsOrder<QTableView>(this, order, sort);
}

void DBTableView::saveColumnsOrder()
{
    DBAbstractViewInterface::saveColumnsOrder();
}

void DBTableView::observerUnregistered()
{
    DBAbstractViewInterface::observerUnregistered();
    bool blockState = blockSignals(true);
    setModel(NULL);
    reset();
    blockSignals(blockState);
}

/*!
  Si hay una columna con checkbox, permite marcarla o desmarcarla entera...
  */
void DBTableView::checkAllItems(bool checked)
{
    FilterBaseBeanModel *mdlFilter = qobject_cast<FilterBaseBeanModel *>(model());
    if ( mdlFilter == NULL )
    {
        return;
    }
    BaseBeanModel *mdl = qobject_cast<BaseBeanModel *>(mdlFilter->sourceModel());
    if ( mdl == NULL )
    {
        return;
    }
    mdl->checkAllItems(checked);
}

void DBTableView::itemClicked(const QModelIndex &idx)
{
    DBAbstractViewInterface::itemClicked(idx);
}

/*!
  Devuelve un listado de los beans que han sido checkeados por el usuario
  */
QScriptValue DBTableView::checkedBeans()
{
    if ( engine() == NULL )
    {
        return QScriptValue();
    }
    QScriptValue array = engine()->newArray();

    FilterBaseBeanModel *mdlFilter = qobject_cast<FilterBaseBeanModel *>(model());
    if ( mdlFilter == NULL )
    {
        return array;
    }
    BaseBeanModel *mdl = qobject_cast<BaseBeanModel *>(mdlFilter->sourceModel());
    if ( mdl == NULL )
    {
        return array;
    }
    QModelIndexList checkedItems = mdl->checkedItems();
    int index = 0;
    d->m_checkedBeans.clear();
    d->m_checkedBeans = mdl->beans(checkedItems);
    foreach ( BaseBeanSharedPointer bean, d->m_checkedBeans )
    {
        if ( !bean.isNull() )
        {
            QScriptValue scriptBean = engine()->newQObject(bean.data(), QScriptEngine::QtOwnership);
            array.setProperty(index, scriptBean);
            index++;
        }
    }
    return array;
}

void DBTableView::setCheckedBeansByPk(QVariantList list, bool checked)
{
    // TODO: Hay codigo similar hecho para DBListView. Lo suyo sería hacerlo común en DBAbstractView
    Q_UNUSED(list)
    Q_UNUSED(checked)
}

/*!
  Devuelve un listado de los beans que han sido checkeados por el usuario.
  Esta función puede ser llamada desde Javascript, en el constructor del widget. En ese
  caso los modelos aún no se han creado, por lo que se guardarán en una estructura
  intermedia las primary keys de los beans pasados. No se guardan los beans, porque
  estos pueden haber sido borrados previamente por el motor de javascript
  */
void DBTableView::setCheckedBeans(BaseBeanSharedPointerList list, bool checked)
{
    // TODO: Hay codigo similar hecho para DBListView. Lo suyo sería hacerlo común en DBAbstractView
    Q_UNUSED(list)
    Q_UNUSED(checked)
}

void DBTableView::setFieldCheckBox(const QString &name)
{
    d->m_itemCheckBox = name;
    if ( !d->m_itemCheckBox.isEmpty() && filterModel() )
    {
        QStringList fields;
        fields << d->m_itemCheckBox;
        filterModel()->setCheckColumns(fields);
    }
}

QString DBTableView::fieldCheckBox()
{
    return d->m_itemCheckBox;
}

void DBTableView::setAllowedEdit(bool value)
{
    DBAbstractViewInterface::setAllowedEdit(value);
    if ( !value )
    {
        setSelectionBehavior(QAbstractItemView::SelectRows);
    }
    else
    {
        setSelectionBehavior(QAbstractItemView::SelectItems);
    }
}

bool DBTableView::canMoveRows() const
{
    return d->m_canMoveRows;
}

void DBTableView::setCanMoveRows(bool value)
{
    d->m_canMoveRows = value;
    if ( value && filterModel() )
    {
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
        verticalHeader()->setMovable(true);
#else
        verticalHeader()->setSectionsMovable(true);
#endif
        connect(verticalHeader(), SIGNAL(sectionMoved(int,int,int)), this, SLOT(newRowOrder(int,int,int)));
        setDragEnabled(true);
        setDragDropMode(QAbstractItemView::InternalMove);
        RelationBaseBeanModel *mdl = qobject_cast<RelationBaseBeanModel *> (filterModel()->sourceModel());
        if ( mdl != NULL )
        {
            mdl->setCanMoveRows(d->m_canMoveRows);
        }
    }
    else
    {
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
        verticalHeader()->setMovable(false);
#else
        verticalHeader()->setSectionsMovable(false);
#endif
        setDragEnabled(false);
    }
}

void DBTableView::setFilter(const QString &value)
{
    DBAbstractViewInterface::setFilter(value);
    setFieldCheckBox(d->m_itemCheckBox);
}

/*!
  Ordena por el field marcado
  */
void DBTableView::sortByColumn(const QString &field, Qt::SortOrder order)
{
    FilterBaseBeanModel *mdl = qobject_cast<FilterBaseBeanModel*>(model());
    if ( mdl == 0 )
    {
        return;
    }
    QList<DBFieldMetadata *> list = mdl->visibleFields();
    for ( int i = 0 ; i < list.size() ; i++ )
    {
        if ( list.at(i)->dbFieldName() == field )
        {
            QTableView::sortByColumn(i, order);
            return;
        }
    }
}

/*!
  Tenemos que decirle al motor de scripts, que DBFormDlg se convierte de esta forma a un valor script
  */
QScriptValue DBTableView::toScriptValue(QScriptEngine *engine, DBTableView * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBTableView::fromScriptValue(const QScriptValue &object, DBTableView * &out)
{
    out = qobject_cast<DBTableView *>(object.toQObject());
}

void DBTableView::headerContextMenu(QPoint pos)
{
    DBAbstractViewInterface::headerContextMenu(pos);
}

void DBTableView::fromMenuHideColumn()
{
    DBAbstractViewInterface::fromMenuHideColumn();
}

void DBTableView::showContextMenu(const QPoint &point)
{
    if ( !editTriggers().testFlag(QAbstractItemView::NoEditTriggers) )
    {
        DBAbstractViewInterface::showContextMenu(point);
    }
    else
    {
        if ( !d->m_emittingShowContextMenu )
        {
            d->m_emittingShowContextMenu = true;
            emit showContextMenu(point);
            d->m_emittingShowContextMenu = false;
        }
    }
}

/**
 * @brief DBTableView::applyRowSpan
 * Una consulta, puede mostrar mismos beans, en filas diferentes. Aquí se hace un rowSpan adecuado.
 */
void DBTableView::applyRowSpan()
{
    if ( model() == NULL )
    {
        return;
    }
    for (int row = 0 ; row < model()->rowCount() ; row++ )
    {
        int rowSpan = 1;
        QModelIndex idx1 = model()->index(row, 0);
        QModelIndex idx2 = model()->index(row+1, 0);
        if ( idx1.data(AlephERP::RowFetchedRole).toBool() && idx2.data(AlephERP::RowFetchedRole).toBool() )
        {
            QVariant dbOid1 = idx1.data(AlephERP::DBOidRole);
            QVariant dbOid2 = idx2.data(AlephERP::DBOidRole);
            if (dbOid1.toInt() != 0 && dbOid2.toInt() != 0)
            {
                bool canContinue = true;
                while ( canContinue &&
                        dbOid1 == dbOid2 &&
                        idx2.row() < model()->rowCount() )
                {
                    rowSpan++;
                    idx2 = model()->index(idx2.row()+1, 0);
                    if ( idx2.isValid() )
                    {
                        if ( !idx2.data(AlephERP::RowFetchedRole).toBool() )
                        {
                            canContinue = false;
                        }
                        else
                        {
                            dbOid2 = idx2.data(AlephERP::DBOidRole);
                        }
                    }
                }
                if ( rowSpan > 1 && canContinue )
                {
                    for ( int col = 0 ; col < model()->columnCount() ; col++ )
                    {
                        QModelIndex idx1 = model()->index(row, col);
                        QModelIndex idx2 = model()->index(row+1, col);
                        QVariant d1 = model()->data(idx1);
                        QVariant d2 = model()->data(idx2);
                        if ( d1 == d2 )
                        {
                            setSpan(row, col, rowSpan, 1);
                        }
                    }
                }
            }
        }
    }
}

void DBTableView::exportSpreadSheet()
{
    AERPSpreadSheetUtil::instance()->exportSpreadSheet(filterModel(), this);
}
