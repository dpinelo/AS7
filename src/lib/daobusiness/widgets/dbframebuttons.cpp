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

#include "dao/beans/basebean.h"
#include "dbframebuttons.h"
#include "globales.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/dbrelation.h"
#include "dao/basedao.h"
#include "dao/dbrelationobserver.h"
#include "forms/dbrecorddlg.h"

class DBFrameButtonsPrivate
{
    Q_DECLARE_PUBLIC(DBFrameButtons)
public:
    /** Contiene todos los botones que se irán creando */
    QButtonGroup m_buttons;
    /** Los datos se leen de base de datos o bien del bean del formulario que contiene a este control */
    bool m_internalData;
    /** Tabla de la que se leerán los datos, si internalData es false */
    QString m_tableName;
    /** Campo que se mostrará en los botones */
    QString m_fieldView;
    /** Campo que se guardará si se utiliza este control para guardar información. Será el que
      se devuelva en value */
    QString m_fieldToSave;
    /** Filtro que puede aplicarse para obtener los datos */
    QString m_filter;
    /** Orden con el que se presentan los datos */
    QString m_order;
    /** Layout que contendrá los botones */
    QHBoxLayout *m_layoutButtons;
    QString m_syncronizeWith;
    /** Conjunto de beans asociados a cada botón */
    BaseBeanSharedPointerList m_list;
    DBFrameButtons * q_ptr;
    /** Indicará si el control se ha iniciado o no */
    bool m_init;

    DBFrameButtonsPrivate(DBFrameButtons * qq) : q_ptr (qq)
    {
        m_internalData = true;
        m_init = false;
        m_layoutButtons = NULL;
    }
    void addButton (const QString &texto);
};


DBFrameButtons::DBFrameButtons(QWidget * parent )
    : QFrame(parent), DBBaseWidget(), d(new DBFrameButtonsPrivate(this))
{
    d->m_buttons.setExclusive(true);
    d->m_layoutButtons = new QHBoxLayout;
    QWidget::setMinimumSize(1, 20);
    d->m_layoutButtons->setContentsMargins(0, 0, 0, 0);
    if ( layout() != 0 )
    {
        delete layout();
    }
    setLayout(d->m_layoutButtons);

    connect (&d->m_buttons, SIGNAL (buttonClicked(int)), this, SLOT(buttonIsClicked(int)));
}

DBFrameButtons::~DBFrameButtons()
{
    emit destroyed(this);
    delete d->m_layoutButtons;
    delete d;
}

bool DBFrameButtons::internalData()
{
    return d->m_internalData;
}

void DBFrameButtons::setInternalData(bool value)
{
    d->m_internalData = value;
}

QString DBFrameButtons::tableName()
{
    return d->m_tableName;
}

void DBFrameButtons::setTableName(const QString &value)
{
    d->m_tableName = value;
}

void DBFrameButtons::setFieldView(const QString &value)
{
    d->m_fieldView = value;
    if ( d->m_fieldToSave.isEmpty() )
    {
        d->m_fieldToSave = value;
    }
}

QString DBFrameButtons::fieldView()
{
    return d->m_fieldView;
}

QString DBFrameButtons::fieldToSave()
{
    return d->m_fieldToSave;
}

void DBFrameButtons::setFieldToSave(const QString &value)
{
    d->m_fieldToSave = value;
}

QString DBFrameButtons::filter()
{
    return d->m_filter;
}

void DBFrameButtons::setFilter(const QString &value)
{
    d->m_filter = value;
}

QString DBFrameButtons::order()
{
    return d->m_order;
}

void DBFrameButtons::setOrder(const QString &value)
{
    d->m_order = value;
}

QString DBFrameButtons::syncronizeWith()
{
    return d->m_syncronizeWith;
}

void DBFrameButtons::setSyncronizeWith(const QString &value)
{
    d->m_syncronizeWith = value;
}

/*!
  Algunas propiedades serán visibles dependiendo de si internalData es true o no
  */
bool DBFrameButtons::internalDataPropertyVisible()
{
    if ( d->m_internalData )
    {
        return true;
    }
    return false;
}

bool DBFrameButtons::externalDataPropertyVisible()
{
    if ( !d->m_internalData )
    {
        return true;
    }
    return false;
}

/*!
	Este es un slot interno, utilizado para que cuando se pulse un botón, se emitan
	las diferentes señales de este control.
 */
