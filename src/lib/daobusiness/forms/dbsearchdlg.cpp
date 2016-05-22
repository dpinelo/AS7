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
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtScript>
#include <QtCore>
#include <QtUiTools>
#include "configuracion.h"
#include "qlogger.h"
#include <globales.h>
#include "dbsearchdlg.h"
#include "uiloader.h"
#include "dao/basedao.h"
#include "dao/beans/basebean.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/dbfield.h"
#include "models/dbbasebeanmodel.h"
#include "models/filterbasebeanmodel.h"
#include "models/treebasebeanmodel.h"
#include "models/basebeanmodel.h"
#include "models/relationbasebeanmodel.h"
#include "widgets/dbnumberedit.h"
#include "widgets/dblineedit.h"
#include "widgets/dbcombobox.h"
#include "widgets/dbcheckbox.h"
#include "widgets/dbchooserecordbutton.h"
#include "forms/dbrecorddlg.h"
#include "scripts/perpscript.h"
#include "scripts/perpscriptwidget.h"
#include "widgets/dbdatetimeedit.h"
#include "business/aerploggeduser.h"
#include "ui_dbsearchdlg.h"
#ifdef ALEPHERP_ADVANCED_EDIT
#include "forms/aerpsystemobjecteditdlg.h"
#endif

typedef QHash<QString, QVariant> QHashVariant;

#define OP_BETWEEN  "BETWEEN"
#define OP_AND      "AND"
#define OP_LIKE     "LIKE"
#define OP_EQUAL    "="
#define OP_GREATER  ">"
#define OP_LESS     "<"

/**
 * @brief The SearchWidgetsValues class
 * Esta clase contendrá las búsquedas realizadas ya.
 */
class SearchPerformed
{
public:
    /** Valores de la búsqueda: por cada control de la ventana de búsqueda sobre los que el usuario puede interactuar
     * se almacena el valor de la búsqueda realizada. Se utiliza Set para las comparaciones = */
    QList<QHashVariant> m_widgetsSearchValues;
    /** Número de valores que se obtuvieron para esa búsqueda */
    int m_results;

    SearchPerformed()
    {
        m_results = 0;
    }

    bool isEvolution (SearchPerformed *other);
    QHashVariant item(const QString &widgetName);
};

QHashVariant SearchPerformed::item(const QString &widgetName)
{
    foreach (const QHashVariant &item, m_widgetsSearchValues)
    {
        if ( item["fieldName"] == widgetName )
        {
            return item;
        }
    }
    return QHashVariant();
}

class DBSearchDlgPrivate
{
//	Q_DECLARE_PUBLIC(DBSearchDlg)
public:
    DBSearchDlg *q_ptr;
    /** Bean maestro para conocer los datos a editar */
    QPointer<BaseBeanMetadata> m_metadata;
    /** Bean template (sin uso) para asociar a los widgets que se introduzcan dentro de este control */
    QPointer<BaseBean> m_templateBean;
    /** Bean en el que quedarán los datos que el usuario ha seleccionado en su búsqueda */
    BaseBeanSharedPointer m_selectedBean;
    /** Es posible restringir la búsqueda a los children de una determinada relación de un bean determinado
     * y pasado a este formulario. Se almacena aquí. */
    BaseBeanPointer m_masterBean;
    /** Este formulario no tiene potestad para insertar registros. Eso significa que el botón que se presenta
     * sólo se habilitará si se pasa un registro en blanco. */
    BaseBeanSharedPointer m_beanToInsert;
    /** En el caso de que este registro esté a no nulo, se ignorará m_beanToInsert (Es un workaround para trabajar
     * con el father, que es un basebeanpointer). No me gusta. */
    BaseBeanPointer m_insertFather;
    /** Modelo sobre el que se realizará la búsqueda */
    QPointer<BaseBeanModel> m_model;
    /** Proxy que hace el filtro */
    QPointer<FilterBaseBeanModel> m_filter;
    /** Filtro fuerte que establece los datos que puede ver este formulario */
    QString m_filterData;
    /** Interfaz leída de la base de datos */
    QPointer<QWidget> m_widget;
    /** Para tratar con las selecciones */
    QPointer<QItemSelectionModel> m_modelSelection;
    /** Indica si el usuario realizó la búsqueda o pinchó el botón de salir sin buscar */
    bool m_userClickOk;
    /** El usuario ha insertado un nuevo registro */
    bool m_userInsertNewRecord;
    /** Histórico de búsquedas y resultados */
    QList<SearchPerformed *> m_searchHistory;
    bool m_canSelectSeveral;
    BaseBeanSharedPointerList m_checkedBeans;
    /** Indicará si el formulario es autogenerado */
    bool m_autogeneratedWidgets;
    /** Valores por defecto para la búsqueda */
    QHash<QString, QVariant> m_defaultValues;
    /** Timer para controlar las pulsaciones de teclas y evitar muchas llamadas para búsquedas */
    QTimer m_keyPressTimer;
    /** Si este diálogo se abre desde otra ventana (un record) con contexto, puede controlar si los formularios de edición o apertura de registros
     * pertenecen a esa transacción o no*/
    QString m_contextName;

    DBSearchDlgPrivate(DBSearchDlg *qq) : q_ptr(qq)
    {
        m_userClickOk = false;
        m_userInsertNewRecord = false;
        m_canSelectSeveral = false;
        m_autogeneratedWidgets = false;
    }

    void setupWidget();
    bool setupExternalWidget();
    void setCheckStateToPartially();
    void setCombosNotSelected();
    void setupQs();
    void connectObjectsToSearch();
    void initTreeModel();
    void initTableModel();

    bool haveToApplySqlFilter(SearchPerformed *searchValues);
    void applyProxyFilter(SearchPerformed *searchState);
    void applySqlFilter(SearchPerformed *searchState);
    void resetFilter();
    QString sqlWhere(SearchPerformed *searchState) const;
    QList<QHashVariant> searchState();
    void searchStateOnAutogeneratedBetweenWidgets(QWidget *widget, DBFieldMetadata *fld, QList<QHashVariant> &widgetsState, QStringList &fieldsOnState);
    void searchStateOnAutogeneratedWidgetsWithComboSelector(QWidget *widget, QComboBox *cb, DBFieldMetadata *fld, QList<QHashVariant> &widgetsState, QStringList &fieldsOnState);
    void searchStateOnAutogeneratedWidget(QWidget *widget, DBFieldMetadata *fld, QList<QHashVariant> &widgetsState, QStringList &fieldsOnState);
    void searchStateOnProvidedWidget(QWidget *widget, DBFieldMetadata *fld, QList<QHashVariant> &widgetsState, QStringList &fieldsOnState);
    void searchStateOnBetweenWidgets(DBBaseWidget *wid1, DBBaseWidget *wid2, DBFieldMetadata *fld, QList<QHashVariant> &widgetsState, QStringList &fieldsOnState);
    void setToolTipForWidgets();
    bool isEmptySearch();
    SearchPerformed *findEvolution(SearchPerformed *searchValues);
    BaseBeanPointerList fathersForTreeView();
};

DBSearchDlg::DBSearchDlg(QWidget *parent) :
    AERPBaseDialog(parent),
    ui(new Ui::DBSearchDlg),
    d(new DBSearchDlgPrivate(this))
{
    setOpenSuccess(true);
}

DBSearchDlg::DBSearchDlg(const QString &tableName, const QString &contextName, QWidget *parent) :
    AERPBaseDialog(parent),
    ui(new Ui::DBSearchDlg),
    d(new DBSearchDlgPrivate(this))
{
    d->m_contextName = contextName;
    if ( setTableName(tableName) )
    {
        setOpenSuccess(true);
    }
}

DBSearchDlg::~DBSearchDlg()
{
    qDeleteAll(d->m_searchHistory);
    delete d;
    delete ui;
}

