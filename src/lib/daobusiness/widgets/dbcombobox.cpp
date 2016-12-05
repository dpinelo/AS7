/***************************************************************************
 *   Copyright (C) 2007 by David Pinelo   *
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

#include "dao/beans/basebean.h"
#include "dbcombobox.h"
#include <aerpcommon.h>
#include "models/dbbasebeanmodel.h"
#include "models/filterbasebeanmodel.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/observerfactory.h"
#include "dao/dbfieldobserver.h"
#include "business/aerploggeduser.h"
#include "qlogger.h"

class DBComboBoxPrivate
{
    Q_DECLARE_PUBLIC(DBComboBox)
public:
    QPointer<DBBaseBeanModel> m_model;
    QPointer<FilterBaseBeanModel> m_filterModel;
    /** Nombre de la tabla o vista cuyos valores poblarán el combo */
    QString m_listTableModel;
    /** Nombre de la columna cuyos datos se mostrarán en el combo */
    QString m_listColumnName;
    /** Nombre de la columna cuyos datos se almacenarán */
    QString m_listColumnToSave;
    /** Filtro para la columna de datos */
    QString m_listFilter;
    /** Filtro SQL para esa columna de datos */
    QString m_listSqlFilter;
    /** Indica si el listado de ITEMs del combo se ha iniciado */
    bool m_inited;
    /** Semáforo que evita llamadas recursivas con currentIndexChanged */
    bool m_currentIndexChangedEmitted;
    /** Controla la apertura automática de la lista */
    bool m_openListOnGetFocus;
    /** Vamos a almacenar una copia del valor de este campo... */
    QVariant m_value;
    bool m_customItem;
    QVariant m_customItemValue;
    QString m_customItemText;
    QString m_customItemIcon;
    /** El método init puede tener una componente recursiva que evitamos */
    bool m_initing;

    DBComboBox *q_ptr;

    explicit DBComboBoxPrivate(DBComboBox *qq) : q_ptr(qq)
    {
        m_inited = false;
        m_currentIndexChangedEmitted = false;
        m_openListOnGetFocus = false;
        m_customItem = false;
        m_initing = false;
    }

    void initFromOptionList();
};

DBComboBox::DBComboBox(QWidget * parent ) : QComboBox(parent), DBBaseWidget(), d (new DBComboBoxPrivate(this))
{
    // Conectamos para emitir nuestra propia señal.
    conexiones();
}

DBComboBox::~DBComboBox()
{
    emit destroyed(this);
    delete d;
}

void DBComboBox::conexiones()
{
    // currentIndexChanged y editTextChanged se emiten tanto cuando el usuario interacciona
    // o cuando se hace programáticamente. activated sólo cuando el usuario interacciona.
    connect (this, SIGNAL(currentIndexChanged(int)), this, SLOT(itemChanged(int)));
    connect (this, SIGNAL(activated(int)), this, SLOT(userModifiedCombo(int)));
    if ( lineEdit() != NULL )
    {
        connect (lineEdit(), SIGNAL(textEdited(QString)), this, SLOT(currentTextChanged(QString)));
    }
    QComboBox::setInsertPolicy(QComboBox::NoInsert);
}

void DBComboBox::desconexiones()
{
    disconnect(SIGNAL(currentIndexChanged (int)));
    disconnect(SIGNAL(activated(int)));
    if ( lineEdit() != NULL )
    {
        disconnect(lineEdit(), SIGNAL(textEdited(QString)));
    }
}

void DBComboBox::setListTableModel(const QString &name)
{
    d->m_listTableModel = name;
    d->m_inited = false;
    if ( !d->m_model.isNull() )
    {
        delete d->m_model;
    }
}

QString DBComboBox::listTableModel() const
{
    return d->m_listTableModel;
}

