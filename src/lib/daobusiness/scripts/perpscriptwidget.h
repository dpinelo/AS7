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
#ifndef AERPSCRIPTWIDGET_H
#define AERPSCRIPTWIDGET_H

#include <QtCore>
#include <QtScript>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <alepherpglobal.h>
#include "widgets/dbbasewidget.h"

class BaseBean;
class AERPScriptWidgetPrivate;

/**
  Esta clase tomará su funcionalidad de un script asociado. Tomará su interfaz de
  un UI predefinido en base de datos. Se le podrá exigir que almaecene y muestre
  los valores de algunos DBFields de un determinado BaseBean. Según los valores
  que se le den a las propiedades, accederá a uno objeto u otro.
  <ul>
	<li>fieldName relleno: El script tendrá acceso el field indicado del bean principal
	al que da soporte el formulario en el que se encuentra este AERPScriptWidget</li>
    <li>relationName relleno: Tendrá acceso los children de esta relación, salvo que
	se indique un determinado filter</li>
	<li>relationFilter: Indicará a qué beans se tiene acceso</li>
  </ul>

Esta modificación es importante pero creo que da coherencia al tema. Tú sabes que un DBRecord tiene asociado un bean
(el registro de base de datos que se edita). Dentro de ese DBRecord puedo crear un AERPScriptWidget. Ese AERPScriptWidget
es un widget "contenedor" de cosas, que puede tener un código Javascript propio asignado y que existe para poder
reutilizar código.

Ahora bien, ese AERPScriptWidget existe, "per se". Y tiene una lógica parecida a DBRecord (para que tenga coherencia
al desarrollador en Javascript), y por esa regla de tres, tiene también un bean asociado (al igual que el DBRecord).
¿Qué bean? Depende:

-Si en el Designer, cuando incluyes el AERPScriptWidget no rellena el campos "relationName" de AERPScriptWidget,
éste cogerá como bean, el mismo que el DBRecord. Y por tanto los DB-controles que tenga dentro el AERPScriptWidget leerán
y tirarán del bean del DBRecord (casi como si estuvieran incluidos directamente en el DBRecord y no en el AERPScriptWidget).
-Si en el Designer, se rellena el relationName y relationFilter, el bean que se asigna a AERPScriptWidget será el que se
obtenga de buscar en la relación "relationName" del bean del DBRecord aplicándole el filtro relationFilter (si esto da más
de un bean, se coge el primero).

¿Esto qué significa? Lo siguiente: Imagínate que tienes la siguiente jerarquía:

DBRecord->AERPScriptWidget->DBLineEdit

AERPScriptWidget tiene relationName = ""
Si DBLineEdit tiene fieldName="nombre", ese line edit presentará el valor del field "nombre" del bean de DBRecord.

Ahora bien, AERPScriptWidget tiene relationName="alepherp_cliente".
Si DBLineEdit tiene fieldName="nombre", el line edit cogerá el valor del primer hijo de la relación "alepherp_cliente" del bean
DBRecord, y concretamente de su field "nombre".

Con esto (que todavía no está 100% pero esta tarde lo estará) no necesitarás la función "asignaValores" de
alepherp_cliente.widget.qs ya que el propio motor se encargará de mostrar los datos por tí.

Ejemplo: Cambias de cliente, en el bean vendí, los fields del widget deberían de refrescarse.

Por otro lado: Si el widget contiene código como este para obtener el bean del formulario padre

var vendiHijo = AERPScriptCommon.getOpenForm("alepherp_vendi.dbrecord.ui");
var beanVendiHijo = vendiHijo.bean();

Esa vía es buena. Sin embargo hay una más sencilla:

1.- El AERPScriptWidget tiene un objeto ya definido en Javascript (como bien sabes que tiene el DBRecord), que es thisForm. Ese objeto thisForm apunta
al formulario padre (al que contiene al AERPScriptWidget)

De modo que la línea anterior te quedaría:

var beanVendiHijo = thisForm.bean();
  */
