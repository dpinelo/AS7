/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo                                    *
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
#ifndef RELATEDELEMENTSRECORDMODEL_H
#define RELATEDELEMENTSRECORDMODEL_H

#include "models/basebeanmodel.h"

class RelatedElementsRecordModelPrivate;

/**
 * @brief The RelatedElementsRecordModel class
 * Este modelo presenta los elementos relacionados pero sólo de tipo "registros de otras tablas".
 * Por eso, hereda de BaseBeanModel
 */
class RelatedElementsRecordModel : public BaseBeanModel
{
    Q_OBJECT
    Q_DISABLE_COPY(RelatedElementsRecordModel)
    Q_DECLARE_PRIVATE(RelatedElementsRecordModel)

    /** Esta propiedad nos chivará si el modelo está basado en BaseBean, con lo cual
      podrá obtener, a través de la propiedad tableName, el bean base que editan los modelos */
    Q_PROPERTY (bool baseBeanModel READ baseBeanModel)
    /** Los registros que se presentan pueden filtrarse por las tablas que se presentan */
    Q_PROPERTY (QStringList allowedMetadatas READ allowedMetadatas WRITE setAllowedMetadatas)
    Q_PROPERTY (QStringList categories READ categories WRITE setCategories)
    Q_PROPERTY (QStringList visibleFields READ visibleFields WRITE setVisibleFields)

public:
    enum CategoriesRule { AllOfThem = 0x01, OneOfThem = 0x02 };

private:
    RelatedElementsRecordModelPrivate *d;

public:
    explicit RelatedElementsRecordModel(BaseBean *bean, AlephERP::RelatedElementCardinalities cardinality,
                                        const QStringList &allowedMetadatas, const QStringList &categories,
                                        CategoriesRule categoriesRules, QObject *parent = 0);
    explicit RelatedElementsRecordModel(BaseBeanPointerList bean, AlephERP::RelatedElementCardinalities cardinality,
                                        const QStringList &allowedMetadatas, const QStringList &categories,
                                        CategoriesRule categoriesRules, QObject *parent = 0);
    ~RelatedElementsRecordModel();

    void addRootBean(BaseBean *bean);
    void removeRootBean(BaseBean *bean);
    bool baseBeanModel()
    {
        return true;
    }
    QStringList allowedMetadatas() const;
    void setAllowedMetadatas(const QStringList &value);
    QStringList categories() const;
    void setCategories(const QStringList &value);
    CategoriesRule categoriesRule() const;
    void setCategoriesRule(CategoriesRule rule);

    QStringList visibleFields() const;
    void setVisibleFields(const QStringList &value);
    void removeRelatedElement(RelatedElement *element);

    // Funciones virtuales de QStandardItemModel que deben ser implementadas
    int	rowCount (const QModelIndex & parent = QModelIndex()) const;
    int columnCount (const QModelIndex & parent = QModelIndex()) const;
    QVariant headerData (int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags (const QModelIndex & index) const;
    QModelIndex index (int row, int column, const QModelIndex & parent = QModelIndex()) const;
    QVariant data (const QModelIndex & item, int role) const;
    bool removeRows (int row, int count, const QModelIndex & parent = QModelIndex());
    RelatedElement *insertRow (const QString &tableName, const QStringList &category, const BaseBeanPointer &bean = NULL, bool takeOwnerShip = true);
    QModelIndex parent(const QModelIndex &child) const;

    // Estas funciones ya son propias
    BaseBeanSharedPointer bean (const QModelIndex &index, bool reloadIfNeeded = true) const;
    BaseBeanSharedPointer bean (int row, bool reloadIfNeeded = true) const;
    BaseBeanSharedPointerList beans(const QModelIndexList &list);
    RelatedElement *element (int row);
    RelatedElement *element (const QModelIndex &index);

    BaseBeanMetadata * metadata() const;

    QModelIndex indexByPk(const QVariant &value);

    bool isLinkColumn(int section) const;

signals:

private slots:
    void relatedElementAdded(RelatedElement *element);
    void relatedElementModified(RelatedElement *element);
    void relatedElementModified();
    void relatedElementDeleted(RelatedElement *element);

public slots:
    void refresh(bool force = false);
};

#endif // RELATEDELEMENTSRECORDMODEL_H
