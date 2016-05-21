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
#include "dbmapposition.h"
#include "ui_dbmapposition.h"
#include "models/envvars.h"
#include "business/aerpgeocodedatamanager.h"
#include "forms/perpbasedialog.h"
#include "globales.h"
#include <QtWebEngineWidgets>
#include <cmath>

struct MapMarker
{
    double latitude;
    double longitude;
    QString caption;
    QString infoWindow;

    MapMarker()
    {
        latitude = 0;
        longitude = 0;
        caption = "";
        infoWindow = "";
    }

    MapMarker(double lat, double lng, QString text, QString info)
    {
        latitude = lat;
        longitude = lng;
        caption = text;
        infoWindow = info;
    }
};

class DBMapPositionPrivate
{
public:
    QPointer<QProgressBar> m_progressBar;
    QPointer<DBMapPosition> q_ptr;
    QPointer<DBMapPositionJSObject> m_bindingObject;
    bool m_userCanSearch;
    double m_latitude;
    double m_longitude;
    double m_initLatitude;
    double m_initLongitude;
    QString m_licenseKey;
    int m_initZoom;
    bool m_mapInited;
    bool m_fitToShowAllMarkersPending;
    QList<MapMarker *> m_markers;
    QString m_title;
    QString m_infoWindow;
    QString m_engineAddress;
    QString m_engine;
    QString m_scriptAfterChange;
    QHash< QString, QHash<QString, QString> > m_engineValues;
    bool m_showInfo;

    QPointer<AERPGeocodeDataManager> m_geoCoder;
    QHash<QString, QString> m_geocoderError;
    QHash<QString, bool> m_geocoderSuccess;
    QHash<QString, QList<AlephERP::AERPMapPosition> > m_geocoderLastSearch;
    bool m_geocoderTimeout;

    DBMapPositionPrivate (DBMapPosition *qq) : q_ptr(qq)
    {
        m_geoCoder = new AERPGeocodeDataManager(q_ptr);
        m_bindingObject = new DBMapPositionJSObject(q_ptr);
        m_userCanSearch = false;
        m_latitude = 0;
        m_longitude = 0;
        m_initLatitude = 0;
        m_initLongitude = 0;
        m_initZoom = 8;
        m_mapInited = false;
        m_title = QObject::trUtf8("Ubicación");
        m_engine = "Google";
        m_geocoderTimeout = false;
        m_fitToShowAllMarkersPending = false;
        m_showInfo = true;
    }

    void initMap();
    void showCoordinates(double latitude, double longitude);
    void showMainMark();
    void removeMark(MapMarker *mark);
    void addMark(MapMarker *mark);
};

void DBMapPosition::showEvent(QShowEvent *ev)
{
    if ( !d->m_mapInited )
    {
        d->initMap();
    }
    DBBaseWidget::showEvent(ev);
    refresh();
    if ( observer() == NULL )
    {
        setShowInfo(false);
    }
}

DBMapPosition::DBMapPosition(QWidget *parent) :
    QWidget(parent), DBBaseWidget(),
    ui(new Ui::DBMapPosition), d(new DBMapPositionPrivate(this))
{
    ui->setupUi(this);

    d->m_licenseKey = EnvVars::instance()->var(AlephERP::stGoogleMapsApiKey).toString();
    if ( d->m_licenseKey.isEmpty() )
    {
        ui->cbSearch->setVisible(false);
    }
    ui->gbSearch->setVisible(false);
    connect (d->m_geoCoder.data(), SIGNAL(coordinatesReady(QString,QList<AlephERP::AERPMapPosition>)), this, SLOT(coordinatesReady(QString,QList<AlephERP::AERPMapPosition>)));
    connect (d->m_geoCoder.data(), SIGNAL(addressReady(QString,AlephERP::AERPMapPosition)), this, SLOT(addressReady(QString,AlephERP::AERPMapPosition)));
    connect (d->m_geoCoder.data(), SIGNAL(errorOccured(QString,QString)), this, SLOT(geocodeError(QString,QString)));
    connect (ui->pbShowSearch, SIGNAL(clicked()), this, SLOT(showOrHideSearch()));
    connect (ui->pbSearch, SIGNAL(clicked()), this, SLOT(searchCoords()));
    connect (ui->pbLocate, SIGNAL(clicked()), this, SLOT(searchAddress()));
    // Eventos que se reciben del web view
    connect (d->m_bindingObject.data(), SIGNAL(newMarkPosition(QString)), this, SLOT(newValueFromUserDrag(QString)));
    connect (ui->webView, SIGNAL(loadFinished(bool)), this, SLOT(mapWasInit(bool)));
}

