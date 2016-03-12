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
#ifndef CONFIGURACION_H
#define CONFIGURACION_H

#include <QtCore>
#if QT_VERSION <= 0x050000
#include <QtGui>
#endif
#if QT_VERSION > 0x050000
#include <QtWidgets/QTableView>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QMainWindow>
#endif

#include <alepherpglobal.h>
#include "qlogger.h"

#define KEY_ITEMVIEW_CONFIG             "generales/itemView"
#define KEY_ITEMVIEW_CONFIG_ORDERS		"generales/itemViewOrders"

/**
	Esta clase servirá de wrapper para las opciones de configuración del programa.
	@author David Pinelo <alepherp@alephsistemas.es>
*/
class ALEPHERP_DLL_EXPORT AlephERPSettings : public QObject
{
    Q_OBJECT

    Q_PROPERTY (QString deviceType READ deviceType)

private:
    static QPointer<AlephERPSettings> m_singleton;

    // Variables propias de la conexión a la BBDD
    QString m_dbServer;
    QString m_userDb;
    QString m_passwordDb;
    int m_dbPort;
    QString m_dbName;
    QString m_dbSchema;
    QString m_dbSchemaErp;
    QString m_connectionType;
    QString m_dsnODBC;
    QString m_fileSystemEncoding;
    QString m_connectOptions;
    QString m_cloudProtocol;

    QLogger::LogLevel m_logLevel;

    int m_lastServer;

    bool m_modeDBA;

    // Nombre de las tablas de sistema
    QString m_systemTablePrefix;

    QDate m_minimumDate;
    QDateTime m_minimunDateTime;

    // Tiempos de espera antes de recargar los datos
    int m_timeBetweenReloads;
    /** Número de registros en un modelo que se supone elevado según el tráfico de red */
    int m_strongFilterRowCountLimit;
    int m_fetchRowCount;
    int m_httpTimeout;

    // Sólo para usuarios que utilizan los servicios de la nube (no afecta al funcionamiento de usuarios en local)
    QString m_licenseKey;
    QString m_computerUUID;
    QString m_clientCert;
    QString m_passwordCert;

    // Herramienta de Diff
    QString m_diffExe;
    bool m_useExternalDiff;

    // Variables relativas al aspecto de la aplicación
    QMap<QString, QByteArray> m_dimensionesForms;
    QMap<QString, QPoint> m_posForms;
    QMap<QString, QString> m_toolBarActions;
    QString m_toolBars;

    bool m_debuggerEnabled;
    bool m_advancedUser;

    // Imágenes a usar
    QString m_externalResource;

    mutable QMutex m_mutex;

    // Estado de la ventana principal
    QByteArray m_mainWindowState;

    /** Último usuario logado */
    QString m_lastLoggedUser;

    QString m_check;

    QString m_dataPath;

    QString m_externalPDFViewer;
    bool m_internalPDFViewer;

    bool m_mdiTabView;

    int m_humanKeyPressIntervalMsecs;

    QString m_lookAndFeel;
    bool m_lookAndFeelDark;

    QString m_reportEngine;

    // Variables para el acceso a la API de configuración.
    QSettings *m_settings;
    QLocale *m_locale;

    // Primer día de la semana para los calendarios
    Qt::DayOfWeek m_firstDayOfWeek;

    // Frecuencia de refresco de los datos
    int m_modelRefreshTimeout;
    bool m_modelsRefresh;

    bool m_allowSystemTray;

    int m_reportsToShowCombobox;

    bool m_userWritesHistory;

