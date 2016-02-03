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
#ifndef DBSEARCHDLG_H
#define DBSEARCHDLG_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <alepherpglobal.h>
#include "forms/perpbasedialog.h"
#include "dao/beans/basebean.h"

class BaseBean;
class DBBaseBeanModel;
class BaseBeanModel;
class FilterBaseBeanModel;
class BaseBeanMetadata;
class DBSearchDlgPrivate;

namespace Ui
{
class DBSearchDlg;
}

/**
  La idea es tener un formulario de búsqueda genérico. Para ello, el interfaz se definirá a través
  de base de datos, donde se almacenará el archivo .ui, de tal manera que sea fácilmente editable.
  El pie del formulario serán siempre los mismos botones (Buscar, aplicar filtro, editar y cerrar).
  Este formulario tendrá un objeto de filtro adecuado, y se expondrán métodos de este objeto al
  script asociado para así introducir los datos que el usuario ha ido escribiendo.

  Habrá varios modos de trabajo:
  1.- Se podrá configurar para seleccionar uno o varios (a través de checkboxes) registros de una tabla o vista determinada.
  El resultado de la selección se podrá recoger a través de los métodos \a selectedBean o checkedBeans
  2.- El usuario deberá seleccionar un posible registro de una relación M1: es decir, un padre para una relación de un registro
  En ese caso, no se trabajará con los métodos selectedBean o checkedBeans, sino que directamente se estará trabajando con el
  bean father de la relación
  @author David Pinelo <alepherp@alephsistemas.es>
  */
class ALEPHERP_DLL_EXPORT DBSearchDlg : public AERPBaseDialog
{
    Q_OBJECT

    /** Permite al usuario seleccionar varios registros utilizando para ello checkboxes.
    Esta propiedad DEBE establecerse antes de llamar a la función init, o no hará nada. */
    Q_PROPERTY (bool canSelectSeveral READ canSelectSeveral WRITE setCanSelectSeveral)
    Q_PROPERTY (DBSearchButtons visibleButtons READ visibleButtons WRITE setVisibleButtons)
    Q_PROPERTY (BaseBeanSharedPointer selectedBean READ selectedBean)
    Q_PROPERTY (BaseBeanSharedPointerList checkedBeans READ checkedBeans)
    Q_PROPERTY (bool userClickOk READ userClickOk)
    Q_PROPERTY (bool userInsertNewRecord READ userInsertNewRecord)
    Q_PROPERTY (QString filterData READ filterData WRITE setFilterData)
    Q_PROPERTY (bool canInsertRecords READ canInsertRecords WRITE setCanInsertRecords)
    Q_PROPERTY (BaseBeanPointer masterBean READ masterBean WRITE setMasterBean)

    Q_FLAGS (DBSearchButtons)

    friend class DBSearchDlgPrivate;

public:

    enum DBSearchButtonsFlag
    {
        Ok = 0x001,
        Close = 0x002,
        EditRecord = 0x008,
        NewRecord = 0x010
    };
    Q_DECLARE_FLAGS (DBSearchButtons, DBSearchButtonsFlag)

private:
    Ui::DBSearchDlg *ui;
    DBSearchDlgPrivate *d;

protected:
    void showEvent (QShowEvent * event);

public:
    explicit DBSearchDlg(QWidget *parent = 0);
    explicit DBSearchDlg(const QString &tableName, QWidget *parent = 0);
    ~DBSearchDlg();

    BaseBeanSharedPointer selectedBean();
    BaseBeanSharedPointerList checkedBeans();

    bool setTableName(const QString &value);

    bool userClickOk() const;
    bool userInsertNewRecord() const;

    QString filterData() const;
    void setFilterData(const QString &value);

    bool canInsertRecords() const;
    void setCanInsertRecords(bool value);

    bool canSelectSeveral();
    void setCanSelectSeveral(bool value);

    DBSearchButtons visibleButtons();
    void setVisibleButtons(DBSearchButtons buttons);

    BaseBean *templateBean();

    BaseBeanPointer masterBean() const;
    void setMasterBean(BaseBeanPointer bean);

    QWidget *contentWidget() const;

    static QScriptValue toScriptValue(QScriptEngine *aerpQsEngine, DBSearchDlg * const &in);
    static void fromScriptValue(const QScriptValue &object, DBSearchDlg * &out);

signals:
    void beanAboutToBeInserted(BaseBeanPointer bean);

public slots:
    bool init();
    void setColumnFilter (const QString &name, const QVariant &value, const QString &op);
    void setColumnBetweenFilter (const QString &name, const QVariant &fecha1, const QVariant &fecha2);
    void select(const QModelIndex &index = QModelIndex());
    void search();
    void view();
    void edit();
    void newRecord();
    void setDateForAnotherDate();
    void addDefaultValue(const QString &dbFieldName, const QVariant &value);
    void setDefaultValues(const QHash<QString, QVariant> &values);
    void setInsertFather(BaseBeanPointer bean);
};

Q_DECLARE_METATYPE(DBSearchDlg*)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBSearchDlg, DBSearchDlg*)

#endif // DBSEARCHDLG_H
