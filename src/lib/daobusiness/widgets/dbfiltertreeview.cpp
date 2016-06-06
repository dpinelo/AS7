/***************************************************************************
 *   Copyright (C) 2012 by David Pinelo   *
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

#include <globales.h>
#include "dao/beans/basebean.h"
#include "dbfiltertreeview.h"
#include "ui_dbabstractfilterview.h"
#include "dao/basedao.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbrelationmetadata.h"
#include "models/dbbasebeanmodel.h"
#include "models/filterbasebeanmodel.h"
#include "models/treebasebeanmodel.h"
#include "scripts/perpscript.h"
#include "widgets/dbtableviewcolumnorderform.h"
#include "widgets/dbnumberedit.h"
#include "widgets/dbabstractfilterview_p.h"
#include "widgets/dbtreeview.h"
#include "widgets/dbabstractfilterview.h"


class DBFilterTreeViewPrivate
{
    Q_DECLARE_PUBLIC(DBFilterTreeView)
public:
    DBFilterTreeView *q_ptr;
    /** Estructura del árbol */
    QVariantList m_treeDefinition;
    /** Listado de tablas del árbol */
    QStringList m_tableNames;
    QStringList m_visibleFields;
    QStringList m_images;
    QStringList m_sorts;
    QStringList m_filtersFields;
    QList<bool> m_allowInsert;
    QList<bool> m_allowEdit;
    QList<bool> m_allowDelete;
    QPointer<QWidget> m_branchFilter;
    QSize m_branchFilterSize;
    QList<QLineEdit *> m_lineEditsBranchFilter;
    QPointer<QPropertyAnimation> m_animation;
    QPointer<QPushButton> m_pbBranchTree;

    DBFilterTreeViewPrivate(DBFilterTreeView *qq) : q_ptr(qq)
    {
        m_branchFilter = NULL;
    }

    void initDataStructures();
};

/**
 * @brief DBFilterTreeViewPrivate::initDataStructures
 * Según los metadatos del bean, se inician las estructuras adecuadas: Tablas, campos visibles, imágenes, ordenaciones, filtros...
 */
void DBFilterTreeViewPrivate::initDataStructures()
{
    if ( !m_tableNames.isEmpty() )
    {
        return;
    }
    foreach ( QVariant item, m_treeDefinition )
    {
        QHash<QString, QVariant> hash = item.toHash();
        BaseBeanMetadata *metadata = BeansFactory::metadataBean(hash.value("name").toString());
        m_tableNames.append(hash.value("name").toString());
        m_visibleFields.append(hash.value("visibleField").toString());
        m_images.append(hash.value("image").toString());
        m_allowInsert.append(hash.value("allowInsert").toString() == QStringLiteral("true") ? true : false);
        m_allowEdit.append(hash.value("allowEdit").toString() == QStringLiteral("true") ? true : false);
        m_allowDelete.append(hash.value("allowDelete").toString() == QStringLiteral("true") ? true : false);
        if ( metadata != NULL )
        {
            if ( metadata->initOrderSort().isEmpty() )
            {
                QList<DBFieldMetadata *> pkFields = metadata->pkFields();
                if ( pkFields.size() == 0 )
                {
                    m_sorts.append("");
                }
                else
                {
                    m_sorts.append(pkFields.first()->dbFieldName() + " DESC");
                }
            }
            else
            {
                m_sorts.append(metadata->initOrderSort());
            }
        }
        else
        {
            m_sorts.append("");
        }
        m_filtersFields.append("");
    }
    m_tableNames.append(q_ptr->tableName());
    m_visibleFields.append("*");
    m_images.append("");
    m_allowInsert.append(true);
    m_sorts.append(q_ptr->initSortForModel());
    m_filtersFields.append("");
}

