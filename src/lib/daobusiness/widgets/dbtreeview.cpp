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
#include <QtScript>
#include "dbtreeview.h"
#include "globales.h"
#include "configuracion.h"
#include "models/beantreeitem.h"
#include "models/treebasebeanmodel.h"
#include "models/filterbasebeanmodel.h"
#include "models/aerphtmldelegate.h"
#include "models/aerpimageitemdelegate.h"
#include "models/aerpmoviedelegate.h"
#include "dao/beans/basebean.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/observerfactory.h"

class DBTreeViewPrivate
{
//    Q_DECLARE_PUBLIC(DBTreeView)
public:

    /** Tablas referenciadas que se utilizarán para mostrar los datos (es
      decir, deben existir relaciones de padres a hijas definidas en los XML).
      Se separan por puntos y coma, por ejemplo: familias;subfamilias;articulos
      */
    QString m_tableNames;
    /** De cada tabla que se muestra de forma jerárquica, este campo indica qué campos
      se muestran al usuario. Por ejemplo: nombre;nombre;descripcion.
      Deben tener el mismo número de entradas que m_tableNames y en el mismo orden */
    QString m_visibleColumns;
    /** Por cada nivel jerárquico, indica qué valores proporciona value */
    QString m_keyColumns;
    /** Filtros a aplicar */
    QString m_filter;
    /** Indica si se mostrará alguna imagen. La imagen puede ser de un archivo que se indique,
      o bien de una imagen que se lea de la propia tabla que muestra los datos. El formato sería:
      file:/home/david/tmp/imagen.png;field:imagen
          */
    QString m_images;
    /** Tamaño de las imágenes que se mostrarán */
    QSize m_imagesSize;
    /** Niveles de los que se puede seleccionar */
    QString m_selectLevels;
    /** Indica si se presenta un checkbox al lado de cada item */
    bool m_itemCheckBox;
    /** Se pueden asignar beans a ser checkeados, antes de que el control se haya iniciado.
      En ese caso, se guardan previamente. Como los beans pueden venir del motor javascript,
    mejor guardos los PK */
    QVariantList m_initedCheckedBeansPk;
    /** Esto se utiliza para garantizar la persistencia de los beans proporcionados al motor Javascript
      mientras viva este objeto */
    BaseBeanSharedPointerList m_checkedBeans;
    AlephERP::DBRecordStates m_visibleRecords;
    /** Permite llevar un control interno de qué items se han ido desplegando, para ir poder devolver el
     * control al estado origiinal */
    QStringList m_expandedItems;
    QPointer<AERPMovieDelegate> m_movieDelegate;
    bool m_restoringState;
    bool m_retrievingAllModel;
    QPointer<QProgressDialog> m_progressDialog;
    // Evitar recursividad emitiendo algunas señales
    bool m_emittingShowContextMenu;

    bool m_checkFatherCheckChildrens;
    bool m_viewIntermediateNodesWithoutChildren;

    DBTreeView *q_ptr;

    void connections();
    void disconnect();
    DBTreeViewPrivate(DBTreeView *qq);
    QString configurationName();
    BaseBeanMetadata * lastTableMetadata();
    QString lastTableName();
    QList<DBFieldMetadata *> visibleFields (BaseBeanMetadata *metadata);
    void restoreState(const QModelIndex &parent = QModelIndex());
    void expandParents(const QModelIndex &child);
};

DBTreeViewPrivate::DBTreeViewPrivate(DBTreeView *qq) :  q_ptr(qq)
{
    m_itemCheckBox = false;
    m_checkFatherCheckChildrens = false;
    m_movieDelegate = new AERPMovieDelegate(qq, qq);
    m_movieDelegate->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_restoringState = false;
    m_retrievingAllModel = false;
    m_viewIntermediateNodesWithoutChildren = false;
    m_emittingShowContextMenu = false;
}