bool DBSearchDlg::userClickOk() const
{
    return d->m_userClickOk;
}

/**
 * @brief DBSearchDlg::userInsertNewRecord
 * Si el usuario insertó un nuevo registro, se ha editado directamente el father bean de la relación M1
 * del workedBean. Por lo tanto, no será necesario invocar en ningún caso a selectedBean.
 * @return
 */
bool DBSearchDlg::userInsertNewRecord() const
{
    return d->m_userInsertNewRecord;
}

QString DBSearchDlg::filterData() const
{
    return d->m_filterData;
}

void DBSearchDlg::setFilterData(const QString &value)
{
    d->m_filterData = value;
}

bool DBSearchDlg::canInsertRecords() const
{
    return ui->pbNewRecord->isVisible();
}

void DBSearchDlg::setCanInsertRecords(bool value)
{
    ui->pbNewRecord->setVisible(value);
}

bool DBSearchDlg::canEditRecords() const
{
    return ui->pbEdit->isVisible();
}

void DBSearchDlg::setCanEditRecords(bool value)
{
    ui->pbEdit->setVisible(value);
}

bool DBSearchDlg::canSelectSeveral()
{
    return d->m_canSelectSeveral;
}

void DBSearchDlg::setCanSelectSeveral(bool value)
{
    d->m_canSelectSeveral = value;
}

/*!
  Devuelve el bean seleccionado en el QTableView
  */
BaseBeanSharedPointer DBSearchDlg::selectedBean()
{
    if ( d->m_userInsertNewRecord )
    {
        if ( d->m_insertFather.isNull() )
        {
            return d->m_beanToInsert;
        }
        else
        {
            //return d->m_insertFather;
            return BaseBeanSharedPointer();
        }
    }
    return d->m_selectedBean;
}

BaseBeanSharedPointerList DBSearchDlg::checkedBeans()
{
    return d->m_checkedBeans;
}

bool DBSearchDlg::setTableName(const QString &value)
{
    AERPBaseDialog::setTableName(value);
    if ( !AERPLoggedUser::instance()->checkMetadataAccess('r', value) )
    {
        QMessageBox::warning(this, qApp->applicationName(), trUtf8("No tiene permisos para acceder a estos datos."), QMessageBox::Ok);
        setOpenSuccess(false);
        close();
        return false;
    }
    d->m_metadata = BeansFactory::metadataBean(value);
    d->m_templateBean = BeansFactory::instance()->newBaseBean(value, true, false);
    d->m_templateBean->setParent(this);
    ui->setupUi(this);
    if ( d->m_metadata == NULL )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, trUtf8("DBSearchDlg: No existe la tabla: [%1]").arg(value));
        close();
        setOpenSuccess(false);
        return false;
    }
    setObjectName(QString("%1.search.ui").arg(value));
    d->m_userClickOk = false;
    d->setupWidget();
    installEventFilters();
    return true;
}

BaseBean *DBSearchDlg::templateBean()
{
    return d->m_templateBean;
}

BaseBeanPointer DBSearchDlg::masterBean() const
{
    return d->m_masterBean;
}

void DBSearchDlg::setMasterBean(BaseBeanPointer bean)
{
    d->m_masterBean = bean;
}

QWidget *DBSearchDlg::contentWidget() const
{
    return d->m_widget;
}