DBFilterTreeView::DBFilterTreeView(QWidget *parent) : DBAbstractFilterView(parent), d(new DBFilterTreeViewPrivate(this))
{
    DBTreeView *tv = new DBTreeView(this);
    setItemView(tv);
    tv->setAutomaticName(false);
    tv->setExternalModel(true);
    tv->setSelectionBehavior(QAbstractItemView::SelectRows);
    tv->setSelectionMode(QAbstractItemView::SingleSelection);
    tv->setAlternatingRowColors(true);
    tv->setEditTriggers(QTreeView::NoEditTriggers);
    layout()->addWidget(tv);

    d->m_pbBranchTree = new QPushButton(ui->gbFilter);
    d->m_pbBranchTree->setObjectName(QString("pbBranchTree"));
    QIcon icon;
    icon.addFile(QString(":/generales/images/down.png"), QSize(), QIcon::Normal, QIcon::Off);
    d->m_pbBranchTree->setIcon(icon);
    d->m_pbBranchTree->setCheckable(true);
    QHBoxLayout *lay = qobject_cast<QHBoxLayout *>(ui->gbFilter->layout());
    if ( lay != NULL )
    {
        lay->insertWidget(0, d->m_pbBranchTree);
    }

    connect (d->m_pbBranchTree, SIGNAL(clicked()), this, SLOT(showOrHideBranchFilter()));
}

DBFilterTreeView::~DBFilterTreeView()
{
    delete d;
}

QVariantList DBFilterTreeView::treeDefinition()
{
    return d->m_treeDefinition;
}

void DBFilterTreeView::setTreeDefinition(const QVariantList &value)
{
    d->m_treeDefinition = value;
}

/**
 * @brief DBFilterTreeView::init
 * Inicia el control con las estructuras de datos adecuadas
 * @param createStrongFilter Se utilizaran filtros fuertes o no (con SQL)
 */
void DBFilterTreeView::init(bool initStrongFilter)
{
    d->initDataStructures();
    createBranchFilterWidget();

    if (initStrongFilter)
    {
        createStrongFilter();
        createSubTotals();
    }
    buildFilterWhere();

    TreeBaseBeanModel *model = new TreeBaseBeanModel(d->m_tableNames, false, this);
    setSourceModel(model);
    setFilterLevel(d->m_tableNames.size());
    model->setDeleteFromDB(true);
    model->setImages(d->m_images);
    model->setSize(QSize(16,16));
    model->setFieldsView(d->m_visibleFields);
    model->setSorts(d->m_sorts);
    model->setAllowInsert(d->m_allowInsert);
    model->setAllowEdit(d->m_allowEdit);
    model->setAllowDelete(d->m_allowDelete);
    model->setupInitialData();
    DBAbstractFilterView::init(initStrongFilter);
    calculateSubTotals();
}

/**
  Crea el widget que permitirá el filtrado a niveles del árbol
  */
void DBFilterTreeView::createBranchFilterWidget()
{
    if ( d->m_branchFilter == NULL )
    {
        int width = 0;
        int height = 0;
        QFrame *frame = new QFrame(this);
        frame->setAutoFillBackground(true);
        frame->setFrameStyle(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Raised);
        frame->setLineWidth(3);
        // Le añadimos una sombra molona.
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(2);
        frame->setGraphicsEffect(shadow);
        d->m_branchFilter = frame;
        QGridLayout *layout = new QGridLayout();
        for ( int i = 0 ; i < d->m_tableNames.size() - 1 ; i++ )
        {
            BaseBeanMetadata *m = BeansFactory::metadataBean(d->m_tableNames.at(i));
            if ( m != NULL )
            {
                QLineEdit *filterEdit = new QLineEdit(frame);
                d->m_lineEditsBranchFilter.append(filterEdit);
                QLabel *filterLabel = new QLabel(trUtf8("Filtro para %1").arg(m->alias()), frame);
                filterEdit->resize(200, filterEdit->height());
                width = filterLabel->width() + filterEdit->width() + 20;
                height += filterEdit->height() + 5;
                filterEdit->setObjectName(QString("filterLevel%1").arg(i));
                filterEdit->setProperty(AlephERP::stFilterLevelText, m->tableName());
                filterEdit->installEventFilter(this);
                DBFieldMetadata *field = m->field(d->m_visibleFields.at(i));
                if ( field != NULL )
                {
                    filterEdit->setProperty(AlephERP::stDbFieldName, field->dbFieldName());
                }
                layout->addWidget(filterLabel, i, 0);
                layout->addWidget(filterEdit, i, 1);
                connect(filterEdit, SIGNAL(textChanged(QString)), this, SLOT(fastFilterByTextIntermediateLevels()));
            }
        }
        frame->setLayout(layout);
        d->m_branchFilter->show();
        d->m_branchFilterSize = QSize(width, height);
        d->m_branchFilter->resize(QSize(0, 0));
    }
}

