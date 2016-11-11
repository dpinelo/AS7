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
#ifndef DBDETAILVIEW_H
#define DBDETAILVIEW_H

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

class DBRelation;
class RelationBaseBeanModel;
class AbstractObserver;
class DBDetailViewPrivate;

namespace Ui
{
class DBDetailView;
}

/**
  Presenta y permite modificar los datos de una relación DBRelation contenida en un
  BaseBean. Es ideal para editar los registros descendientes. Presenta tres botones
  que abren un diálogo si lo hubiera, y permiten agregar, modificar o eliminar
  registros.

  De por sí este componente no ejecuta ninguna acción en base de datos. El almacenamiento
  o no en base de datos, dependen del procesamiento que se realice en el BaseBean Padre.

  Este plugin, y esta clase, junto con las otras clases utilizadas están escritas MUY bien,
  siguiendo todos los paradigmas de POO y de patrones de diseño. Es por ejemplo, muy destacable
  la utilización de los filtros y QItemSelectionModel para todas las interacciones con el modelo.

  @see BaseBean
  @see DBRelation
  @author David Pinelo <alepherp@alephsistemas.es>
  */
class ALEPHERP_DLL_EXPORT DBDetailView :
        public QWidget,
        public DBAbstractViewInterface,
        public QScriptable
{
    Q_OBJECT
    Q_FLAGS(Buttons)
    Q_ENUMS(Button)
    Q_FLAGS(WorkModes)
    Q_ENUMS(WorkMode)
    Q_FLAGS(RecordStates)
    Q_ENUMS(RecordState)
    Q_DISABLE_COPY(DBDetailView)

    Q_PROPERTY (QString relationName READ relationName WRITE setRelationName)
    Q_PROPERTY (QString relationFilter READ relationFilter WRITE setRelationFilter)
    Q_PROPERTY (bool dataEditable READ dataEditable WRITE setDataEditable)
    Q_PROPERTY (bool aerpControl READ aerpControl)
    Q_PROPERTY (bool addToThisForm READ addToThisForm)
    Q_PROPERTY (bool aerpControlRelation READ aerpControlRelation)
    Q_PROPERTY (QString order READ order WRITE setOrder)
    Q_PROPERTY (QAbstractItemView::EditTriggers editTriggers READ editTriggers WRITE setEditTriggers)
    /** La edición de registros se realizará directamente sobre los DBDetailView de tal modo
      que no aparecerá el botón Editar */
    Q_PROPERTY (bool inlineEdit READ inlineEdit WRITE setInlineEdit)
    Q_PROPERTY (DBDetailView::Buttons visibleButtons READ visibleButtons WRITE setVisibleButtons)
    /** Devuelve los beans checkeados */
    Q_PROPERTY (QScriptValue checkedBeans READ checkedBeans)
    Q_PROPERTY (DBDetailView::WorkModes workMode READ workMode WRITE setWorkMode)
    /** Para el modo de trabajo ExistingPrevious, indica si se visualizaon todos los registros, o sólo
     * aquellos que no están asignados */
    Q_PROPERTY (bool showUnassignmentRecords READ showUnassignmentRecords WRITE setShowUnassignmentRecords)
    /** Estados que serán visibles */
    Q_PROPERTY (AlephERP::DBRecordStates visibleRecords READ visibleRecords WRITE setVisibleRecords)
    /** ¿Es posible abrir registros con enlaces en modo edición? */
    Q_PROPERTY (bool allowedEdit READ allowedEdit WRITE setAllowedEdit)
    /** Es posible definir una columna de ordenación. El usuario podrá entonces mover las filas, y cuando esto ocurra
     * se actualizará el valor de las columnas de ordenación de los diferentes registros. Si esta propiedad está
     * establecida, se obvia el valor de \a order. La columna de ordenación se define en los metadatos */
    Q_PROPERTY (bool canMoveRows READ canMoveRows WRITE setCanMoveRows)
    /** Es posible hacer algunas columnas de modo readonly, y para esta instancia de widget, evitar que
     * sean de escritura. Los nombres de las columnas se separan con ; */
    Q_PROPERTY (QString readOnlyColumns READ readOnlyColumns WRITE setReadOnlyColumns)
    /** Indica si los botones de agregar/modificar... presentan los shortcut que se definen por defecto */
    Q_PROPERTY (bool useDefaultShortcut READ useDefaultShortcut WRITE setUseDefaultShortcut)
    /** Indica si, en modo edición, al insertar llegar al final de la línea se inserta un nuevo registro */
    Q_PROPERTY (bool atRowsEndNewRow READ atRowsEndNewRow WRITE setAtRowsEndNewRow DESIGNABLE enableAtRowsEndNewRow)
    /** Columnas adicionales que se desean visualizar, y no necesariamente están en los metadatos */
    Q_PROPERTY (QString visibleColumns READ visibleColumns WRITE setVisibleColumns)
    /** Pide confirmación para la eliminación de un registro */
    Q_PROPERTY (bool promptForDelete READ promptForDelete WRITE setPromptForDelete)
    /** Utiliza su propia transacción para los registros hijos */
    Q_PROPERTY(bool useNewContextDirectDescents READ useNewContextDirectDescents WRITE setUseNewContextDirectDescents)
    /** Utiliza su propia transacción para los registros hijos */
    Q_PROPERTY(bool useNewContextExistingPrevious READ useNewContextExistingPrevious WRITE setUseNewContextExistingPrevious)
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
    /** Permite especificar que se carguen en segundo plano los hijos */
    Q_PROPERTY (bool loadOnBackground READ loadOnBackground WRITE setLoadOnBackground)

    friend class DBDetailViewPrivate;

protected:
    Ui::DBDetailView *ui;
    DBDetailViewPrivate *d;

    virtual void showEvent(QShowEvent *event);
    virtual void hideEvent(QHideEvent *event)
    {
        DBBaseWidget::hideEvent(event);
        QWidget::hideEvent(event);
    }
    virtual void focusInEvent(QFocusEvent *ev);

    virtual bool init(bool onShowEvent = false);
    virtual bool setupInternalModel();
    virtual bool eventFilter(QObject *target, QEvent *event);

public:
    explicit DBDetailView(QWidget *parent = 0);
    virtual ~DBDetailView();

    /** Modo de trabajo de este control */
    enum WorkMode
    {
        /** Modo de trabajo normal, y hará referencia a la edición de registros hijos del principal */
        DirectDescents = 0x01,
        /** Útil cuando se quiere que se puedan agregar a este control otros registros ya existentes. Por lo tanto, lo que
         * se mostrará es la posibilidad de anexar o seleccionar otro registro existente */
        ExistingPrevious = 0x02
    };
    Q_DECLARE_FLAGS(WorkModes, WorkMode)

    /** Botones que serán visibles */
#ifdef DELETE
#undef DELETE
#endif
    // TODO: Habría que renombrar absolutamente TODOS estos botones.
    enum Button
    {
        // ESTO ES LEGACY
        INSERT = 0x01,
        UPDATE = 0x02,
        DELETE = 0x04,
        VIEW = 0x08,
        INSERTEXISTING = 0x10,
        REMOVEEXISTING = 0x20,
        // Estos son los que habría que utilizar
        None = 0x100,
        InsertButton = 0x001,
        UpdateButton = 0x002,
        DeleteButton = 0x004,
        ViewButton = 0x008,
        InsertExistingButton = 0x010,
        RemoveExistingButton = 0x020,
        ExportSpreadSheetButton = 0x040
    };
    Q_DECLARE_FLAGS(Buttons, Button)

    virtual AlephERP::ObserverType observerType(BaseBean *)
    {
        return AlephERP::DbRelation;
    }
    virtual void applyFieldProperties();
    virtual QVariant value()
    {
        return QVariant();
    }

    virtual void setRelationFilter(const QString &value);

    virtual QAbstractItemView::EditTriggers editTriggers () const;
    virtual void setEditTriggers (QAbstractItemView::EditTriggers triggers);

    virtual bool inlineEdit () const;
    virtual void setInlineEdit(bool value);

    virtual Buttons visibleButtons();
    virtual void setVisibleButtons(Buttons buttons);

    virtual WorkModes workMode();
    virtual void setWorkMode(WorkModes);

    virtual bool showUnassignmentRecords();
    virtual void setShowUnassignmentRecords(bool value);

    virtual bool canMoveRows() const;
    virtual void setCanMoveRows(bool value);

    virtual bool useDefaultShortcut() const;
    virtual void setUseDefaultShortcut(bool value);

    virtual void setAtRowsEndNewRow(bool value);
    bool enableAtRowsEndNewRow();

    bool useNewContextDirectDescents() const;
    void setUseNewContextDirectDescents(bool value);
    bool useNewContextExistingPrevious() const;
    void setUseNewContextExistingPrevious(bool value);

    bool promptForDelete() const;
    void setPromptForDelete(bool value);

    virtual void setReadOnlyColumns(const QString &value);
    virtual void setVisibleColumns(const QString &value);

    virtual BaseBeanSharedPointerList selectedBeans();
    virtual QItemSelectionModel *itemViewSelectionModel();

    static QScriptValue toScriptValue(QScriptEngine *engine, DBDetailView * const &in);
    static void fromScriptValue(const QScriptValue &object, DBDetailView * &out);

    Q_INVOKABLE virtual QScriptValue checkedBeans();
    Q_INVOKABLE virtual void setCheckedBeans(BaseBeanSharedPointerList list, bool checked = true);
    Q_INVOKABLE virtual void setCheckedBeansByPk(QVariantList list, bool checked = true);

    Q_INVOKABLE virtual void orderColumns(const QStringList &order);
    Q_INVOKABLE virtual void sortByColumn(const QString &field, Qt::SortOrder order = Qt::DescendingOrder);
    Q_INVOKABLE QPushButton *createPushButton(int position, const QString &text, const QString &toolTip = "", const QString &img = "", const QString &methodNameToInvokeOnClicked = "");

public slots:
    virtual void setValue(const QVariant &value);
    virtual void refresh();
    virtual void observerUnregistered();
    virtual void saveColumnsOrder(const QStringList &order, const QStringList &sort);
    virtual void saveColumnsOrder();
    void setFocus()
    {
        QWidget::setFocus();
    }
    void resetCursor()
    {
        DBAbstractViewInterface::resetCursor();
    }
    virtual void exportSpreadSheet();

protected slots:
    virtual void editRecord(const QString &action = "view");
    virtual void deleteRecord();
    virtual void addExisting();
    virtual void removeExisting();
    virtual void enableButtonsDependsOnSelection();

signals:
    void valueEdited(const QVariant &value);
    void destroyed(QWidget *widget);
    void fieldNameChanged();
    void addedExistingRecords(BaseBeanSharedPointerList beans);
    void addedExistingRecords();
    void removedExistingRecords(BaseBeanSharedPointerList beans);
    void removedExistingRecords();
    void userEditedData(BaseBean *bean);
};

Q_DECLARE_METATYPE(DBDetailView*)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBDetailView, DBDetailView*)

#endif // DBDETAILVIEW_H
