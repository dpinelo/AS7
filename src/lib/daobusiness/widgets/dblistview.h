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

#ifndef DBLISTVIEW_H
#define DBLISTVIEW_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtScript>
#include <alepherpglobal.h>
#include "widgets/dbabstractview.h"

class BaseBeanMetadata;
class DBListViewPrivate;

class ALEPHERP_DLL_EXPORT DBListView : public QListView, public DBAbstractViewInterface, public QScriptable
{
    Q_OBJECT
    Q_DISABLE_COPY(DBListView)

    /** ¿Se leen los datos de base de dato o del Bean del formulario que contiene este control? */
    Q_PROPERTY(bool internalData READ internalData WRITE setInternalData)

    /** Si los datos se leen de base de datos, entonces se introduce este tableName. Esta propiedad
    está activa si internalData es false. */
    Q_PROPERTY (QString tableName READ tableName WRITE setTableName DESIGNABLE externalDataPropertyVisible)
    /** Este filtro aplica a los datos que se leen de base de datos Esta propiedad
    está activa si internalData es false. */
    Q_PROPERTY (QString filter READ filter WRITE setFilter DESIGNABLE externalDataPropertyVisible)

    /** Campo en el que se almacenará el valor seleccionado. Esto tiene sentido si internalData es false,
        es decir, cuando se utiliza este widget para guardar datos de una columna de base de datos. */
    Q_PROPERTY (QString fieldName READ fieldName WRITE setFieldName)

    /** El relationName puede venir en la forma: presupuestos_ejemplares.presupuestos_actividades ...
    En ese caso, debemos iterar hasta dar con la relación adecuada. Esa iteración se realiza en
    combinación con el filtro establecido. El filtro en ese caso debe mostrar primero
    la primary key de presupuestos_ejemplares, y después el filtro que determina presupuestos_actividades. En ese
    caso, por ejemplo una combinación válida es:
    relationName: presupuestos_cabecera.presupuestos_ejemplares.presupuestos_actividades
    relationFilter: id_presupuesto=2595;id_numejemplares=24442;id_categoria=2;y más filtros para presupuestos_actividades */
    Q_PROPERTY(QString relationName READ relationName WRITE setRelationName DESIGNABLE internalDataPropertyVisible)
    Q_PROPERTY (QString relationFilter READ relationFilter WRITE setRelationFilter DESIGNABLE internalDataPropertyVisible)

    Q_PROPERTY (bool dataEditable READ dataEditable WRITE setDataEditable)
    Q_PROPERTY (bool aerpControl READ aerpControl)
    Q_PROPERTY (bool aerpControlRelation READ aerpControlRelation)

    /** Indica qué field del BaseBean (ya sea interno o externo) es visible */
    Q_PROPERTY (QString visibleField READ visibleField WRITE setVisibleField)
    /** Indica qué fields del BaseBean se devolverán con value. Irán separados por ;
      Si esta propiedad está vacía, se devolverá la primery key. */
    Q_PROPERTY (QString order READ order WRITE setOrder)

    /** Indicará si este control mostrará checkboxes clickeables para sus opciones. Debe tenerse en cuenta una cosa
     * y es que si este control tiene un dbFieldName, y por tanto está ligado a una columna de base de datos
     * marcar varios check boxes en él, implicará que sólo habrá un check marcado */
    Q_PROPERTY (bool itemCheckBox READ itemCheckBox WRITE setItemCheckBox)

    /** Establece de qué columnas o fields del base bean devolvera valores separados por ","
     * el método value.
     * Por ejemplo: keyField: id value devolverá los id marcados o seleccionados separados por ",".
     * Si en lugar de "," se quiere utilizar otro separador, se definirá en \a valueSeparator */
    Q_PROPERTY (QString keyField READ keyField WRITE setKeyField)

    /** Devuelve el objeto bean que se encuentra seleccionado (no tiene porqué ser lo que devuelve value*/
    Q_PROPERTY (BaseBeanSharedPointerList selectedBeans READ selectedBeans)

    /** Devuelve el valor mostrado o introducido en el control. Los valores
    que devuelve dependen de keyField, y serán, por ejemplo, para
    keyField="username, password":
    value te devolverá un array con datos de los beans asociados a los checks marcados.
    ¿Como? Así:
    	var v = listView.value;
    	entonces, v es
    	v[0] = "david,mipassword"
    	v[1] = "jose,mipassword"
    Si la propiedad valueSeparator contiene valor, value devolverá un string con los valores
    separados por coma
    	*/
    Q_PROPERTY (QVariant value READ value WRITE setValue)
    /** Devuelve los beans checkeados */
    Q_PROPERTY (QScriptValue checkedBeans READ checkedBeans)
    Q_PROPERTY (QScriptValue checkedMetadatas READ checkedMetadatas)
    Q_PROPERTY (QString valueSeparator READ valueSeparator WRITE setValueSeparator)

    friend class DBListViewPrivate;

private:
    DBListViewPrivate *d;

    bool setupInternalModel();
    bool init(bool onShowEvent);

protected:
    void showEvent ( QShowEvent * event );

public:
    explicit DBListView(QWidget *parent = 0);
    ~DBListView();

    QString visibleField();
    void setVisibleField(const QString &value);
    QString keyField();
    void setKeyField(const QString &value);
    void setItemCheckBox(bool value);
    bool itemCheckBox();
    QString valueSeparator() const;
    void setValueSeparator(const QString &value);

    virtual AlephERP::ObserverType observerType(BaseBean *);

    QVariant value();
    void applyFieldProperties();

    Q_INVOKABLE QScriptValue checkedBeans();
    Q_INVOKABLE void setCheckedBeans(BaseBeanSharedPointerList list, bool checked = true);
    Q_INVOKABLE void setCheckedBeansByPk(QVariantList list, bool checked = true);

    Q_INVOKABLE virtual void orderColumns(const QStringList &order);
    Q_INVOKABLE virtual void sortByColumn(const QString &field, Qt::SortOrder order = Qt::DescendingOrder);

    static QScriptValue toScriptValue(QScriptEngine *engine, DBListView * const &in);
    static void fromScriptValue(const QScriptValue &object, DBListView * &out);

signals:
    void destroyed(QWidget *widget);
    void valueEdited(const QVariant &value);
    void fieldNameChanged();

public slots:
    void refresh();
    void observerUnregistered();
    void setValue(const QVariant &value);
    virtual void saveColumnsOrder(const QStringList &order, const QStringList &sort);
    virtual void saveColumnsOrder();
    virtual void valueWasModified();
    virtual void askToRecalculateCounterField()
    {
        DBBaseWidget::askToRecalculateCounterField();
    }
    void setFocus()
    {
        QWidget::setFocus();
    }
    void resetCursor()
    {
        DBAbstractViewInterface::resetCursor();
    }

private slots:
};

Q_DECLARE_METATYPE(DBListView*)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBListView, DBListView*)

#endif // DBLISTVIEW_H