QScriptValue DBSearchDlg::toScriptValue(QScriptEngine *engine, DBSearchDlg * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBSearchDlg::fromScriptValue(const QScriptValue &object, DBSearchDlg *&out)
{
    out = qobject_cast<DBSearchDlg *>(object.toQObject());
}

bool DBSearchDlg::init()
{
    if ( d->m_metadata == NULL )
    {
        return false;
    }
    // Creemos los objetos de negocio
    if ( d->m_metadata->showOnTree() )
    {
        ui->swPages->setCurrentIndex(1);
        d->initTreeModel();
    }
    else
    {
        ui->swPages->setCurrentIndex(0);
        d->initTableModel();
    }

    // Ponemos la marca de los check boxes, en el primer elemento visible
    if ( d->m_canSelectSeveral && d->m_model )
    {
        QList<DBFieldMetadata *> list = d->m_metadata->fields();
        bool setCheckedColumn = false;
        foreach ( DBFieldMetadata *fld, list )
        {
            if ( fld->visibleGrid() && !setCheckedColumn )
            {
                QStringList fieldsList;
                fieldsList.append(fld->dbFieldName());
                setCheckedColumn = true;
                d->m_model->setCheckColumns(fieldsList);
            }
        }
        if ( !setCheckedColumn )
        {
            d->m_canSelectSeveral = false;
        }
    }
    // Importante que estén aquí
    ui->pbCheckAll->setVisible(d->m_canSelectSeveral);
    ui->pbUncheckAll->setVisible(d->m_canSelectSeveral);
    if ( d->m_canSelectSeveral )
    {
        connect(ui->pbCheckAll, SIGNAL(clicked(bool)), this, SLOT(checkAll()));
        connect(ui->pbUncheckAll, SIGNAL(clicked(bool)), this, SLOT(uncheckAll()));
    }

    setWindowTitle(trUtf8("Búsqueda de %1 [*]").arg(d->m_metadata->alias()));
    d->setToolTipForWidgets();

    // Establecemos antes de abrir el Qs, los valores por defecto
    QList<QWidget *> list = findChildren<QWidget *>();
    foreach (QWidget *widget, list)
    {
        if ( widget->property(AlephERP::stAerpControl).toBool() )
        {
            DBBaseWidget *baseWidget = dynamic_cast<DBBaseWidget *>(widget);
            QHashIterator<QString, QVariant> it(d->m_defaultValues);
            while (it.hasNext())
            {
                it.next();
                if ( baseWidget->fieldName() == it.key() )
                {
                    baseWidget->setValue(it.value());
                }
            }
            // Por defecto todos los controles en el DBSearch son editables.
            baseWidget->setDataEditable(true);
        }
    }
    d->setupQs();
    if ( !d->m_defaultValues.isEmpty() )
    {
        search();
    }
    connect(ui->pbClose, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui->tvResults, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(select(QModelIndex)));
    connect(ui->pbOk, SIGNAL(clicked()), this, SLOT(select()));
    connect(ui->pbEdit, SIGNAL(clicked()), this, SLOT(edit()));
    connect(ui->pbView, SIGNAL(clicked()), this, SLOT(view()));
    connect(ui->pbNewRecord, SIGNAL(clicked()), this, SLOT(newRecord()));
    connect(ui->tvResults, SIGNAL(enterPressedOnValidIndex(QModelIndex)), this, SLOT(select()));
    connect(ui->tvResults, SIGNAL(doubleClickOnValidIndex(const QModelIndex&)), this, SLOT(select()));
    connect(ui->treeResults, SIGNAL(enterPressedOnValidIndex(QModelIndex)), this, SLOT(select()));
    connect(ui->treeResults, SIGNAL(doubleClickOnValidIndex(const QModelIndex&)), this, SLOT(select()));
    connect(&d->m_keyPressTimer, SIGNAL(timeout()), this, SLOT(search()));
    d->m_keyPressTimer.setSingleShot(true);
    d->connectObjectsToSearch();

    setFocusOnFirstWidget();
    return true;
}

void DBSearchDlgPrivate::initTableModel()
{
    if ( m_metadata == NULL )
    {
        return;
    }
    // En este formulario, siempre ordenaremos por la primera columna visible...
    QString firstVisible, initOrder;
    foreach (DBFieldMetadata *fld, m_metadata->fields())
    {
        if ( firstVisible.isEmpty() && fld->visibleGrid() && fld->isOnDb() )
        {
            firstVisible = fld->dbFieldName();
        }
    }
    if ( !firstVisible.isEmpty() )
    {
        initOrder = QString("%1 ASC").arg(firstVisible);
    }
    // El modelo que se presentará dependerá de si se ha pasado un masterBean o no
    if ( !m_masterBean.isNull() )
    {
        DBRelation *rel = m_masterBean->relation(q_ptr->tableName());
        if ( rel != NULL && rel->metadata()->type() == DBRelationMetadata::ONE_TO_MANY )
        {
            m_model = new RelationBaseBeanModel(rel, true, "", q_ptr);
        }
    }
    if ( m_model.isNull() )
    {
        m_model = new DBBaseBeanModel(q_ptr->tableName(), m_filterData, initOrder, true, true, true, q_ptr);
    }
    m_filter = new FilterBaseBeanModel(q_ptr);
    // Sólo se visualizan registros con permisos seleccionable
    m_filter->setAccessFilter('s');
    m_modelSelection = new QItemSelectionModel(m_filter, q_ptr);
    m_filter->setSourceModel(m_model);

    // Debe hacerse en este orden para que no se produzcan retrasos de ordenación.
    // Es importante indicar la columna por la que se ordene y que está ordenado
    // antes de introducir el modelo. El modelo aseguramos que viene en ese orden.
    // Lo que se garantiza con esto es que de entrada, FilterBaseBeanModel no ordene
    // y la ordenación la haga la base de datos.
    QList<DBFieldMetadata *> list = m_filter->visibleFields();
    for ( int i = 0 ; i < list.size() ; i++ )
    {
        if ( list.at(i)->dbFieldName() == firstVisible )
        {
            q_ptr->ui->tvResults->QTableView::sortByColumn(i, Qt::AscendingOrder);
            q_ptr->ui->tvResults->setSortingEnabled(true);
            q_ptr->ui->tvResults->horizontalHeader()->setSortIndicatorShown(true);
            break;
        }
    }

    q_ptr->ui->tvResults->setModel(m_filter);
    q_ptr->ui->tvResults->setSelectionModel(m_modelSelection);
}

void DBSearchDlg::showEvent(QShowEvent *event)
{
    AERPBaseDialog::showEvent(event);
    if ( d->m_templateBean.isNull() )
    {
        return;
    }
    // Esto es necesario para permitir que haya dos controles que compartan mismo fieldName pero diferentes
    // values y nos permita simular un between
    foreach (DBField *fld, d->m_templateBean->fields())
    {
        fld->disconnect(SIGNAL(valueModified(QVariant)));
    }
}

void DBSearchDlgPrivate::initTreeModel()
{
    QStringList tableNames, visibleFields, images, sorts;

    if ( m_metadata == NULL )
    {
        return;
    }
    foreach ( const QVariant &item, m_metadata->treeDefinitions() )
    {
        QHashVariant hash = item.toHash();
        BaseBeanMetadata *metadata = BeansFactory::metadataBean(hash.value("name").toString());
        tableNames.append(hash.value("name").toString());
        visibleFields.append(hash.value("visibleField").toString());
        images.append(hash.value("image").toString());
        if ( metadata != NULL )
        {
            if ( metadata->initOrderSort().isEmpty() )
            {
                QList<DBFieldMetadata *> pkFields = metadata->pkFields();
                if ( pkFields.size() == 0 )
                {
                    sorts.append("");
                }
                else
                {
                    sorts.append(pkFields.first()->dbFieldName() + " DESC");
                }

            }
            else
            {
                sorts.append(metadata->initOrderSort());
            }
        }
        else
        {
            sorts.append("");
        }
    }
    tableNames.append(m_metadata->tableName());
    visibleFields.append("*");
    images.append("");
    sorts.append(m_metadata->initOrderSort());

    if ( m_model.isNull() )
    {
        TreeBaseBeanModel *treeModel = new TreeBaseBeanModel(tableNames, false, q_ptr);
        treeModel->setDeleteFromDB(false);
        treeModel->setImages(images);
        treeModel->setSize(QSize(16,16));
        treeModel->setFieldsView(visibleFields);
        treeModel->setSorts(sorts);
        treeModel->setupInitialData();
        // No tiene sentido en esta ventana de búsqueda estar recargando el modelo...
        treeModel->freezeModel();
        m_model = treeModel;
    }
    m_filter = new FilterBaseBeanModel(q_ptr);
    m_filter->setAccessFilter('s');
    m_modelSelection = new QItemSelectionModel(m_filter, q_ptr);
    m_filter->setSourceModel(m_model);
    q_ptr->ui->treeResults->setModel(m_filter);
    q_ptr->ui->treeResults->setSelectionModel(m_modelSelection);
    QObject::connect(m_model, SIGNAL(beansLoadFinished()), m_filter, SLOT(invalidate()));
}

DBSearchDlg::DBSearchButtons DBSearchDlg::visibleButtons()
{
    DBSearchDlg::DBSearchButtons flag;
    if ( ui->pbClose->isVisible() )
    {
        flag |= DBSearchDlg::Close;
    }
    if ( ui->pbEdit->isVisible() )
    {
        flag |= DBSearchDlg::EditRecord;
    }
    if ( ui->pbNewRecord->isVisible() )
    {
        flag |= DBSearchDlg::NewRecord;
    }
    if ( ui->pbOk->isVisible() )
    {
        flag |= DBSearchDlg::Ok;
    }
    return flag;
}

void DBSearchDlg::setVisibleButtons(DBSearchDlg::DBSearchButtons buttons)
{
    ui->pbClose->setVisible(buttons.testFlag(DBSearchDlg::Close));
    ui->pbEdit->setVisible(buttons.testFlag(DBSearchDlg::EditRecord));
    ui->pbNewRecord->setVisible(buttons.testFlag(DBSearchDlg::NewRecord));
    ui->pbOk->setVisible(buttons.testFlag(DBSearchDlg::Ok));
}

/**
 * @brief DBSearchDlgPrivate::setupWidget
 * Establece el widget de búsqueda
 */
void DBSearchDlgPrivate::setupWidget()
{
    if ( setupExternalWidget() )
    {
        // Establecemos algunos valores por defecto
        QList<DBDateTimeEdit *> list = q_ptr->findChildren<DBDateTimeEdit *>();
        foreach (DBDateTimeEdit *de, list)
        {
            de->setDate(alephERPSettings->minimumDate());
            QObject::connect (de, SIGNAL(valueEdited(QVariant)), q_ptr, SLOT(setDateForAnotherDate()));
        }
        setCombosNotSelected();
        setCheckStateToPartially();
        m_autogeneratedWidgets = false;
        return;
    }
    QLayout *layout = q_ptr->ui->gbSearchFields->layout();
    if ( layout != NULL )
    {
        delete layout;
    }
    layout = new QGridLayout();
    QHash<int, QWidgetList> widgets = q_ptr->setupWidgetFromBaseBeanMetadata(m_metadata, qobject_cast<QGridLayout *>(layout), true);
    if ( widgets.keys().size() > 0 )
    {
        q_ptr->ui->gbSearchFields->setVisible(true);
        q_ptr->ui->gbSearchFields->setLayout(layout);
    }
    else
    {
        delete layout;
        q_ptr->ui->gbSearchFields->setVisible(false);
    }

    setCombosNotSelected();
    setCheckStateToPartially();
    m_autogeneratedWidgets = true;
}

/**
 * @brief DBSearchDlg::setupExternalWidget
 * El widget está definido por el programador QS
 * @return
 */
bool DBSearchDlgPrivate::setupExternalWidget()
{
    QString fileName;
    QLogger::QLog_Debug(AlephERP::stLogOther, QString("DBSearchDlgPrivate::setupExternalWidget: Tabla: %1 tiene definido el dbsearch: %2").
                        arg(q_ptr->tableName()).arg(m_metadata->uiDbSearch()));
    if ( m_metadata->uiDbSearch().isEmpty() )
    {
        fileName = QString("%1/%2.search.ui").arg(QDir::fromNativeSeparators(alephERPSettings->dataPath())).
                   arg(q_ptr->tableName());
    }
    else
    {
        if ( !m_metadata->uiDbSearch().endsWith(".search.ui" ))
        {
            fileName = QString("%1/%2.search.ui").arg(QDir::fromNativeSeparators(alephERPSettings->dataPath())).
                       arg(m_metadata->uiDbSearch());
        }
        else
        {
            fileName = QString("%1/%2").arg(QDir::fromNativeSeparators(alephERPSettings->dataPath())).
                       arg(m_metadata->uiDbSearch());
        }
    }
    QFile file (fileName);
    bool result;

    if ( file.exists() )
    {
        file.open( QFile::ReadOnly );
        m_widget = AERPUiLoader::instance()->load(&file, 0);
        if ( m_widget != NULL )
        {
            m_widget->setParent(q_ptr);
            q_ptr->ui->widgetLayout->setContentsMargins(0, 0, 0, 0);
            q_ptr->ui->widgetLayout->addWidget(m_widget);
            m_widget->show();
        }
        else
        {
            QMessageBox::warning(q_ptr, qApp->applicationName(),
                                 QObject::trUtf8("No se ha podido cargar la interfaz de usuario de este formulario <i>%1</i>. "
                                                 "Existe un problema en la definición de las tablas de sistema de su programa.").arg(fileName),
                                 QMessageBox::Ok);
            q_ptr->close();
        }
        result = true;
    }
    else
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("DBSearchDlgPrivate::setupExternalWidget: No existe el fichero %1").arg(fileName));
        result = false;
    }
    file.close();
    return result;
}

