/***************************************************************************
 *   Copyright (C) 2011 by David Pinelo   *
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
#ifndef AERPSCRIPTCOMMON_H
#define AERPSCRIPTCOMMON_H

#include <QtCore>
#include <QtScript>
#include <QtPrintSupport>
#include "business/aerpgeocodedatamanager.h"

class AERPScriptCommonPrivate;
class DBField;
class BaseBean;
class BaseBeanMetadata;

/*!
  Conjunto de funciones habituales y comunes disponibles en los scripts a partir
  del objeto AERPScriptCommon
  */
class AERPScriptCommon : public QObject, public QScriptable
{
    Q_OBJECT
    Q_ENUMS(DatesInterval)

    Q_PROPERTY(QString lastError READ lastError)

private:
    AERPScriptCommonPrivate *d_ptr;
    Q_DECLARE_PRIVATE(AERPScriptCommon)

public:
    explicit AERPScriptCommon(QObject *parent = 0);
    ~AERPScriptCommon();

    enum DatesInterval
    {
        SECONDS = 1,
        MINUTES = 2,
        HOURS = 4,
        DAYS = 8,
        MONTHS = 16,
        YEARS = 32
    };

    void setPropertiesForScriptObject(QScriptValue &obj);

    static QScriptValue toScriptValue(QScriptEngine *engine, AERPScriptCommon* const &in);
    static void fromScriptValue(const QScriptValue &object, AERPScriptCommon* &out);

    // -----------------------------------
    // GENERALES
    // -----------------------------------
    QString lastError() const;

    // --------------------------------------
    // FUNCIONES PARA EL TRABAJO CON ARCHIVOS
    // --------------------------------------
    Q_INVOKABLE bool saveToFile(const QString &fileName, const QString &content, bool overwrite = false);
    Q_INVOKABLE bool appendToFile(const QString &fileName, const QString &content);
    Q_INVOKABLE QString readFromFile(const QString &fileName);
    Q_INVOKABLE QString saveToTempFile(const QString &content, const QString &extension = "");
    Q_INVOKABLE QString readFromTempFile(const QString &fileName, bool remove = true);
    Q_INVOKABLE QString tempPath();
    Q_INVOKABLE QString scriptFunctionsPath();
    Q_INVOKABLE QString getExistingDirectory(const QString &label);
    Q_INVOKABLE QString getOpenFileName(const QString &label, const QString &filter = "");
    Q_INVOKABLE QStringList getOpenFileNames(const QString &label, const QString &filter = "");
    Q_INVOKABLE QString getSaveFileName(const QString &label, const QString &filter = "");
    Q_INVOKABLE void methodToInvokeOnDirChange(const QString &method);
    Q_INVOKABLE void methodToInvokeOnFileChange(const QString &method);
    Q_INVOKABLE void addPathToWatch(const QString &path, bool recursiveSearch = true);

    // -------------------------------------------------------------
    // FUNCIONES PARA ACCESO A BASE DE DATOS Y TRABAJO CON REGISTROS
    // -------------------------------------------------------------
    Q_INVOKABLE QScriptValue createBean(const QString &tableName);
    Q_INVOKABLE QScriptValue bean(const QString &tableName, const QString &where);
    Q_INVOKABLE QScriptValue beanByPk(const QString &tableName, const QVariant &value);
    Q_INVOKABLE QScriptValue beanByField(const QString &tableName, const QString &fieldName, const QVariant &value);
    Q_INVOKABLE QScriptValue beans(const QString &tableName, const QString &where, const QString &order);
    Q_INVOKABLE QScriptValue sqlSelect(const QString &sql, const QString &connectionName = "");
    Q_INVOKABLE QVariant sqlSelectFirstColumn(const QString &sql, const QString &connectionName = "");
    Q_INVOKABLE QScriptValue sqlSelectFirst(const QString &sql, const QString &connectionName = "");
    Q_INVOKABLE bool sqlExecute(const QString &sql, const QString &connectionName = "");
    Q_INVOKABLE int sqlCount(const QString &tableName, const QString &where, const QString &connectionName = "");
    Q_INVOKABLE bool addConnection(const QString &type,
                                   const QString &connectionName,
                                   const QString &hostName = "",
                                   const QString &databaseName = "",
                                   const QString &userName = "",
                                   const QString &password = "",
                                   int port = -1,
                                   const QString &connectOptions = "");