DBMapPosition::~DBMapPosition()
{
    qDeleteAll (d->m_markers);
    delete ui;
    delete d;
}

bool DBMapPosition::userCanSearch() const
{
    return d->m_userCanSearch;
}

void DBMapPosition::setUserCanSearch(bool value)
{
    d->m_userCanSearch = value;
}

QString DBMapPosition::initCoordinates() const
{
    return QString("%1,%2").arg(d->m_initLatitude).arg(d->m_initLongitude);
}

void DBMapPosition::setInitCoordinates(const QString &value)
{
    QStringList parts = value.split(",");
    if ( parts.size() != 2 )
    {
        return;
    }
    d->m_initLatitude = parts.at(0).toDouble();
    d->m_initLongitude = parts.at(1).toDouble();
}

int DBMapPosition::initZoom() const
{
    return d->m_initZoom;
}

void DBMapPosition::setInitZoom(int value)
{
    d->m_initZoom = value;
}

QString DBMapPosition::title() const
{
    return d->m_title;
}

void DBMapPosition::setTitle(const QString &value)
{
    d->m_title = value;
}

QString DBMapPosition::infoWindow() const
{
    return d->m_infoWindow;
}

void DBMapPosition::setInfoWindow(const QString &value)
{
    d->m_infoWindow = value;
}

QString DBMapPosition::coordinates() const
{
    return QString("%1,%2").arg(d->m_latitude).arg(d->m_longitude);
}

bool DBMapPosition::setCoordinates(const QString &value)
{
    ui->leLatitude->clear();
    ui->leLongitude->clear();
    QStringList parts = value.split(",");
    if ( parts.size() != 2 )
    {
        return false;
    }
    bool ok;
    d->m_latitude = parts.at(0).trimmed().toDouble(&ok);
    if ( !ok )
    {
        return false;
    }
    d->m_longitude = parts.at(1).trimmed().toDouble(&ok);
    if ( !ok )
    {
        return false;
    }
    ui->leLatitude->setText(QString("%1").arg(d->m_latitude));
    ui->leLongitude->setText(QString("%1").arg(d->m_longitude));
    return true;
}

double DBMapPosition::latitude() const
{
    return d->m_latitude;
}

void DBMapPosition::setLatitude(double value)
{
    d->m_latitude = value;
    d->showMainMark();
}

double DBMapPosition::longitude() const
{
    return d->m_longitude;
}

void DBMapPosition::setLongitude(double value)
{
    d->m_longitude = value;
    d->showMainMark();
}

QString DBMapPosition::engineAddress() const
{
    return d->m_engineAddress;
}

void DBMapPosition::setEngineAddress(const QString &value)
{
    d->m_engineAddress = value;
}

QString DBMapPosition::engine() const
{
    return d->m_engine;
}

void DBMapPosition::setEngine(const QString &value)
{
    d->m_engine = value;
    d->m_geoCoder->setServer(d->m_engine);
}

QString DBMapPosition::scriptAfterChange() const
{
    return d->m_scriptAfterChange;
}

void DBMapPosition::setScriptAfterChange(const QString &value)
{
    d->m_scriptAfterChange = value;
}

bool DBMapPosition::showInfo() const
{
    return d->m_showInfo;
}

void DBMapPosition::setShowInfo(bool value)
{
    d->m_showInfo = value;
    ui->gbInfo->setVisible(d->m_showInfo);
}

void DBMapPosition::setValue(const QVariant &value)
{
    if ( ! coordsEquals(coordinates(), value.toString()) )
    {
        if ( setCoordinates(value.toString()) )
        {
            d->showMainMark();
        }
    }
}

QVariant DBMapPosition::value()
{
    return QString("%1,%2").arg(d->m_latitude).arg(d->m_longitude);
}