void DBTreeViewPrivate::connections()
{
    if ( q_ptr->filterModel() == NULL )
    {
        return;
    }

    QObject::connect(q_ptr->filterModel(), SIGNAL(endLoadingData()), q_ptr, SLOT(hideAnimation()));
    QObject::connect(q_ptr->filterModel(), SIGNAL(initLoadingData()), q_ptr, SLOT(showAnimation()));

    QObject::connect(q_ptr->filterModel() , SIGNAL(modelAboutToBeReset()), q_ptr, SLOT(saveState()));
    QObject::connect(q_ptr->filterModel() , SIGNAL(modelReset()), q_ptr, SLOT(restoreState()));
    // El FilterBaseBeanModel y en general el ProxyModel, hacen que con cualquier cambio
    // en el modelo en árbol, se invoque un cambio total del layout... es obligatorio restaurar
    // el estado y estas llamadas son importantes.
    QObject::connect(q_ptr->filterModel() , SIGNAL(layoutAboutToBeChanged()), q_ptr, SLOT(saveState()));
    QObject::connect(q_ptr->filterModel() , SIGNAL(layoutChanged()), q_ptr, SLOT(restoreState()), Qt::QueuedConnection);
}

void DBTreeViewPrivate::disconnect()
{
    if ( q_ptr->filterModel() == NULL )
    {
        return;
    }
    TreeBaseBeanModel *treeModel = qobject_cast<TreeBaseBeanModel *>(q_ptr->filterModel()->sourceModel());
    if ( treeModel != NULL && q_ptr->m_metadata != NULL && q_ptr->m_metadata->showOnTreePreloadRecords() )
    {
        QObject::disconnect(treeModel, SIGNAL(populateAllModel(int)), m_progressDialog.data(), SLOT(setMaximum(int)));
        QObject::disconnect(treeModel, SIGNAL(populateAllModelProgress(int)), m_progressDialog.data(), SLOT(setValue(int)));
    }
    QObject::disconnect(q_ptr->filterModel() , SIGNAL(initLoadingData()), m_progressDialog.data(), SLOT(show()));
    QObject::disconnect(q_ptr->filterModel() , SIGNAL(endLoadingData()), m_progressDialog.data(), SLOT(hide()));
    QObject::disconnect(q_ptr->filterModel() , SIGNAL(endLoadingData()), q_ptr, SLOT(hideAnimation()));
    QObject::disconnect(q_ptr->filterModel() , SIGNAL(initLoadingData()), q_ptr, SLOT(showAnimation()));

    QObject::disconnect(q_ptr->filterModel() , SIGNAL(modelAboutToBeReset()), q_ptr, SLOT(saveState()));
    QObject::disconnect(q_ptr->filterModel() , SIGNAL(modelReset()), q_ptr, SLOT(restoreState()));
    // El FilterBaseBeanModel y en general el ProxyModel, hacen que con cualquier cambio
    // en el modelo en árbol, se invoque un cambio total del layout... es obligatorio restaurar
    // el estado y estas llamadas son importantes.
    QObject::disconnect(q_ptr->filterModel() , SIGNAL(layoutAboutToBeChanged()), q_ptr, SLOT(saveState()));
    QObject::disconnect(q_ptr->filterModel() , SIGNAL(layoutChanged()), q_ptr, SLOT(restoreState()));
}

BaseBeanMetadata *DBTreeViewPrivate::lastTableMetadata()
{
    if ( !m_tableNames.isEmpty() )
    {
        QStringList listTableNames = m_tableNames.split(QRegExp(QStringLiteral(";|,")));
        return BeansFactory::metadataBean(listTableNames.last());
    }
    FilterBaseBeanModel *mdl = qobject_cast<FilterBaseBeanModel *> (q_ptr->model());
    if ( mdl == NULL )
    {
        return NULL;
    }
    TreeBaseBeanModel *sourceModel = qobject_cast<TreeBaseBeanModel *> (mdl->sourceModel());
    if ( sourceModel == NULL )
    {
        return NULL;
    }
    return BeansFactory::metadataBean(sourceModel->tableNames().last());
}

QString DBTreeViewPrivate::lastTableName()
{
    BaseBeanMetadata * m = lastTableMetadata();
    if ( m )
    {
        return m->tableName();
    }
    return QString();
}

QList<DBFieldMetadata *> DBTreeViewPrivate::visibleFields (BaseBeanMetadata *metadata)
{
    QList<DBFieldMetadata *> list;
    foreach ( DBFieldMetadata *fld, metadata->fields() )
    {
        if ( fld->visibleGrid() )
        {
            list.append(fld);
        }
    }
    return list;
}