    Q_INVOKABLE QString xmlQuery(const QString &xml, const QString &query);

    Q_INVOKABLE bool addToTransaction(BaseBean *bean);
    Q_INVOKABLE bool addToTransaction(BaseBean *bean, const QString &contextName);
    Q_INVOKABLE bool addPreviousSqlToContext(const QString &sql, const QString &contextName);
    Q_INVOKABLE bool addFinalSqlToContext(const QString &sql, const QString &contextName);
    Q_INVOKABLE void discardContext(const QString &contextName = "");
    Q_INVOKABLE QScriptValue beansOnTransaction();
    Q_INVOKABLE bool commit();
    Q_INVOKABLE bool rollback();

    Q_INVOKABLE QScriptValue orderMetadatasForInsertUpdate(QList<BaseBeanMetadata *> list);
    Q_INVOKABLE bool openLocalServerConnection();

    // -----------------------------------------------
    // FUNCIONES PARA EL PARSEADO Y FORMATEO DE DATOS
    // -----------------------------------------------
    Q_INVOKABLE QString formatNumber(const QVariant &number, int numDecimals = 2);
    Q_INVOKABLE QString formatDate(const QScriptValue &date, const QString &format = QString());
    Q_INVOKABLE QString formatDateTime(const QScriptValue &date, const QString &format = QString());
    Q_INVOKABLE double parseDouble(const QString &number);
    Q_INVOKABLE QString sqlDate(const QDate &date);
    Q_INVOKABLE QString sqlDateTime(const QDateTime &dateTime);

    // -----------------------------------------------
    // FUNCIONES PARA EL TRABAJO CON CADENAS
    // -----------------------------------------------
    Q_INVOKABLE QString rightJustified(const QString &string, const QString &fillValue, int length);
    Q_INVOKABLE QString leftJustified(const QString &string, const QString &fillValue, int length);
    Q_INVOKABLE QString trimed(const QString &string);
    Q_INVOKABLE QString stringRightJustified(const QString &string, int width, const QString &fillChar);
    Q_INVOKABLE QString joinArray(const QScriptValue &arr, const QString &separator);

    // ---------------------
    // VALIDACIONES DIVERSAS
    // ---------------------
    Q_INVOKABLE bool validateCIF(const QString &cif);
    Q_INVOKABLE bool validateNIF(const QString &nif);
    Q_INVOKABLE bool validateCreditCard(const QString &creditCard);

    // ----------------------------------------------------
    // TRABAJO CON DATOS: NUMÉRICOS, FECHAS, CONTADORES ...
    // ----------------------------------------------------
    Q_INVOKABLE QScriptValue nextCounter(DBField *fld, const QString &sqlFilter = "", bool considerEnvVar = true);

    Q_INVOKABLE QScriptValue date(const QString &stringDate, const QString &format);
    Q_INVOKABLE QScriptValue addIntervalToDate(const QVariant &date, DatesInterval interval, int value);
    Q_INVOKABLE double round(double x, int precision = 2);
    Q_INVOKABLE bool equalsWithAllowedError(const QScriptValue &scriptX, const QScriptValue & scriptY, const QScriptValue & scriptError = 5);
    Q_INVOKABLE double toDouble(const QScriptValue &value, double defaultValue);

    Q_INVOKABLE QString spellNumber(int number, const QString &language = "es");
    Q_INVOKABLE QString spellNumber(double number, int decimalPlaces = 2, const QString &language = "es");

    Q_INVOKABLE QString extractDigits(const QString &data);

    Q_INVOKABLE QString generateUuid();

    // -----------------------------------------------
    // ACCESO HTTP e EMAIL
    // -----------------------------------------------
    Q_INVOKABLE QString httpGet(const QString &scheme, const QString &host, const QString &path, const QScriptValue &queryList,
                                const QString &userName = "", const QString &password = "", int port = 80, bool usePreemptiveAuthentication = false);
    Q_INVOKABLE QString httpGet(const QString &strUrl, const QString &userName = "", const QString &password = "", bool usePreemptiveAuthentication = false);
    Q_INVOKABLE bool sendEmail(const QString &to, const QString &cc, const QString &bcc, const QString &subject, const QString &body);