void DBMapPosition::applyFieldProperties()
{
    if ( !dataEditable() )
    {
        ui->pbShowSearch->setVisible(false);
        ui->gbSearch->setVisible(false);
        if ( qApp->focusWidget() == this )
        {
            focusNextChild();
        }
    }
}

void DBMapPosition::addMark(const QString &coords, const QString &caption, const QString &infoWindow)
{
    QStringList parts = coords.split(",");
    if ( parts.size() != 2 )
    {
        return;
    }
    bool ok;
    double latitude = parts.at(0).trimmed().toDouble(&ok);
    if ( !ok )
    {
        return;
    }
    double longitude = parts.at(1).trimmed().toDouble(&ok);
    if ( !ok )
    {
        return;
    }
    addMark(latitude, longitude, caption, infoWindow);
}

void DBMapPosition::addMark(double latitude, double longitude, const QString &caption, const QString &infoWindow)
{
    removeMark(caption);

    MapMarker *mark = new MapMarker(latitude, longitude, caption, infoWindow);
    d->m_markers.append(mark);

    if ( d->m_mapInited )
    {
        d->addMark(mark);
    }
}

void DBMapPosition::removeMark(const QString &caption)
{
    for (int  i = 0; i < d->m_markers.size() ; ++i)
    {
        if ( d->m_markers[i]->caption == caption )
        {
            if ( d->m_mapInited )
            {
                d->removeMark(d->m_markers.at(i));
            }
            MapMarker *mark = d->m_markers.takeAt(i);
            delete mark;
            i = 0;
        }
    }
}

void DBMapPosition::clearMarks()
{
    for (int  i = 0; i < d->m_markers.size() ; ++i)
    {
        if ( d->m_mapInited )
        {
            d->removeMark(d->m_markers.at(i));
        }
        MapMarker *mark = d->m_markers.takeAt(i);
        delete mark;
            i = 0;
    }
}

void DBMapPosition::showCoordinates(double latitude, double longitude)
{
    d->showCoordinates(latitude, longitude);
}

QString DBMapPosition::engineValue(const QString &engineKey)
{
    return engineValue(d->m_title, engineKey);
}

QString DBMapPosition::engineValue(const QString &markName, const QString &engineKey)
{
    if ( d->m_engineValues.contains(markName) )
    {
        return d->m_engineValues[markName][engineKey];
    }
    return QString();

}

bool DBMapPosition::coordsEquals(const QString &coords1, const QString &coords2)
{
    QString procCoords1(coords1);
    QString procCoords2(coords2);
    procCoords1.replace(" ", "");
    procCoords2.replace(" ", "");
    if ( procCoords1 == procCoords2 )
    {
        return true;
    }

    QStringList parts1 = procCoords1.split(",");
    QStringList parts2 = procCoords2.split(",");
    if ( parts1.size() != 2 || parts2.size() != 2 )
    {
        return false;
    }
    bool ok;
    double latitude1 = parts1.at(0).trimmed().toDouble(&ok);
    if ( !ok )
    {
        return false;
    }
    double latitude2 = parts2.at(0).trimmed().toDouble(&ok);
    if ( !ok )
    {
        return false;
    }
    double longitude1 = parts1.at(1).trimmed().toDouble(&ok);
    if ( !ok )
    {
        return false;
    }
    double longitude2 = parts2.at(1).trimmed().toDouble(&ok);
    if ( !ok )
    {
        return false;
    }
    double diff1 = latitude1 - latitude2;
    double diff2 = longitude1 - longitude2;
    double space1 = fabs(diff1) * 1000;
    double space2 = fabs(diff2) * 1000;
    if ( space1 < 0.5 && space2 < 0.5)
    {
        return true;
    }
    return false;
}

void DBMapPosition::fitToShowAllMarkers()
{
    if ( d->m_mapInited )
    {
        QString str("fitToShowAllMarkers();");
        ui->webView->page()->runJavaScript(str);
        d->m_fitToShowAllMarkersPending = false;
    }
    else
    {
        d->m_fitToShowAllMarkersPending = true;
    }
}

void DBMapPosition::showOrHideSearch()
{
    if ( ui->pbShowSearch->isChecked() )
    {
        ui->gbSearch->setVisible(true);
    }
    else
    {
        ui->gbSearch->setVisible(false);
    }
}