DBTreeView::DBTreeView(QWidget *parent) :
    QTreeView(parent),
    DBAbstractViewInterface(this, header()),
    AERPBackgroundAnimation(viewport()),
    d(new DBTreeViewPrivate(this))
{
    setAutomaticName(true);
    setMouseTracking(true);

    connect(this, SIGNAL(enterPressedOnValidIndex(QModelIndex)), this, SLOT(nextCellOnEnter(QModelIndex)));
    connect(this, SIGNAL(clicked(QModelIndex)), this, SLOT(itemClicked(QModelIndex)));
    connect(m_hideColumn, SIGNAL(triggered()), this, SLOT(fromMenuHideColumn()));
    connect(this, SIGNAL(expanded(QModelIndex)), this, SLOT(expandedIndex(QModelIndex)));
    connect(this, SIGNAL(collapsed(QModelIndex)), this, SLOT(collapsedIndex(QModelIndex)));
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

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

    m_header->installEventFilter(m_eventForwarder);
    connect(m_eventForwarder, SIGNAL(entered()), this, SLOT(resetCursor()));
}

DBTreeView::~DBTreeView()
{
    saveState();
    delete d;
}

QString DBTreeView::tableNames()
{
    return d->m_tableNames;
}

void DBTreeView::setTableNames(const QString &value)
{
    d->m_tableNames = value;
}

QString DBTreeView::visibleColumns()
{
    return d->m_visibleColumns;
}

void DBTreeView::setVisibleColumns(const QString &value)
{
    d->m_visibleColumns = value;
    if ( d->m_itemCheckBox )
    {
        setItemCheckBox(true);
    }
}

QString DBTreeView::keyColumns()
{
    return d->m_keyColumns;
}

void DBTreeView::setKeyColumns(const QString &value)
{
    d->m_keyColumns = value;
}

QString DBTreeView::filter()
{
    return d->m_filter;
}

void DBTreeView::setFilter(const QString &value)
{
    d->m_filter = value;
    if ( !d->m_filter.isEmpty() )
    {
        clearModels();
        init();
    }
}

void DBTreeView::setItemCheckBox(bool value)
{
    d->m_itemCheckBox = value;
    if ( value )
    {
        if ( filterModel() )
        {
            QStringList fields = d->m_visibleColumns.split(QRegExp(QStringLiteral(";|,")));
            QStringList tables = d->m_tableNames.split(QRegExp(QStringLiteral(";|,")));
            QStringList checkColumns;
            for ( int i = 0 ; i < fields.size() ; i++ )
            {
                if ( i < tables.size() )
                {
                    checkColumns.append(QString("%1.%2").arg(tables.at(i)).arg(fields.at(i)));
                }
                else
                {
                    checkColumns.append(fields.at(i));
                }
            }
            filterModel()->setCheckColumns(checkColumns);
        }
    }
}

bool DBTreeView::itemCheckBox()
{
    return d->m_itemCheckBox;
}

void DBTreeView::setCheckFatherCheckChildrens(bool value)
{
    d->m_checkFatherCheckChildrens = value;
    if ( filterModel() )
    {
        TreeBaseBeanModel *mdl = qobject_cast<TreeBaseBeanModel *>(filterModel()->sourceModel());
        if ( mdl != NULL )
        {
            mdl->setCheckFatherCheckChildrens(value);
        }
    }
}

bool DBTreeView::checkFatherCheckChildrens()
{
    return d->m_checkFatherCheckChildrens;
}

bool DBTreeView::viewIntermediateNodesWithoutChildren() const
{
    return d->m_viewIntermediateNodesWithoutChildren;
}

void DBTreeView::setViewIntermediateNodesWithoutChildren(bool value)
{
    d->m_viewIntermediateNodesWithoutChildren = value;
    if ( filterModel() )
    {
        TreeBaseBeanModel *model = qobject_cast<TreeBaseBeanModel *>(filterModel()->sourceModel());
        if ( model != NULL )
        {
            model->setViewIntermediateNodesWithoutChildren(value);
        }
    }
}

QString DBTreeView::images()
{
    return d->m_images;
}

void DBTreeView::setImages(const QString &value)
{
    d->m_images = value;
}

