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
#ifndef DBRELATEDELEMENTSVIEW_H
#define DBRELATEDELEMENTSVIEW_H

#include <QtCore>
#include <QtScript>
#include <alepherpglobal.h>
#include "widgets/dbbasewidget.h"
#include "widgets/dbabstractview.h"

namespace Ui
{
class DBRelatedElementsView;
}

class DBRelatedElementsViewPrivate;

/**
 * @brief The DBRelatedElementsView class
 * Esta clase presentará un conjunto de elementos relacionados, de tipo AlephERP::Record.
 * Su funcionamiento será parecido al del DBDetailView pero con elementos relacionados.
 */
class ALEPHERP_DLL_EXPORT DBRelatedElementsView : public QWidget, public DBAbstractViewInterface, public QScriptable
{
    Q_OBJECT
    Q_FLAGS(Buttons)
    Q_ENUMS(Button)
    Q_ENUMS(CategoriesRule)
    Q_DISABLE_COPY(DBRelatedElementsView)

    /** Indicará, separados por "," qué tipo de registros pueden anexarse o relacionarse con el bean del formulario
     * en el que está contenido este control */
    Q_PROPERTY(QString allowedMetadatas READ allowedMetadatas WRITE setAllowedMetadatas)
    /** Categorías que se presentan */
    Q_PROPERTY(QString categories READ categories WRITE setCategories)
    /** Regla de inclusión de las categorías: Se muestran los registros que cumplan todas las categorías, una de ellas
     * o los que no cumplan ninguna */
    Q_PROPERTY (DBRelatedElementsView::CategoriesRule categoriesRule READ categoriesRule WRITE setCategoriesRule)
    Q_PROPERTY (bool dataEditable READ dataEditable WRITE setDataEditable)
    Q_PROPERTY (bool aerpControl READ aerpControl)
    Q_PROPERTY (bool userModified READ userModified WRITE setUserModified)
    Q_PROPERTY (DBRelatedElementsView::Buttons visibleButtons READ visibleButtons WRITE setVisibleButtons)
    Q_PROPERTY (QAbstractItemView::EditTriggers editTriggers READ editTriggers WRITE setEditTriggers)
    /** Separado por ; los nombres de los dbFieldName que quiere que aparezcan en la tabla relacionada. En caso de estar vacío
     * aparecerán 3 columnas: Categoría, Nombre de la tabla y "propiedad toString" de los elementos relacionados */
    Q_PROPERTY (QString visibleColumns READ visibleColumns WRITE setVisibleColumns)
    /** Por defecto, siempre se visualizará los elementos relacionados del registro que se está editando actualmente en el formulario
     * en el que está incluído este control. Sin embargo, puede ser interesante, visualizar los elementos relacionados de los
     * registros de una relación de este bean. Aquí se puede especificar esa ruta de navegación */
    Q_PROPERTY (QString relationName READ relationName WRITE setRelationName)
    /** Tipo de elementos relacionados que se muestran */
    Q_PROPERTY (AlephERP::RelatedElementCardinalities cardinality READ cardinality WRITE setCardinality)
    /** Los fields que se presenten en esta propiedad, se presentarán como un "enlace", como un link, que abrirá directamente
     * el registro, en modo lectura o edición, dependiendo de los botones activos. */
    Q_PROPERTY (QString linkColumns READ linkColumns WRITE setLinkColumns)
    /** Si el control tiene asignadas varias categorías, ¿se pregunta al usuario por la que quiere agregar o se le agregan todas? */
    Q_PROPERTY (bool askForCategories READ askForCategories WRITE setAskForCategories)
    Q_PROPERTY (QString beforeAddScript READ beforeAddScript WRITE setBeforeAddScript)
    Q_PROPERTY (QString afterAddScript READ afterAddScript WRITE setAfterAddScript)
    Q_PROPERTY (QString beforeAddExistingScript READ beforeAddExistingScript WRITE setBeforeAddExistingScript)
    Q_PROPERTY (QString afterAddExistingScript READ afterAddExistingScript WRITE setAfterAddExistingScript)
    Q_PROPERTY (QString beforeEditScript READ beforeEditScript WRITE setBeforeEditScript)
    Q_PROPERTY (QString afterEditScript READ afterEditScript WRITE setAfterEditScript)
    Q_PROPERTY (QString beforeDeleteScript READ beforeDeleteScript WRITE setBeforeDeleteScript)
    Q_PROPERTY (QString afterDeleteScript READ afterDeleteScript WRITE setAfterDeleteScript)
    Q_PROPERTY (QString beforeRemoveExistingScript READ beforeRemoveExistingScript WRITE setBeforeRemoveExistingScript)
    Q_PROPERTY (QString afterRemoveExistingScript READ afterRemoveExistingScript WRITE setAfterRemoveExistingScript)
    Q_PROPERTY (bool useNewContext READ useNewContext WRITE setUseNewContext)
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
    Q_PROPERTY (QStringList _visibleForUsers READ visibleForUsers WRITE setVisibleForUsers)
    /** El widget estará habilitado, sólo si esta propiedad está vacía o para los usuarios aquí presentes */
    Q_PROPERTY (QStringList dataEditableForUsers READ dataEditableForUsers WRITE setDataEditableForUsers)

