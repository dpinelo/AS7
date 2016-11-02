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
#ifndef DBTREEVIEW_H
#define DBTREEVIEW_H

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
class DBTreeViewPrivate;

/**
  Esta clase extiende a QTreeView añadiéndole las propiedades de un DBBaseWidget de esta aplicación.
  Presentar datos anidados.

  @author David Pinelo <alepherp@alephsistemas.es>
  */
class ALEPHERP_DLL_EXPORT DBTreeView : public QTreeView, public DBAbstractViewInterface, public AERPBackgroundAnimation, public QScriptable
{
    Q_OBJECT
    Q_DISABLE_COPY(DBTreeView)

    /** Si no está vacío, el TreeView seleccionará el item correspondiente al valor del field de nombre
      fieldName del bean asignado al DBRecordDlg en el que se encuentra este control */
    Q_PROPERTY (QString fieldName READ fieldName WRITE setFieldName)
    /** Si no está vacío, marcará el bean o la relación sobre la que se buscará el bean que
      seleccionará el item */
    Q_PROPERTY (QString relationName READ relationName WRITE setRelationName)
    Q_PROPERTY (QString relationFilter READ relationFilter WRITE setRelationFilter)
    Q_PROPERTY (bool dataEditable READ dataEditable WRITE setDataEditable)
    Q_PROPERTY (bool aerpControl READ aerpControl)
    Q_PROPERTY (bool addToThisForm READ addToThisForm)
    /** Tablas referenciadas que se utilizarán para mostrar los datos (es
      decir, deben existir relaciones de padres a hijas definidas en los XML).
      Se separan por puntos y coma, por ejemplo: familias;subfamilias;articulos
      */
    Q_PROPERTY (QString tableNames READ tableNames WRITE setTableNames)
    /** De cada tabla que se muestra de forma jerárquica, este campo indica qué campos
      se muestran al usuario. Por ejemplo: nombre;nombre;descripcion.
      Deben tener el mismo número de entradas que m_tableNames y en el mismo orden */
    Q_PROPERTY (QString visibleColumns READ visibleColumns WRITE setVisibleColumns)
    Q_PROPERTY (QString keyColumns READ keyColumns WRITE setKeyColumns)
    /** Filtro para cada nivel del árbol. Los filtros se separan por ';', que determinan
      cada nivel. Los filtros serán de la forma: */
    Q_PROPERTY (QString filter READ filter WRITE setFilter)
    /** Indica si se mostrará alguna imagen. La imagen puede ser de un archivo que se indique,
      o bien de una imagen que se lea de la propia tabla que muestra los datos. El formato sería:
      file:/home/david/tmp/imagen.png;field:imagen
          */
    Q_PROPERTY (QString images READ images WRITE setImages)
    /** Devuelve el objeto bean que se encuentra seleccionado */
    Q_PROPERTY (BaseBeanSharedPointerList selectedBeans READ selectedBeans)
    /** Devuelve los beans checkeados */
    Q_PROPERTY (QScriptValue checkedBeans READ checkedBeans)
    /** Tamaño de las imágenes en el TreeView */
    Q_PROPERTY (QSize imagesSize READ imagesSize WRITE setImagesSize)
    /** Indica de qué niveles del árbol, se pueden seleccionar items. Esto es útil cuando se utiliza
      este TreeView como selector de items para agregar a otro contenedor (como un table view). Con esta
      propiedad se indica que sólo se pueden seleccionar items de los niveles indicados y separados
      por ;
      */
    Q_PROPERTY (QString selectLevels READ selectLevels WRITE setSelectLevels)
    Q_PROPERTY (QVariant value READ value WRITE setValue)
    /** Un control puede estar dentro de un AERPScriptWidget. ¿De dónde lee los datos? Los puede leer
      del bean asignado al propio AERPScriptWidget, o bien, leerlos del bean del formulario base
      que lo contiene. Esta propiedad marca esto */
    Q_PROPERTY (bool dataFromParentDialog READ dataFromParentDialog WRITE setDataFromParentDialog)