void DBSearchDlgPrivate::setCheckStateToPartially()
{
    // Los checks tienen historia: ¿Seleccionados o no seleccionados? Funcionamiento incómodo. Por ello, inicialmente los ponemos en
    // modo "triestado", y si el usuario no ha seleccionada nada, no se incluyen en el filtro
    QList<DBCheckBox *> chks = q_ptr->findChildren<DBCheckBox *>();
    foreach (DBCheckBox *chk, chks)
    {
        chk->setCheckState(Qt::PartiallyChecked);
    }
}

void DBSearchDlgPrivate::setCombosNotSelected()
{
    QList<DBComboBox *> widgets = q_ptr->findChildren<DBComboBox *>();
    foreach (DBComboBox *widget, widgets)
    {
        widget->setCurrentIndex(-1);
    }
}

/**
 * @brief DBSearchDlg::setupQs
 * Ejecuta el código QScript asociado.
 */
void DBSearchDlgPrivate::setupQs()
{
    QString qsName;
    QString uiName;
    if ( m_metadata->uiDbSearch().isEmpty() )
    {
        uiName = QString("%1.search.ui").arg(q_ptr->tableName());
    }
    else
    {
        uiName = m_metadata->uiDbSearch();
    }
    if ( m_metadata->qsDbSearch().isEmpty() )
    {
        qsName = QString ("%1.search.qs").arg(q_ptr->tableName());
    }
    else
    {
        qsName = m_metadata->qsDbSearch();
    }
    QHashIterator<QString, QObject *> itObjects(q_ptr->thisFormObjectProperties());
    QHashIterator<QString, QVariant> itData(q_ptr->thisFormValueProperties());

    // Ejecutamos el script asociado. La filosofía fundamental de ese script es proporcionar
    // algo de código básico que justifique este formulario de búsqueda
    if ( !BeansFactory::systemScripts.contains(qsName) ||
            !BeansFactory::systemWidgets.contains(uiName) )
    {
        return;
    }

    q_ptr->aerpQsEngine()->setScript(BeansFactory::systemScripts.value(qsName), qsName);
    q_ptr->aerpQsEngine()->setDebug(BeansFactory::systemScriptsDebug.value(qsName));
    q_ptr->aerpQsEngine()->setOnInitDebug(BeansFactory::systemScriptsDebugOnInit.value(qsName));
    q_ptr->aerpQsEngine()->setScriptObjectName("DBSearchDlg");
    q_ptr->aerpQsEngine()->setPrototypeScript(BeansFactory::systemScripts.value(m_metadata->qsPrototypeDbSearch()));
    q_ptr->aerpQsEngine()->setUi(m_widget);
    q_ptr->aerpQsEngine()->setThisFormObject(q_ptr);
    while ( itObjects.hasNext() )
    {
        itObjects.next();
        q_ptr->aerpQsEngine()->addPropertyToThisForm(itObjects.key(), itData.value());
    }
    while ( itData.hasNext() )
    {
        itData.next();
        q_ptr->aerpQsEngine()->addPropertyToThisForm(itData.key(), itData.value());
    }
    AERPBaseDialog::exposeAERPControlToQsEngine(q_ptr, q_ptr->aerpQsEngine());
    CommonsFunctions::setOverrideCursor(QCursor(Qt::ArrowCursor));
    bool result = q_ptr->aerpQsEngine()->createQsObject();
    CommonsFunctions::restoreOverrideCursor();
    if ( !result )
    {
        QMessageBox::warning(q_ptr, qApp->applicationName(),
                             QObject::trUtf8("Ha ocurrido un error al cargar el script asociado a este "
                                             "formulario. Es posible que algunas funciones no estén disponibles."),
                             QMessageBox::Ok);
    }
}

/*!
  Este slot será al que se conectará el script de Qt que controla este formulario, para introducir
  los datos de filtrado del usuario
  */
void DBSearchDlg::setColumnFilter (const QString &name, const QVariant &value, const QString &op)
{
    /** Por defecto, y por el momento, este formulario sólo realizará búsquedas en la hoja del árbol, si
      se está presentando un DBTreeView */
    int level;
    if ( d->m_metadata->showOnTree() )
    {
        level = d->m_metadata->treeDefinitions().count() + 1;
    }
    else
    {
        level = -1;
    }
    // Si el variant es un QList, entonces es un tipo primary key, y seguro que trae un conjunto
    // de pares columna:valor
    if ( value.canConvert<QMap<QString, QVariant> >() )
    {
        d->m_filter->setFilterPkColumn(value, level);
    }
    else
    {
        d->m_filter->setFilterKeyColumn(name, value, op, level);
    }
    d->m_filter->invalidate();
    // Nos aseguramos que las ramas intermedias estén extendidas a lo largo de toda la fila
    ui->treeResults->spanOnIntermediateBranchs();
}

/*!
  Este slot será al que se conectará el script de Qt que controla este formulario, para introducir
  los datos de filtrado del usuario. Esta función está pensada para intervalos de fecha
  */
void DBSearchDlg::setColumnBetweenFilter (const QString &name, const QVariant &value1, const QVariant &value2)
{
    int level;
    if ( d->m_metadata->showOnTree() )
    {
        level = d->m_metadata->treeDefinitions().count() + 1;
    }
    else
    {
        level = -1;
    }
    d->m_filter->setFilterKeyColumnBetween(name, value1, value2, level);
    d->m_filter->invalidate();
    // Nos aseguramos que las ramas intermedias estén extendidas a lo largo de toda la fila
    ui->treeResults->spanOnIntermediateBranchs();

}

/*!
  Responde al evento de DobleClick en el QTableView, selecciona el item, y cierra el formulario
  */