QSize DBTreeView::imagesSize()
{
    return d->m_imagesSize;
}

void DBTreeView::setImagesSize(const QSize &value)
{
    d->m_imagesSize = value;
}

void DBTreeView::setSelectLevels(const QString &level)
{
    d->m_selectLevels = level;
}

QString DBTreeView::selectLevels()
{
    return d->m_selectLevels;
}

/*!
  Devuelve un listado de los beans correspondiente a los items actualmente seleccionados
  */
BaseBeanSharedPointerList DBTreeView::selectedBeans()
{
    BaseBeanSharedPointerList list;
    QStringList levels = d->m_selectLevels.split(QRegExp(QStringLiteral(";|,")));

    if ( filterModel() )
    {
        QModelIndexList lIndex = selectionModel()->selectedIndexes();
        foreach ( QModelIndex filterIndex, lIndex )
        {
            BaseBeanSharedPointer bean = filterModel()->bean(filterIndex);
            QVariant vItem = filterIndex.data(AlephERP::TreeItemRole);
            BeanTreeItem *item = (BeanTreeItem *)(vItem.value<void *>());
            if ( item != NULL &&
                 (levels.contains(QString("%1").arg(item->level())) || d->m_selectLevels.isEmpty()) &&
                 !bean.isNull() &&
                 !list.contains(bean) )
            {
                list << bean;
            }
        }
    }
    return list;
}

void DBTreeView::applyFieldProperties()
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

QVariant DBTreeView::value()
{
    QVariant value;
    if ( filterModel() )
    {
        QModelIndex idx = selectionModel()->currentIndex();
        BaseBeanSharedPointer bean = filterModel()->bean(idx);
        if ( !bean.isNull() )
        {
            DBField *fld = bean->field(this->m_fieldName);
            if ( fld != NULL )
            {
                value = fld->value();
            }
        }
    }
    return value;
}

/*!
  Este value debe ser una primary key
  */
void DBTreeView::setValue(const QVariant &value)
{
    if ( value.type() != QVariant::Map || filterModel() == NULL )
    {
        return;
    }
    QModelIndex idxFilter = filterModel()->indexByPk(value);
    selectionModel()->select(idxFilter, QItemSelectionModel::Select);
}

void DBTreeView::refresh()
{
    saveState();
    if ( filterModel() )
    {
        filterModel()->invalidate();
    }
    restoreState();
}

/*!
  Al mostrarse el control es cuando se crean los modelos y demás
  */
void DBTreeView::showEvent(QShowEvent * event)
{
    DBBaseWidget::showEvent(event);

    if ( !d->m_tableNames.isEmpty() )
    {
        if ( !property(AlephERP::stInited).toBool() )
        {
            init();
            setProperty(AlephERP::stInited, true);
        }
        setCheckFatherCheckChildrens(d->m_checkFatherCheckChildrens);
        setItemCheckBox(d->m_itemCheckBox);
        if ( d->m_initedCheckedBeansPk.size() > 0 )
        {
            setCheckedBeansByPk(d->m_initedCheckedBeansPk);
            d->m_initedCheckedBeansPk.clear();
        }
        if ( observer() )
        {
            observer()->sync();
        }
    }
    restoreState();
}

void DBTreeView::keyPressEvent ( QKeyEvent * event )
{
    QModelIndex index;

    if ( event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter )
    {
        index = currentIndex();
        if ( index.isValid() )
        {
            emit enterPressedOnValidIndex(index );
            event->accept();
            return;
        }
    }
    else if ( event->key() == Qt::Key_Escape )
    {
        index = currentIndex();
        if ( index.isValid() )
        {
            int row = index.row();
            if ( model()->rowCount() == 1 )
            {
                model()->removeRow(row);
            }
            else
            {
                model()->removeRow(row);
                QModelIndex prev = model()->index(row-1, 0);
                setCurrentIndex(prev);
            }
            event->accept();
        }
    }
    QTreeView::keyPressEvent(event);
}

void DBTreeView::mouseDoubleClickEvent (QMouseEvent *event)
{
    QModelIndex index;

    if ( event->button() == Qt::LeftButton )
    {
        index = currentIndex();
        if ( index.isValid() )
        {
            emit doubleClickOnValidIndex(index);
            event->accept();
            return;
        }
    }
    QTreeView::mouseDoubleClickEvent(event);
}

