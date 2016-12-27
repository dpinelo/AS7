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
#ifndef TREEVIEWMODEL_H
#define TREEVIEWMODEL_H

#include <QtCore>
#include "treeitem.h"
#include "models/basebeanmodel.h"

class BaseBean;
class TreeViewModelPrivate;

/**
    @author David Pinelo <alepherp@alephsistemas.es>
*/
class ALEPHERP_DLL_EXPORT TreeViewModel : public BaseBeanModel
{
    Q_OBJECT

protected:
    /** Item raiz. Borrándolo, se borrarán todos los items que dependen y cuelgan de él */
    TreeItem *m_rootItem;
    /** Lista con las imágenes a mostrar, si las hubiera. Se agrupan como lista. El orden en la lista
      se corresponde con el nivel del item en el árbol. Además, tienen la siguiente terminología:
      file:/home/david ... lee la imagen del sistema de ficherso
      field:imagen ... lee la imagen de la base de datos, de la columna del mismo nombnre
      */
    QStringList m_images;
    /** Tamaño de las imágenes a mostrar */
    QSize m_size;
    /** Cuando se marca el check de un item que tiene hijos, esta opción marca si se maracarán también
      todos los hijos */
    bool m_checkFatherCheckChildren;

    virtual void checkAllChildren(const QModelIndex &parent, int state);
    virtual bool isPartiallyChecked(const QModelIndex &parent) const;

    virtual void setupInitialData() = 0;

public:
    explicit TreeViewModel(QObject *parent = 0);
    virtual ~TreeViewModel();

    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &index) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual bool hasChildren(const QModelIndex & parent = QModelIndex() ) const;
    virtual void fetchMore(const QModelIndex &parent) = 0;
    virtual bool canFetchMore(const QModelIndex &parent) const = 0;

    virtual void setImages(const QStringList &img);
    void setSize(const QSize &value);

    virtual TreeItem *item(const QModelIndex &idx);
    virtual BaseBeanMetadata * metadata() const = 0;
    virtual QModelIndex indexByPk(const QVariant &value);

    virtual void setCheckFatherCheckChildrens(bool value);
    virtual bool checkFatherCheckChildrens();
    virtual QModelIndexList checkedItems(const QModelIndex &idx = QModelIndex());
    virtual void setCheckedItems(const QModelIndexList &list, bool checked = true);
    virtual void setCheckedItem(const QModelIndex &idx, bool checked = true);
    virtual void setCheckedItem(int row, bool checked = true);
    virtual void checkAllItems(bool checked = true);

public slots:
    virtual void refresh(bool force = false);
};

#endif
