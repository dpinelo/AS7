/***************************************************************************
 *   Copyright (C) 2011 by David Pinelo                                    *
 *   alepherp@alephsistemas.es                                             *
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
#ifndef BASEBEANMODEL_H
#define BASEBEANMODEL_H

#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <alepherpglobal.h>
#include <aerpcommon.h>
#include "dao/beans/basebean.h"

class BaseBean;
class BaseBeanMetadata;
class BaseBeanModelPrivate;
class DBFieldMetadata;

class ALEPHERP_DLL_EXPORT BaseBeanModel : public QAbstractItemModel
{
    Q_OBJECT
private:
    BaseBeanModelPrivate *d;
    Q_DECLARE_PRIVATE(BaseBeanModel)
    Q_DISABLE_COPY(BaseBeanModel)

    /** Cuando utilizamos un completer en un DBLineEdit, tenemos la opción de utilizar wildcard. Por ejemplo
     * para localizar el registro con valor de campo AL000000002, el usuario podría escribir AL.2.
     * QCompleter no entiende de wildcard, por lo que lo hacemos a través de una ruta especial, que es utilizando
     * el rol AlephERP::ReplaceWildCards para el método data. Así, en lugar de AL0000002, devolverá AL.2
     * y por tanto existirá match en el completer */
    Q_PROPERTY (QChar wildCard READ wildCard WRITE setWildCard)
    /** Autoexplicativo */
    Q_PROPERTY (bool canShowCheckBoxes READ canShowCheckBoxes WRITE setCanShowCheckBoxes)
    Q_PROPERTY (QString lastErrorMessage READ lastErrorMessage WRITE setLastErrorMessage)

protected:
    QHash<int, bool> m_checkedItems;
    BaseBeanSharedPointer m_lastInsertedBean;

    virtual QString sqlSelectFieldsClausule(BaseBeanMetadata *metadata, QList<DBFieldMetadata *> fields, bool includeOid, const QString &alias = "");
    virtual QString buildSqlSelect(BaseBeanMetadata *metadata, const QString &where, const QString &order);
    virtual QString addEnvVarWhere(BaseBeanMetadata *metadata, const QString &initialWhere);
    virtual void timerEvent (QTimerEvent * event);
    virtual int timerId();
    virtual void setLoadingData(bool value);

public:
    explicit BaseBeanModel(QObject *parent = 0);
    virtual ~BaseBeanModel();

    virtual BaseBeanMetadata * metadata() const = 0;

    QString lastErrorMessage() const;
    void setLastErrorMessage(const QString &message);

    virtual QVariant data(const QModelIndex &item, int role) const;
    virtual QVariant data(DBField *field, const QModelIndex &item, int role) const;
    virtual QVariant headerData(DBFieldMetadata *fld, int section, Qt::Orientation orientation, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    virtual BaseBeanSharedPointer bean(int row, bool reloadIfNeeded = true) const;
    virtual BaseBeanSharedPointer bean(const QModelIndex &index, bool reloadIfNeeded = true) const = 0;
    virtual BaseBeanSharedPointerList beans(const QModelIndexList &list) = 0;
    virtual BaseBeanSharedPointerList beansToBeEdited(const QModelIndexList &list);
    virtual BaseBeanSharedPointer beanToBeEdited(int row);
    virtual BaseBeanSharedPointer beanToBeEdited(const QModelIndex &index);
    virtual BaseBeanSharedPointer lastInsertedBean() const;
    virtual void setLastInsertedBean(BaseBeanSharedPointer bean);

    virtual QString where() const;
    virtual void setWhere(const QString &where, const QString &order = "");
    virtual void setInternalOrderClausule(const QString &order, bool refresh = true);
    virtual QString internalOrderClausule() const;

    virtual QModelIndex indexByPk(const QVariant &value) = 0;

    virtual bool canShowCheckBoxes() const;
    virtual void setCanShowCheckBoxes(bool value);
    virtual QModelIndexList checkedItems(const QModelIndex &idx = QModelIndex());
    virtual void setCheckedItems(const QModelIndexList &list, bool checked = true);
    virtual void setCheckedItem(const QModelIndex &index, bool checked = true);
    virtual void setCheckedItem(int row, bool checked = true);
    virtual void checkAllItems(bool checked = true);
    virtual void setCheckColumns(const QStringList &fieldNames);
    virtual void setCheckColumn(int index);
    virtual QStringList checkColumns() const;

    virtual QStringList readOnlyColumns() const;
    virtual void setReadOnlyColumns(const QStringList &list);

    virtual QStringList visibleFields() const;
    virtual void setVisibleFields(const QStringList &list);
    virtual QList<DBFieldMetadata *> visibleFieldsMetadata() const;
    virtual bool visibleFieldsFromMetadata() const;

    virtual bool isFrozenModel() const;
    virtual void freezeModel();
    virtual void defrostModel();

    virtual bool isLoadingData() const;

    QChar wildCard() const;
    void setWildCard(const QChar &value);

    QStringList linkColumns() const;
    void setLinkColumns(const QStringList &value);
    virtual bool isLinkColumn(int column) const;

    virtual QMimeData *mimeData(const QModelIndexList &indexes) const;

    bool exportToSpreadSheet(QAbstractItemModel *model, BaseBeanMetadata *m, const QString &file, const QString &type);

    virtual int totalRecordCount() const;

    virtual BaseBeanPointerList ancestorsBeans(const QModelIndex &idx);

    virtual bool columnIsFunction(int column) const;
    virtual DBFieldMetadata *functionMetadata(int column) const;

    virtual void removeInsertedRows(const QModelIndex &parent = QModelIndex());

    virtual bool canEmitDataChanged() const;
    virtual bool setCanEmitDataChanged(bool value);

    QString contextName() const;

signals:
    /** Esta señal será útil para saber en qué momento el modelo entra en un estado de carga de datos,
     * por ejemplo interaccionando con la base de datos, de manera que se presente al usuario una ventana
     * de carga */
    void initLoadingData();
    /** Finalización del proceso de carga iniciado antes */
    void endLoadingData();
    /** Esta señal nos indicará si se ha producido una carga de registros (puede ser necesario para actualizar los filtros) */
    void beansLoadFinished();
    void rowProcessed(int row);
    void initRefresh();
    void endRefresh();
    void itemChecked(QModelIndex,bool);

public slots:
    virtual void refresh(bool force = false) = 0;
    /** No utilizamos el método submit de Qt a posta. Este método se invoca desde diferentes puntos... y no hay mucho
     * control sobre quién invoca y porqué. Es mejor que el código sepa qué hace cuando quiera pasar los cambios a base de datos
     * */
    virtual bool commit();
    /** Misma explicación para el método revert */
    virtual void rollback();
    virtual bool exportToSpreadSheet(const QString &file, const QString &type);
    virtual void cancelExportToSpreadSheet();

protected slots:
    virtual void stopReloading();
    virtual void startReloading();
};

#endif // BASEBEANMODEL_H