void DBTreeView::mouseMoveEvent(QMouseEvent *event)
{
    DBAbstractViewInterface::mouseMoveEvent(event);
    QTreeView::mouseMoveEvent(event);
}

void DBTreeView::closeEvent(QCloseEvent *event)
{
    saveColumnsOrder();
    QTreeView::closeEvent(event);
}

void DBTreeView::paintEvent(QPaintEvent *event)
{
    paintAnimation(event);
    QTreeView::paintEvent(event);
}

void DBTreeView::init()
{
    hideAnimation();
    if ( automaticName() )
    {
        setObjectName (configurationName());
    }
    // Esto introducirá varias mejoras en el rendimiento (ver documentación).
    // Nos obliga a que todas las filas tengan misma altura, lo que tiene sentido.
    setUniformRowHeights(true);

    if ( !m_externalModel )
    {
        QStringList tableNames = d->m_tableNames.split(QRegExp(";|,"));
        QStringList fieldsView = d->m_visibleColumns.split(QRegExp(";|,"));
        QStringList images = d->m_images.split(QRegExp(";|,"));
        QStringList filters = d->m_filter.split(QRegExp(";|,"));

        if ( tableNames.size() == fieldsView.size() )
        {
            TreeBaseBeanModel *sourceModel = new TreeBaseBeanModel(tableNames, false, this);
            sourceModel->setFieldsView(fieldsView);
            sourceModel->setFilterLevels(filters);
            sourceModel->setImages(images);
            sourceModel->setSize(d->m_imagesSize);
            sourceModel->setLinkColumns(m_linkColumns);
            sourceModel->setupInitialData();
            setSourceModel(sourceModel);

            // Importante hacer esto aquí para que la animación funcione correctamente.
            setAnimation(":/generales/images/animatedWait.gif");
            // OJO: No llamar a DBTreeView::setModel ya que internamente llama a setSourceModel
            // que borra la creación anterior.
            QTreeView::setModel(filterModel());
            // Animación en espera de carga de los items... Pero ojo: Sólo si no hay un itemDelegate
            for (int i = 0 ; i < filterModel()->columnCount() ; ++i)
            {
                if ( itemDelegateForColumn(i) == NULL )
                {
                    setItemDelegate(d->m_movieDelegate);
                }
            }
            d->connections();
            prepareColumns();
            spanRow();
        }
    }
}

BaseBeanSharedPointer DBTreeView::selectedBean()
{
    BaseBeanSharedPointer bean;
    if ( filterModel() )
    {
        QModelIndex idx = selectionModel()->currentIndex();
        bean = filterModel()->bean(idx);
    }
    return bean;
}

void DBTreeView::observerUnregistered()
{
    DBBaseWidget::observerUnregistered();
    bool blockState = blockSignals(true);
    reset();
    blockSignals(blockState);
}

/*!
  Tenemos que decirle al motor de scripts, que DBFormDlg se convierte de esta forma a un valor script
  */
QScriptValue DBTreeView::toScriptValue(QScriptEngine *engine, DBTreeView * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBTreeView::fromScriptValue(const QScriptValue &object, DBTreeView * &out)
{
    out = qobject_cast<DBTreeView *>(object.toQObject());
}

/*!
  Devuelve un listado de los beans que han sido checkeados por el usuario
  */
QScriptValue DBTreeView::checkedBeans()
{
    if ( engine() == NULL )
    {
        return QScriptValue();
    }
    QScriptValue list = engine()->newArray();
    FilterBaseBeanModel *mdlFilter = qobject_cast<FilterBaseBeanModel *>(model());
    if ( mdlFilter == NULL )
    {
        return list;
    }
    TreeBaseBeanModel *mdl = qobject_cast<TreeBaseBeanModel *>(mdlFilter->sourceModel());
    if ( mdl == NULL )
    {
        return list;
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
            list.setProperty(index, scriptBean);
            index++;
        }
    }
    return list;
}

/*!
  Si hay una columna con checkbox, permite marcarla o desmarcarla entera...
  */
void DBTreeView::checkAllItems(bool checked)
{
    if ( filterModel() != NULL )
    {
        filterModel()->checkAllItems(checked);
    }
}