    QMap<QString, QVariant> m_scheduleMode;
    QMap<QString, QVariant> m_scheduleAdjustRow;

#ifdef ALEPHERP_DOC_MANAGEMENT
    QString m_scanDevice;
    QStringList m_hotFolders;
    QString m_hotFolderLocal;
#endif

#ifdef ALEPHERP_LOCALMODE
    // Indicará si cuando se salió del sistema, éste se encontraba en modo de trabajo local
    bool m_localMode;
    QDateTime m_initLocalMode;
    bool m_batchLoadBackground;
    int m_secondsLoadBackground;
#endif

#ifdef ALEPHERP_QSCISCINTILLA
    QFont m_codeFont;
#endif

#ifdef ALEPHERP_CLOUD_SUPPORT
    QString m_mainCloudServer;
#endif

    void init (void);

public:
    explicit AlephERPSettings(QObject *parent = 0);
    virtual ~AlephERPSettings();

    static AlephERPSettings *instance();
    static void release();

    Q_INVOKABLE QVariant key(const QString &value, const QVariant defaultValue = QVariant());
    Q_INVOKABLE void setKey(const QString &key, const QVariant &value);
    Q_INVOKABLE QString scriptKey(const QString &value);
    Q_INVOKABLE void setScriptKey(const QString &key, const QVariant &value);

    QString deviceType();
    QSize deviceSize();
    QString deviceTypeSize();

    bool firstUse() const;

    void setDbServer (const QString& theValue);
    QString dbServer() const;
    void setDbUser (const QString& theValue);
    QString dbUser() const;
    void setDbPassword (const QString& theValue);
    QString dbPassword() const;
    void setDbName (const QString& theValue);
    QString dbName() const;
    int dbPort() const;
    void setDbPort (int theValue);
    void setDbSchema (const QString& theValue);
    QString dbSchema() const;
    void setConnectionType (const QString& theValue);
    QString connectionType() const;
    void setDsnODBC (const QString& theValue);
    QString dsnODBC() const;
    QString systemTablePrefix() const;
    void setSystemTablePrefix (const QString &value);
    QString fileSystemEncoding() const;
    void setSileSystemEncoding (const QString &value);
    QString connectOptions();
    void setConnectOptions(const QString &value);
    QString cloudProtocol();
    void setCloudProtocol(const QString &value);
    int httpTimeout();
    void setHttpTimeout(int value);
    bool allowSystemTray() const;
    void setAllowSystemTray(bool value);

    bool userWritesHistory() const;

    int lastServer();
    void setLastServer(int id);
    QLogger::LogLevel logLevel() const;
    void setLogLevel(QLogger::LogLevel &level);

    bool dbaUser();
    void setDbaUser(bool value);
    bool advancedUser();
    void setAdvancedUser(bool value);

    QString reportEngine() const;
    void setReportEngine(const QString &value);

    QString licenseKey();
    void setLicenseKey(const QString &value);
    QString computerUUID();

    int timeBetweenReloads();
    void setTimeBetweenReloads(int time);
    int strongFilterRowCountLimit();
    void setStrongFilterRowCountLimit(int limit);
    int fetchRowCount();
    void setFetchRowCount(int count);

    void applyDimensionForm (QWidget *formulario) const;
    void applyAnimatedDimensionForm (QWidget *form);
    void applyAnimatedDimensionForm (QWidget *form, const QRect &geometry);
    void saveDimensionForm (QWidget *formulario);
    void applyPosForm(QWidget *formulario) const;
    void savePosForm(QWidget *formulario);
    QStringList toolBars();
    void saveToolBarActions(QToolBar *toolBar);
    void restoreToolBarActions(QToolBar *toolBar);
    void restoreState(QMainWindow *window);
    void saveState(QMainWindow *window);

    bool debuggerEnabled();
    void setDebuggerEnabled(bool value);

    bool mdiTabView();
    void setMdiTabView(bool value);

    int humanKeyPressIntervalMsecs() const;

    int reportsToShowCombobox() const;
    void setReportsToShowCombobox(int value);

    int scheduleMode(const QString &schedule) const;
    void setScheduleMode(const QString &schedule, int value);
    bool scheduleAdjustRow(const QString &schedule) const;
    void setScheduleAdjustRow(const QString &schedule, bool value);

