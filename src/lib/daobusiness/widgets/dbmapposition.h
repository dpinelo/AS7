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
#ifndef DBMAPPOSITION_H
#define DBMAPPOSITION_H

#include <QWidget>
#include <alepherpglobal.h>
#include "widgets/dbbasewidget.h"

namespace Ui {
class DBMapPosition;
}

class DBMapPositionPrivate;

/**
 * @brief The DBMapPosition class
 * Control que permite geolocalizar información en un mapa.
 */
class ALEPHERP_DLL_EXPORT DBMapPosition : public QWidget, public DBBaseWidget
{
    Q_OBJECT
    Q_PROPERTY (QString fieldName READ fieldName WRITE setFieldName)
    Q_PROPERTY (QString relationFilter READ relationFilter WRITE setRelationFilter)
    Q_PROPERTY (bool dataEditable READ dataEditable WRITE setDataEditable)
    Q_PROPERTY (bool aerpControl READ aerpControl)
    Q_PROPERTY (bool userModified READ userModified WRITE setUserModified)
    Q_PROPERTY (QVariant value READ value WRITE setValue)
    /** ¿Puede el usuario realizar búsquedas? */
    Q_PROPERTY (bool userCanSearch READ userCanSearch WRITE setUserCanSearch)
    /** Coordenadas iniciales donde se iniciará el mapa */
    Q_PROPERTY (QString initCoordinates READ initCoordinates WRITE setInitCoordinates)
    /** Nivel de zoom con el que se iniciará el mapa */
    Q_PROPERTY (int initZoom READ initZoom WRITE setInitZoom)
    /** Título identificativo o nombre de la marca principal, la que se almacena o accede con "value" */
    Q_PROPERTY (QString title READ title WRITE setTitle)
    /** Ventana informativa que puede mostrarse de la marca principal. HTML a mostrar */
    Q_PROPERTY (QString infoWindow READ infoWindow WRITE setInfoWindow)
    /** Coordenadas de la marca principal. Es igual que value */
    Q_PROPERTY (QString coordinates READ coordinates WRITE setCoordinates)
    /** Latitude de la marca principal */
    Q_PROPERTY (double latitude READ latitude WRITE setLatitude)
    /** Longitud de la marca principal */
    Q_PROPERTY (double longitude READ longitude WRITE setLongitude)
    /** Dirección formateada según el motor de búsqueda */
    Q_PROPERTY (QString engineAddress READ engineAddress WRITE setEngineAddress)
    /** Motor de búsqueda a utilizar. Google, OpenStreetMap... */
    Q_PROPERTY (QString engine READ engine WRITE setEngine)
    /** Script a ejecutar cuando cambian las coordenadas de la marca principal */
    Q_PROPERTY (QString scriptAfterChange READ scriptAfterChange WRITE setScriptAfterChange)
    Q_PROPERTY (bool showInfo READ showInfo WRITE setShowInfo)

    friend class DBMapPositionPrivate;

private:
    Ui::DBMapPosition *ui;
    DBMapPositionPrivate *d;

protected:
    void showEvent(QShowEvent *);

public:
    explicit DBMapPosition(QWidget *parent = 0);
    ~DBMapPosition();

    bool userCanSearch() const;
    void setUserCanSearch(bool value);
    QString initCoordinates() const;
    void setInitCoordinates(const QString &value);
    int initZoom() const;
    void setInitZoom(int value);
    QString title() const;
    void setTitle(const QString &value);
    QString infoWindow() const;
    void setInfoWindow(const QString &value);
    QString coordinates() const;
    bool setCoordinates(const QString &value);
    double latitude() const;
    void setLatitude(double value);
    double longitude() const;
    void setLongitude(double value);
    QString engineAddress () const;
    void setEngineAddress(const QString &value);
    QString engine() const;
    void setEngine(const QString &value);
    QString scriptAfterChange() const;
    void setScriptAfterChange(const QString &value);
    bool showInfo() const;
    void setShowInfo(bool value);

    AlephERP::ObserverType observerType(BaseBean *)
    {
        return AlephERP::DbField;
    }

    virtual QVariant value();
    virtual void applyFieldProperties();

public slots:
    virtual void setValue(const QVariant &value);
    void refresh();
    void setFocus()
    {
        QWidget::setFocus();
    }
    virtual void observerUnregistered();

    void addMark(const QString &coords, const QString &caption ="", const QString &infoWindow = "");
    void addMark(double latitude, double longitude, const QString &caption = "", const QString &infoWindow = "");
    void removeMark(const QString &caption);
    void clearMarks();
    void showCoordinates(double latitude, double longitude);
    void fitToShowAllMarkers();
    QString engineValue(const QString &engineKey);
    QString engineValue(const QString &markName, const QString &engineKey);
    bool coordsEquals(const QString &coords1, const QString &coords2);

    void showOrHideSearch();

#ifdef ALEPHERP_DEVTOOLS
    void showInspector();
#endif

protected slots:
    void searchCoords();
    void searchAddress();
    void newValueFromUserDrag(const QString &markName);
    void callQs(const QString &markName, const QString &coords);
    void coordinatesReady(const QString &uuid, const QList<AlephERP::AERPMapPosition> &data);
    void addressReady(const QString &uuid, const AlephERP::AERPMapPosition &data);
    void geocodeTimeout();
    void geocodeError(const QString &uuid, QString error);
    void mapWasInit(bool value);

signals:
    void valueEdited(const QVariant &value);
    void destroyed(QWidget *widget);
    void userDragMark(QString markName, QString coordinate);
};

/**
 * @brief The DBMapPositionJSObject class
 * Este objeto nos permitirá obtener algunas interacciones del usuario en el mapa. Por ejemplo, pillar la dirección exacta
 */
class DBMapPositionJSObject : public QObject
{
    Q_OBJECT

private:
    QHash<QString, AlephERP::AERPMapPosition> m_data;

public:
    DBMapPositionJSObject(QObject *parent);
    virtual ~DBMapPositionJSObject();

public slots:
    void userDragInit(const QString &markName);
    void userDragEnd(const QString &markName);
    void userMovesMark(const QString &markName, const QString &coordinates);
    void userMovesMarkFormattedAddress(const QString &markName, const QString &address);
    void userMovesMarkAddressComponent(const QString &markName, const QString &type, const QString &value);
    AlephERP::AERPMapPosition data(const QString &markName);

signals:
    void newMarkPosition(const QString &markName);
};

#endif // DBMAPPOSITION_H