void DBComboBox::setListColumnName ( const QString &dbVisibleColumn )
{
    d->m_listColumnName = dbVisibleColumn;
    if ( d->m_listColumnToSave.isEmpty() )
    {
        d->m_listColumnToSave = d->m_listColumnName;
    }
    // Visualizamos la columna correspondiente
    setModelColumn();
}

QString DBComboBox::listColumnName () const
{
    return d->m_listColumnName;
}

void DBComboBox::setListColumnToSave ( const QString &dbColumn )
{
    d->m_listColumnToSave = dbColumn;
}

QString DBComboBox::listColumnToSave () const
{
    return d->m_listColumnToSave;
}

void DBComboBox::setListFilter(const QString &filter)
{
    d->m_listFilter = filter;
    d->m_listFilter = DBBaseWidget::processSqlWhere(filter);
    if ( !d->m_filterModel.isNull() )
    {
        d->m_filterModel->resetFilter();
        d->m_filterModel->setFilter(filter);
        d->m_filterModel->invalidate();
    }
}

QString DBComboBox::listFilter() const
{
    return d->m_listFilter;
}

QString DBComboBox::listSqlFilter() const
{
    return d->m_listSqlFilter;
}

void DBComboBox::setListSqlFilter(const QString &filter)
{
    d->m_listSqlFilter = filter;
    d->m_listSqlFilter = DBBaseWidget::processSqlWhere(filter);
    if ( !d->m_filterModel.isNull() && !d->m_model.isNull() )
    {
        delete d->m_filterModel;
        delete d->m_model;
        init();
    }
}

bool DBComboBox::openListOnGetFocus()
{
    return d->m_openListOnGetFocus;
}

void DBComboBox::setOpenListOnGetFocus(bool value)
{
    d->m_openListOnGetFocus = value;
}

bool DBComboBox::addCustomItem() const
{
    return d->m_customItem;
}

void DBComboBox::setAddCustomItem(bool value)
{
    d->m_customItem = value;
}

QVariant DBComboBox::customItemValue() const
{
    return d->m_customItemValue;
}

void DBComboBox::setCustomItemValue(const QVariant &value)
{
    d->m_customItemValue = value;
}

QString DBComboBox::customItemText() const
{
    return d->m_customItemText;
}

void DBComboBox::setCustomItemText(const QString &value)
{
    d->m_customItemText = value;
}

QString DBComboBox::customItemIcon() const
{
    return d->m_customItemIcon;
}

void DBComboBox::setCustomItemIcon(const QString &value)
{
    d->m_customItemIcon = value;
}

bool DBComboBox::enableCustomItem()
{
    return m_fieldName.isEmpty();
}

void DBComboBox::showEvent(QShowEvent *event)
{
    // Este orden es importante
    if ( !d->m_inited )
    {
        init();
    }
    DBBaseWidget::showEvent(event);
}

void DBComboBoxPrivate::initFromOptionList()
{
    DBFieldObserver *obs = qobject_cast<DBFieldObserver *>(q_ptr->observer(false));
    if ( obs != NULL && !m_inited )
    {
        DBField *fld = qobject_cast<DBField *> (obs->entity());
        if ( fld != NULL && !fld->metadata()->optionsList().isEmpty() )
        {
            QMap<QString, QString> optionList = fld->metadata()->optionsList();
            QMap<QString, QString> optionIcons = fld->metadata()->optionsIcons();
            QMapIterator<QString, QString> it (optionList);
            bool blockState = q_ptr->blockSignals(true);
            while ( it.hasNext() )
            {
                it.next();
                if ( optionIcons.contains(it.key()) )
                {
                    q_ptr->QComboBox::addItem(QIcon(optionIcons.value(it.key())), it.value(), it.key());
                }
                else
                {
                    q_ptr->addItem(it.value(), it.key());
                }
            }
            // Importante esto para evitar llamadas recursivas
            m_inited = true;
            q_ptr->setValue(fld->value());
            q_ptr->blockSignals(blockState);
        }
        m_inited = true;
        obs->sync();
    }
}