void DBFrameButtons::buttonIsClicked(int dataBoton)
{
    if ( !signalsBlocked() )
    {
        syncronizeWithOthers();
        emit buttonClicked();
        emit buttonClicked(dataBoton);
        emit buttonClicked(this, dataBoton);
        if ( dataBoton > -1 && dataBoton < d->m_list.size() )
        {
            BaseBeanSharedPointer bean = selectedBean();
            if ( !bean.isNull() )
            {
                emit buttonClicked(bean->fieldValue(d->m_fieldToSave));
                emit buttonClicked(this, bean->fieldValue(d->m_fieldToSave));
                emit valueEdited(bean->fieldValue(d->m_fieldToSave));
                DBBaseWidget::askToRecalculateCounterField();
            }
        }
    }
}

void DBFrameButtons::showEvent ( QShowEvent * event )
{
    DBBaseWidget::showEvent(event);
    if ( d->m_buttons.buttons().isEmpty() )
    {
        if ( !d->m_init )
        {
            init();
        }
        else
        {
            refresh();
        }
    }
}

/*!
  Inicializa el control
  */
void DBFrameButtons::init()
{
    if ( !d->m_internalData )
    {
        if ( !BaseDAO::select(d->m_list, d->m_tableName, d->m_filter, d->m_order) )
        {
            qWarning() << "DBFrameButtons::init(): Ocurrió un error tratando de leer datos de tableName: " <<
                       d->m_tableName << " : d->m_filter: " << d->m_filter << " : d->m_order " << d->m_order;
            return;
        }
    }
    else
    {
        d->m_init = true;
        BaseBean *bean = beanFromContainer();
        if ( bean != NULL )
        {
            QList<DBObject *> objectList = bean->navigateThrough(relationName(), relationFilter());
            if ( objectList.size() > 0 )
            {
                DBRelation *rel = qobject_cast<DBRelation *> (objectList.at(0));
                DBRelationObserver *obs = qobject_cast<DBRelationObserver *> (rel->observer());
                if ( rel != NULL )
                {
                    d->m_list = rel->sharedChildren(d->m_order);
                }
                if ( obs != NULL )
                {
                    QStringList fieldsToInform;
                    fieldsToInform << d->m_fieldView;
                    fieldsToInform << d->m_fieldToSave;
                    obs->informOnFieldChanges(this, fieldsToInform);
                }
            }
        }
    }
    foreach ( BaseBeanSharedPointer bean, d->m_list )
    {
        if ( bean->dbState() == BaseBean::INSERT || bean->dbState() == BaseBean::UPDATE )
        {
            // Campo visible de la relación
            DBField *fld = bean->field(d->m_fieldView);
            if ( fld != NULL )
            {
                d->addButton(fld->displayValue());
            }
        }
    }
    // Si solo hay un botón, o es el primero, se da por pulsado
    if ( d->m_buttons.buttons().size() > 0 )
    {
        d->m_buttons.buttons().at(0)->setChecked(true);
        buttonIsClicked(0);
    }
}

/*!
	Sirve para añadir un botón a este control, con el texto dado en el primer argumento.
	El argumento data será información adicional a ese botón que puede consultarse.
	Probablemente sea una primary key de un bean
 */
void DBFrameButtonsPrivate::addButton(const QString & texto)
{
    QPushButton *boton;
    QString strButtonName;
    // id del botón
    int id = m_buttons.buttons().size();

    // Los botones se ponen dentro del frame, en el layout correspondiente.
    // De no existir, se le crea uno. El padre es este widget, así garantizamos
    // que se borran con este.
    boton = new QPushButton (texto, q_ptr);
    // Nombre específico del botón
    QTextStream (&strButtonName) << "btn" << q_ptr->objectName() << texto;
    boton->setObjectName(strButtonName);
    boton->setCheckable(true);
    boton->setAttribute(Qt::WA_DeleteOnClose);
    m_layoutButtons->addWidget(boton);
    // Hay que pasarle el id del botón. Siempre será el tamaño de la lista, más uno
    m_buttons.addButton(boton, id);
}

/*!
  Devuelve el bean asociado al botón actualmente pulsado.
  */
BaseBeanSharedPointer DBFrameButtons::selectedBean()
{
    BaseBeanSharedPointer data;
    int id = d->m_buttons.checkedId();
    if ( id > -1 && id < d->m_list.size() )
    {
        data = d->m_list.at(id);
    }
    return data;
}

/*!
	Devuelve el id (orden) del botón cuyo data es el pasado en el argumento. data hace
	referencia al fieldValue "fieldToSave" del bean asociado al botón
 */