    // -----------------------------------------------
    // FUNCIONES PARA EL ACCESO A VARIABLES DE ENTORNO
    // -----------------------------------------------
    Q_INVOKABLE void setEnvVar(const QString &varName, const QVariant &v);
    Q_INVOKABLE void setDbEnvVar(const QString &varName, const QVariant &v);
    Q_INVOKABLE QVariant envVar(const QString &name);
    Q_INVOKABLE bool setDbEnvVar(const QString userName, const QString &varName, const QVariant &v);
    Q_INVOKABLE QVariant envVar(const QString &userName, const QString &varName);

    // ---------------------------------------------------
    // FUNCIONES PARA EL ACCESO A DATOS DEL USUARIO LOGADO
    // ---------------------------------------------------
    Q_INVOKABLE QString username();
    Q_INVOKABLE QStringList roles();
    Q_INVOKABLE bool hasRole(const QString &role);
    Q_INVOKABLE QScriptValue login();
    Q_INVOKABLE QScriptValue newLoginUser();
    Q_INVOKABLE bool changeUserPassword();
    Q_INVOKABLE bool createSystemUser(const QString &userName, const QString &password);

    // ----------------------------------------------------------------------------------
    // FUNCIONES DE ACCESO A REGISTRO, PARAMETROS DE CONFIGURACIÓN Y RUNTIME DE EJECUCIÓN
    // ----------------------------------------------------------------------------------
    Q_INVOKABLE QVariant registryValue(const QString &key);
    Q_INVOKABLE void setRegistryValue(const QString &key, const QString &value);

    Q_INVOKABLE QScriptValue getOpenForm(const QString &name);

    Q_INVOKABLE bool checkMd5(const QString &valueToCheck, const QString &md5Hash);
    Q_INVOKABLE QScriptValue md5(const QString &value);

    Q_INVOKABLE QScriptValue importResource();
    Q_INVOKABLE QScriptValue checkQS(const QString &script);

    Q_INVOKABLE void generateDefinitionFileSql(const QString &module, const QString &fileName, const QString &dialect);

    Q_INVOKABLE bool importData(const QString &file, const QString &progressDialogText, const QString &tableName = "");

    Q_INVOKABLE QString getSystemObjectPath(const QString &objectName, const QString &type);

#ifdef ALEPHERP_DEVTOOLS
    Q_INVOKABLE QString calculatePatch(const QString &original, const QString &dest);
    Q_INVOKABLE QString prettyPatch(const QString &original, const QString &dest);
#endif

    // Si el plugin indicado lo permite, abrirá el formulario de edición del informe
    Q_INVOKABLE bool editReport(const QString &reportName);

    Q_INVOKABLE void clearObjectCache();

    // ---------------------------------------------------------------------------
    // FORMULARIOS PARA COMUNICACIÓN DE DATOS O PETICIÓN DE INFORMACIÓN AL USUARIO
    // ---------------------------------------------------------------------------
    Q_INVOKABLE int getInt(const QString &label, int value = 0, int cancelValue = 0,  int min = -2147483647, int max = 2147483647);
    Q_INVOKABLE double getDouble(const QString &label, double value = 0, double cancelValue = 0, double min = -2147483647, double max = 2147483647, int decimals = 2);
    Q_INVOKABLE double getDouble(const QScriptValue &parent, const QString &label, double value = 0, double cancelValue = 0, double min = -2147483647, double max = 2147483647, int decimals = 2);
    Q_INVOKABLE QString getText(const QString & label);
    Q_INVOKABLE QScriptValue getDate(const QString &label, const QDate &defaultDate = QDate::currentDate());
    Q_INVOKABLE QScriptValue getDateTime(const QString &label, const QDateTime &defaultDateTime = QDateTime::currentDateTime());
    Q_INVOKABLE QScriptValue chooseItemFromArray(const QString &label, const QScriptValue &list);
    Q_INVOKABLE QString chooseString(const QString &label, const QStringList &list);
    Q_INVOKABLE void fadeMessage(const QString &message, int msecsToHide = 2000);
    Q_INVOKABLE void hideFadeMessage();
    Q_INVOKABLE void showProgressDialog(const QString &labelText, int maximum);
    Q_INVOKABLE void setValueProgressDialog(int value);
    Q_INVOKABLE void setLabelProgressDialog(const QString label);
    Q_INVOKABLE void closeProgressDialog();
    Q_INVOKABLE QScriptValue dataTable();
    Q_INVOKABLE void dataTable(const QString &tableName, const QString &where = "", const QString &order = "");
    Q_INVOKABLE void showTrayIconMessage(const QString &message);
    Q_INVOKABLE void waitCursor();
    Q_INVOKABLE void restoreCursor();