void DBMapPosition::observerUnregistered()
{
    DBBaseWidget::observerUnregistered();
    d->m_latitude = 0;
    d->m_longitude = 0;
    if ( d->m_mapInited )
    {
        removeMark(d->m_title);
        d->showCoordinates(d->m_latitude, d->m_longitude);
    }
}

void DBMapPosition::searchCoords()
{
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    QString uuid = d->m_geoCoder->coordinates(ui->leSearch->text());

    d->m_geocoderError[uuid] = "";
    d->m_geocoderSuccess[uuid] = false;
    d->m_geocoderTimeout = false;

    QTimer::singleShot(60 * 1000, this, SLOT(geocodeTimeout()));
    while ( d->m_geoCoder->isWorking(uuid) && !d->m_geocoderTimeout && (!d->m_geocoderSuccess[uuid] || d->m_geocoderError[uuid].isEmpty()) )
    {
        CommonsFunctions::processEvents();
    }
    CommonsFunctions::restoreOverrideCursor();

    if ( !d->m_geocoderSuccess.value(uuid) || d->m_geocoderTimeout )
    {
        if ( d->m_geocoderTimeout )
        {
            d->m_geocoderError[uuid] = trUtf8("El servidor no ha respondido.");
        }
        QMessageBox::warning(this, qApp->applicationName(), trUtf8("Ha ocurrido un error en la búsqueda de las coordenadas. \nEl error es: [%1]").arg(d->m_geocoderError[uuid]));
        d->m_geocoderError.remove(uuid);
        d->m_geocoderSuccess.remove(uuid);
        return;
    }

    d->m_geocoderError.remove(uuid);
    d->m_geocoderSuccess.remove(uuid);

    bool found = false;
    AlephERP::AERPMapPosition selectedSearch;
    if ( d->m_geocoderLastSearch[uuid].size() > 1 )
    {
        QStringList list;
        foreach (const AlephERP::AERPMapPosition &pos, d->m_geocoderLastSearch[uuid])
        {
            list.append(pos.formattedAddress);
        }
        bool ok;
        QString result = QInputDialog::getItem(this,
                                               qApp->applicationName(),
                                               trUtf8("La búsqueda que ha introducido ha devuelto varias direcciones. Escoja la correcta."),
                                               list,
                                               0,
                                               false,
                                               &ok);
        if ( !ok )
        {
            return;
        }
        else
        {
            foreach (const AlephERP::AERPMapPosition &pos, d->m_geocoderLastSearch[uuid])
            {
                if (pos.formattedAddress == result)
                {
                    selectedSearch = pos;
                    found = true;
                }
            }
        }
    }
    else if ( d->m_geocoderLastSearch[uuid].size() == 1 )
    {
        selectedSearch = d->m_geocoderLastSearch[uuid].first();
        found = true;
    }
    else
    {
        QMessageBox::information(this, qApp->applicationName(), trUtf8("La dirección que ha introducido no corresponde a ninguna dirección válida."), QMessageBox::Ok);
    }
    if ( found && !coordsEquals(selectedSearch.coordinates, coordinates()) )
    {
        setCoordinates(selectedSearch.coordinates);
        d->m_engineValues[d->m_title] = selectedSearch.engineValues;
        d->showMainMark();
        emit valueEdited(selectedSearch.coordinates);
        callQs(d->m_title, selectedSearch.coordinates);
    }
    d->m_geocoderLastSearch.remove(uuid);
}

