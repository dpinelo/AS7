/***************************************************************************
 *   Copyright (C) 2014 by David Pinelo   *
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
#ifndef DBSCHEDULEDVIEW_H
#define DBSCHEDULEDVIEW_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include "widgets/aerpscheduleview/aerpscheduleview.h"
#include "dbabstractview.h"
#include <alepherpglobal.h>

class DBScheduleViewPrivate;

/**
 * Visualización de registros en un calendario.
 * @brief The DBScheduledView class
 */
class ALEPHERP_DLL_EXPORT DBScheduleView : public AERPScheduleView, public DBAbstractViewInterface, public QScriptable
{
    Q_OBJECT

    /** ¿Se leen los datos de base de datos o del Bean del formulario que contiene este control? */
    Q_PROPERTY(bool internalData READ internalData WRITE setInternalData)

    /** Si los datos se leen de base de datos, entonces se introduce este tableName */
    Q_PROPERTY (QString tableName READ tableName WRITE setTableName DESIGNABLE externalDataPropertyVisible)
    /** Este filtro aplica a los datos que se leen de base de datos */
    Q_PROPERTY (QString filter READ filter WRITE setFilter DESIGNABLE externalDataPropertyVisible)
    /** Orden inicial con el que se visualizarán las filas */
    Q_PROPERTY (QString order READ order WRITE setOrder DESIGNABLE externalDataPropertyVisible)
    /** Es posible definir una columna de ordenación. El usuario podrá entonces mover las filas, y cuando esto ocurra
     * se actualizará el valor de las columnas de ordenación de los diferentes registros. Si esta propiedad está
     * establecida, se obvia el valor de \a order. La columna de ordenación se define en los metadatos */
    Q_PROPERTY (bool canMoveItems READ canMoveItems WRITE setCanMoveItems)

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

    /** Por defecto, los DBDetailView tienen un nombre generado automáticamente. Aquí se puede
      habilitar o desactivar esa característica */
    Q_PROPERTY (bool automaticName READ automaticName WRITE setAutomaticName)
    /** Estados que serán visibles */
    Q_PROPERTY (AlephERP::DBRecordStates visibleRecords READ visibleRecords WRITE setVisibleRecords)
    /** ¿Es posible abrir registros con enlaces en modo edición? */
    Q_PROPERTY (bool allowedEdit READ allowedEdit WRITE setAllowedEdit)

    /** Precisión máxima con el que pueden darse de alta registros */
    Q_PROPERTY (int stepInMinutes READ stepInMinutes WRITE setStepInMinutes)
    /** Zoom del eje de tiempos */
    Q_PROPERTY (int currentZoomDepthInMinutes READ currentZoomDepthInMinutes WRITE setCurrentZoomDepthInMinutes)
    /** Inicio de visualización */
    Q_PROPERTY (QDate initRange READ initRange WRITE setInitRange)
    Q_PROPERTY (QString htmlTodayCellColor READ htmlTodayCellColor WRITE setHtmlTodayCellColor)

private:
    DBScheduleViewPrivate *d;

    bool setupInternalModel();
    bool setupExternalModel();

public:
    explicit DBScheduleView(QWidget *parent = 0);
    virtual ~DBScheduleView();

    bool canMoveItems() const;
    void setCanMoveItems(bool value);
    int stepInMinutes() const;
    void setStepInMinutes(int value);
    int currentZoomDepthInMinutes() const;
    void setCurrentZoomDepthInMinutes(int value);
    QDate initRange() const;
    void setInitRange(const QDate &value);

    virtual void setViewMode(const AERPScheduleView::ViewMode mode);

    virtual QScriptValue checkedBeans();
    virtual void setCheckedBeans(BaseBeanSharedPointerList list, bool checked = true);
    virtual void setCheckedBeansByPk(QVariantList list, bool checked = true);

    virtual void orderColumns(const QStringList &order);
    virtual void sortByColumn(const QString &field, Qt::SortOrder order = Qt::DescendingOrder);
    virtual void saveColumnsOrder(const QStringList &order, const QStringList &sort);

    virtual QVariant value();
    virtual void applyFieldProperties();

    virtual void setModel(QAbstractItemModel *model);

protected:
    virtual void showEvent (QShowEvent * event);
    virtual void keyPressEvent (QKeyEvent * event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseDoubleClickEvent (QMouseEvent * event);

signals:
    void valueEdited(const QVariant &value);
    void destroyed(QWidget *widget);
    void enterPressedOnValidIndex(QModelIndex index);
    void doubleClickOnValidIndex(QModelIndex index);

public slots:
    void setRelationName(const QString &name);
    void setTableName(const QString &name);
    virtual void setValue(const QVariant &value);
    virtual void refresh();
    void itemClicked(const QModelIndex &idx);
    virtual void nextView();
    virtual void previousView();
    virtual void adjustToVisualSize();
    virtual void setAdjustToVisualSize(bool value);
    void setFocus() { QWidget::setFocus(); }
};

Q_DECLARE_METATYPE(DBScheduleView*)

#endif // DBSCHEDULEDVIEW_H