void DBFilterTreeView::showEvent(QShowEvent *ev)
{
    DBAbstractFilterView::showEvent(ev);
    DBTreeView *tv = qobject_cast<DBTreeView *>(itemView());
    if ( tv )
    {
        // En el modelo en árbol, y por cómo se construye internamentre la estructura de datos, es obligatorio
        // forzar la ordenación
        tv->setSortingEnabled(true);
    }
}

void DBFilterTreeView::showOrHideBranchFilter()
{
    if ( d->m_animation == NULL )
    {
        d->m_animation = new QPropertyAnimation(d->m_branchFilter, "geometry", this);
    }
    QPoint pos = d->m_pbBranchTree->pos() + QPoint(d->m_pbBranchTree->frameGeometry().width(), d->m_pbBranchTree->frameGeometry().height());
    QRect startValue(pos, QSize(0, 0));
    QRect stopValue(pos, d->m_branchFilterSize);
    if ( d->m_pbBranchTree->isChecked() )
    {
        d->m_animation->setStartValue(startValue);
        d->m_animation->setEndValue(stopValue);
        if ( d->m_lineEditsBranchFilter.size() > 0 )
        {
            d->m_lineEditsBranchFilter.at(0)->setFocus();
        }
    }
    else
    {
        d->m_animation->setStartValue(d->m_branchFilter->geometry());
        d->m_animation->setEndValue(startValue);
    }
    d->m_animation->setDuration(125);
    d->m_animation->start(QAbstractAnimation::KeepWhenStopped);
}

void DBFilterTreeView::refresh()
{
    DBAbstractFilterView::refresh();
    // Nos aseguramos que las ramas intermedias estén extendidas a lo largo de toda la fila
    (qobject_cast<DBTreeView *>(itemView()))->spanOnIntermediateBranchs();
}

/*!
  Tenemos que decirle al motor de scripts, que DBFormDlg se convierte de esta forma a un valor script
  */
QScriptValue DBFilterTreeView::toScriptValue(QScriptEngine *engine, DBFilterTreeView * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBFilterTreeView::fromScriptValue(const QScriptValue &object, DBFilterTreeView * &out)
{
    out = qobject_cast<DBFilterTreeView *>(object.toQObject());
}

void DBFilterTreeView::fastFilterByTextIntermediateLevels()
{
    foreach ( QLineEdit *le, d->m_lineEditsBranchFilter )
    {
        QString tableIdx = le->objectName();
        QString dbFieldName = le->property(AlephERP::stDbFieldName).toString();
        tableIdx = tableIdx.replace("filterLevel", "");
        int level = tableIdx.toInt() + 1;
        filterModel()->setFilterKeyColumn(dbFieldName, QRegExp(le->text()), level);
        filterModel()->invalidate();
        // Nos aseguramos que las ramas intermedias estén extendidas a lo largo de toda la fila
        (qobject_cast<DBTreeView *>(itemView()))->spanOnIntermediateBranchs();
    }
}