void DBTreeView::setModel(QAbstractItemModel *model)
{
    d->disconnect();
    setSourceModel(model);
    QTreeView::setModel(filterModel());
    // Animación en espera de carga de los items... Pero ojo: Sólo si no hay un itemDelegate
    for (int i = 0 ; i < model->columnCount() ; ++i)
    {
        if ( itemDelegateForColumn(i) == NULL )
        {
            setItemDelegate(d->m_movieDelegate);
        }
    }
    d->connections();
    prepareColumns();
    spanRow();
}

/**
 * @brief DBTreeView::spanRow Hace span de la rama intermedia
 * Es una función recursiva
 * @param index
 */
void DBTreeView::spanRow(const QModelIndex &parent)
{
    if ( !model() )
    {
        return;
    }
    QModelIndex parentIndex = parent;
    if ( !parentIndex.isValid() )
    {
        parentIndex = rootIndex();
    }
    if ( parentIndex.isValid() && model() != parentIndex.model() )
    {
        qWarning() << "DBTreeView::spanRow: INDICE Y MODELO NO COINCIDEN";
        return;
    }
    int rowCount = model()->rowCount(parentIndex);
    for ( int row = 0 ; row < rowCount ; row++ )
    {
        QModelIndex childIndex = model()->index(row, 0, parentIndex);
        if ( childIndex.isValid() && !isIndexHidden(childIndex) )
        {
            QVariant vItem = childIndex.data(AlephERP::TreeItemRole);
            BeanTreeItem *item = (BeanTreeItem *)(vItem.value<void *>());
            if ( item != NULL && item->tableName() != d->lastTableName() )
            {
                if ( !isFirstColumnSpanned(row, parentIndex) )
                {
                    setFirstColumnSpanned(row, parentIndex, true);
                }
                spanRow(childIndex);
            }
        }
    }
}

/**
 * @brief DBTreeView::spanOnIntermediateBranchs Recorre todas las ramas intermedias y realiza el span correspondiente
 */
void DBTreeView::spanOnIntermediateBranchs()
{
    spanRow(rootIndex());
}

/*!
  Nos almacenará los anchos que el usuario haya seleccionado
  */
void DBTreeView::saveHeaderSize(int logicalSection, int oldSize, int newSize)
{
    Q_UNUSED (logicalSection)
    if ( oldSize != newSize )
    {
        alephERPSettings->saveViewState<QTreeView>(this);
    }
}

/*!
  Ordena las columnas de visualización según el la lista indicada. Esta función sólo tiene sentido
  si la última tabla que se presenta del árbol, tiene los fields visibles como "*", es decir, se presentan
  a modo de tabla.
  */
void DBTreeView::orderColumns(const QStringList &order)
{
    Q_UNUSED(order)
    /*
     * La ordenación de columnas queda muy mal con el spanRow. Mejor comentarlo.
     */
    /*
    FilterBaseBeanModel *mdl = qobject_cast<FilterBaseBeanModel *> (model());
    if ( mdl == NULL )
    {
        return;
    }
    TreeBaseBeanModel *sourceModel = qobject_cast<TreeBaseBeanModel *> (mdl->sourceModel());
    if ( sourceModel == NULL )
    {
        return;
    }
    if ( sourceModel->fieldsView().last() == QStringLiteral("*") )
    {
        BaseBeanMetadata *metadata = BeansFactory::metadataBean(sourceModel->tableNames().last());
        if ( metadata != NULL )
        {
            QHeaderView *header = this->header();
            QList<DBFieldMetadata *> list = d->visibleFields(metadata);
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
    }
    */
}

/*!
  Ordena por el field marcado
  */
void DBTreeView::sortByColumn(const QString &field, Qt::SortOrder order)
{
    BaseBeanMetadata *metadata = d->lastTableMetadata();
    if ( metadata != NULL )
    {
        QList<DBFieldMetadata *> list = d->visibleFields(metadata);
        for ( int i = 0 ; i < list.size() ; i++ )
        {
            if ( list.at(i)->dbFieldName() == field )
            {
                QTreeView::sortByColumn(i, order);
                return;
            }
        }
    }
}

