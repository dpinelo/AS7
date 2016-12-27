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
#ifndef DBTABLEVIEW_H
#define DBTABLEVIEW_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtScript>
#include <alepherpglobal.h>
#include <aerpcommon.h>
#include "widgets/dbabstractview.h"
#include "widgets/aerpbackgroundanimation.h"

class BaseBean;
class RelationBaseBeanModel;
class AERPHtmlDelegate;
class DBTableViewPrivate;
class BaseBeanMetadata;
class DBFieldMetadata;

/**
  Es la extensión básica de QTableView, que añade características especiales para
  poder tratar con BaseBean. Es una tabla usada por otros componentes como
  DBDetailView. Puede presentar datos internos (si internalData está a true, a partir
  del bean del observador adecuado) o externos.

  @see BaseBean
  @see DBDetailView
  @author David Pinelo <alepherp@alephsistemas.es>
*/
class ALEPHERP_DLL_EXPORT DBTableView :
        public QTableView,
        public DBAbstractViewInterface,
        public AERPBackgroundAnimation,
        public QScriptable
{
    Q_OBJECT
    Q_DISABLE_COPY(DBTableView)

    /** Si los datos se leen de base de datos, entonces se introduce este tableName */
    Q_PROPERTY (QString tableName READ tableName WRITE setTableName DESIGNABLE externalDataPropertyVisible)
    /** Este filtro aplica a los datos que se leen de base de datos */
    Q_PROPERTY (QString filter READ filter WRITE setFilter DESIGNABLE externalDataPropertyVisible)
    /** Orden inicial con el que se visualizarán las filas */
    Q_PROPERTY (QString order READ order WRITE setOrder DESIGNABLE externalDataPropertyVisible)
    /** Es posible definir una columna de ordenación. El usuario podrá entonces mover las filas, y cuando esto ocurra
     * se actualizará el valor de las columnas de ordenación de los diferentes registros. Si esta propiedad está
     * establecida, se obvia el valor de \a order. La columna de ordenación se define en los metadatso */
    Q_PROPERTY (bool canMoveRows READ canMoveRows WRITE setCanMoveRows)

    /** El relationName puede venir en la forma: presupuestos_ejemplares.presupuestos_actividades ...
    En ese caso, debemos iterar hasta dar con la relación adecuada. Esa iteración se realiza en
    combinación con el filtro establecido. El filtro en ese caso debe mostrar primero
    la primary key de presupuestos_ejemplares, y después el filtro que determina presupuestos_actividades. En ese
    caso, por ejemplo una combinación válida es:
    relationName: presupuestos_cabecera.presupuestos_ejemplares.presupuestos_actividades
    relationFilter: id_presupuesto=2595;id_numejemplares=24442;id_categoria=2;y más filtros para presupuestos_actividades */
    Q_PROPERTY(QString relationName READ relationName WRITE setRelationName)
    Q_PROPERTY (QString relationFilter READ relationFilter WRITE setRelationFilter DESIGNABLE internalDataPropertyVisible)

    Q_PROPERTY (bool dataEditable READ dataEditable WRITE setDataEditable)
    Q_PROPERTY (bool aerpControl READ aerpControl)
    Q_PROPERTY (bool addToThisForm READ addToThisForm)
    Q_PROPERTY (bool aerpControlRelation READ aerpControlRelation)

    /** Devuelve el objeto bean que se encuentra seleccionado */
    Q_PROPERTY (BaseBeanSharedPointerList selectedBeans READ selectedBeans)

    /** Puede definirse que una columna del listado, tenga un boton de checkbox. Después
      será posible obtener los beans que se han marcado con checkedBeans. Esta propiedad
      define esa columna */
    Q_PROPERTY (QString fieldCheckBox READ fieldCheckBox WRITE setFieldCheckBox)

    /** Por defecto, los DBDetailView tienen un nombre generado automáticamente. Aquí se puede
      habilitar o desactivar esa característica */
    Q_PROPERTY (bool automaticName READ automaticName WRITE setAutomaticName)
    /** Permite navegar usando el ENTER. Cuando el usuario pincha enter, se va pasando de celda
      en celda */
    Q_PROPERTY (bool navigateOnEnter READ navigateOnEnter WRITE setNavigateOnEnter)
    /** Navegando con enter, al final de los registros, ¿inserta una nueva fila?*/
    Q_PROPERTY (bool atRowsEndNewRow READ atRowsEndNewRow WRITE setAtRowsEndNewRow)
    /** Devuelve los beans checkeados */
    Q_PROPERTY (QScriptValue checkedBeans READ checkedBeans)
    /** Estados que serán visibles */
    Q_PROPERTY (AlephERP::DBRecordStates visibleRecords READ visibleRecords WRITE setVisibleRecords)
    /** ¿Es posible abrir registros con enlaces en modo edición? */
    Q_PROPERTY (bool allowedEdit READ allowedEdit WRITE setAllowedEdit)
    /** Es posible hacer algunas columnas de modo readonly, y para esta instancia de widget, evitar que
     * sean de escritura. Los nombres de las columnas se separan con ; */
    Q_PROPERTY (QString readOnlyColumns READ readOnlyColumns WRITE setReadOnlyColumns)
    /** Columnas que se visualizarán, que pueden ser a partir de metadatos o relaciones más elaboradas
        <property name="additionalColumns">
         <string>stocksmovimientos.father.tipo;stocksmovimientos.father.descripcion</string>
        </property>
    */
    Q_PROPERTY (QString visibleColumns READ visibleColumns WRITE setVisibleColumns)
    /** Las columnas con el nombre indicado, se presentarán como un enlace */
    Q_PROPERTY (QString linkColumns READ linkColumns WRITE setLinkColumns)
    /** Nos indica si se está editando __inline el qabstractitemview */
    Q_PROPERTY (bool isOnEditingState READ isOnEditingState)
    /** Indica si el currentIndex se encuentra actualmente en un registro "nuevo" */
    Q_PROPERTY (bool currentIndexOnNewRow READ currentIndexOnNewRow)
    /** El widget estará habilitado, sólo si esta propiedad está vacía o para el usuario con roles
     * que estén aquí presentes */
    Q_PROPERTY (QStringList enabledForRoles READ enabledForRoles WRITE setEnabledForRoles)
    /** El widget estará visible, sólo si esta propiedad está vacía o para el usuario con roles
     * que estén aquí presentes */
    Q_PROPERTY (QStringList visibleForRoles READ visibleForRoles WRITE setVisibleForRoles)
    /** El widget estará marcado como editable, sólo si esta propiedad está vacía o para el usuario con roles
     * que estén aquí presentes */
    Q_PROPERTY (QStringList dataEditableForRoles READ dataEditableForRoles WRITE setDataEditableForRoles)
    /** El widget estará habilitado, sólo si esta propiedad está vacía o para los usuarios aquí presentes */
    Q_PROPERTY (QStringList enabledForUsers READ enabledForUsers WRITE setEnabledForUsers)
    /** El widget estará habilitado, sólo si esta propiedad está vacía o para los usuarios aquí presentes */
    Q_PROPERTY (QStringList visibleForUsers READ visibleForUsers WRITE setVisibleForUsers)
    /** El widget estará habilitado, sólo si esta propiedad está vacía o para los usuarios aquí presentes */
    Q_PROPERTY (QStringList dataEditableForUsers READ dataEditableForUsers WRITE setDataEditableForUsers)
    /** Indica si muestra animación en la carga de muchos registros (con un movie delegate) */
    Q_PROPERTY (bool showAnimationOnDataLoad READ showAnimationOnDataLoad WRITE setShowAnimationOnDataLoad)
    /** Permite especificar que se carguen en segundo plano los hijos */
    Q_PROPERTY (bool loadOnBackground READ loadOnBackground WRITE setLoadOnBackground)

private:
    DBTableViewPrivate *d;

    bool setupInternalModel();
    bool setupExternalModel();

protected:
    virtual void showEvent (QShowEvent * event);
    virtual void keyPressEvent (QKeyEvent * event);
    virtual void closeEvent (QCloseEvent * event);
    virtual void mouseDoubleClickEvent (QMouseEvent * event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual bool viewportEvent(QEvent * event);
    virtual void paintEvent(QPaintEvent *event);
    virtual bool init(bool onShowEvent = false);

public:
    explicit DBTableView( QWidget * parent = 0 );
    ~DBTableView();

    AlephERP::ObserverType observerType(BaseBean *bean);

    void setFieldCheckBox(const QString &name);
    QString fieldCheckBox();
    virtual void setAllowedEdit(bool value);
    bool canMoveRows() const;
    void setCanMoveRows(bool value);
    bool isOnEditingState() const
    {
        return state() == QAbstractItemView::EditingState;
    }
    bool showAnimationOnDataLoad() const;
    void setShowAnimationOnDataLoad(bool value);

    /** Devuelve el valor mostrado o introducido en el control */
    QVariant value();
    /** Ajusta el control y sus propiedades a lo definido en el field */
    void applyFieldProperties();

    Q_INVOKABLE BaseBeanSharedPointer addBean();
    Q_INVOKABLE void deleteSelectedsBean();
    Q_INVOKABLE QScriptValue checkedBeans();
    Q_INVOKABLE void setCheckedBeans(BaseBeanSharedPointerList list, bool checked = true);
    Q_INVOKABLE void setCheckedBeansByPk(QVariantList list, bool checked = true);

    void orderColumns(const QStringList &order);
    void sortByColumn(const QString &field, Qt::SortOrder order = Qt::DescendingOrder);

    static QScriptValue toScriptValue(QScriptEngine *engine, DBTableView * const &in);
    static void fromScriptValue(const QScriptValue &object, DBTableView * &out);

protected slots:
    void saveHeaderSize(int logicalSection, int oldSize, int newSize);
    void nextCellOnEnter(const QModelIndex &actualCell);
    void headerContextMenu(QPoint pos);
    void fromMenuHideColumn();
    void showContextMenu(const QPoint &point);

public slots:
    /** Para refrescar los controles: Piden nuevo observador si es necesario */
    void refresh();
    /** Establece el valor a mostrar en el control */
    void setValue(const QVariant &value);
    void observerUnregistered();
    void saveColumnsOrder(const QStringList &order, const QStringList &sort);
    void saveColumnsOrder();
    void checkAllItems(bool checked = true);
    void itemClicked(const QModelIndex &idx);
    void setRelationName(const QString &name);
    void setTableName(const QString &value);
    void setFilter(const QString &value);
    void setModel(QAbstractItemModel *model);
    void hideColumn(const QString &fieldName);
    void newRowOrder(int logicalIndex, int oldVisualIndex, int newVisualIndex);
    void setFocus()
    {
        QWidget::setFocus();
    }
    void applyRowSpan();
    void copy()
    {
        DBAbstractViewInterface::copy();
    }
    void paste()
    {
        DBAbstractViewInterface::paste();
    }
    void resetCursor()
    {
        DBAbstractViewInterface::resetCursor();
    }
    void showAnimation()
    {
        AERPBackgroundAnimation::showAnimation();
    }
    void hideAnimation()
    {
        AERPBackgroundAnimation::hideAnimation();
    }
    virtual void exportSpreadSheet();
    virtual void editCurrentCell();

signals:
    void valueEdited(const QVariant &value);
    void recordChangedByKey (const QModelIndex &index);
    /** Esta señal indicará cuándo se borra un widget. No se puede usar destroyed(QObject *)
      ya que cuando ésta se emite, se ha ejecutado ya el destructor de QWidget */
    void destroyed(QWidget *widget);
    void enterPressedOnValidIndex(const QModelIndex &index);
    void doubleClickOnValidIndex(const QModelIndex &index);
    void itemChecked(const QModelIndex &index);
    void fieldNameChanged();

};

Q_DECLARE_METATYPE(DBTableView*)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBTableView, DBTableView*)

#endif