class ALEPHERP_DLL_EXPORT AERPScriptWidget : public QWidget, public DBBaseWidget
{
    Q_OBJECT
    /** Nombre del widget para buscar en base de datos el .ui y el .qs que le darán funcionalidad.
      Si m_name está vacío se utilizará objectName() */
    Q_PROPERTY (QString name READ name WRITE setName)
    /** Este widget puede acceder al bean del formulario padre que lo contiene, o a un bean de una relación de
      éste especificando aquí esa relación */
    Q_PROPERTY (QString relationName READ relationName WRITE setRelationName)
    /** El bean sobre el que trabaja este widget, puede ser afinado por este relationFilter. Si por ejemplo, relationName
      está asociado a una relación M1, entonces se sabe directamente que es el bean padre, pero si no es así, habrá
      que seleccionar el bean utilizando relationFilter, con relationFilter = codcuenta='0001' */
    Q_PROPERTY (QString relationFilter READ relationFilter WRITE setRelationFilter)
    /** El código Javascript asociado, es una clase. El nombre de esa clase se define aquí. Si qsClassName está
      vacío, el nombre de la clase que controlará este script será "Widget" */
    Q_PROPERTY (QString qsClassName READ qsClassName WRITE setQsClassName)
    /** Indica si los controles dentro de este widget son editables o no */
    Q_PROPERTY (bool dataEditable READ dataEditable WRITE setDataEditable)
    Q_PROPERTY (bool aerpControl READ aerpControl)

private:
    AERPScriptWidgetPrivate *d;
    Q_DECLARE_PRIVATE(AERPScriptWidget)

protected:
    void showEvent(QShowEvent *event);
    void closeEvent (QCloseEvent * event);
    void focusInEvent(QFocusEvent * event);
    AbstractObserver *observer();

public:
    explicit AERPScriptWidget(QWidget *parent = 0);
    ~AERPScriptWidget();

    Q_INVOKABLE BaseBean *bean();
    Q_INVOKABLE BaseBean *dialogBean();

    void setName(const QString &value);
    QString name();
    void setQsClassName(const QString &value);
    QString qsClassName();

    AlephERP::ObserverType observerType(BaseBean *)
    {
        return AlephERP::BaseBean;
    }

    /** Devuelve el valor mostrado o introducido en el control */
    QVariant value();
    /** Ajusta el control y sus propiedades a lo definido en el field */
    void applyFieldProperties();
    void setRelationName(const QString &name);
    void setRelationFilter(const QString &name);

    Q_INVOKABLE QScriptValue callQSMethod(const QString &method);
    // Es igual que callQSMethod pero se mantiene por compatibilidad con las versiones instaladas en algunos clientes
    Q_INVOKABLE QScriptValue callMethod(const QString &method)
    {
        return callQSMethod(method);
    }

signals:
    /** Esta señal indicará cuándo se borra un widget. No se puede usar destroyed(QObject *)
      ya que cuando ésta se emite, se ha ejecutado ya el destructor de QWidget */
    void destroyed(QWidget *widget);
    /** El widget podrá emitir señales arbitrarias, a través de esta señal */
    void signalEmitted(const QString &signalName, const QVariant &value);
    void valueEdited(const QVariant &value);
    /** Este widget carga dinámicamente el UI de base de datos. Es interesante saber
      cuándo ha terminado su carga. Esta señal, se emite al terminar de inicializarse */
    void ready();
    void fieldNameChanged();

public slots:
    bool initQs();
    void emitSignal(const QString &signalName, const QVariant &value);
    /** Establece el valor a mostrar en el control */
    void setValue(const QVariant &value);
    /** Para refrescar los controles: Piden nuevo observador si es necesario */
    void refresh();
    void observerUnregistered();
    virtual void setFocus() { QWidget::setFocus(); }
};


Q_DECLARE_METATYPE(AERPScriptWidget*)

#endif // AERPSCRIPTWIDGET_H