void DBMapPosition::searchAddress()
{
    QString coords = QString("%1,%2").arg(ui->leLatitude->text()).arg(ui->leLongitude->text());
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    QString uuid = d->m_geoCoder->address(coords);

    d->m_geocoderError[uuid] = "";
    d->m_geocoderSuccess[uuid] = false;
    d->m_geocoderTimeout = false;

    QTimer::singleShot(60 * 1000, this, SLOT(geocodeTimeout()));
    while ( d->m_geoCoder->isWorking(uuid) && !d->m_geocoderTimeout && (!d->m_geocoderSuccess[uuid] || d->m_geocoderError[uuid].isEmpty()) )
    {
        CommonsFunctions::processEvents();
    }
    CommonsFunctions::restoreOverrideCursor();

    if ( !d->m_geocoderSuccess.value(uuid) || d->m_geocoderTimeout )
    {
        if ( d->m_geocoderTimeout )
        {
            d->m_geocoderError[uuid] = trUtf8("El servidor no ha respondido.");
        }
        QMessageBox::warning(this, qApp->applicationName(), trUtf8("Ha ocurrido un error en la búsqueda de la dirección. \nEl error es: [%1]").arg(d->m_geocoderError[uuid]));
        d->m_geocoderError.remove(uuid);
        d->m_geocoderSuccess.remove(uuid);
        return;
    }

    d->m_geocoderError.remove(uuid);
    d->m_geocoderSuccess.remove(uuid);

    AlephERP::AERPMapPosition selectedSearch;

    if ( d->m_geocoderLastSearch[uuid].size() == 0 )
    {
        QMessageBox::information(this, qApp->applicationName(), trUtf8("Las coordenadas que ha introducido no corresponden a ninguna dirección postal válida."), QMessageBox::Ok);
        return;
    }
    selectedSearch = d->m_geocoderLastSearch[uuid].at(0);
    setCoordinates(selectedSearch.coordinates);
    d->m_engineValues[d->m_title] = selectedSearch.engineValues;
    d->showMainMark();
    emit valueEdited(selectedSearch.coordinates);
    callQs(d->m_title, selectedSearch.coordinates);
    d->m_geocoderLastSearch.remove(uuid);
}

void DBMapPosition::coordinatesReady(const QString &uuid, const QList<AlephERP::AERPMapPosition> &data)
{
    d->m_geocoderLastSearch[uuid] = data;
    d->m_geocoderSuccess[uuid] = true;
}

void DBMapPosition::addressReady(const QString &uuid, const AlephERP::AERPMapPosition &data)
{
    QList<AlephERP::AERPMapPosition> lst;
    lst.append(data);
    d->m_geocoderLastSearch[uuid] = lst;
    d->m_geocoderSuccess[uuid] = true;
}

void DBMapPosition::geocodeError(const QString &uuid, QString error)
{
    d->m_geocoderError[uuid] = error;
    d->m_geocoderSuccess[uuid] = false;
}

void DBMapPosition::geocodeTimeout()
{
    d->m_geocoderTimeout = true;
}

void DBMapPosition::mapWasInit(bool value)
{
    d->m_mapInited = value;
    if ( d->m_mapInited )
    {
        refresh();
    }
}

void DBMapPosition::newValueFromUserDrag(const QString &markName)
{
    AlephERP::AERPMapPosition data = d->m_bindingObject->data(markName);
    emit userDragMark(markName, data.coordinates);
    if ( d->m_title == markName )
    {
        if ( !coordsEquals(data.coordinates, coordinates()) )
        {
            setCoordinates(data.coordinates);
            d->m_engineValues[markName] = data.engineValues;
            emit valueEdited(data.coordinates);
            callQs(markName, data.coordinates);
        }
    }
}

void DBMapPosition::callQs(const QString &markName, const QString &coords)
{
    AERPBaseDialog *thisForm = CommonsFunctions::aerpParentDialog(this);
    QScriptValueList args;
    args.append(QScriptValue(markName));
    args.append(QScriptValue(coords));
    if ( thisForm != NULL )
    {
        // Invocamos al método tras escoger o insertar un nuevo registro.
        if ( !d->m_scriptAfterChange.isEmpty() )
        {
            thisForm->callQSMethod(d->m_scriptAfterChange, args);
        }
        else
        {
            QString scriptName = QString("%1AfterChangeCoords").arg(objectName());
            thisForm->callQSMethod(scriptName, args);
        }
    }
}

void DBMapPosition::refresh()
{
    for (int i = 0 ; i < d->m_markers.size() ; i++)
    {
        if ( d->m_mapInited )
        {
            d->removeMark(d->m_markers.at(i));
            d->addMark(d->m_markers.at(i));
        }
    }
    if ( d->m_mapInited && d->m_fitToShowAllMarkersPending )
    {
        fitToShowAllMarkers();
    }
}