    // -------------------------------------------------------------
    // INFORMACIÓN SOBRE VERSIONES Y ALGUNOS PARÁMETROS DE CONFIGURACIÓN
    // -------------------------------------------------------------
    Q_INVOKABLE QString appVersion();
    Q_INVOKABLE QString appMainVersion();
    Q_INVOKABLE QString appRevision();
    Q_INVOKABLE QString sqlDriver();

    // --------------------------------------------------
    // MODO DE TRABAJO LOCAL
    // --------------------------------------------------
    Q_INVOKABLE bool enterOnBatchMode();
    Q_INVOKABLE bool finishBatchMode(QString &report);
    Q_INVOKABLE bool isOnBatchMode();

    // ------------------------------------------------
    // FUNCIONES DE AYUDA PARA EL TRABAJO CON REGISTROS
    // ------------------------------------------------
    Q_INVOKABLE QScriptValue chooseRecordFromComboBox(const QString &tableName,
                                                      const QString &fieldToShow,
                                                      const QString &where = "",
                                                      const QString &order = "",
                                                      const QString &label = "");
    Q_INVOKABLE QScriptValue chooseRecordsFromTable(const QString &tableName,
                                                    const QString &where = "",
                                                    const QString &order = "",
                                                    const QString &label = "",
                                                    bool userEnvVars = true);
    Q_INVOKABLE QScriptValue chooseRecordFromTable(const QString &tableName,
                                                   const QString &where = "",
                                                   const QString &order = "",
                                                   const QString &label = "",
                                                   bool userEnvVars = true);
    Q_INVOKABLE QScriptValue chooseChildFromComboBox(BaseBean *bean,
                                                     const QString &relationName,
                                                     const QString &fieldToShow,
                                                     const QString &label = "",
                                                     const QString &filter = "",
                                                     const QString &messageIfEmpty = "");
    Q_INVOKABLE void openRecordDialog(BaseBean *bean,
                                      AlephERP::FormOpenType openType,
                                      bool useNewContext,
                                      QWidget* parent = 0);

    // ------------------------------------------------
    // TRABAJO CON HOJAS DE CÁLCULOS
    // ------------------------------------------------
    Q_INVOKABLE QScriptValue openSpreadSheet(const QString &file, const QString &type = "", int rowInit = -1, int rowCount = -1);
    Q_INVOKABLE QScriptValue createSpreadSheet(const QString &file, const QString &type);

    // ------------------------------------------------
    // TRABAJO CON COORDENADAS, DATOS DE GEOLOCALIZACIÓN
    // ------------------------------------------------
    Q_INVOKABLE QScriptValue coordinates(const QString &address, const QString &server = "");

    // ------------------------------------------------
    // FUNCIONES DE AYUDA A LA IMPRESIÓN DE DOCUMENTOS
    // ------------------------------------------------
    Q_INVOKABLE void printPreviewHtml(const QString &html);
    Q_INVOKABLE void printHtml(const QString &html);

    Q_INVOKABLE QStringList availablePrinterNames() const;
    Q_INVOKABLE QString defaultPrinterName() const;

signals:
    void directoryChanged(QString dir);
    void fileChanged(QString file);

private slots:
    void geocoderAvailable(const QString &uuid, QList<AlephERP::AERPMapPosition> data);
    void geocodeError(const QString &uuid, QString error = "");
    void geocodeTimeout();
    void print(QPrinter *printer);

public slots:

};

Q_DECLARE_METATYPE(AERPScriptCommon*)
Q_SCRIPT_DECLARE_QMETAOBJECT(AERPScriptCommon, QObject*)
Q_DECLARE_METATYPE(AERPScriptCommon::DatesInterval)

#endif // AERPSCRIPTCOMMON_H