/*!
  Almacena en registro el orden y tipo de ordenación de las columnas del DBTableView
  */
void DBTreeView::saveColumnsOrder(const QStringList &order, const QStringList &sort)
{
    alephERPSettings->saveViewColumnsOrder<QTreeView>(this, order, sort);
}

void DBTreeView::setCheckedBeansByPk(QVariantList list, bool checked)
{
    QModelIndexList checkedItems ;
    for ( int i = 0 ; i < list.size() ; i++ )
    {
        for ( int row = 0 ; row < filterModel()->rowCount() ; row++ )
        {
            BaseBeanSharedPointer bean = filterModel()->bean(row);
            if ( bean->pkEqual(list.at(i)) )
            {
                checkedItems.append(filterModel()->index(row, 0));
            }
        }
    }
    if ( checkedItems.size() > 0 )
    {
        filterModel()->setCheckedItems(checkedItems, checked);
    }
}

/*!
  Devuelve un listado de los beans que han sido checkeados por el usuario.
  Esta función puede ser llamada desde Javascript, en el constructor del widget. En ese
  caso los modelos aún no se han creado, por lo que se guardarán en una estructura
  intermedia las primary keys de los beans pasados. No se guardan los beans, porque
  estos pueden haber sido borrados previamente por el motor de javascript
  */
void DBTreeView::setCheckedBeans(BaseBeanSharedPointerList list, bool checked)
{
    for ( int i = 0 ; i < list.size() ; i++ )
    {
        if ( !list.at(i).isNull() )
        {
            d->m_initedCheckedBeansPk.append(list.at(i)->pkValue());
        }
    }
    if ( filterModel() != NULL )
    {
        setCheckedBeansByPk(d->m_initedCheckedBeansPk, checked);
    }
}

void DBTreeView::nextCellOnEnter(const QModelIndex &actualCell)
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

void DBTreeView::saveColumnsOrder()
{
    DBAbstractViewInterface::saveColumnsOrder();
}

void DBTreeView::itemClicked(const QModelIndex &idx)
{
    DBAbstractViewInterface::itemClicked(idx);
}

void DBTreeView::headerContextMenu(QPoint pos)
{
    DBAbstractViewInterface::headerContextMenu(pos);
}

void DBTreeView::fromMenuHideColumn()
{
    DBAbstractViewInterface::fromMenuHideColumn();
}

void DBTreeView::collapsedIndex(QModelIndex index)
{
    if ( d->m_restoringState )
    {
        return;
    }
    QString displayValue = index.data(AlephERP::AllParentsSerializedPrimaryKeyRole).toString();
    if ( d->m_expandedItems.contains(displayValue) )
    {
        d->m_expandedItems.removeAll(displayValue);
    }
}

void DBTreeView::expandedIndex(QModelIndex index)
{
    if ( d->m_restoringState )
    {
        return;
    }
    QString displayValue = index.data(AlephERP::AllParentsSerializedPrimaryKeyRole).toString();
    if ( !d->m_expandedItems.contains(displayValue) )
    {
        d->m_expandedItems.append(displayValue);
    }
    spanRow(index);
}

void DBTreeView::showContextMenu(const QPoint &point)
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
 * @brief DBTreeView::saveState
 * Almacena la información de los elementos desplegados del árbol
 */
void DBTreeView::saveState()
{
    if ( d->m_restoringState || d->m_retrievingAllModel )
    {
        return;
    }
    if ( !isOnInit() )
    {
        alephERPSettings->saveTreeViewExpandedState(d->m_expandedItems, objectName());
        alephERPSettings->save();
    }
}

void DBTreeView::restoreState()
{
    if ( d->m_restoringState || d->m_retrievingAllModel || filterModel() == NULL || !isRestoreSaveStateEnabled() )
    {
        return;
    }
    d->m_restoringState = true;
    bool isFrozenModel = filterModel()->isFrozenModel();
    if ( !isFrozenModel )
    {
        filterModel()->freezeModel();
    }
    if ( d->m_expandedItems.isEmpty() )
    {
        d->m_expandedItems = alephERPSettings->restoreTreeViewExpandedState(objectName());
    }
    if ( !d->m_expandedItems.isEmpty() )
    {
        CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
        d->restoreState();
        CommonsFunctions::restoreOverrideCursor();
    }
    spanRow(rootIndex());
    if ( !isFrozenModel )
    {
        filterModel()->defrostModel();
    }
    d->m_restoringState = false;
}