    void saveTreeViewExpandedState(const QStringList &list, const QString &treeViewWidgetName);
    QStringList restoreTreeViewExpandedState(const QString &treeViewWidgetName);

#ifdef ALEPHERP_QSCISCINTILLA
    QFont codeFont() const;
    void setCodeFont(const QFont &font);
#endif

#ifdef ALEPHERP_DOC_MANAGEMENT
    QString scanDevice() const;
    void setScanDevice(const QString &id);
    QStringList hotFolders() const;
    void setHotFolders(const QStringList &list);
    QString hotFolderLocal() const;
    void setHotFoldersLocal(const QString &value);
#endif

    QString externalResource();
    void setExternalResource(const QString &temp);

    Qt::DayOfWeek firstDayOfWeek();
    void setFirstDayOfWeek(const QString &temp);

    template<typename T>
    QString viewIndicatorColumnOrder(T *tw)
    {
        if ( tw == NULL )
        {
            return QString();
        }
        QString key = QString("%1/%2").arg(KEY_ITEMVIEW_CONFIG).arg(tw->metaObject()->className());
        key = key + "/" + tw->objectName();
        key = key + "OrderIndicator";

        QVariant valor = m_settings->value(key);
        return valor.toString();
    }

    template<typename T>
    QString viewIndicatorColumnSort(T *tw)
    {
        if ( tw == NULL )
        {
            return QString();
        }
        QString key = QString("%1/%2").arg(KEY_ITEMVIEW_CONFIG_ORDERS).arg(tw->metaObject()->className());
        key = key + "/" + tw->objectName();
        key = key + "SortIndicator";

        QVariant valor = m_settings->value(key);
        return valor.toString();
    }

    /*!
      Salva el orden de las columnas del QTableView \a tw. \a columnNames hace referencia
      al DBField (dbFieldName) o nombre de la columna del modelo BaseBean. Se guarda el nombre
      de la columna por un caso raro que se ve en DBRecordDlg. Se almacena, también el orden
      de las mismas, "ASC", "DESC"...
      */
    template<typename T>
    void saveViewIndicatorColumnOrder(T *tw, const QString &columnName, const QString &columnOrder)
    {
        if ( tw == NULL )
        {
            return;
        }

        QString key = QString("%1/%2").arg(KEY_ITEMVIEW_CONFIG).arg(tw->metaObject()->className());
        key = key + "/" + tw->objectName();
        key = key + "OrderIndicator";
        m_settings->setValue(key, columnName);
        key = QString("%1/%2").arg(KEY_ITEMVIEW_CONFIG_ORDERS).arg(tw->metaObject()->className());
        key = key + "/" + tw->objectName();
        key = key + "SortIndicator";
        m_settings->setValue(key, columnOrder);
        m_settings->sync();
    }

    template<typename T>
    QStringList viewColumnsOrder(T *tw)
    {
        if ( tw == NULL )
        {
            return QStringList();
        }

        QString key = QString("%1/%2").arg(KEY_ITEMVIEW_CONFIG).arg(tw->metaObject()->className());
        key = key + "/" + tw->objectName();
        key = key + "Order";

        QVariant valor = m_settings->value(key);
        return valor.toStringList();
    }

    template<typename T>
    QStringList viewColumnsSort(T *tw)
    {
        if ( tw == NULL )
        {
            return QStringList();
        }

        QString key = QString("%1/%2").arg(KEY_ITEMVIEW_CONFIG_ORDERS).arg(tw->metaObject()->className());
        key = key + "/" + tw->objectName();
        key = key + "Sort";

        QVariant valor = m_settings->value(key);
        return valor.toStringList();
    }