void DBSearchDlg::select (const QModelIndex &index)
{
    if ( d->m_filter.isNull() || d->m_model.isNull() )
    {
        return;
    }
    QModelIndex source = d->m_filter->mapToSource(index);
    if ( !d->m_canSelectSeveral )
    {
        if ( index.isValid() )
        {
            d->m_selectedBean = d->m_model->bean(source);
        }
        else
        {
            QModelIndex idx = d->m_modelSelection->currentIndex();
            if ( idx.isValid() )
            {
                source = d->m_filter->mapToSource(idx);
                BaseBeanSharedPointer bean = d->m_model->bean(source);
                QString controlTableName = d->m_metadata->viewOnGrid().isEmpty() ? d->m_metadata->tableName() : d->m_metadata->viewOnGrid();
                if ( bean->metadata()->tableName() != controlTableName )
                {
                    QMessageBox::warning(this,qApp->applicationName(), trUtf8("Debe seleccionar un elemento de tipo <strong>%1</strong>. "
                                         "Ha intentado seleccionar un elemento de tipo %2. Debe escoger un elemento final del árbol.").
                                         arg(d->m_metadata->alias()).arg(bean->metadata()->alias()), QMessageBox::Ok);
                    return;
                }
                d->m_selectedBean = bean;
            }
        }
    }
    else
    {
        QModelIndexList list = d->m_model->checkedItems();
        d->m_checkedBeans = d->m_model->beans(list);
    }
    d->m_userClickOk = true;
    close();
}

/*!
  Conecta todos los objetos creados para la búsqueda con la propia búsqueda. Cuando se modifiquen los objetos,
  se invocará al slot "search" que realizará la búsqueda
  */
void DBSearchDlgPrivate::connectObjectsToSearch()
{
    QList<QWidget *> widgets = q_ptr->findChildren<QWidget *>();
    foreach ( QWidget *widget, widgets )
    {
        if ( widget->property(AlephERP::stAerpControl).toBool() )
        {
            QObject::connect(widget, SIGNAL(valueEdited(QVariant)), q_ptr, SLOT(search()));
        }
    }
}

/**
 * @brief DBSearchDlg::edit
 * Slot para editar el registro actual
 */
void DBSearchDlg::edit()
{
    AlephERP::FormOpenType openType = AlephERP::Update;
    QString functionName = "beforeEdit";

    if ( aerpQsEngine()->existQsFunction(functionName) )
    {
        QScriptValue result;
        if ( aerpQsEngine()->callQsObjectFunction(result, functionName) )
        {
            if ( !result.toBool() )
            {
                return;
            }
        }
    }
    CommonsFunctions::setOverrideCursor(QCursor(Qt::WaitCursor));

    QModelIndex idx = d->m_modelSelection->currentIndex();
    if ( idx.isValid() )
    {
        QModelIndex source = d->m_filter->mapToSource(idx);
        d->m_selectedBean = d->m_model->bean(source);
    }
    if ( !d->m_selectedBean.isNull() )
    {
        QPointer<DBRecordDlg> dlg;
        if ( d->m_selectedBean->metadata()->tableName() == QString("%1_system").arg(alephERPSettings->systemTablePrefix()) )
        {
#if defined(ALEPHERP_ADVANCED_EDIT) && defined (ALEPHERP_DEVTOOLS)
            dlg = new AERPSystemObjectEditDlg(d->m_selectedBean.data(), openType, d->m_contextName, this);
#else
            return;
#endif
        }
        else
        {
            dlg = new DBRecordDlg(d->m_selectedBean.data(), openType, d->m_contextName, this);
        }
        CommonsFunctions::restoreOverrideCursor();
        if ( dlg->openSuccess() && dlg->init() )
        {
            dlg->setModal(true);
            dlg->exec();
        }
        delete dlg;
    }
    else
    {
        QMessageBox::warning(this,qApp->applicationName(), trUtf8("Debe seleccionar un registro a editar."));
    }
}

/**
 * @brief DBSearchDlg::view
 * Slot para visualizar el registro actual
 */
void DBSearchDlg::view()
{
    AlephERP::FormOpenType openType = AlephERP::ReadOnly;

    CommonsFunctions::setOverrideCursor(QCursor(Qt::WaitCursor));

    QModelIndex idx = d->m_modelSelection->currentIndex();
    if ( idx.isValid() )
    {
        QModelIndex source = d->m_filter->mapToSource(idx);
        d->m_selectedBean = d->m_model->bean(source);
    }
    if ( !d->m_selectedBean.isNull() )
    {
        QPointer<DBRecordDlg> dlg;
        if ( d->m_selectedBean->metadata()->tableName() == QString("%1_system").arg(alephERPSettings->systemTablePrefix()) )
        {
#if defined(ALEPHERP_ADVANCED_EDIT) && defined (ALEPHERP_DEVTOOLS)
            dlg = new AERPSystemObjectEditDlg(d->m_selectedBean.data(), openType, d->m_contextName, this);
#else
            return;
#endif
        }
        else
        {
            dlg = new DBRecordDlg(d->m_selectedBean.data(), openType, d->m_contextName, this);
        }
        CommonsFunctions::restoreOverrideCursor();
        if ( dlg->openSuccess() && dlg->init() )
        {
            dlg->setModal(true);
            dlg->exec();
        }
        delete dlg;
    }
    else
    {
        QMessageBox::warning(this,qApp->applicationName(), trUtf8("Debe seleccionar un registro que visualizar."));
    }
}

/**
 * @brief DBSearchDlg::newRecord
 * Permite insertar un nuevo registro, cuando se realiza una búsqueda.
 */
void DBSearchDlg::newRecord()
{
    AlephERP::FormOpenType openType = AlephERP::Insert;
    QString functionName = "beforeInsert";

    if ( aerpQsEngine()->existQsFunction(functionName) )
    {
        QScriptValue result;
        if ( aerpQsEngine()->callQsObjectFunction(result, functionName) )
        {
            if ( !result.toBool() )
            {
                return;
            }
        }
    }

    BaseBeanPointer insertBean;
    if ( d->m_insertFather.isNull() )
    {
        if ( d->m_beanToInsert.isNull() )
        {
            if ( !d->m_masterBean.isNull() )
            {
                DBRelation *rel = d->m_masterBean->relation(tableName());
                if ( rel == NULL )
                {
                    QString message = trUtf8("Ha ocurrido un error: No existe la relación %1").arg(tableName());
                    QMessageBox::warning(this, qApp->applicationName(), message, QMessageBox::Ok);
                    QLogger::QLog_Error(AlephERP::stLogOther, QString("DBSearchDlg::newRecord: %1").arg(message));
                    return;
                }
                if ( rel->metadata()->type() == DBRelationMetadata::ONE_TO_MANY )
                {
                    d->m_beanToInsert = rel->newChild();
                }
            }
            else
            {
                d->m_beanToInsert = BeansFactory::instance()->newQBaseBean(d->m_templateBean->metadata()->tableName(), true, d->fathersForTreeView());
            }
        }
        if ( d->m_beanToInsert.isNull() )
        {
            QString message = trUtf8("Ha ocurrido un error inesperado. No es posible abrir el formulario de inserción de registros.");
            QMessageBox::warning(this, qApp->applicationName(), message, QMessageBox::Ok);
            return;
        }
        insertBean = d->m_beanToInsert.data();
    }
    else
    {
        insertBean = d->m_insertFather;
        insertBean->setDefaultValues(d->fathersForTreeView());
    }
    emit beanAboutToBeInserted(d->m_beanToInsert.data());

    CommonsFunctions::setOverrideCursor(QCursor(Qt::WaitCursor));
    QScopedPointer<DBRecordDlg> dlg (new DBRecordDlg(insertBean, openType, d->m_contextName, this));
    CommonsFunctions::restoreOverrideCursor();
    if ( dlg->openSuccess() && dlg->init() )
    {
        dlg->setModal(true);
        dlg->exec();
        if ( dlg->userSaveData() )
        {
            d->m_userClickOk = true;
            d->m_userInsertNewRecord = true;
            close();
        }
    }
}