void DBComboBox::setModelColumn()
{
    if ( !d->m_model.isNull() && !d->m_filterModel.isNull() )
    {
        BaseBeanMetadata *m = d->m_model->metadata();
        if ( m != NULL )
        {
            int fieldIndex = m->fieldIndex(d->m_listColumnName);
            if ( fieldIndex != -1 )
            {
                int visibleColumn = d->m_filterModel->columnFieldIndex(d->m_listColumnName);
                if ( visibleColumn == -1 )
                {
                    QLogger::QLog_Error(AlephERP::stLogOther, QString("DBComboBox::setModelColumn: ComboBox: [%1]. "
                                        "El filtro no contiene ninguna columna visible de nombre [%2] para la tabla [%3]").
                                        arg(objectName(), d->m_listColumnName, m->tableName()));
                }
                else
                {
                    QComboBox::setModelColumn(visibleColumn);
                }
            }
            else
            {
                QLogger::QLog_Error(AlephERP::stLogOther, QString("DBComboBox::setModelColumn: ComboBox: [%1]. "
                                    "La columna [%2] no se ha encontrado entre los campos de [%3]").
                                    arg(objectName(), d->m_listColumnName, m->tableName()));
            }
        }
    }
}

void DBComboBox::init()
{
    if ( d->m_initing )
    {
        return;
    }
    d->m_initing = true;
    desconexiones();
    if ( d->m_listTableModel.isEmpty() )
    {
        DBFieldObserver *obs = qobject_cast<DBFieldObserver *>(observer(false));
        if ( obs != NULL )
        {
            DBField *fld = qobject_cast<DBField *> (obs->entity());
            if ( fld != NULL )
            {
                if ( !fld->metadata()->optionsList().isEmpty() )
                {
                    d->initFromOptionList();
                }
                else if ( fld->relations(AlephERP::ManyToOne).size() == 1 &&
                          fld->relations(AlephERP::ManyToOne).first() != NULL )
                {
                    DBRelation *rel = fld->relations(AlephERP::ManyToOne).first();
                    d->m_listTableModel = rel->metadata()->tableName();
                    // Si además, la columna a guardar es nula, buscamos la primary key
                    if ( d->m_listColumnToSave.isEmpty() )
                    {
                        BaseBeanMetadata *relatedBean = BeansFactory::metadataBean(d->m_listTableModel);
                        if ( relatedBean != NULL && relatedBean->pkFields().size() == 1 )
                        {
                            DBFieldMetadata *relatedFld = relatedBean->pkFields().at(0);
                            if ( fld->metadata()->type() == relatedFld->type() )
                            {
                                d->m_listColumnToSave = relatedFld->dbFieldName();
                            }
                        }
                    }
                }
                conexiones();
            }
        }
    }
    // Al crearse el modelo, y este venir de base de datos, no se permite la carga en segundo plano.
    // No tendría sentido almacenar muchos datos en el combobox
    if ( !d->m_model.isNull() )
    {
        delete d->m_model;
    }
    BaseBeanMetadata *m = BeansFactory::metadataBean(d->m_listTableModel);
    if ( m == NULL )
    {
        d->m_initing = false;
        return;
    }
    d->m_model = new DBBaseBeanModel(d->m_listTableModel, d->m_listSqlFilter, m->initOrderSort(), true, true, false, this);
    d->m_filterModel = new FilterBaseBeanModel(this);
    d->m_filterModel->setAccessFilter('s');
    d->m_filterModel->setFilter(d->m_listFilter);
    d->m_filterModel->setForceToLoadBeans(true);
    d->m_filterModel->setSourceModel(d->m_model);
    if ( d->m_customItem )
    {
        d->m_model->addStaticRow(QIcon(d->m_customItemIcon), d->m_customItemText);
    }
    if ( !d->m_model.isNull() && !d->m_filterModel.isNull() )
    {
        bool signalsBlocked = blockSignals(true);
        // Poner el modelo, puede invocar eventos que descadenen llamadas recursivas.
        QComboBox::setModel(d->m_filterModel.data());
        setModelColumn();
        setMaxVisibleItems(15);
        QComboBox::setCurrentIndex(-1);
        blockSignals(signalsBlocked);
        if ( completer() != NULL )
        {
            connect(completer(), SIGNAL(activated(QModelIndex)), this, SLOT(setValueFromModel(QModelIndex)));
        }
    }
    if ( view() != NULL )
    {
        // Vamos a monitorizar los eventos sobre la vista: La idea es capturar la pulsación de la tecla
        // enter, para seleccionar automáticamente el item y pasar de control.
        view()->installEventFilter(this);
    }
    conexiones();
    d->m_inited = true;
    d->m_initing = false;
}