void DBMapPositionPrivate::initMap()
{
    QFile f(":/dev/googlemaps.html");
    if ( !f.open(QIODevice::ReadOnly) )
    {
        return;
    }
    QString html (f.readAll());
    if ( !m_licenseKey.isEmpty() )
    {
        html.replace("${LICENSEKEY}", QString("key=").arg(m_licenseKey));
    }
    else
    {
        html.replace("${LICENSEKEY}", QString(""));
    }
    html.replace(QString("${INITLATITUDE}"), QString("%1").arg(m_latitude));
    html.replace(QString("${INITLONGITUDE}"), QString("%1").arg(m_longitude));
    html.replace(QString("${INITZOOM}"), QString("%1").arg(m_initZoom));

    if ( m_progressBar.isNull() )
    {
        m_progressBar = new QProgressBar(q_ptr->ui->webView);
        m_progressBar->setMaximum(100);
        m_progressBar->move(q_ptr->ui->webView->rect().center());
        QObject::connect(q_ptr->ui->webView, SIGNAL(loadStarted()), m_progressBar.data(), SLOT(show()));
        QObject::connect(q_ptr->ui->webView, SIGNAL(loadProgress(int)), m_progressBar.data(), SLOT(setValue(int)));
        QObject::connect(q_ptr->ui->webView, SIGNAL(loadFinished(bool)), m_progressBar.data(), SLOT(hide()));
    }
    q_ptr->ui->webView->setHtml(html);
    // q_ptr->ui->webView->page()->currentFrame()->addToJavaScriptWindowObject("alephERPBinding", m_bindingObject.data(), QWebFrame::QtOwnership);

    f.close();
}

void DBMapPositionPrivate::showCoordinates(double latitude, double longitude)
{
    if ( m_mapInited )
    {
        QString str =
                QString("var newLoc = new google.maps.LatLng(%1, %2);").arg(latitude).arg(longitude) +
                QString("map.setCenter(newLoc);");

        q_ptr->ui->webView->page()->runJavaScript(str);
    }
}

void DBMapPositionPrivate::showMainMark()
{
    if ( q_ptr->observer() != NULL )
    {
        showCoordinates(m_latitude, m_longitude);
        q_ptr->addMark(m_latitude, m_longitude, m_title, m_infoWindow);
        q_ptr->fitToShowAllMarkers();
    }
}

void DBMapPositionPrivate::removeMark(MapMarker *mark)
{
    QString str = QString("removeMark('%1');").arg(mark->caption);
    q_ptr->ui->webView->page()->runJavaScript(str);
}

void DBMapPositionPrivate::addMark(MapMarker *mark)
{
    QString str = QString("addMark(%1, %2, '%3', '%4');").
            arg(mark->latitude).
            arg(mark->longitude).
            arg(mark->caption).
            arg(mark->infoWindow);

    q_ptr->ui->webView->page()->runJavaScript(str);
}


DBMapPositionJSObject::DBMapPositionJSObject(QObject *parent) : QObject(parent)
{
}

DBMapPositionJSObject::~DBMapPositionJSObject()
{
}

void DBMapPositionJSObject::userDragInit(const QString &markName)
{
    if ( m_data.contains(markName) )
    {
        m_data[markName].coordinates.clear();
        m_data[markName].engineValues.clear();
        m_data[markName].formattedAddress.clear();
        m_data[markName].markName.clear();
    }
    else
    {
        m_data[markName] = AlephERP::AERPMapPosition();
        m_data[markName].markName = markName;
    }
}

void DBMapPositionJSObject::userDragEnd(const QString &markName)
{
    emit newMarkPosition(markName);
}

void DBMapPositionJSObject::userMovesMark(const QString &markName, const QString &coordinates)
{
    m_data[markName].coordinates = coordinates;
}

void DBMapPositionJSObject::userMovesMarkFormattedAddress(const QString &markName, const QString &address)
{
    m_data[markName].formattedAddress = address;
}

void DBMapPositionJSObject::userMovesMarkAddressComponent(const QString &markName, const QString &type, const QString &value)
{
    m_data[markName].engineValues[type] = value;
}

AlephERP::AERPMapPosition DBMapPositionJSObject::data(const QString &markName)
{
    return m_data[markName];
}