/**
 * @brief DBSearchDlg::setDateForAnotherDate
 * Cuando tenemos dos campos de fecha, que harán un between, y se establezca el valor en un date time, se busca
 * el relacionado y se le establece la misma fecha
 * @param date
 */
void DBSearchDlg::setDateForAnotherDate()
{
    DBDateTimeEdit *deOrigin = qobject_cast<DBDateTimeEdit *>(sender());
    if ( deOrigin == NULL )
    {
        return;
    }
    QList<DBDateTimeEdit *> dateTimes = findChildren<DBDateTimeEdit *>();
    foreach (DBDateTimeEdit *otherDe, dateTimes)
    {
        if ( deOrigin->fieldName() == otherDe->fieldName() && otherDe != deOrigin )
        {
            if ( !otherDe->value().isValid() )
            {
                otherDe->setValue(deOrigin->value());
            }
        }
    }
}

void DBSearchDlg::addDefaultValue(const QString &dbFieldName, const QVariant &value)
{
    d->m_defaultValues[dbFieldName] = value;
}

void DBSearchDlg::setDefaultValues(const QHash<QString, QVariant> &values)
{
    d->m_defaultValues = values;
}

/**
 * @brief DBSearchDlg::setInsertBean
 * @param bean
 * Para algunos casos especiales, debemos indicar, si el usuario pincha en insertar un nuevo registro, qué registro se creará.
 */
void DBSearchDlg::setInsertFather(BaseBeanPointer bean)
{
    d->m_insertFather = bean;
}

void DBSearchDlg::checkAll()
{
    if ( d->m_canSelectSeveral && d->m_model != NULL )
    {
        bool v = d->m_model->setCanEmitDataChanged(true);
        d->m_model->checkAllItems(true);
        d->m_model->setCanEmitDataChanged(v);
    }
}

void DBSearchDlg::uncheckAll()
{
    if ( d->m_canSelectSeveral && d->m_model != NULL )
    {
        bool v = d->m_model->setCanEmitDataChanged(true);
        d->m_model->checkAllItems(false);
        d->m_model->setCanEmitDataChanged(v);
    }
}

/*!
  Selecciona de todos los controles, la lista de valores por los que filtrar...
  Este slot es el que realmente filtra.
  */
void DBSearchDlg::search()
{
    bool senderIsComboBox = false;

    // Si el objeto es un combo, siempre habrá que realizar una búsqueda SQL
    QComboBox *cb = qobject_cast<QComboBox *>(sender());
    if ( cb != NULL )
    {
        senderIsComboBox = true;
    }

    // Vamos a ver si el usuario está realizando pulsaciones rápidas de teclas...
    // Si es así, vamos a programar una búsqueda para dentro de unos instantes, y evitamos la
    // saturación de peticiones al modelo con consultas SQL
    if ( !lastKeyPressTimeStamp().isNull() && lastKeyPressTimeStamp().isValid() && !senderIsComboBox && lastKeyPressTimeStamp().msecsTo(QDateTime::currentDateTime()) < MSECS_BETWEEN_KEYPRESS )
    {
        // Importante no usar el disparador singleShot estático, ya que desencadenaría todas las búsquedas posteriormente.
        d->m_keyPressTimer.start(100);
        // Vamos a bloquear las señales de la vista...
        ui->tvResults->setModel(NULL);
        ui->tvResults->showAnimation();
        return;
    }
    else
    {
        if ( ui->tvResults->model() == NULL )
        {
            ui->tvResults->hideAnimation();
            ui->tvResults->setModel(d->m_filter);
            ui->tvResults->setSelectionModel(d->m_modelSelection);
        }
    }

    bool fromHistory = false;
    SearchPerformed *searchPerformed = NULL;
    QList<QHashVariant> actualSearch = d->searchState();

    // ¿El usuario ha vaciado de nuevo todos los campos de búsqueda?
    if ( d->isEmptySearch() )
    {
        d->resetFilter();
        if ( d->m_metadata->showOnTree() )
        {
            ui->treeResults->setViewIntermediateNodesWithoutChildren(true);
            d->m_filter->invalidate();
        }
        return;
    }
    // ¿La búsqueda está en el histórico?
    foreach (SearchPerformed *historySearch, d->m_searchHistory)
    {
        // Según la documentación:
        // Two hashes are considered equal if they contain the same (key, value) pairs.
        // Two lists are considered equal if they contain the same values in the same order.
        if ( historySearch->m_widgetsSearchValues == actualSearch )
        {
            searchPerformed = historySearch;
            fromHistory = true;
        }
    }
    if ( !fromHistory )
    {
        searchPerformed = new SearchPerformed();
        searchPerformed->m_widgetsSearchValues = actualSearch;
        // Eliminamos los filtros que hubiera
        d->resetFilter();
        d->applySqlFilter(searchPerformed);
    }
    else
    {
        if ( senderIsComboBox || d->haveToApplySqlFilter(searchPerformed) )
        {
            d->applySqlFilter(searchPerformed);
        }
        else
        {
            d->applyProxyFilter(searchPerformed);
        }
    }
    if ( d->m_metadata->showOnTree() )
    {
        ui->treeResults->setViewIntermediateNodesWithoutChildren(false);
        d->m_filter->invalidate();
    }
    if ( !fromHistory )
    {
        d->m_searchHistory.append(searchPerformed);
        searchPerformed->m_results = d->m_model->rowCount();
    }
}

/*!
  En modelos con muchos datos, y dado que se utiliza un FilterProxy para mostrar los datos, si sólo utilizamos este
  modelo, podrían obtenerse de golpe todos los registros del modelo, lo que es muy gravoso en tiempo.
  Para inicios de filtros, es mejor establecer una sentencia SQL. La decisión sobre si aplicar un filtro proxy
  o uno SQL, la toma esta función.
  */