    /*!
      Salva el orden de las columnas del QTableView \a tw. \a columnNames hace referencia
      al DBField (dbFieldName) o nombre de la columna del modelo BaseBean. Se guarda el nombre
      de la columna por un caso raro que se ve en DBRecordDlg. Se almacena, también el orden
      de las mismas, "ASC", "DESC"...
      */
    template<typename T>
    void saveViewColumnsOrder(T *tw, QStringList columnNames, QStringList columnOrder)
    {
        if ( tw == NULL )
        {
            return;
        }

        QString key = QString("%1/%2").arg(KEY_ITEMVIEW_CONFIG).arg(tw->metaObject()->className());
        key = key + "/" + tw->objectName();
        key = key + "Order";
        m_settings->setValue(key, columnNames);
        key = QString("%1/%2").arg(KEY_ITEMVIEW_CONFIG_ORDERS).arg(tw->metaObject()->className());
        key = key + "/" + tw->objectName();
        key = key + "Sort";
        m_settings->setValue(key, columnOrder);
        m_settings->sync();
    }

    template<typename T>
    void applyViewState(T *tw)
    {
        if ( tw == NULL )
        {
            return;
        }

        QString key = QString("%1/%2").arg(KEY_ITEMVIEW_CONFIG).arg(tw->metaObject()->className());
        key = key + "/" + tw->objectName();

        QVariant valor = m_settings->value(key);
        if ( !valor.isNull() && valor.isValid() && tw->model() != NULL )
        {
            QList<QVariant> anchos = valor.toList();
            if ( anchos.size() == tw->model()->columnCount() )
            {
                for ( int i = 0 ; i < tw->model()->columnCount() ; i++ )
                {
                    if ( anchos.at(i).canConvert<int>() )
                    {
                        tw->setColumnWidth(i, anchos.at(i).toInt());
                    }
                }
            }
        }
    }

    template<typename T>
    void saveViewState(T *tw)
    {
        if ( tw == NULL )
        {
            return;
        }

        QList<QVariant> widths;
        QString key = QString("%1/%2").arg(KEY_ITEMVIEW_CONFIG).arg(tw->metaObject()->className());
        key = key + "/" + tw->objectName();

        if ( tw->model() != NULL )
        {
            for ( int i = 0 ; i < tw->model()->columnCount() ; i++ )
            {
                widths << tw->columnWidth(i);
            }
            m_settings->setValue(key, widths);
            m_settings->sync();
        }
    }

    QString dataPath() const;
    QLocale* locale() const;
    QString check();

    QDate minimumDate();
    QDateTime minimumDateTime();

    void saveRegistryValue(const QString &key, const QVariant &value);
    QVariant loadRegistryValue(const QString &key);
    QMap<QString, QString> groupValues(const QString &key);

    QString lastLoggedUser();
    void setLastLoggerUser(const QString &user);

    void setLookAndFeel (const QString &theValue);
    QString lookAndFeel() const;
    bool lookAndFeelDark() const;
    void setLookAndFeelDark(bool value);

    void setInternalPDFViewer ( bool theValue );
    bool internalPDFViewer() const;

    int modelRefreshTimeout();
    void setModelRefreshTimeout(int value);
    bool modelsRefresh() const;
    void setModelsRefresh(bool value);

    void setExternalPDFViewer ( const QString& theValue );
    QString externalPDFViewer() const;

    Q_INVOKABLE qlonglong uniqueId();

#ifdef ALEPHERP_LOCALMODE
    bool localMode() const;
    void setLocalMode(bool localMode);
    QDateTime initLocalMode() const;
    void setInitLocalMode(const QDateTime &initLocalMode);
    bool batchLoadBackground();
    void setBatchLoadBackground(bool value);
    int secondsLoadBackground() const;
    void setSecondsLoadBackground(int secondsLoadBackground);
#endif

    QString passwordCert() const;
    void setPasswordCert(const QString &passwordCert);

public slots:
    void save (void);

    static void importFromIniFile(const QString &iniFile);

};

#define alephERPSettings (static_cast<AlephERPSettings *>(AlephERPSettings::instance()))

Q_DECLARE_METATYPE(AlephERPSettings*)

#endif