void DBComboBox::setValue(const QVariant &v)
{
    if ( !d->m_inited )
    {
        init();
    }
    if ( v != value() )
    {
        if ( d->m_customItem && d->m_customItemValue == v )
        {
            QComboBox::setCurrentIndex(count()-1);
        }
        if ( !d->m_listTableModel.isEmpty() )
        {
            setCurrentIndexByDbColumn(d->m_listColumnToSave, v);
        }
        else
        {
            int index = findData(v);
            if ( index != currentIndex() )
            {
                AbstractObserver *obs = qobject_cast<AbstractObserver *>(sender());
                if ( obs != NULL )
                {
                    blockSignals(true);
                }
                QComboBox::setCurrentIndex(index);
                if ( obs != NULL )
                {
                    blockSignals(false);
                }
            }
            else
            {
                qDebug() << "DBComboBox::setValue: No se ha encontrado la opcion " << v.toString();
            }
        }
    }
}

QVariant DBComboBox::value()
{
    QVariant v;
    if ( !d->m_inited )
    {
        init();
    }
    if ( !d->m_listTableModel.isEmpty() )
    {
        FilterBaseBeanModel *mdl = qobject_cast<FilterBaseBeanModel *>(model());
        if ( mdl != NULL )
        {
            QModelIndex index;
            if ( isEditable() && completer() != NULL )
            {
                index = mdl->index(completer()->currentRow(), 0);
            }
            else
            {
                index = mdl->index(currentIndex(), 0);
            }
            if ( d->m_customItem && index.data(AlephERP::StaticRowRole).toBool() )
            {
                return d->m_customItemValue;
            }
            BaseBeanSharedPointer bean = mdl->bean(index);
            if ( !bean.isNull() )
            {
                v = bean->fieldValue(d->m_listColumnToSave);
            }
        }
    }
    else
    {
        if ( d->m_customItem )
        {
            QVariant data = itemData(currentIndex(), AlephERP::StaticRowRole);
            if ( data.toBool() )
            {
                return d->m_customItemValue;
            }
        }
        QVariant data = itemData(currentIndex(), Qt::UserRole);
        if ( data.isValid() )
        {
            v = data;
        }
        else
        {
            v = currentText();
        }
    }
    return v;
}

int DBComboBox::currentIndex() const
{
    return QComboBox::currentIndex();
}

void DBComboBox::setCurrentIndex ( int index )
{
    if ( !d->m_inited )
    {
        init();
    }
    if ( count() > 0 ) {
        QComboBox::setCurrentIndex(index);
    }
}

/*!
  Selecciona del combo el item indicado, según el valor de la columna
  de base de datos
  */