    /** Por defecto, los DBDetailView tienen un nombre generado automáticamente. Aquí se puede
      habilitar o desactivar esa característica */
    Q_PROPERTY (bool automaticName READ automaticName WRITE setAutomaticName)
    /** Permite navegar usando el ENTER. Cuando el usuario pincha enter, se va pasando de celda
      en celda */
    Q_PROPERTY (bool navigateOnEnter READ navigateOnEnter WRITE setNavigateOnEnter)
    /** Navegando con enter, al final de los registros, ¿inserta una nueva fila?*/
    Q_PROPERTY (bool atRowsEndNewRow READ atRowsEndNewRow WRITE setAtRowsEndNewRow)
    Q_PROPERTY (bool itemCheckBox READ itemCheckBox WRITE setItemCheckBox)
    /** Si se marca un check del árbol, que tiene hijos, ¿se marcan todos los hijos? */
    Q_PROPERTY (bool checkFatherCheckChildrens READ checkFatherCheckChildrens WRITE setCheckFatherCheckChildrens)
    /** Columnas que se marcarán como enlaces */
    Q_PROPERTY (QString linkColumns READ linkColumns WRITE setLinkColumns)
    /** Al aplicar un filtro, especifica si serán visibles nodos del árbol sin hijos */
    Q_PROPERTY (bool viewIntermediateNodesWithoutChildren READ viewIntermediateNodesWithoutChildren WRITE setViewIntermediateNodesWithoutChildren)
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

    friend class DBTreeViewPrivate;
    friend class AERPImageItemDelegate;

private:
    DBTreeViewPrivate *d;

    void init();

protected:
    virtual void showEvent(QShowEvent *event);
    virtual void keyPressEvent (QKeyEvent *event);
    virtual void mouseDoubleClickEvent (QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void closeEvent (QCloseEvent *event);
    virtual void paintEvent(QPaintEvent *event);

public:
    explicit DBTreeView(QWidget *parent = 0);
    ~DBTreeView();

    QString tableNames();
    void setTableNames(const QString &value);
    QString visibleColumns();
    void setVisibleColumns(const QString &value);
    QString keyColumns();
    void setKeyColumns(const QString &value);
    QString filter();
    void setFilter(const QString &value);
    QString rootName();
    void setRootName(const QString &value);
    QString images();
    void setImages(const QString &value);
    QSize imagesSize();
    void setImagesSize(const QSize &value);
    BaseBeanSharedPointerList selectedBeans();
    void setSelectLevels(const QString &level);
    QString selectLevels();
    void setItemCheckBox(bool value);
    bool itemCheckBox();
    void setCheckFatherCheckChildrens(bool value);
    bool checkFatherCheckChildrens();
    bool viewIntermediateNodesWithoutChildren() const;
    void setViewIntermediateNodesWithoutChildren(bool value);
    bool isOnEditingState() const
    {
        return state() == QAbstractItemView::EditingState;
    }

    AlephERP::ObserverType observerType(BaseBean *)
    {
        return AlephERP::DbField;
    }
    void applyFieldProperties();
    QVariant value();
    QScriptValue checkedBeans();
    void setCheckedBeans(BaseBeanSharedPointerList list, bool checked = true);
    void setCheckedBeansByPk(QVariantList list, bool checked = true);
    void setModel(QAbstractItemModel * model);

    void orderColumns(const QStringList &order);
    void sortByColumn(const QString &field, Qt::SortOrder order = Qt::DescendingOrder);

    Q_INVOKABLE BaseBeanSharedPointer selectedBean();

    static QScriptValue toScriptValue(QScriptEngine *engine, DBTreeView * const &in);
    static void fromScriptValue(const QScriptValue &object, DBTreeView * &out);

signals:
    void valueEdited(const QVariant &value);
    void destroyed(QWidget *widget);
    void enterPressedOnValidIndex(const QModelIndex &index);
    void doubleClickOnValidIndex(const QModelIndex &index);
    void recordChangedByKey (const QModelIndex &index);
    void fieldNameChanged();

public slots:
    void setValue(const QVariant &value);
    void refresh();
    void observerUnregistered();
    void spanRow(const QModelIndex &parentIndex = QModelIndex());
    void checkAllItems(bool checked = true);
    void saveColumnsOrder(const QStringList &order, const QStringList &sort);
    void saveColumnsOrder();
    void itemClicked(const QModelIndex &idx);
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

    void spanOnIntermediateBranchs();
    virtual void saveState();
    virtual void restoreState();
    void setFocus()
    {
        QWidget::setFocus();
    }
    void populateAllModel();
    void expandFirstBranchWithData();

protected slots:
    void saveHeaderSize(int logicalSection, int oldSize, int newSize);
    void nextCellOnEnter(const QModelIndex &actualCell);
    void headerContextMenu(QPoint pos);
    void fromMenuHideColumn();
    void collapsedIndex(QModelIndex);
    void expandedIndex(QModelIndex);
    void showContextMenu(const QPoint &point);
};

Q_DECLARE_METATYPE(DBTreeView*)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBTreeView, DBTreeView*)

#endif // DBTREEVIEW_H