    friend class DBRelatedElementsViewPrivate;

private:
    Ui::DBRelatedElementsView *ui;
    DBRelatedElementsViewPrivate *d;

protected:
    void showEvent(QShowEvent *event);

public:
    explicit DBRelatedElementsView(QWidget *parent = 0);
    virtual ~DBRelatedElementsView();

    /** Botones que serán visibles */
    enum Button { Insert = 0x01, Update = 0x02, Delete = 0x04, View = 0x08, InsertExisting = 0x10, RemoveExisting = 0x20 };
    Q_DECLARE_FLAGS(Buttons, Button)

    enum CategoriesRule { AllOfThem = 0x01, OneOfThem = 0x02 };

    QString allowedMetadatas() const;
    void setAllowedMetadatas(const QString &value);
    QString categories() const;
    void setCategories(const QString &value);
    CategoriesRule categoriesRule() const;
    void setCategoriesRule(CategoriesRule rule);
    bool aerpControl()
    {
        return true;
    }
    Buttons visibleButtons();
    void setVisibleButtons(Buttons buttons);
    virtual QAbstractItemView::EditTriggers editTriggers () const;
    virtual void setEditTriggers (QAbstractItemView::EditTriggers triggers);
    QString visibleColumns() const;
    void setVisibleColumns(const QString &value);
    AlephERP::RelatedElementCardinalities cardinality() const;
    void setCardinality(AlephERP::RelatedElementCardinalities value);
    QString linkColumns() const;
    void setLinkColumns(const QString &value);
    bool askForCategories() const;
    void setAskForCategories(bool value);
    virtual bool dataEditable();

    QString beforeAddScript() const;
    void setBeforeAddScript(const QString &value);
    QString afterAddScript() const;
    void setAfterAddScript(const QString &value);
    QString beforeAddExistingScript() const;
    void setBeforeAddExistingScript(const QString &value);
    QString afterAddExistingScript() const;
    void setAfterAddExistingScript(const QString &value);
    QString beforeEditScript() const;
    void setBeforeEditScript(const QString &value);
    QString afterEditScript() const;
    void setAfterEditScript(const QString &value);
    QString beforeDeleteScript() const;
    void setBeforeDeleteScript(const QString &value);
    QString afterDeleteScript() const;
    void setAfterDeleteScript(const QString &value);
    QString beforeRemoveExistingScript() const;
    void setBeforeRemoveExistingScript(const QString &value);
    QString afterRemoveExistingScript() const;
    void setAfterRemoveExistingScript(const QString &value);
    bool useNewContext() const;
    void setUseNewContext(bool value);

    AlephERP::ObserverType observerType(BaseBean *)
    {
        return AlephERP::DbMultipleRelation;
    }
    virtual void setValue(const QVariant &value);
    virtual QVariant value();
    virtual void applyFieldProperties();
    virtual void refresh();

    virtual QScriptValue checkedBeans();
    virtual void setCheckedBeans(BaseBeanSharedPointerList list, bool checked = true);
    virtual void setCheckedBeansByPk(QVariantList list, bool checked = true);

    virtual void orderColumns(const QStringList &order);
    virtual void sortByColumn(const QString &field, Qt::SortOrder order = Qt::DescendingOrder);
    virtual void saveColumnsOrder(const QStringList &order, const QStringList &sort);

    QString askForMetadata(bool *cancel);
    QStringList askForCategory(bool *cancel);

signals:
    /** Esta señal indicará cuándo se borra un widget. No se puede usar destroyed(QObject *)
      ya que cuando ésta se emite, se ha ejecutado ya el destructor de QWidget */
    void destroyed(QWidget *widget);
    void valueEdited(const QVariant &value);

private slots:
    void relationChildBeanInserted(BaseBean *bean, int index);
    void relationChildBeanDeleted(BaseBean *bean, int index);
    void itemClicked(const QModelIndex &idx);

public slots:
    void addRelatedElement();
    void addExisting();
    void deleteRelatedElement();
    void editRelatedElement();
    void viewRelatedElement();
    void removeExisting();
    void setFocus() { QWidget::setFocus(); }

};

Q_DECLARE_METATYPE(DBRelatedElementsView*)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBRelatedElementsView, DBRelatedElementsView*)

#endif // DBRELATEDELEMENTSVIEW_H