void DBComboBox::setCurrentIndexByDbColumn(const QString &dbModelColumn, const QVariant &value)
{
    if ( d->m_model.isNull() || d->m_model->metadata() == NULL )
    {
        return;
    }
    // Realizamos la búsqueda en el modelo original, y después, traducimos al modelo proxy
    int column = d->m_model->metadata()->fieldIndex(dbModelColumn);
    QModelIndex start = d->m_model->index(0, column);
    QModelIndexList list = d->m_model->match(start, AlephERP::RawValueRole, value, 1, Qt::MatchExactly);
    // Para establecer la fila, debemos convertir al modelo
    if ( !list.isEmpty() && list.at(0).isValid() )
    {
        int sourceVisibleColumn = d->m_model->metadata()->fieldIndex(d->m_listColumnName);
        QModelIndex sourceRowIndex = d->m_model->index(list.at(0).row(), sourceVisibleColumn);
        QModelIndex filterRowIndex = d->m_filterModel->mapFromSource(sourceRowIndex);
        bool blockState = blockSignals(true);
        QComboBox::setCurrentIndex(filterRowIndex.row());
        blockSignals(blockState);
    }
    if ( list.isEmpty() )
    {
        bool blockState = blockSignals(true);
        QComboBox::setCurrentIndex(-1);
        blockSignals(blockState);
    }
}

/*!
  Para el elemento seleccionado, copia en bean el bean que corresponde a ese elemento.
  bean debe ser un objeto válido
  */
BaseBeanSharedPointer DBComboBox::bean(int index)
{
    FilterBaseBeanModel *mdl = qobject_cast<FilterBaseBeanModel *>(model());
    if ( mdl == NULL )
    {
        return BaseBeanSharedPointer();
    }
    QModelIndex indice = mdl->index(index, 0);
    if ( indice.isValid() )
    {
        return mdl->bean(indice);
    }
    return BaseBeanSharedPointer();
}

/*!
  Se dispara cuando el usuario cambia el item seleccionado
  */
void DBComboBox::itemChanged(int index)
{
    // Obtengamos el id interno del bean, y lanzemos la señal
    if ( !d->m_currentIndexChangedEmitted )
    {
        bool validCustomItem = false;
        if ( d->m_customItem && model() )
        {
            QModelIndex idx = model()->index(currentIndex(), 0);
            if ( idx.data(AlephERP::StaticRowRole).toBool() )
            {
                validCustomItem = true;
            }
        }
        QVariant v = value();
        if ( (v.isValid() && v != d->m_value) || validCustomItem )
        {
            bool needToAskRecalculateCounterField = DBBaseWidget::wasCounterFieldValid();
            emit valueEdited(v);
            d->m_value = v;
            if ( needToAskRecalculateCounterField )
            {
                DBBaseWidget::askToRecalculateCounterField();
            }
        }
        d->m_currentIndexChangedEmitted = true;
        emit currentIndexChanged(index);
        d->m_currentIndexChangedEmitted = false;
    }
}

/*!
  Se dispara cuando el usuario cambia el item seleccionado
  */
void DBComboBox::currentTextChanged(const QString &text)
{
    // Veamos si el texto coincide con alguno de los items en la lista
    QVariant v = value();
    if ( !v.isValid() )
    {
        QModelIndex index;
        FilterBaseBeanModel *mdl = qobject_cast<FilterBaseBeanModel *> (model());
        if ( mdl != NULL )
        {
            if ( completer() != NULL )
            {
                index = mdl->index(completer()->currentRow(), 0);
            }
            else
            {
                int row = QComboBox::findText(text);
                index = mdl->index(row, 0);
            }
            if ( index.isValid() && index.row() != currentIndex() )
            {
                BaseBeanSharedPointer b = mdl->bean(index);
                if ( !b.isNull() )
                {
                    v = b->fieldValue(m_fieldName);
                }
            }
        }
    }
    if ( v.isValid() && v != d->m_value )
    {
        emit valueEdited(v);
        d->m_value = v;
    }
    m_userModified = true;
}

void DBComboBox::userModifiedCombo(int index)
{
    Q_UNUSED(index)
    // Podemos hacer esto porque viene de la señal activated
    m_userModified = true;
}

void DBComboBox::setValueFromModel(const QModelIndex &index)
{
    FilterBaseBeanModel *mdl = qobject_cast<FilterBaseBeanModel *>(model());
    if ( mdl != NULL )
    {
        BaseBeanSharedPointer bean = mdl->bean(index);
        if ( !bean.isNull() )
        {
            setValue(bean->fieldValue(d->m_listColumnToSave));
        }
    }
}