bool DBSearchDlgPrivate::haveToApplySqlFilter(SearchPerformed *searchValues)
{
    int index = m_searchHistory.indexOf(searchValues);
    if ( index > -1 )
    {
        if ( m_searchHistory[index]->m_results > alephERPSettings->strongFilterRowCountLimit() )
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        // Veamos si lo buscado es una evolución (por ejemplo, cambiar un valor de un combobox es una búsqueda
        // nueva, pero que el usuario pase de escribir "BLA" a "BLAN" implica una afinación de la búsqueda
        SearchPerformed *evolution = findEvolution(searchValues);
        if ( evolution != NULL )
        {
            if ( evolution->m_results > alephERPSettings->strongFilterRowCountLimit() )
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }
    // Llegados a este punto es que es una búsqueda totalmente nueva, y en ese caso, el primer filtro será SQL.
    if ( m_model->rowCount() > alephERPSettings->strongFilterRowCountLimit() )
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * @brief DBSearchDlgPrivate::applyProxyFilter
 * Obtiene los valores de los textos, y los introduce en el proxy que realiza el filtrado.
 * @return
 */
void DBSearchDlgPrivate::applyProxyFilter(SearchPerformed *searchState)
{
    int level;
    if ( m_metadata->showOnTree() )
    {
        level = m_metadata->treeDefinitions().count() + 1;
    }
    else
    {
        level = -1;
    }
    for (int i = 0 ; i < searchState->m_widgetsSearchValues.size() ; i++)
    {
        QHashVariant searchValues = searchState->m_widgetsSearchValues.at(i);
        QString fieldName = searchValues["fieldName"].toString();
        if ( searchValues["operator"] != OP_BETWEEN && searchValues["operator"] != OP_AND )
        {
            m_filter->setFilterKeyColumn(fieldName, searchValues["value"], OP_EQUAL, level);
        }
        else
        {
            if ( i < searchState->m_widgetsSearchValues.size() - 1 )
            {
                if ( searchValues["operator"] == OP_BETWEEN )
                {
                    QHashVariant andValues = searchState->m_widgetsSearchValues.at(i+1);
                    m_filter->setFilterKeyColumnBetween(fieldName, searchValues["value"], andValues["value"], level);
                }
            }
        }
    }
    m_filter->invalidate();
    // Nos aseguramos que las ramas intermedias estén extendidas a lo largo de toda la fila
    q_ptr->ui->treeResults->spanOnIntermediateBranchs();
}

/**
 * @brief DBSearchDlgPrivate::applySqlFilter
 * Aplica un filtro SQL
 * @param searchState
 */
void DBSearchDlgPrivate::applySqlFilter(SearchPerformed *searchState)
{
    QString where = sqlWhere(searchState);
    BaseBeanModel *model = qobject_cast<BaseBeanModel *>(m_model);
    if ( m_model != NULL )
    {
        if ( !m_filterData.isEmpty() )
        {
            where.prepend(QString("%1 AND ").arg(m_filterData));
        }
        model->setWhere(where);
    }
    applyProxyFilter(searchState);
}

void DBSearchDlgPrivate::resetFilter()
{
    m_filter->resetFilter();
    BaseBeanModel *model = qobject_cast<BaseBeanModel *>(m_model);
    if ( m_model != NULL )
    {
        model->setWhere(m_filterData);
        return;
    }
}

/**
 * @brief DBSearchDlgPrivate::sqlWhere
 * @param searchState
 * A la hora de aplicar un filtro SQL, aquí se crea el mismo
 * @return
 */
QString DBSearchDlgPrivate::sqlWhere(SearchPerformed *searchState) const
{
    QString where;
    foreach (const QHashVariant &hashValue, searchState->m_widgetsSearchValues)
    {
        QVariant value = hashValue["value"];
        DBFieldMetadata *fld = m_metadata->field(hashValue["fieldName"].toString());
        if ( !value.isNull() && value.isValid() && fld != NULL )
        {
            QString sqlValue = fld->sqlValue(hashValue["value"]);
            if ( fld->type() == QVariant::String )
            {
                if ( hashValue["operator"].toString() == OP_LIKE )
                {
                    sqlValue = QString("'%%1%'").arg(value.toString().toUpper());
                }
                if ( hashValue["operator"].toString() == OP_AND )
                {
                    where += QString("%1 %2").arg(hashValue["operator"].toString()).arg(sqlValue);
                }
                else
                {
                    if ( !where.isEmpty() )
                    {
                        where = where + " AND ";
                    }
                    where += QString("upper(%1) %2 %3").arg(fld->dbFieldName()).arg(hashValue["operator"].toString()).arg(sqlValue.toUpper());
                }
            }
            else
            {
                if ( hashValue["operator"].toString() == OP_AND )
                {
                    where += QString("%1 %2").arg(hashValue["operator"].toString()).arg(sqlValue);
                }
                else
                {
                    if ( !where.isEmpty() )
                    {
                        where = where + " AND ";
                    }
                    where += QString("%1 %2 %3").arg(fld->dbFieldName()).arg(hashValue["operator"].toString()).arg(sqlValue);
                }
            }
        }
    }
    return where;
}

/**
 * @brief DBSearchDlgPrivate::searchState
 * Recorre todos los widgets, escogiendo sus valores, y creando el patrón de búsqueda que debe aplicarse.
 * Este patrón de búsqueda se almacenará para así poder utilizarlo en el histórico.
 * @return
 */
QList<QHashVariant> DBSearchDlgPrivate::searchState()
{
    QStringList fieldsOnState;
    QList<QHashVariant> widgetsState;
    QList<QWidget *> widgets = q_ptr->findChildren<QWidget *>();
    foreach ( QWidget *widget, widgets )
    {
        QVariant propertyFieldName = widget->property(AlephERP::stFieldName);
        if ( propertyFieldName.isValid() )
        {
            DBFieldMetadata *fld = m_metadata->field(propertyFieldName.toString());
            if ( fld != NULL && !fieldsOnState.contains(fld->dbFieldName()) )
            {
                if ( m_autogeneratedWidgets )
                {
                    QComboBox *cb = q_ptr->findChild<QComboBox *>(QString("cb_%1").arg(propertyFieldName.toString()));
                    if ( cb != NULL )
                    {
                        // El código contenido aquí es el que se ejecutará para los controles autogenerados, donde los nombres de los
                        // mismos siguen las reglas que aparecen:
                        if ( cb->currentIndex() == BETWEEN_ELECTION_INDEX )
                        {
                            searchStateOnAutogeneratedBetweenWidgets(widget, fld, widgetsState, fieldsOnState);
                        }
                        else
                        {
                            searchStateOnAutogeneratedWidgetsWithComboSelector(widget, cb, fld, widgetsState, fieldsOnState);
                        }
                    }
                    else
                    {
                        searchStateOnAutogeneratedWidget(widget, fld, widgetsState, fieldsOnState);
                    }
                }
                else
                {
                    searchStateOnProvidedWidget(widget, fld, widgetsState, fieldsOnState);
                }
            }
        }
    }
    return widgetsState;
}

void DBSearchDlgPrivate::searchStateOnAutogeneratedBetweenWidgets(QWidget *widget, DBFieldMetadata *fld, QList<QHashVariant> &widgetsState, QStringList &fieldsOnState)
{
    if ( widget->objectName().contains("_1") )
    {
        QWidget *widget2 = q_ptr->findChild<QWidget *>(QString("db_%1_2").arg(fld->dbFieldName()));
        if ( widget2 != NULL )
        {
            DBBaseWidget *wid1 = dynamic_cast<DBBaseWidget *> (widget);
            DBBaseWidget *wid2 = dynamic_cast<DBBaseWidget *> (widget2);
            if ( wid1->userModified() || wid2->userModified() )
            {
                searchStateOnBetweenWidgets(wid1, wid2, fld, widgetsState, fieldsOnState);
            }
        }
    }
}

void DBSearchDlgPrivate::searchStateOnAutogeneratedWidgetsWithComboSelector(QWidget *widget, QComboBox *cb, DBFieldMetadata *fld, QList<QHashVariant> &widgetsState, QStringList &fieldsOnState)
{
    QHashVariant valueState;
    DBBaseWidget *wid = dynamic_cast<DBBaseWidget *> (widget);
    if ( wid != NULL && wid->userModified() )
    {
        QVariant v = wid->value();
        // OJO: Los number edit devuelven un valor válido, aunque el control esté vacío...
        QLineEdit *le = qobject_cast<QLineEdit *>(widget);
        if ( le != NULL && le->text().isEmpty() ) {
            v = QVariant();
        }
        valueState["value"] = v;
        valueState["fieldName"] = fld->dbFieldName();
        if ( cb->currentIndex() == MINUS_ELECTION )
        {
            valueState["operator"] = OP_LESS;
        }
        else if ( cb->currentIndex() == EQUAL_ELECTION )
        {
            valueState["operator"] = OP_EQUAL;
        }
        else if ( cb->currentIndex() == MORE_ELECTION )
        {
            valueState["operator"] = OP_GREATER;
        }
        widgetsState.append(valueState);
        fieldsOnState.append(fld->dbFieldName());
    }
}

void DBSearchDlgPrivate::searchStateOnAutogeneratedWidget(QWidget *widget, DBFieldMetadata *fld, QList<QHashVariant> &widgetsState, QStringList &fieldsOnState)
{
    DBBaseWidget *wid = dynamic_cast<DBBaseWidget *> (widget);
    if ( wid != NULL && wid->userModified() )
    {
        QHashVariant valueState;
        valueState["value"] = wid->value();
        valueState["fieldName"] = fld->dbFieldName();
        valueState["DBFieldMetadata"] = QVariant::fromValue((void*)fld);
        if ( fld->type() == QVariant::String && (qobject_cast<QLineEdit *>(widget) != NULL || qobject_cast<QTextEdit *>(widget) != NULL) )
        {
            valueState["operator"] = OP_LIKE;
        }
        else
        {
            valueState["operator"] = OP_EQUAL;
        }
        widgetsState.append(valueState);
        fieldsOnState.append(fld->dbFieldName());
    }
}

void DBSearchDlgPrivate::searchStateOnProvidedWidget(QWidget *widget, DBFieldMetadata *fld, QList<QHashVariant> &widgetsState, QStringList &fieldsOnState)
{
    DBBaseWidget *wid = dynamic_cast<DBBaseWidget *> (widget);
    if ( wid != NULL && wid->userModified() )
    {
        QHashVariant valueState;
        // ¿Hay otro widget con el mismo nombre? Si lo hay, entonces tenemos que hacer un between
        QWidget *widget2 = NULL;
        QList<QWidget *> widgets = q_ptr->findChildren<QWidget *>();
        for ( int i = 0 ; i < widgets.size() ; i++ )
        {
            QVariant property2FieldName = widgets.at(i)->property(AlephERP::stFieldName);
            if ( property2FieldName.isValid() && widget != widgets.at(i) && fld->dbFieldName() == property2FieldName.toString() )
            {
                widget2 = widgets.at(i);
            }
        }
        if ( widget2 != NULL )
        {
            DBBaseWidget *wid1 = dynamic_cast<DBBaseWidget *> (widget);
            DBBaseWidget *wid2 = dynamic_cast<DBBaseWidget *> (widget2);
            if ( wid1->userModified() || wid2->userModified() )
            {
                searchStateOnBetweenWidgets(wid1, wid2, fld, widgetsState, fieldsOnState);
            }
        }
        else
        {
            valueState["value"] = wid->value();
            valueState["fieldName"] = fld->dbFieldName();
            if ( fld->type() == QVariant::String && (qobject_cast<QLineEdit *>(widget) != NULL || qobject_cast<QTextEdit *>(widget) != NULL) )
            {
                valueState["operator"] = OP_LIKE;
            }
            else
            {
                valueState["operator"] = OP_EQUAL;
            }
            widgetsState.append(valueState);
            fieldsOnState.append(fld->dbFieldName());
        }
    }
}

void DBSearchDlgPrivate::searchStateOnBetweenWidgets(DBBaseWidget *wid1, DBBaseWidget *wid2, DBFieldMetadata *fld, QList<QHashVariant> &widgetsState, QStringList &fieldsOnState)
{
    if ( wid1->value() == wid2->value() )
    {
        QHashVariant valueState;
        valueState["value"] = wid1->value();
        valueState["fieldName"] = fld->dbFieldName();
        if ( fld->type() == QVariant::String )
        {
            valueState["operator"] = OP_LIKE;
        }
        else
        {
            valueState["operator"] = OP_EQUAL;
        }
        widgetsState.append(valueState);
        fieldsOnState.append(fld->dbFieldName());
    }
    else
    {
        QHashVariant value1, value2;
        if ( wid1->value() < wid2->value() )
        {
            value1["value"] = wid1->value();
            value2["value"] = wid2->value();
        }
        else
        {
            value1["value"] = wid2->value();
            value2["value"] = wid1->value();
        }
        value1["operator"] = OP_BETWEEN;
        value2["operator"] = OP_AND;
        value1["fieldName"] = fld->dbFieldName();
        value2["fieldName"] = fld->dbFieldName();
        widgetsState.append(value1);
        widgetsState.append(value2);
        fieldsOnState.append(fld->dbFieldName());
    }
}

void DBSearchDlgPrivate::setToolTipForWidgets()
{
    QList<QWidget *> widgets = q_ptr->findChildren<QWidget *>();
    foreach ( QWidget *widget, widgets )
    {
        QVariant propertyFieldName = widget->property(AlephERP::stFieldName);
        if ( propertyFieldName.isValid() )
        {
            DBFieldMetadata *fld = m_metadata->field(propertyFieldName.toString());
            if ( fld != NULL && fld->type() == QVariant::String )
            {
                QString message = QObject::trUtf8("<p>La búsqueda utilizando ese campo se hará de la siguiente forma:</p>"
                                                  "<p>Se buscará los registros cuyo valor para este campo contenga los carácters que introduzca.</p>");
                widget->setToolTip(message);
                message.append(QObject::trUtf8("<p>Por ejemplo si su cadena de búsqueda es \"<strong>PED</strong>\" se encontrarán el registro con valor de campo \"<strong>PED</strong>RO\", pero también el registro con el valor \"ANTONIO <strong>PED</strong>RO\".</p>"));
                widget->setWhatsThis(message);
            }
        }
    }
}

/**
 * @brief SearchWidgetsValues::isEvolution
 * Comprueba si other es una evolución en la búsqueda de esta búsqueda. Es decir, el usuario
 * ha pasado de escribir "BLA" a "BLAN"
 * @param other
 * @return
 */
bool SearchPerformed::isEvolution(SearchPerformed *other)
{
    foreach (const QHashVariant &item, m_widgetsSearchValues)
    {
        QHashVariant otherItem = other->item(item["fieldName"].toString());
        if ( !otherItem.isEmpty() )
        {
            QString otherValue = otherItem["value"].toString();
            QString thisValue = item["value"].toString();
            if ( otherValue != thisValue && !thisValue.isEmpty() && !thisValue.startsWith(otherValue, Qt::CaseInsensitive) )
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    return true;
}

/**
 * @brief DBSearchDlgPrivate::isEmptySearch
 * Comprueba si lo que se tiene es todos los controles vacíos
 * @return
 */
bool DBSearchDlgPrivate::isEmptySearch()
{
    foreach (const QHashVariant &item, searchState())
    {
        if ( item.contains("value") && item["value"].isValid() )
        {
            return false;
        }
    }
    return true;
}

/**
 * @brief DBSearchDlgPrivate::findEvolution
 * Realiza una búsqueda entre el histórico de búsquedas, tratando de localizar aquella que pudiese ser previa a esta actual.
 * La idea, es que si el usuario ha pasado de escribir "BLA" a "BLAN" esta función devuelva la búsqueda "BLA".
 * @param searchValues
 * @return
 */
SearchPerformed *DBSearchDlgPrivate::findEvolution(SearchPerformed *actualSearch)
{
    int matchesChar = 0;
    SearchPerformed *previous = NULL;
    QList<SearchPerformed *> matches;
    // Busquemos primero todas las búsquedas coincidentes, para después determinar la más cercana
    foreach (SearchPerformed *searchPerformed, m_searchHistory)
    {
        if ( actualSearch->isEvolution(searchPerformed) )
        {
            matches.append(searchPerformed);
        }
    }
    foreach (SearchPerformed *searchPerformed, matches)
    {
        // El más actual o parecido a actualSearch será el que tenga más caracteres.
        int performedSearchChars = 0;
        foreach (const QHashVariant &item, searchPerformed->m_widgetsSearchValues)
        {
            QString performedValue = item["value"].toString();
            performedSearchChars += performedValue.size();
        }
        if ( performedSearchChars > matchesChar )
        {
            previous = searchPerformed;
            matchesChar = performedSearchChars;
        }
    }
    return previous;
}

BaseBeanPointerList DBSearchDlgPrivate::fathersForTreeView()
{
    BaseBeanPointerList list;
    if ( !m_metadata->showOnTree() || !m_modelSelection || !m_model)
    {
        return list;
    }
    QModelIndex filterIdx = m_modelSelection->currentIndex();
    QModelIndex sourceIdx = m_filter->mapToSource(filterIdx);
    if ( sourceIdx.isValid() )
    {
        list = m_model->ancestorsBeans(sourceIdx);
    }
    return list;
}

