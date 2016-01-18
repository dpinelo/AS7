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

#ifndef DBFORMDLG_H
#define DBFORMDLG_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtCore>
#include <QtScript>
#include <alepherpglobal.h>
#include "dao/beans/basebean.h"

namespace Ui
{
class DBFormDlg;
}

class AERPScriptQsObject;
class DBFormDlgPrivate;
class DBFilterTableView;
class DBFilterTreeView;
class DBAbstractFilterView;

/*!
  Este es el formulario basico de la herramienta. Es donde se presentan el conjunto de registros
  por los que el usuario va navegando. Incluye internamente un DBFilterTableView que permite
  realizar un filtrado de registros. Desde aquí se permite añadir, editar y/o eliminar registros.
  Antes de ejecutar cualquier edición de registros:

  DBFormDlg.prototype.beforeDelete = function ()
  DBFormDlg.prototype.beforeInsert = function ()
  DBFormDlg.prototype.beforeUpdate = function ()

  Si estas funciones devuelven true o no existen, se ejecuta la acción. Si devuelven false, no se
  ejecutan.
  */
class ALEPHERP_DLL_EXPORT DBFormDlg : public QWidget, public QScriptable
{
    Q_OBJECT
    Q_FLAGS(DBFormButtons)

    /** Propiedad para establecer botones visibles o no al usuario. Por defecto, cuando se abre
      el formulario, siempre están todos los botones visibles, pero con esta propiedad y a traves
      de QtScript pueden articularse qué botones se verán y cuáles no */
    Q_PROPERTY (DBFormButtons visibleButtons READ visibleButtons WRITE setVisibleButtons)
    /** Bean actualmente seleccionado, o null si no hubiera ninguno seleccionado */
    Q_PROPERTY (BaseBeanPointer selectedBean READ selectedBean)
    /** Widget con los registros */
    Q_PROPERTY (QWidget* dbRecordsView READ dbRecordsView)
    Q_PROPERTY (QString tableName READ tableName)

    friend class DBFormDlgPrivate;

private:
    Ui::DBFormDlg *ui;
    DBFormDlgPrivate *d;

    void execQs();
    void actions();
    void init(const QString &value);
    bool construct(const QString &tableName);
    QHash<QString, QVariant> filterValuesToSetOnBean();

protected:
    void closeEvent(QCloseEvent * event);
    void keyPressEvent(QKeyEvent * ev);
    void showEvent(QShowEvent *ev);
    void hideEvent(QHideEvent *ev);
    bool eventFilter(QObject *target, QEvent *event);
    bool event(QEvent *e);

    void setOpenSuccess(bool value);

    AERPScriptQsObject *engine();
    void exposeAERPControlToQsEngine();

public:
    explicit DBFormDlg(QWidget *parent = 0, Qt::WindowFlags f = 0);
    explicit DBFormDlg(const QString &tableName, QWidget* parent = 0, Qt::WindowFlags f = 0);
    ~DBFormDlg();

    /** Identifica a los botones disponibles para el usuario. Utilizado en las funciones que
      ocultan o hacen visible los botones al usuario. Son visibles desde el motor  de Script a través
      de propiedades, esto es: thisForm.CREATE */
#ifdef DELETE
#undef DELETE
#endif
    enum DBFormButton
    {
        CREATE = 0x001,
        EDIT = 0x002,
        DELETE = 0x004,
        SEARCH = 0x008,
        COPY = 0x010,
        ADJUST_LINES = 0x020,
        EXIT = 0x040,
        OK = 0x080,
        COMMIT = 0x100,
        PRINT = 0x200,
        CreateButton = 0x001,
        EditButton = 0x002,
        DeleteButton = 0x004,
        SearchButton = 0x008,
        CopyButton = 0x010,
        AdjustLineButton = 0x020,
        ExitButton = 0x040,
        OkButton = 0x080,
        CommitButton = 0x100,
        PrintButton = 0x200,
        RelatedElementsButton = 0x400,
        EmailButton = 0x800,
        WizardButton = 0x1000,
        ViewButton = 0x2000,
        ExportSpreadSheet = 0x4000
    };
    Q_DECLARE_FLAGS(DBFormButtons, DBFormButton)

    DBFormButtons visibleButtons() const;
    void setVisibleButtons(DBFormButtons buttons);
    QString tableName() const;
    QWidget *dbRecordsView() const;

    BaseBeanPointer selectedBean();

    bool openSuccess();
    bool checkPermissionsToOpen();

    bool isFrozenModel() const;

    static QScriptValue toScriptValue(QScriptEngine *engine, DBFormDlg * const &in);
    static void fromScriptValue(const QScriptValue &object, DBFormDlg * &out);

public slots:
    void refreshFilterTableView();
    void freezeModel();
    void defrostModel();
    void setFilterKeyColumn(const QString &dbFieldName, const QString &op, const QVariant &value, int level = -1);
    void setFilter(const QString &filter);
    void resetFilter();

    void setStaticModel() { freezeModel(); }
    void disableStaticModel() { defrostModel(); }

    QScriptValue callQSMethod(const QString &method);
    QPushButton *createPushButton(int position,
                                  const QString &text,
                                  const QString &toolTip = "",
                                  const QString &img = "",
                                  const QString &methodNameToInvokeOnClicked = "",
                                  int width = -1,
                                  int height = -1);
    QPushButton *createEditButton(int position,
                                  const QString &text,
                                  const QString &toolTip,
                                  const QString &img,
                                  DBFormButtons editType,
                                  const QString &uiCode,
                                  const QString &qsCode);
    QLabel *createLabel(int position, const QString &text);

protected slots:
    void edit();
    void edit(const QString &insert);
    void edit(const QString &insert, const QString &uiCode, const QString &qsCode);
    void insertChild();
    void wizard();
    void deleteRecord(void);
    void search(void);
    void copy();
    void ok();
    void printRecord();
    void emailRecord();
    void showOrHideRelatedElements();
    void availableButtonsFromBean();
    void setBeanOnRelatedWidget();
    void showHelp();
    void view();
    void exportSpreadSheet();
    void showContextMenu(const QPoint &point);

private slots:
    void specialEdit(const QString code);
    void recordDlgClosed();
    void recordDlgCanceled();

signals:
    void sigSplashInfo(const QString &);
    void closingWindow(QWidget *objeto);
    void beforeDelete();
    void afterDelete(bool wasDeleted);
    void afterEdit(bool wasSaved);
};

Q_DECLARE_METATYPE(DBFormDlg*)
Q_DECLARE_METATYPE(DBFormDlg::DBFormButtons)
Q_DECLARE_OPERATORS_FOR_FLAGS(DBFormDlg::DBFormButtons)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBFormDlg, DBFormDlg*)

#endif