int DBFrameButtons::idButtonByData(const QVariant &data)
{
    int i = 0 ;

    foreach (BaseBeanSharedPointer bean, d->m_list)
    {
        DBField *fld = bean->field(d->m_fieldToSave);
        if ( fld != NULL )
        {
            if ( fld->checkValue(data) )
            {
                return i;
            }
        }
        i++;
    }
    return -1;
}

/*!
	Establece como clickado el botón cuyo id es el pasado en el argumento. El id no es más
	que el orden en el que se encuentran los botones.
 */
void DBFrameButtons::setCheckedButtonById (int id)
{
    if ( !d->m_init )
    {
        init();
    }
    QAbstractButton *boton = d->m_buttons.button(id);

    if ( boton != 0 && d->m_buttons.checkedId() != id )
    {
        boton->setChecked(true);
    }
}

/*!
    Borra todos los botones
 */
void DBFrameButtons::reset()
{
    QList<QAbstractButton *> ptrBotones = d->m_buttons.buttons();
    foreach ( QAbstractButton *button, ptrBotones )
    {
        d->m_buttons.removeButton(button);
        delButtonFromLayout(button);
    }
    d->m_list.clear();
    d->m_init = false;
}

void DBFrameButtons::delButtonFromLayout(QAbstractButton * button)
{
    d->m_layoutButtons->removeWidget(button);
    button->close();
}

/*!
 Establece el valor a mostrar en el control.
 Establece como clickado el botón cuyo data es el pasado en el argumento.
*/
void DBFrameButtons::setValue(const QVariant &data)
{
    if ( !d->m_init )
    {
        init();
    }
    int i = idButtonByData(data);

    AbstractObserver *obs = qobject_cast<AbstractObserver *>(sender());
    if ( obs != NULL )
    {
        blockSignals(true);
    }
    if ( i != -1 && d->m_buttons.checkedId() != i )
    {
        QAbstractButton *boton = d->m_buttons.button(i);
        if ( boton != NULL )
        {
            boton->setChecked(true);
        }
    }
    if ( obs != NULL )
    {
        blockSignals(false);
    }
}

/*!
  Devuelve el valor mostrado o introducido en el control
*/
QVariant DBFrameButtons::value()
{
    QVariant v;
    if ( !d->m_init )
    {
        init();
    }
    BaseBeanSharedPointer bean = selectedBean();
    if ( !bean.isNull() )
    {
        v = bean->fieldValue(d->m_fieldToSave);
    }
    return v;
}

/*!
  Ajusta el control y sus propiedades a lo definido en el field
*/
void DBFrameButtons::applyFieldProperties()
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
}

/*!
  Para refrescar los controles: Piden nuevo observador si es necesario
*/
void DBFrameButtons::refresh()
{
    QVariant data;
    if ( d->m_init )
    {
        data = this->value();
    }
    reset();
    init();
    if ( data.isValid() )
    {
        setValue(data);
    }
}

/*!
	Sincroniza, en botón pulsado, los DBFrameButtons que pertenezcan al mismo formulario
	y que se encuentren en la propiedad syncronizeWith
*/
void DBFrameButtons::syncronizeWithOthers()
{
    QDialog *dlg = CommonsFunctions::parentDialog(this);
    int id = d->m_buttons.checkedId();

    if ( d->m_syncronizeWith.isEmpty() )
    {
        return;
    }

    QStringList othersFrames = d->m_syncronizeWith.split(";");
    if ( dlg == NULL )
    {
        return;
    }
    foreach ( QString widName, othersFrames )
    {
        if ( widName != objectName() )
        {
            DBFrameButtons * frm = dlg->findChild<DBFrameButtons *> (widName.trimmed());
            if ( frm != NULL )
            {
                bool blockState = frm->blockSignals(true);
                frm->setCheckedButtonById(id);
                frm->blockSignals(blockState);
            }
        }
    }
}

void DBFrameButtons::observerUnregistered()
{
    DBBaseWidget::observerUnregistered();
    bool blockState = blockSignals(true);
    reset();
    blockSignals(blockState);
}

/*!
  Tenemos que decirle al motor de scripts, que DBFormDlg se convierte de esta forma a un valor script
  */
QScriptValue DBFrameButtons::toScriptValue(QScriptEngine *engine, DBFrameButtons * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBFrameButtons::fromScriptValue(const QScriptValue &object, DBFrameButtons * &out)
{
    out = qobject_cast<DBFrameButtons *>(object.toQObject());
}