void DBTreeView::populateAllModel()
{
    if ( d->m_retrievingAllModel )
    {
        return;
    }
    TreeBaseBeanModel *sourceModel;
    FilterBaseBeanModel *mdl = qobject_cast<FilterBaseBeanModel *> (model());
    if ( mdl != NULL )
    {
        sourceModel = qobject_cast<TreeBaseBeanModel *> (mdl->sourceModel());
    }
    else
    {
        sourceModel = qobject_cast<TreeBaseBeanModel *>(model());
    }
    if ( sourceModel == NULL )
    {
        return;
    }
    if ( d->m_progressDialog.isNull() )
    {
        d->m_progressDialog = new QProgressDialog(this);
        d->m_progressDialog->hide();
        if ( sourceModel != NULL &&
             sourceModel->metadata() &&
             sourceModel->metadata()->showOnTreePreloadRecords() )
        {
            d->m_progressDialog->setLabelText(trUtf8("Carga de '%1'").arg(sourceModel->metadata()->alias()));
            QObject::connect(sourceModel, SIGNAL(populateAllModel(int)), d->m_progressDialog.data(), SLOT(setMaximum(int)));
            QObject::connect(sourceModel, SIGNAL(populateAllModelProgress(int)), d->m_progressDialog.data(), SLOT(setValue(int)));
        }
        QObject::connect(filterModel(), SIGNAL(initLoadingData()), d->m_progressDialog.data(), SLOT(show()));
        QObject::connect(filterModel(), SIGNAL(endLoadingData()), d->m_progressDialog.data(), SLOT(hide()));
    }
    d->m_retrievingAllModel = true;
    sourceModel->populateAllModel();
    d->m_retrievingAllModel = false;
    restoreState();
}

void DBTreeViewPrivate::restoreState(const QModelIndex &parentIndex)
{
    if ( !q_ptr->model() )
    {
        return;
    }
    QModelIndex parent = parentIndex;
    if ( !parent.isValid() )
    {
        parent = q_ptr->rootIndex();
    }
    if ( parent.isValid() && q_ptr->model() != parent.model() )
    {
        qWarning() << "DBTreeViewPrivate::restoreState: Índice y modelo no coinciden";
        return;
    }
    foreach (const QString &child,  m_expandedItems)
    {
        if ( !child.isEmpty() )
        {
            QModelIndex firstChild = q_ptr->model()->index(0, 0, parent);
            QModelIndexList items = q_ptr->model()->match(firstChild, AlephERP::AllParentsSerializedPrimaryKeyRole, QVariant::fromValue(child), -1);
            if (!items.isEmpty())
            {
                foreach (const QModelIndex &idx, items)
                {
                    if ( idx.isValid() )
                    {
                        if ( !q_ptr->isExpanded(idx) )
                        {
                            q_ptr->expand(idx);
                            expandParents(idx);
                        }
                        restoreState(idx);
                    }
                }
            }
        }
    }
}

void DBTreeViewPrivate::expandParents(const QModelIndex &child)
{
    if ( !q_ptr->model() || !child.isValid() )
    {
        return;
    }
    if ( q_ptr->model() != child.model() )
    {
        qWarning() << "DBTreeViewPrivate::restoreState: Índice y modelo no coinciden";
        return;
    }

    if ( child.parent().isValid() )
    {
        if ( !q_ptr->isExpanded(child.parent()) )
        {
            q_ptr->expand(child.parent());
            expandParents(child.parent());
        }
    }
}

void DBTreeView::expandFirstBranchWithData()
{
    QModelIndex idx = model()->index(0, 0, QModelIndex());
    expand(idx);
    do
    {
        idx = model()->index(0, 0, idx);
        if ( idx.isValid() )
        {
            expand(idx);
            QVariant vItem = idx.data(AlephERP::TreeItemRole);
            BeanTreeItem *item = (BeanTreeItem *)(vItem.value<void *>());
            if ( item != NULL && item->tableName() == d->lastTableName() )
            {
                return;
            }
        }
    }
    while (model()->hasChildren(idx));
}