bool DBComboBox::eventFilter(QObject * obj, QEvent * event)
{
    QModelIndex index;
    if ( obj == view() && event->type() == QEvent::KeyPress )
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if ( keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return )
        {
            // Al pulsar enter, seleccionamos, si lo hubiera, el elemento seleccionado de la vista
            index = view()->currentIndex();
            if ( index.isValid() )
            {
                QComboBox::setCurrentIndex(index.row());
            }
            hidePopup();
            focusNextChild();
            return true;
        }
        else if ( keyEvent->key() == Qt::Key_Tab )
        {
            hidePopup();
            focusNextChild();
            return true;
        }
        else if ( keyEvent->key() == Qt::Key_Backtab )
        {
            hidePopup();
            focusPreviousChild();
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

void DBComboBox::keyPressEvent(QKeyEvent * event)
{
    QComboBox::keyPressEvent(event);
}

void DBComboBox::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED (event)
    // Si el QComboBox es editable, y tiene un listTableModel establecido, y un campo, aseguramos que el dato que debe
    // guardar, debe ser del modelo y no es un texto al azar introducido
    if ( isEditable() && !d->m_listTableModel.isEmpty() && !currentText().isEmpty() )
    {
        QVariant v = value();
        if ( !v.isValid() )
        {
            bool blockState = blockSignals(true);
            setCurrentIndex(-1);
            blockSignals(blockState);
        }
        else
        {
            // Si se ha editado desde el completer, los items están en mayúsculas y el usuario ha
            // escrito en minúsculas, queda feo... lo ajustamos
            bool blockState = blockSignals(true);
            if ( completer() != NULL )
            {
                setEditText(completer()->currentCompletion());
            }
            blockSignals(blockState);
        }
    }
}

void DBComboBox::focusInEvent(QFocusEvent * event)
{
    if ( isEditable() )
    {
        lineEdit()->selectAll();
    }
    if ( !event->spontaneous() && d->m_openListOnGetFocus && isEditable())
    {
        QAbstractItemView *list = QComboBox::view();
        if ( !list->isVisible() )
        {
            // QComboBox::showPopup();
            QMouseEvent event1(QEvent::MouseButtonPress, QPoint(0,0), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(this, &event1);
            setFocus();
        }
    }
    QComboBox::focusInEvent(event);
}

/*!
  Ajusta los parámetros de visualización de este Widget en función de lo definido en DBField d->m_field
  */
void DBComboBox::applyFieldProperties()
{
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
    setEnabled(dataEditable());
}

void DBComboBox::observerUnregistered()
{
    DBBaseWidget::observerUnregistered();
    bool blockState = blockSignals(true);
    this->clear();
    d->m_inited = false;
    blockSignals(blockState);
}

int DBComboBox::findText(const QString &text, Qt::MatchFlags flags) const
{
    return QComboBox::findText(text, flags);
}

/*!
  Pide un nuevo observador si es necesario
  */
void DBComboBox::refresh()
{
    observer();
    if ( m_observer != NULL )
    {
        m_observer->sync();
    }
}

void DBComboBox::addItem (const QString & text, const QVariant & userData)
{
    QComboBox::addItem(text, userData);
}

QVariant DBComboBox::itemData (int index, int role) const
{
    return QComboBox::itemData(index, role);
}

BaseBeanSharedPointer DBComboBox::selectedBean()
{
    return bean(currentIndex());
}

/*!
  Tenemos que decirle al motor de scripts, que DBFormDlg se convierte de esta forma a un valor script
  */
QScriptValue DBComboBox::toScriptValue(QScriptEngine *engine, DBComboBox * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBComboBox::fromScriptValue(const QScriptValue &object, DBComboBox * &out)
{
    out = qobject_cast<DBComboBox *>(object.toQObject());
}
