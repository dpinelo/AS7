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
#include <QtCore>
#include "configuracion.h"
#include "qlogger.h"
#include "dbfieldmetadata.h"
#include "globales.h"
#include "dao/beans/basebean.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/database.h"
#include "dao/basedao.h"
#include "scripts/perpscript.h"
#include "business/aerpbuiltinexpressioncalculator.h"
#include "models/envvars.h"

class DBFieldMetadataPrivate
{
//	Q_DECLARE_PUBLIC(DBFieldMetadata)
public:
    DBFieldMetadata *q_ptr;
    /** Número cardinal que identifica al field */
    int m_index;
    /** Nombre en base de datos de la columna donde se almacena el dato */
    QString m_dbFieldName;
    /** Nombre interno de la aplicación con el que se accede a este método */
    QString m_fieldName;
    QString m_fieldNameScript;
    QString m_calculatedFieldName;
    /** Indica si es un campo de sólo lectura */
    bool m_readOnly;
    /** Indica si este campo se creará como un campo de índice en la base de datos */
    bool m_dbIndex;
    /** ¿Puede este campo ser nulo ? */
    bool m_null;
    /** ¿Es este campo una primary key? */
    bool m_primaryKey;
    /** ¿El campo es único? No tiene porqué ser una primary key para ser único */
    bool m_unique;
    /** Puede ser que un campo deba ser único sólo en el ámbito que marca otra columna. Por ejemplo,
      supongamos que el registro tiene un campo "código_de_empresa". El valor que tenga este field será
      único para cada valor de código_de_empresa. Así, puede haber un valor de este campo '01' para
      código_de_empresa=1 y otro valor '01' para código_de_empresa=2 */
    QString m_uniqueOnFilterField;
    bool m_uniqueCompound;
    /** Tipo del campo, segun los valores de QVariant */
    enum QVariant::Type m_type;
    /** Longitud del campo */
    int m_length;
    QString m_lengthScript;
    /** Valor por defecto */
    QVariant m_defaultValue;
    /** Longitud de la parte entera si es un numero */
    int m_partI;
    /** Longitud de la parte decimal si es un numero */
    int m_partD;
    /** Indica si es autonumerico */
    bool m_serial;
    /** Indica si se visualiza en los modelos como columna */
    bool m_visibleGrid;
    /** Para campos cuyos valores estan fijos se utiliza esto: son los valores que ve el usuario y los que se almacenan en base de datos */
    QMap<QString, QString> m_optionsList;
    /** Para campos cuyos valores estan fijos se utiliza esto: son los valores que ve el usuario y los que se almacenan en base de datos */
    QMap<QString, QString> m_optionsIcons;
    /** Algunos campos pueden ser calculados */
    bool m_calculated;
    /** Pero ese cálculo puede que sólo se haga una vez y quede almacenado, para optimizar
      recursos */
    bool m_calculatedOneTime;
    /** Un campo calculado, podría querer guardarse en base de datos... Raro, pero puede ocurrir, si es fruto de un
      cálculo complejo y quiere ahorrarse trabajo en informes, por ejemplo */
    bool m_calculatedSaveOnDB;
    /** A veces es interesante que los cálculos se hagan sólo mientras el bean sea INSERT. Después ya no será
     * modificado */
    bool m_calculatedOnlyOnInsert;
    /** Con determinados campos calculados, puede ser deseable estar conectados a las modificaciones
      que se producen en campos de beans de relaciones dependientes. Esta opción puede causar
      un determinado proceso recursivo que puede producirse cuando hay vinculaciones entre campos calculados
      de los beans hijos y de este field, que se invocan de forma recursiva. Por ello, esta propiedad
      hay que utilizarla con suma cautela */
    bool m_calculatedConnectToChildModifications;
    /** Algunos campos podrán ser calculados, a partir de los datos del propio bean.
      El cálculo de esos campos se realiza mediante el script que almacena esta variable. */
    QString m_script;
    /** El campo calculado puede ser, un aggregate, esto es, su valor depende de otros beans como él
      que pertenecen al bean referencia. Por ejemplo, una suma de un determinado campo. Este campo
      marca si lo es, y sobre qué field realiza el cálculo */
    bool m_aggregate;
    /** Conjunto: relación, campo y filtro. El conjunto proporciona la cuenta */
    AggregateCalc m_aggregateCalc;
    /** Tipo de operación de agregado que se realiza. Puede ser suma (sum), media (avg), cuenta (count)... */
    QString m_aggregateOperation;
    bool m_coordinates;
    /** Si el campo es un String, indicará si almacena código HTML */
    bool m_html;
    /** Indica si el campo almacena una dirección de correo electrónico, lo que permitirá el envío de los mismos */
    bool m_email;
    /** Script a ejecutar para obtener el default value */
    QString m_scriptDefaultValue;
    BuiltInExpressionDef m_builtInExpressionDefaultValue;
    StringExpression m_builtInStringExpressionDefaultValue;
    /** Flag para activar el debug de los scripts de cálculo de este field */
    bool m_debug;
    /** Flag para activar el debug de los scripts de cálculo de este field */
    bool m_onInitDebug;
    QList<DBRelationMetadata *> m_relations;
    /** Para el cálculo de los campos calculados */
    AERPScript *m_engine;
    /** Indica si el campo es un memo. Es necesario que type sea QVariant::String */
    bool m_memo;
    /** Script para las reglas de validación antes de guardar en base de datos. Por ejemplo, esta regla de validación
      tiene en cuenta si la edad es mayor de 18
    <field>
    	<name>fecha_nacimiento</name>
    	<alias>Fecha de Nacimento</alias>
    	<null>false</null>
    	<pk>false</pk>
    	<type>date</type>
    	<visiblegrid>true</visiblegrid>
    	<validationRule>
    <![CDATA[
    // Esta función devuelve un array con lo siguientes 2 valores:
    // 1.- true o false si la validación es correcta
    // 2.- Mensaje formateados a presentar al usuario
    // Ej:
    // return [false, "<i>El cliente no tiene la edad suficiente</i>"];
    function validationRule() {
    var hoy=new Date();
    var edad;
    var ano = fecha.getFullYear();
    var mes = fecha.getMonth();
    var dia = fecha.getDay()
    edad=hoy.getFullYear() - 1- ano;
    if (hoy.getMonth() - mes > 0){
    	edad = edad+1;
    }

    if ((hoy.getMonth() == mes) && (hoy.getDay() - 1 - dia >= 0)){
    	edad = edad + 1
    }
    if (edad<18){
    	return [false, "<i>El cliente no tiene la edad suficiente</i>"];
    }else{
    	return [true, ""];
    }
    }
    ]]>
    	</validationRule>
    </field>
    </table>
    */
    /** Esta estructura permitirá definir el cálculo de campos calculados */
    AlephERP::DbFieldCounterDefinition m_counterDefinition;
    QString m_counterPrefix;
    /** Regla de validación específica en QScript */
    QString m_validationRuleScript;
    /** Indica si este campo se crea en los DBSearchDlg generados automáticamente */
    QVariant m_includeOnGeneratedSearchDlg;
    QVariant m_includeOnGeneratedRecordDlg;
    /** El programador QS puede describir un formato específico de visualización de los field. Lo hará mediante
      una función QS definida en los metadatos de la tabla y que se encuentra aquí */
    QString m_displayValueScript;
    /** Expresión sencilla para display */
    StringExpression m_displayValueExpression;
    /** Un field, definido dentro de un BaseBean puede no ser visible en un abstractview normal, pero su edición en abstractview con inlineEdit sí es relevante.
      Aquí se introduce esa definición. Se le define un comportamiento en paralelo
    <behaviourOnInlineEdit>
        <!-- ¿Qué se presenta en esa celda del abstract view cuando no se está editando? -->
        <viewOnRead>actividadesproductivas.nombre</viewOnRead>
        <!-- ¿Qué se presenta en esa celda del abstract view cuando se está editando? -->
        <viewOnEdit>DBChooseButtonRecord</viewOnEdit>
    </behaviourOnInlineEdit>
    */
    QHash<QString, QVariant> m_behaviourOnInlineEdit;
    /** Este field se presenta como un enlace que permite abrir, en un diálogo asociado, el registro. */
    bool m_link;
    /** La anterior apertura puede hacerse en modo lectura o escritura */
    bool m_linkOpenReadOnly;
    /** Para la presentación de los registros en abstracts views, puede ser interesante proporcionar un Tooltip adicional, que permita al usuario
      obtener datos interesantes. Este tooltip se calcula mediante una función de script, que se define aquí */
    QString m_toolTipScript;
    StringExpression m_toolTipExpression;
    /** Nombre del tipo de campo especificado en los metadatos */
    QString m_metadataTypeName;
    /** En ciertas ocasiones, es necesario obtener una representación del bean (por ejemplo, entre los items
     de un combobox, sin que un se haya definido ninguna información adicional. Se hace con esta propiedad.
     Se proporcionará una representación de tipo: campos marcados como showDefault separados por "-" */
    bool m_showDefault;
    bool m_hasDefaultValue;
    QString m_onChangeScript;
    BuiltInExpressionDef m_expression;
    StringExpression m_stringExpression;
    /** Cada objeto AERPBuiltInExpressionCalculator tendrá dentro una instancia del parseador, que guarda una copia en bytecode
     * de la expresión a calcular, para un rendimiento mayor */
    AERPBuiltInExpressionCalculator m_calc;
    bool m_orderField;
    bool m_scheduleStartTime;
    bool m_scheduleDuration;
    AlephERP::DateTimeParts m_scheduleUnit;
    /** En los metadatos es posible definir atajos, por ejemplo
    <field>
        <name>nombrearticulo</name>
        <expression>{stocks.father.articulos.father.nombre}</expression>
    </field>
    El asunto es que nadie garantiza que stocks o artículos estén ya cargados. Por eso
    almacenamos en un lugar temporal, y después consolidamos
    */
    QString m_tempStringExpression;

    QString m_envDefaultValue;

    /** Fuente a presentar en los grid */
    QFont m_fontOnGrid;
    QColor m_color;
    QColor m_backgroundColor;
    bool m_reloadFromDBAfterSave;
    /** Definición XML de la que proviene el campo */
    QString m_xmlDefinition;

    QStringList m_dependOnFieldsToCalc;

    DBFieldMetadataPrivate (DBFieldMetadata *qq) : q_ptr(qq)
    {
        m_index = 0;
        m_readOnly = false;
        m_dbIndex = false;
        m_null = false;
        m_primaryKey = false;
        m_type = QVariant::Invalid;
        m_length = 0;
        m_partI = 0;
        m_partD = 0;
        m_serial = false;
        m_calculated = false;
        m_calculatedOneTime = false;
        m_calculatedSaveOnDB = false;
        m_calculatedConnectToChildModifications = false;
        m_calculatedOnlyOnInsert = false;
        m_html = false;
        m_coordinates = false;
        m_visibleGrid = true;
        m_debug = false;
        m_aggregate = false;
        m_memo = false;
        m_onInitDebug = false;
        m_reloadFromDBAfterSave = false;
        m_unique = false;
        m_counterDefinition.userCanModified = false;
        m_counterDefinition.calculateOnlyOnInsert = false;
        m_counterDefinition.useTrailingZeros = true;
        m_link = false;
        m_linkOpenReadOnly = false;
        m_showDefault = false;
        m_hasDefaultValue = false;
        m_engine = NULL;
        m_email = false;
        m_orderField = false;
        m_scheduleStartTime = false;
        m_scheduleDuration = false;
        m_scheduleUnit = AlephERP::Hour;
        m_uniqueCompound = false;
    }

    QString fieldName();
};

void DBFieldMetadata::setTempStringExpression(const QString &value)
{
    d->m_tempStringExpression = value;
}

/**
 * @brief DBFieldMetadata::consolidateTemp
 * Para definiciones de campos con metadatos especiales y reglas, esta función consolida los datos temporales que se han ido
 * creando.
 */
void DBFieldMetadata::consolidateTemp()
{
    if ( !d->m_tempStringExpression.isEmpty() )
    {
        StringExpression exp;
        if ( !d->m_tempStringExpression.startsWith("{") )
        {
            d->m_tempStringExpression.prepend("{");
        }
        if ( !d->m_tempStringExpression.endsWith("}") )
        {
            d->m_tempStringExpression.append("}");
        }
        exp.expression = d->m_tempStringExpression;
        exp.valid = true;
        setBuiltInStringExpression(exp);
        setCalculated(true);
        setNull(true);
        d->m_tempStringExpression.replace("{", "").replace("}", "");
        QObject *obj = beanMetadata()->navigateThroughProperties(d->m_tempStringExpression, true);
        DBFieldMetadata *remoteFld = qobject_cast<DBFieldMetadata *>(obj);
        if ( remoteFld != NULL )
        {
            setType(remoteFld->type());
            if ( fieldName().isEmpty() )
            {
                setFieldName(remoteFld->fieldName());
            }
            setLength(remoteFld->length());
        }
    }
}

DBFieldMetadata::DBFieldMetadata(QObject *parent) : QObject(parent), d(new DBFieldMetadataPrivate(this))
{
    d->m_engine = new AERPScript(this);
    d->m_engine->setDebug(d->m_debug);
    d->m_engine->setOnInitDebug(d->m_onInitDebug);
}

DBFieldMetadata::~DBFieldMetadata()
{
    delete d;
}

int DBFieldMetadata::desiredLengthForType(QVariant::Type type)
{
    if ( type == QVariant::String )
    {
        return 250;
    }
    if ( type == QVariant::Double )
    {
        return 15;
    }
    if ( type == QVariant::Int )
    {
        return 15;
    }
    if ( type == QVariant::Date )
    {
        return 10;
    }
    if ( type == QVariant::DateTime )
    {
        return 19;
    }
    if ( type == QVariant::Bool )
    {
        return 1;
    }
    if ( type == QVariant::Pixmap )
    {
        return 4000;
    }
    return 0;
}

int DBFieldMetadata::index() const
{
    return d->m_index;
}

void DBFieldMetadata::setIndex(int value)
{
    d->m_index = value;
}

QString DBFieldMetadata::dbFieldName() const
{
    return d->m_dbFieldName;
}

void DBFieldMetadata::setDbFieldName(const QString &name)
{
    d->m_dbFieldName = name;
}

bool DBFieldMetadata::calculated() const
{
    return d->m_calculated;
}

void DBFieldMetadata::setCalculated(bool value)
{
    d->m_calculated = value;
}

QFont DBFieldMetadata::fontOnGrid() const
{
    return d->m_fontOnGrid;
}

void DBFieldMetadata::setFontOnGrid(const QFont &font)
{
    d->m_fontOnGrid = font;
}

QColor DBFieldMetadata::color() const
{
    return d->m_color;
}

void DBFieldMetadata::setColor(const QColor &color)
{
    d->m_color = color;
}

QColor DBFieldMetadata::backgroundColor() const
{
    return d->m_backgroundColor;
}

void DBFieldMetadata::setBackgroundColor(const QColor &color)
{
    d->m_backgroundColor = color;
}

bool DBFieldMetadata::reloadFromDBAfterSave() const
{
    return d->m_reloadFromDBAfterSave;
}

void DBFieldMetadata::setReloadFromDBAfterSave(bool v)
{
    d->m_reloadFromDBAfterSave = v;
}

QVariant DBFieldMetadata::includeOnGeneratedRecordDlg() const
{
    return d->m_includeOnGeneratedRecordDlg;
}

void DBFieldMetadata::setIncludeOnGeneratedRecordDlg(bool v)
{
    d->m_includeOnGeneratedRecordDlg = v;
}

QVariant DBFieldMetadata::includeOnGeneratedSearchDlg() const
{
    return d->m_includeOnGeneratedSearchDlg;
}

void DBFieldMetadata::setIncludeOnGeneratedSearchDlg(bool v)
{
    d->m_includeOnGeneratedSearchDlg = v;
}

QString DBFieldMetadata::displayValueScript() const
{
    return d->m_displayValueScript;
}

void DBFieldMetadata::setDisplayValueScript(QString v)
{
    d->m_displayValueScript = v;
}

StringExpression DBFieldMetadata::displayValueExpression() const
{
    return d->m_displayValueExpression;
}

void DBFieldMetadata::setDisplayValueExpression(const StringExpression &exp)
{
    d->m_displayValueExpression = exp;
}

bool DBFieldMetadata::calculatedOneTime() const
{
    return d->m_calculatedOneTime;
}

void DBFieldMetadata::setCalculatedOneTime(bool value)
{
    d->m_calculatedOneTime = value;
}

void DBFieldMetadata::setCalculatedSaveOnDb(bool value)
{
    d->m_calculatedSaveOnDB = value;
}

bool DBFieldMetadata::calculatedSaveOnDb() const
{
    return d->m_calculatedSaveOnDB;
}

void DBFieldMetadata::setCalculatedConnectToChildModifications(bool value)
{
    d->m_calculatedConnectToChildModifications = value;
}

bool DBFieldMetadata::calculatedConnectToChildModifications() const
{
    return d->m_calculatedConnectToChildModifications;
}

void DBFieldMetadata::setCalculatedOnlyOnInsert(bool value)
{
    d->m_calculatedOnlyOnInsert = value;
}

bool DBFieldMetadata::calculatedOnlyOnInsert() const
{
    return d->m_calculatedOnlyOnInsert;
}

QString DBFieldMetadata::script() const
{
    return d->m_script;
}

void DBFieldMetadata::setScript(const QString &value)
{
    d->m_script = value;
}

QString DBFieldMetadata::scriptDefaultValue() const
{
    return d->m_scriptDefaultValue;
}

void DBFieldMetadata::setScriptDefaultValue(const QString &string)
{
    d->m_hasDefaultValue = !string.isEmpty();
    d->m_scriptDefaultValue = string;
}

BuiltInExpressionDef DBFieldMetadata::builtInExpressionDefaultValue() const
{
    return d->m_builtInExpressionDefaultValue;
}

void DBFieldMetadata::setBuiltInExpressionDefaultValue(const BuiltInExpressionDef &string)
{
    d->m_hasDefaultValue = string.valid();
    d->m_builtInExpressionDefaultValue = string;
}

StringExpression DBFieldMetadata::builtInStringExpressionDefaultValue() const
{
    return d->m_builtInStringExpressionDefaultValue;
}

void DBFieldMetadata::setBuiltInStringExpressionDefaultValue(const StringExpression &string)
{
    d->m_hasDefaultValue = string.valid;
    d->m_builtInStringExpressionDefaultValue = string;
}

QString DBFieldMetadata::envDefaultValue() const
{
    return d->m_envDefaultValue;
}

void DBFieldMetadata::setEnvDefaultValue(const QString &value)
{
    d->m_envDefaultValue = value;
    d->m_hasDefaultValue = !value.isEmpty();
}

bool DBFieldMetadata::aggregate() const
{
    return d->m_aggregate;
}

void DBFieldMetadata::setAggregate(bool value)
{
    d->m_aggregate = value;
}

AggregateCalc DBFieldMetadata::aggregateCalc() const
{
    return d->m_aggregateCalc;
}

void DBFieldMetadata::setAggregateCalc(const AggregateCalc &calc)
{
    d->m_aggregateCalc = calc;
}

QString DBFieldMetadata::fieldName() const
{
    return d->fieldName();
}

void DBFieldMetadata::setFieldName(const QString &name)
{
    d->m_fieldName = trUtf8(name.toUtf8());
}

QString DBFieldMetadata::fieldNameScript() const
{
    return d->m_fieldNameScript;
}

void DBFieldMetadata::setFieldNameScript(const QString &value)
{
    d->m_fieldNameScript = value;
    d->m_calculatedFieldName.clear();
}

bool DBFieldMetadata::readOnly() const
{
    if ( hasCounterDefinition() )
    {
        if ( d->m_counterDefinition.userCanModified )
        {
            return false;
        }
    }
    if ( d->m_calculated )
    {
        return true;
    }
    return false;
}

bool DBFieldMetadata::dbIndex() const
{
    return d->m_dbIndex;
}

void DBFieldMetadata::setDbIndex(bool value)
{
    d->m_dbIndex = value;
}

bool DBFieldMetadata::html() const
{
    return d->m_html;
}

void DBFieldMetadata::setHtml(bool value)
{
    d->m_html = value;
}

bool DBFieldMetadata::coordinates() const
{
    return d->m_coordinates;
}

void DBFieldMetadata::setCoordinates(bool value)
{
    d->m_coordinates = value;
    if ( d->m_coordinates )
    {
        d->m_type = QVariant::String;
    }
}

bool DBFieldMetadata::email() const
{
    return d->m_email;
}

void DBFieldMetadata::setEmail(bool email)
{
    d->m_email = email;
}

bool DBFieldMetadata::debug() const
{
    return d->m_debug;
}

void DBFieldMetadata::setDebug(bool value)
{
    d->m_debug = value;
    d->m_engine->setDebug(d->m_debug);
}

bool DBFieldMetadata::onInitDebug()
{
    return d->m_onInitDebug;
}

void DBFieldMetadata::setOnInitDebug(bool value)
{
    d->m_onInitDebug = value;
    d->m_engine->setOnInitDebug(d->m_onInitDebug);
}

bool DBFieldMetadata::canBeNull() const
{
    return d->m_null;
}

void DBFieldMetadata::setNull(bool value)
{
    d->m_null = value;
}

bool DBFieldMetadata::primaryKey() const
{
    return d->m_primaryKey;
}

void DBFieldMetadata::setPrimaryKey(bool value)
{
    d->m_primaryKey = value;
}

bool DBFieldMetadata::unique() const
{
    return d->m_unique;
}

void DBFieldMetadata::setUnique(bool value)
{
    d->m_unique = value;
}

QString DBFieldMetadata::uniqueOnFilterField() const
{
    return d->m_uniqueOnFilterField;
}

void DBFieldMetadata::setUniqueOnFilterField(const QString &value)
{
    d->m_uniqueOnFilterField = value;
}

bool DBFieldMetadata::uniqueCompound() const
{
    return d->m_uniqueCompound;
}

void DBFieldMetadata::setUniqueCompound(bool value)
{
    d->m_uniqueCompound = value;
}

enum QVariant::Type DBFieldMetadata::type() const
{
    return d->m_type;
}

void DBFieldMetadata::setType(enum QVariant::Type value)
{
    d->m_type = value;
}

int DBFieldMetadata::length() const
{
    return d->m_length;
}

void DBFieldMetadata::setLength(int value)
{
    d->m_length = value;
}

QString DBFieldMetadata::lengthScript()
{
    return d->m_lengthScript;
}

void DBFieldMetadata::setLengthScript(const QString &value)
{
    d->m_lengthScript = value;
}

int DBFieldMetadata::partI() const
{
    return d->m_partI;
}

void DBFieldMetadata::setPartI(int value)
{
    d->m_partI = value;
}
int DBFieldMetadata::partD() const
{
    return d->m_partD;
}
void DBFieldMetadata::setPartD(int value)
{
    d->m_partD = value;
}

bool DBFieldMetadata::serial() const
{
    return d->m_serial;
}

void DBFieldMetadata::setSerial(bool value)
{
    d->m_serial = value;
}

bool DBFieldMetadata::visibleGrid() const
{
    return d->m_visibleGrid;
}

void DBFieldMetadata::setVisibleGrid(bool value)
{
    d->m_visibleGrid = value;
}

QString DBFieldMetadata::validationRuleScript() const
{
    return d->m_validationRuleScript;
}

void DBFieldMetadata::setValidationRuleScript(const QString &value)
{
    d->m_validationRuleScript = value;
}

bool DBFieldMetadata::memo() const
{
    return d->m_memo;
}

void DBFieldMetadata::setMemo(bool value)
{
    d->m_memo = value;
}

QMap<QString, QString> DBFieldMetadata::optionsList() const
{
    return d->m_optionsList;
}

void DBFieldMetadata::setOptionList(const QMap<QString, QString> &optionList)
{
    d->m_optionsList = optionList;
}

QMap<QString, QString> DBFieldMetadata::optionsIcons() const
{
    return d->m_optionsIcons;
}

void DBFieldMetadata::setOptionIcons(const QMap<QString, QString> &optionList)
{
    d->m_optionsIcons = optionList;
}

QHash<QString, QVariant> DBFieldMetadata::behaviourOnInlineEdit() const
{
    return d->m_behaviourOnInlineEdit;
}

void DBFieldMetadata::setBehaviourOnInlineEdit(const QHash<QString, QVariant> &value)
{
    d->m_behaviourOnInlineEdit = value;
}

bool DBFieldMetadata::link() const
{
    return d->m_link;
}

void DBFieldMetadata::setLink(bool value)
{
    d->m_link = value;
}

bool DBFieldMetadata::linkOpenReadOnly() const
{
    return d->m_linkOpenReadOnly;
}

void DBFieldMetadata::setLinkOpenReadOnly(bool value)
{
    d->m_linkOpenReadOnly = value;
}

QString DBFieldMetadata::toolTipScript() const
{
    return d->m_toolTipScript;
}

void DBFieldMetadata::setToolTipScript(const QString &value)
{
    d->m_toolTipScript = value;
}

StringExpression DBFieldMetadata::toolTipStringExpression() const
{
    return d->m_toolTipExpression;
}

void DBFieldMetadata::setToolTipStringExpression(const StringExpression &string)
{
    d->m_toolTipExpression = string;
}

QString DBFieldMetadata::metadataTypeName() const
{
    return d->m_metadataTypeName;
}

void DBFieldMetadata::setMetadataTypeName(const QString &value)
{
    d->m_metadataTypeName = value;
}

/**
 * @brief DBFieldMetadata::databaseType Devuelve el tipo asociado en la base de datos.
 * @param dialect Tipo de base de datos (SQLITE o PSQL)
 * @return
 */
QString DBFieldMetadata::databaseType(const QString &dialect) const
{
    QString type;
    if ( d->m_metadataTypeName == "string" || d->m_metadataTypeName == "password" )
    {
        type = QString("character varying(%1)").arg(d->m_length == 0 ? 1 : d->m_length);
    }
    else if ( d->m_metadataTypeName == "stringlist" )
    {
        if ( dialect == QString("QIBASE") )
        {
            type = QString("BLOB SUB_TYPE TEXT");
        }
        else
        {
            type = QString("text");
        }
    }
    else if ( d->m_metadataTypeName == "double" )
    {
        type = QString("double precision");
    }
    else if ( d->m_metadataTypeName == "int" || d->m_metadataTypeName == "integer" )
    {
        /* De la documentación de SQLIte: http://www.sqlite.org/lang_createtable.html
         * With one exception, if a table has a primary key that consists of a single column, and the declared type of that
         * column is "INTEGER" in any mixture of upper and lower case, then the column becomes an alias for the rowid.
         * Such a column is usually referred to as an "integer primary key". A PRIMARY KEY column only becomes an integer primary
         * key if the declared type name is exactly "INTEGER". Other integer type names like "INT" or "BIGINT" or "SHORT INTEGER" or
         * "UNSIGNED INTEGER" causes the primary key column to behave as an ordinary table column with integer affinity and a
         * unique index, not as an alias for the rowid.
         * Y esto genera un problema con las foreign key al asumirse como alias de la rowid. Las foreign key no se pueden asociar a un rowid. */
        type = QString("integer");
        if ( dialect == "QSQLITE" )
        {
            if ( d->m_primaryKey )
            {
                type = QString("int");
            }
        }
    }
    else if ( d->m_metadataTypeName == "serial" )
    {
        /* Por la razón anterior, los campos serial sólo se adminten para primary keys. SQLIte los asocia a un rowid.
         * Además, en relaciones de integridad referencial, no se pueden utilizar rowid. Es por ello que no utilizaremos
         * serial en SQL generando las sequencias de forma a mano, desde AlephERP */
        if ( dialect == "QSQLITE" )
        {
            if ( d->m_primaryKey )
            {
                type = QString("INTEGER PRIMARY KEY AUTOINCREMENT");
            }
            else
            {
                type = QString("int");
            }
        }
        else if ( dialect == "QIBASE")
        {
            type = QString("int");
        }
        else
        {
            type = QString("serial");
        }
    }
    else if ( d->m_metadataTypeName == "date" )
    {
        type = QString("date");
    }
    else if ( d->m_metadataTypeName == "datetime" )
    {
        type = QString("timestamp");
    }
    else if ( d->m_metadataTypeName == "bool" || d->m_metadataTypeName == "boolean" )
    {
        type = QString("boolean");
    }
    else if ( d->m_metadataTypeName == "image" )
    {
        if ( dialect == "QIBASE" )
        {
            type = QString("BLOB");
        }
        else
        {
            type = QString("bytea");
        }
    }
    return type;
}

bool DBFieldMetadata::checkDatabaseType(const QString &databaseColumnType, const QString &dialect, QString &err)
{
    Q_UNUSED (dialect)
    if ( type() == QVariant::String )
    {
        if ( memo() && databaseColumnType != "text" )
        {
            err = trUtf8("%1: La columna %2 es de tipo memo en los metadatos pero en la base de datos es de tipo %3 ").
                  arg(beanMetadata()->tableName()).
                  arg(dbFieldName()).arg(databaseColumnType);
            return false;
        }
        if ( !memo() && databaseColumnType != "character varying" )
        {
            err = trUtf8("%1: La columna %2 es de tipo string en los metadatos pero en la base de datos es de tipo %3 ").
                  arg(beanMetadata()->tableName()).
                  arg(dbFieldName()).arg(databaseColumnType);
            return false;
        }
    }
    if ( type() == QVariant::Double )
    {
        if ( databaseColumnType != "double precision" && databaseColumnType != "numeric" )
        {
            err = trUtf8("%1: La columna %2 es de tipo double en los metadatos pero en la base de datos es de tipo %3 ").
                  arg(beanMetadata()->tableName()).
                  arg(dbFieldName()).arg(databaseColumnType);
            return false;
        }
    }
    if ( type() == QVariant::Int )
    {
        if ( databaseColumnType != "integer" )
        {
            err = trUtf8("%1: La columna %2 es de tipo int en los metadatos pero en la base de datos es de tipo %3 ").
                  arg(beanMetadata()->tableName()).
                  arg(dbFieldName()).arg(databaseColumnType);
            return false;
        }
    }
    if ( type() == QVariant::Date )
    {
        if ( databaseColumnType != "date"  && !databaseColumnType.contains("timestamp") )
        {
            err = trUtf8("%1: La columna %2 es de tipo date en los metadatos pero en la base de datos es de tipo %3 ").
                  arg(beanMetadata()->tableName()).
                  arg(dbFieldName()).arg(databaseColumnType);
            return false;
        }
    }
    if ( type() == QVariant::DateTime )
    {
        if ( !databaseColumnType.contains("timestamp") )
        {
            err = trUtf8("%1: La columna %2 es de tipo date en los metadatos pero en la base de datos es de tipo %3 ").
                  arg(beanMetadata()->tableName()).
                  arg(dbFieldName()).arg(databaseColumnType);
            return false;
        }
    }
    if ( type() == QVariant::Bool )
    {
        if ( databaseColumnType != "boolean" )
        {
            err = trUtf8("%1: La columna %2 es de tipo bool en los metadatos pero en la base de datos es de tipo %3 ").
                  arg(beanMetadata()->tableName()).
                  arg(dbFieldName()).arg(databaseColumnType);
            return false;
        }
    }
    if ( type() == QVariant::Pixmap )
    {
        if ( databaseColumnType != "bytea" )
        {
            err = trUtf8("%1: La columna %2 es de tipo image en los metadatos pero en la base de datos es de tipo %3 ").
                  arg(beanMetadata()->tableName()).
                  arg(dbFieldName()).arg(databaseColumnType);
            return false;
        }
    }
    return true;
}

bool DBFieldMetadata::showDefault() const
{
    return d->m_showDefault;
}

void DBFieldMetadata::setShowDefault(bool value)
{
    d->m_showDefault = value;
}

bool DBFieldMetadata::hasDefaultValue() const
{
    return d->m_hasDefaultValue;
}

QString DBFieldMetadata::onChangeScript() const
{
    return d->m_onChangeScript;
}

void DBFieldMetadata::setOnChangeScript(const QString &value)
{
    d->m_onChangeScript = value;
}

BuiltInExpressionDef DBFieldMetadata::builtInExpression() const
{
    return d->m_expression;
}

void DBFieldMetadata::setBuiltInExpression(const BuiltInExpressionDef &def)
{
    d->m_expression = def;
}

StringExpression DBFieldMetadata::builtInStringExpression() const
{
    return d->m_stringExpression;
}

void DBFieldMetadata::setBuiltInStringExpression(const StringExpression &def)
{
    d->m_stringExpression = def;
}

bool DBFieldMetadata::orderField() const
{
    return d->m_orderField;
}

void DBFieldMetadata::setOrderField(bool value)
{
    d->m_orderField = value;
}

bool DBFieldMetadata::scheduleStartTime() const
{
    return d->m_scheduleStartTime;
}

void DBFieldMetadata::setScheduleStartTime(bool value)
{
    d->m_scheduleStartTime = value;
}

bool DBFieldMetadata::scheduleDuration() const
{
    return d->m_scheduleDuration;
}

void DBFieldMetadata::setScheduleDuration(bool value)
{
    d->m_scheduleDuration = value;
}

AlephERP::DateTimeParts DBFieldMetadata::scheduleTimeUnit() const
{
    return d->m_scheduleUnit;
}

void DBFieldMetadata::setScheduleTimeUnit(AlephERP::DateTimeParts value)
{
    d->m_scheduleUnit = value;
}

QString DBFieldMetadata::xmlDefinition() const
{
    return d->m_xmlDefinition;
}

Qt::Alignment DBFieldMetadata::alignment() const
{
    if ( type() == QVariant::Int || type() == QVariant::Double )
    {
        return Qt::AlignRight | Qt::AlignVCenter;
    }
    else if ( type() == QVariant::String )
    {
        return Qt::AlignLeft | Qt::AlignVCenter;
    }
    else if ( type() == QVariant::Bool )
    {
        return Qt::AlignCenter | Qt::AlignVCenter;
    }
    return Qt::AlignRight | Qt::AlignVCenter;
}

QStringList DBFieldMetadata::dependOnFieldsToCalc() const
{
    return d->m_dependOnFieldsToCalc;
}

void DBFieldMetadata::setDependOnFieldsToCalc(const QStringList &data)
{
    d->m_dependOnFieldsToCalc = data;
}

bool DBFieldMetadata::isOnDb() const
{
    if ( !d->m_calculated )
    {
        return true;
    }
    return d->m_calculatedSaveOnDB;
}

void DBFieldMetadata::setXmlDefinition(const QString &value)
{
    d->m_xmlDefinition = value;
}

/**
 * @brief DBFieldMetadata::ddlCreationTable Devuelve la línea de creación del campo dentro de un CREATE TABLE
 * @param dialect
 * @return
 */
QString DBFieldMetadata::ddlCreationTable(const AlephERP::CreationTableSqlOptions &options, const QString &dialect)
{
    QString notNull = d->m_null ? "" : "NOT NULL";
    QString ddl = QString("%1 %2 %3").arg(d->m_dbFieldName).arg(databaseType(dialect)).arg(notNull);
    QString references;

    if ( options.testFlag(AlephERP::WithForeignKeys) && options.testFlag(AlephERP::ForeignKeysOnTableCreation) )
    {
        foreach ( DBRelationMetadata *rel, d->m_relations )
        {
            if ( rel->type() == DBRelationMetadata::MANY_TO_ONE )
            {
                // Además, la tabla relacionada, DEBE existir
                BaseBeanMetadata *related = BeansFactory::metadataBean(rel->tableName());
                if ( related != NULL )
                {
                    QString deleteCascade;
                    if ( rel->deleteCascade() )
                    {
                        deleteCascade = " on delete cascade";
                    }
                    references = QString("references %1(%2) on update cascade%3").
                                 arg(rel->tableName()).
                                 arg(rel->childFieldName()).
                                 arg(deleteCascade);
                }
                else
                {
                    qDebug() << "DBFieldMetadata::ddlCreationTable: No existe la tabla: [" << rel->tableName() << "]";
                }
            }
        }
        if ( !references.isEmpty() )
        {
            ddl = QString("%1 %2").arg(ddl).arg(references);
        }
    }
    return ddl;
}

QList<DBRelationMetadata *> DBFieldMetadata::relations(const AlephERP::RelationTypes &type)
{
    if ( type.testFlag(AlephERP::All) )
    {
        return d->m_relations;
    }
    QList<DBRelationMetadata *> rels;
    foreach ( DBRelationMetadata *rel, d->m_relations )
    {
        if ( type.testFlag(AlephERP::OneToMany) && rel->type() == DBRelationMetadata::ONE_TO_MANY )
        {
            rels << rel;
        }
        else if ( type.testFlag(AlephERP::ManyToOne) && rel->type() == DBRelationMetadata::MANY_TO_ONE )
        {
            rels << rel;
        }
        else if ( type.testFlag(AlephERP::OneToOne) && rel->type() == DBRelationMetadata::ONE_TO_ONE )
        {
            rels << rel;
        }
    }
    return rels;

}

void DBFieldMetadata::addRelation(DBRelationMetadata *rel)
{
    d->m_relations.append(rel);
    /** Aquí exponemos las relaciones al motor Qs */
    QVariant pointer = QVariant::fromValue(rel);
    QByteArray ba = rel->name().toUtf8();
    setProperty(ba.constData(), pointer);
}

BaseBeanMetadata *DBFieldMetadata::beanMetadata() const
{
    return qobject_cast<BaseBeanMetadata *>(parent());
}

/*!
  Formatea la salida de data, según la configuración del campo
  */
QString DBFieldMetadata::displayValue(QVariant data, DBField *parent)
{
    QScriptValue value, displayValue;
    QString result;

    if ( parent != NULL )
    {
        if ( parent->isExecuting(AlephERP::DisplayValue) )
        {
            qDebug() << "DBFieldMetadata::displayValue: EXISTEN LLAMADAS RECURSIVAS EN EL CODIGO: bean: [" <<
                     parent->bean()->metadata()->tableName() << "]. Field: [" << parent->metadata()->dbFieldName() << "].";
            if ( !d->m_displayValueScript.isEmpty() )
            {
                AERPScript::printScriptsStack();
            }
            return "";
        }
        parent->setOnExecution(AlephERP::DisplayValue);
    }
    if ( type() == QVariant::Int )
    {
        displayValue = QScriptValue(alephERPSettings->locale()->toString(data.toInt()));
        value = QScriptValue(data.toInt());
    }
    else if ( type() == QVariant::Double )
    {
        displayValue = QScriptValue(alephERPSettings->locale()->toString(data.toDouble(), 'f', partD()));
        value = QScriptValue(data.toDouble());
    }
    else if ( type() == QVariant::Date )
    {
        displayValue = QScriptValue(alephERPSettings->locale()->toString(data.toDate(), CommonsFunctions::dateFormat()));
        value = d->m_engine->createScriptValue(data.toDateTime());
    }
    else if ( type() == QVariant::DateTime )
    {
        displayValue = QScriptValue(alephERPSettings->locale()->toString(data.toDateTime(), CommonsFunctions::dateTimeFormat()));
        value = d->m_engine->createScriptValue(data.toDateTime());
    }
    else
    {
        displayValue = QScriptValue(data.toString());
        value = QScriptValue(displayValue);
    }
    if ( d->m_displayValueExpression.valid )
    {
        QHash<QString, QString> otherEntries;
        otherEntries["displayValue"] = displayValue.toString();
        result = d->m_displayValueExpression.applyExpressionRules(parent, otherEntries);
    }
    else if ( !d->m_displayValueScript.isEmpty() )
    {
        if ( parent != NULL )
        {
            d->m_engine->addFunctionArgument("bean", parent->bean());
            d->m_engine->addFunctionArgument("dbField", parent);
        }
        else
        {
            d->m_engine->addFunctionArgument("bean", NULL);
            d->m_engine->addFunctionArgument("dbField", NULL);
        }
        d->m_engine->addFunctionArgument("value", value);
        d->m_engine->addFunctionArgument("displayValue", displayValue);
        d->m_engine->setScript(d->m_displayValueScript, QString("%1.%2.displayValue.js").arg(beanMetadata()->tableName()).arg(d->m_dbFieldName));
        QVariant scriptResult = d->m_engine->toVariant(d->m_engine->callQsFunction(QString("displayValueScript")));
        if ( d->m_engine->lastMessage().isEmpty() )
        {
            result = scriptResult.toString();
        }
    }
    else
    {
        result = displayValue.toString();
    }
    if ( parent != NULL )
    {
        parent->restoreOverrideOnExecution();
    }
    return result;
}

int DBFieldMetadata::calculateLength(DBField *parent)
{
    QVariant data;
    int value = d->m_length;

    if ( parent->isExecuting(AlephERP::CalculateLength) )
    {
        qDebug() << "DBFieldMetadata::calculateLength: EXISTEN LLAMADAS RECURSIVAS EN EL CODIGO: bean: [" <<
                 parent->bean()->metadata()->tableName() << "]. Field: [" << parent->metadata()->dbFieldName() << "].";
        if ( !d->m_lengthScript.isEmpty() )
        {
            AERPScript::printScriptsStack();
        }
        return value;
    }
    parent->setOnExecution(AlephERP::CalculateLength);
    parent->registerCalculatingOnBean();
    if ( !d->m_lengthScript.isEmpty() )
    {
        d->m_engine->addFunctionArgument("bean", parent->bean());
        d->m_engine->addFunctionArgument("dbField", parent);
        d->m_engine->setScript(d->m_lengthScript, QString("%1.%2.length.js").arg(beanMetadata()->tableName()).arg(d->m_dbFieldName));
        data = d->m_engine->toVariant(d->m_engine->callQsFunction(QString("length")));
        if ( d->m_type != QVariant::Invalid )
        {
            bool ok;
            value = data.toInt(&ok);
            if ( !ok && value <= d->m_length )
            {
                value = d->m_length;
            }
        }
    }
    parent->unregisterCalculatingOnBean();
    parent->restoreOverrideOnExecution();

    return value;
}

/*!
  Valor por defecto del campo cuando se crea un nuevo bean
  */
QVariant DBFieldMetadata::calculateDefaultValue(DBField *parent)
{
    QElapsedTimer timer;
    QVariant data;
    if ( parent->isExecuting(AlephERP::DefaultValue) )
    {
        qDebug() << "DBFieldMetadata::defaultValue: EXISTEN LLAMADAS RECURSIVAS EN EL CODIGO: bean: [" <<
                 parent->bean()->metadata()->tableName() << "]. Field: [" << parent->metadata()->dbFieldName() << "].";
        if ( !d->m_scriptDefaultValue.isEmpty() )
        {
            AERPScript::printScriptsStack();
        }
        return data;
    }
    timer.start();
    parent->setOnExecution(AlephERP::DefaultValue);
    parent->registerCalculatingOnBean();
    if ( d->m_builtInExpressionDefaultValue.valid() )
    {
        if ( d->m_calc.isWorking() )
        {
            data = parent->rawValue();
        }
        else
        {
            // En una expresión calculada, si el tipo está no definido, pasa a ser double
            if ( d->m_type == QVariant::Invalid )
            {
                d->m_type = QVariant::Double;
            }
            d->m_calc.setFieldType(d->m_type);
            d->m_calc.setField(parent);
            d->m_calc.setExpression(d->m_builtInExpressionDefaultValue);
            data = d->m_calc.value();
        }
    }
    else if ( d->m_builtInStringExpressionDefaultValue.valid )
    {
        if ( d->m_type == QVariant::Invalid )
        {
            d->m_type = QVariant::String;
        }
        data = d->m_builtInStringExpressionDefaultValue.applyExpressionRules(parent);
    }
    else if ( !d->m_scriptDefaultValue.isEmpty() )
    {
        d->m_engine->addFunctionArgument("bean", parent->bean());
        d->m_engine->addFunctionArgument("dbField", parent);
        d->m_engine->setScript(d->m_scriptDefaultValue, QString("%1.%2.defaultValue.js").arg(beanMetadata()->tableName()).arg(d->m_dbFieldName));
        data = d->m_engine->toVariant(d->m_engine->callQsFunction(QString("defaultValue")));
        if ( d->m_type == QVariant::Invalid )
        {
            d->m_type = data.type();
        }
        QString temp = data.toString();
        if ( temp.toLower() == "nan" )
        {
            data = QVariant();
        }
    }
    else if ( !d->m_envDefaultValue.isEmpty() )
    {
        data = EnvVars::instance()->var(d->m_envDefaultValue);
    }
    else if ( hasCounterDefinition() )
    {
        data = parent->calculateCounter();
    }
    else
    {
        data = d->m_defaultValue;
    }
    parent->unregisterCalculatingOnBean();
    parent->restoreOverrideOnExecution();
    qint64 elapsed = timer.elapsed();
    QLogger::QLog_Debug(AlephERP::stLogOther, QString("DBFieldMetadata::calculateDefaultValue: fieldName: %1.%2. Calculate Aggregate: [%3] ms").
               arg(parent->bean()->metadata()->tableName()).
               arg(d->m_dbFieldName).
               arg(elapsed));
    if ( elapsed > AlephERP::warningCalculatedTime )
    {
        QLogger::QLog_Info(AlephERP::stLogScript, "DBFieldMetadata::calculateDefaultValue: Ha tomado mucho tiempo!!!");
    }
    return data;
}

QVariant DBFieldMetadata::defaultValue() const
{
    return d->m_defaultValue;
}

void DBFieldMetadata::setDefaultValue(const QVariant &value)
{
    d->m_defaultValue = value;
    d->m_hasDefaultValue = value.isValid();
}

/**
 * @brief DBFieldMetadata::calculateValue
 * Calcula el valor del campo... Si hay definida una expresión interna, ésta tendrá preferencia sobre el script.
 * @param fld
 * @return
 */
QVariant DBFieldMetadata::calculateValue(DBField *fld)
{
    QVariant data;
    QElapsedTimer timer;

    // Al ser un campo calculado, ejecutamos el script asociado. Este se ejecuta
    // sobre el bean, de modo, que parent debe estar visible en el script
    if ( fld->isExecuting(AlephERP::CalculateValue) )
    {
        qDebug() << "DBFieldMetadata::calculateValue: EXISTEN LLAMADAS RECURSIVAS EN EL CODIGO: bean: [" <<
                 fld->bean()->metadata()->tableName() << "]. Field: [" << fld->metadata()->dbFieldName() << "].";
        if ( !d->m_script.isEmpty() )
        {
            AERPScript::printScriptsStack();
        }
        return fld->rawValue();
    }
    timer.start();
    fld->setOnExecution(AlephERP::CalculateValue);

    if ( d->m_expression.valid() )
    {
        fld->registerCalculatingOnBean();
        if ( d->m_calc.isWorking() )
        {
            data = fld->rawValue();
        }
        else
        {
            if ( d->m_type == QVariant::Invalid )
            {
                d->m_type = QVariant::Double;
            }
            d->m_calc.setFieldType(d->m_type);
            d->m_calc.setField(fld);
            data = d->m_calc.value();
        }
        fld->unregisterCalculatingOnBean();
    }
    else if ( d->m_stringExpression.valid )
    {
        if ( d->m_type == QVariant::Invalid )
        {
            d->m_type = QVariant::String;
        }
        data = d->m_stringExpression.applyExpressionRules(fld);
    }
    else
    {
        d->m_engine->addFunctionArgument("bean", fld->bean());
        d->m_engine->addFunctionArgument("dbField", fld);
        d->m_engine->setScript(d->m_script, QString("%1.%2.value.js").arg(beanMetadata()->tableName()).arg(d->m_dbFieldName));
        fld->registerCalculatingOnBean();
        data = d->m_engine->toVariant(d->m_engine->callQsFunction(QString("value")));
        if ( d->m_type == QVariant::Invalid )
        {
            d->m_type = data.type();
        }
        fld->unregisterCalculatingOnBean();
    }
    fld->restoreOverrideOnExecution();
    qint64 elapsed = timer.elapsed();
    QLogger::QLog_Debug(AlephERP::stLogOther, QString("DBFieldMetadata::calculateValue: fieldName: %1.%2. Calculate Value: [%3] ms").
               arg(fld->bean()->metadata()->tableName()).
               arg(d->m_dbFieldName).
               arg(elapsed));
    if ( elapsed > AlephERP::warningCalculatedTime )
    {
        QLogger::QLog_Info(AlephERP::stLogScript, "DBFieldMetadata::calculateValue: Ha tomado mucho tiempo!!!");
    }
    return data;
}

QVariant DBFieldMetadata::calculateAggregateScript(DBField *fld, const QString &script, BaseBean *relationChildRecord)
{
    // Al ser un campo calculado, ejecutamos el script asociado. Este se ejecuta
    // sobre el bean, de modo, que parent debe estar visible en el script
    QVariant data;
    if ( fld->isExecuting(AlephERP::AggregateValue) )
    {
        QLogger::QLog_Warning(AlephERP::stLogScript, QString("DBFieldMetadata::calculateAggregateScript: EXISTEN LLAMADAS RECURSIVAS EN EL CODIGO: bean: [%1]. Field: [%2]").
                              arg(fld->bean()->metadata()->tableName()).arg(fld->metadata()->dbFieldName()));
        AERPScript::printScriptsStack();
        return fld->rawValue();
    }
    fld->setOnExecution(AlephERP::AggregateValue);
    d->m_engine->addFunctionArgument("bean", fld->bean());
    d->m_engine->addFunctionArgument("dbField", fld);
    if ( relationChildRecord != NULL )
    {
        d->m_engine->addFunctionArgument("relationChildBean", relationChildRecord);
    }
    d->m_engine->setScript(script, QString("%1.%2.value.js").arg(beanMetadata()->tableName()).arg(d->m_dbFieldName));
    fld->registerCalculatingOnBean();
    data = d->m_engine->toVariant(d->m_engine->callQsFunction(QString("value")));
    fld->unregisterCalculatingOnBean();
    fld->restoreOverrideOnExecution();
    return data;
}

QString DBFieldMetadata::toolTip(DBField *parent)
{
    if ( d->m_toolTipScript.isEmpty() && !d->m_toolTipExpression.valid )
    {
        return QString();
    }
    QVariant data;
    if ( parent->isExecuting(AlephERP::ToolTip) )
    {
        qDebug() << "DBFieldMetadata::toolTip: EXISTEN LLAMADAS RECURSIVAS EN EL CODIGO: bean: [" <<
                 parent->bean()->metadata()->tableName() << "]. Field: [" << parent->metadata()->dbFieldName() << "].";
        AERPScript::printScriptsStack();
        return "";
    }
    parent->setOnExecution(AlephERP::ToolTip);
    if ( d->m_toolTipExpression.valid )
    {
        QHash<QString, QString> otherEntries;
        otherEntries["displayValue"] = parent->displayValue();
        data = d->m_toolTipExpression.applyExpressionRules(parent, otherEntries);
    }
    else
    {
        d->m_engine->addFunctionArgument("bean", parent->bean());
        d->m_engine->addFunctionArgument("dbField", parent);
        d->m_engine->setScript(d->m_toolTipScript, QString("%1.%2.toolTip.js").arg(beanMetadata()->tableName()).arg(d->m_dbFieldName));
        data = d->m_engine->toVariant(d->m_engine->callQsFunction(QString("toolTip")));
    }
    parent->restoreOverrideOnExecution();
    return data.toString();
}

/*!
  Esta función formatea value de cara a que sea insertable en una SQL, obviando locales y demás.
  Así un string sería devuelto como 'Hola' con comillas incluídas
  */
QString DBFieldMetadata::sqlValue(const QVariant &value, bool includeQuotes, const QString &dialect)
{
    QString driverName;
    if ( dialect.isEmpty() )
    {
        driverName = QSqlDatabase::database(Database::databaseConnectionForThisThread()).driverName();
    }
    else
    {
        driverName = dialect;
    }
    QString result;
    if ( type() == QVariant::Int )
    {
        result = QString("%1").arg(value.toInt());
    }
    else if ( type() == QVariant::Double )
    {
        result = QString("%1").arg(value.toDouble());
    }
    else if ( type() == QVariant::Date )
    {
        QDate fecha = value.toDate();
        if ( includeQuotes )
        {
            result = QString("\'%1\'").arg(fecha.toString("yyyy-MM-dd"));
        }
        else
        {
            result = fecha.toString("yyyy-MM-dd");
        }
    }
    else if ( type() == QVariant::DateTime )
    {
        QDate fecha = value.toDate();
        if ( includeQuotes )
        {
            result = QString("\'%1\'").arg(fecha.toString("yyyy-MM-dd HH:mm:ss"));
        }
        else
        {

            result = fecha.toString("yyyy-MM-dd HH:mm:ss");
        }
    }
    else if ( type() == QVariant::Bool )
    {
        if ( driverName != "QIBASE" )
        {
            result = (value.toBool() ? QString("true") : QString("false"));
        }
        else
        {
            result = (value.toBool() ? QString("1") : QString("0"));
        }
    }
    else if ( type() == QVariant::String )
    {
        // Debemos escapar el carácter '
        result = value.toString().replace("\'", "\'\'");
        if ( includeQuotes )
        {
            result = QString("\'%1\'").arg(result);
        }
    }
    else
    {
        qCritical() << "sqlValue: DBField: " << dbFieldName() << ". No tiene definido un tipo de datos. Asignando el tipo por defecto.";
        if ( includeQuotes )
        {
            result = QString("\'%1\'").arg(value.toString());
        }
        else
        {
            result = value.toString();
        }
    }
    return result;
}

/**
 * @brief DBFieldMetadata::sqlNullCondition
 * Devuelve una cláusula SQL para incluir en un WHERE donde el campo sea nulo
 * @param dialect
 * @return
 */
QString DBFieldMetadata::sqlNullCondition(const QString &dialect)
{
    Q_UNUSED(dialect)
    QString sql;
    if ( type() == QVariant::Int || type() == QVariant::Double )
    {
        sql = QString("(%1 IS NULL OR %1 = 0)").arg(d->m_dbFieldName);
    }
    else if ( type() == QVariant::Date || type() == QVariant::DateTime || type() == QVariant::Bool )
    {
        sql = QString("(%1 IS NULL)").arg(d->m_dbFieldName);
    }
    else if ( type() == QVariant::String )
    {
        sql = QString("(%1 IS NULL OR %1 = \'%1\')").arg(d->m_dbFieldName);
    }
    return sql;
}

/*!
  Proporciona una claúsula where para la el campo. Útil para updates y deletes
  */
QString DBFieldMetadata::sqlWhere(const QString &op, const QVariant &value, const QString &dialect)
{
    QString result;

    if ( dialect == "QSQLITE" && op == "=" && d->m_type == QVariant::Bool )
    {
        if ( value.toBool() == true )
        {
            result = QString("%1 > 0").arg(dbFieldName());
        }
        else
        {
            result = QString("%1 = 0").arg(dbFieldName());
        }
    }
    else
    {
        result = QString("%1 %2 %3").arg(dbFieldName()).
                 arg(op).arg(sqlValue(value, true, dialect));
    }
    return result;
}

/*!
  Ejecuta la regla de validación asociada al DBField si hay alguna
  */
QString DBFieldMetadata::validateRule(DBField *parent, bool &validate)
{
    // Al ser un campo calculado, ejecutamos el script asociado. Este se ejecuta
    // sobre el bean, de modo, que parent debe estar visible en el script
    QString message = "";
    if ( d->m_validationRuleScript.isEmpty() )
    {
        validate = true;
        return message;
    }
    if ( parent->isExecuting(AlephERP::ValidateRule) )
    {
        qDebug() << "DBFieldMetadata::validateRule: EXISTEN LLAMADAS RECURSIVAS EN EL CODIGO: bean: [" <<
                 parent->bean()->metadata()->tableName() << "]. Field: [" << parent->metadata()->dbFieldName() << "].";
        AERPScript::printScriptsStack();
        validate = true;
        return "";
    }
    parent->setOnExecution(AlephERP::ValidateRule);
    d->m_engine->addFunctionArgument("bean", parent->bean());
    d->m_engine->addFunctionArgument("dbField", parent);
    d->m_engine->setScript(d->m_validationRuleScript, QString("%1.%2.validationRule.js").arg(beanMetadata()->tableName()).arg(d->m_dbFieldName));
    QVariant temp = d->m_engine->toVariant(d->m_engine->callQsFunction(QString("validationRule")));
    if ( temp.isValid() )
    {
        if ( temp.type() == QVariant::List )
        {
            QVariantList list = temp.toList();
            if ( list.size() < 2 )
            {
                validate = false;
            }
            else
            {
                validate = list.at(0).toBool();
                message = list.at(1).toString();
            }
        }
        else
        {
            validate = temp.toBool();
        }
    }
    parent->restoreOverrideOnExecution();
    return message;
}

void DBFieldMetadata::onChangeEvent(DBField *parent)
{
    if ( d->m_onChangeScript.isEmpty() )
    {
        return;
    }
    if ( parent->isExecuting(AlephERP::OnChangeFieldValue) )
    {
        qDebug() << "DBFieldMetadata::onChangeEvent: EXISTEN LLAMADAS RECURSIVAS EN EL CODIGO: bean: [" <<
                 parent->bean()->metadata()->tableName() << "]. Field: [" << parent->metadata()->dbFieldName() << "].";
        AERPScript::printScriptsStack();
        return;
    }
    parent->setOnExecution(AlephERP::OnChangeFieldValue);
    d->m_engine->addFunctionArgument("bean", parent->bean());
    d->m_engine->addFunctionArgument("dbField", parent);
    d->m_engine->setScript(d->m_onChangeScript, QString("%1.%2.onChange.js").arg(beanMetadata()->tableName()).arg(d->m_dbFieldName));
    d->m_engine->callQsFunction(QString("onChange"));
    parent->restoreOverrideOnExecution();
    return;
}

/** Permite, a partir de la representación de un valor como cadena de caracteres, devolver un QVariant
  del valor en el que se traduce esa cadena, según el tipo del DBField. Es una función inversa a sqlValue */
QVariant DBFieldMetadata::parseValue(const QString &v)
{
    QVariant result;
    QString value = v.trimmed();
    if ( type() == QVariant::Int )
    {
        int temp = value.toInt();
        result = QVariant(temp);
    }
    else if ( type() == QVariant::Double )
    {
        double temp = value.toDouble();
        result = QVariant(temp);
    }
    else if ( type() == QVariant::Date )
    {
        QDate fecha = QDate::fromString(value, Qt::ISODate);
        result = QVariant(fecha);
    }
    else if ( type() == QVariant::DateTime )
    {
        QDate fecha = QDate::fromString(value, Qt::ISODate);
        result = QVariant(fecha);
    }
    else if ( type() == QVariant::Bool )
    {
        result = QVariant(value.toLower() == "true" ? true : false);
    }
    else if ( type() == QVariant::String )
    {
        result = QVariant(value);
    }
    else
    {
        qCritical() << "parseValue: DBField: " << dbFieldName() << ". No tiene definido un tipo de datos. Asignando el tipo por defecto.";
        result = QVariant(value);
    }
    return result;
}

/**
 * @brief DBFieldMetadata::nextSerial
 * Devuelve el siguiente valor de una sequencia. Esta función es útil para bases de datos que no soportan secuencias,
 * como SQLite
 * @return
 */
int DBFieldMetadata::nextSerial(DBField *fld)
{
    QString sql = QString("SELECT max(%1) as column1 FROM %2").arg(d->m_dbFieldName).arg(fld->bean()->metadata()->sqlTableName());
    QVariant result;
    if ( !BaseDAO::execute(sql, result) )
    {
        qDebug() << "DBFieldMetadata::nextSerial: No se pudo obtener siguiente contador.";
        return -1;
    }
    bool ok;
    if ( result.toString().isEmpty() )
    {
        return 1;
    }
    int r = result.toInt(&ok);
    if ( ok )
    {
        return (r+1);
    }
    return -1;
}

QVariant DBFieldMetadata::variantValueFromSqlRawData(const QString &data)
{
    QVariant result;
    if ( type() == QVariant::Int )
    {
        int temp = data.toInt();
        result = QVariant(temp);
    }
    else if ( type() == QVariant::Double )
    {
        double temp = data.toDouble();
        result = QVariant(temp);
    }
    else if ( type() == QVariant::Date )
    {
        QDate temp = QDate::fromString(data, Qt::ISODate);
        result = QVariant(temp);
    }
    else if ( type() == QVariant::DateTime )
    {
        QDate temp = QDate::fromString(data, Qt::ISODate);
        result = QVariant(temp);
    }
    else if ( type() == QVariant::Bool )
    {
        bool temp = data == "true" ? true : false;
        result = QVariant(temp);
    }
    else if ( type() == QVariant::String )
    {
        result = QVariant(data);
    }
    else if ( type() == QVariant::Pixmap )
    {
        QByteArray ba = data.toUtf8();
        result = QByteArray::fromBase64(ba);
    }
    else
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString::fromUtf8("DBField::setValueFromSqlRawData: field [%1] tiene un tipo no determinado.").arg(dbFieldName()));
        result = QVariant(data);
    }
    return result;
}

AlephERP::DbFieldCounterDefinition * DBFieldMetadata::counterDefinition()
{
    return &(d->m_counterDefinition);
}

void DBFieldMetadata::setCounterDefinitionFields(const QStringList &fields)
{
    d->m_counterDefinition.fields = fields;
}

void DBFieldMetadata::setCounterDefinitionPrefixOnRelation(const QStringList &prefix)
{
    d->m_counterDefinition.prefixOnRelations = prefix;
}

void DBFieldMetadata::setCounterDefinitionUserCanModified(bool value)
{
    d->m_counterDefinition.userCanModified = value;
    if ( !value )
    {
        d->m_calculated = true;
        d->m_calculatedOneTime = true;
        d->m_calculatedSaveOnDB = true;
    }
}

void DBFieldMetadata::setCounterDefinitionSeparator(const QString &value)
{
    d->m_counterDefinition.separator = value;
}

void DBFieldMetadata::setCounterDefinitionUseTrailingZeros(bool value)
{
    d->m_counterDefinition.useTrailingZeros = value;
}

void DBFieldMetadata::setCounterDefinitionExpression(const QString &value)
{
    d->m_counterDefinition.expression = value;
}

bool DBFieldMetadata::hasCounterDefinition() const
{
    return (d->m_counterDefinition.fields.size() > 0 || !d->m_counterDefinition.expression.isEmpty());
}

void DBFieldMetadata::setCounterDefinitionCalculateOnlyOnInsert(bool value)
{
    d->m_counterDefinition.calculateOnlyOnInsert = value;
}

void DBFieldMetadata::setCounterPrefix(const QString &value)
{
    d->m_counterPrefix = value;
}

QString DBFieldMetadata::counterPrefix() const
{
    return d->m_counterPrefix;
}

/*!
  Tenemos que decirle al motor de scripts, que BaseBean se convierte de esta forma a un valor script
  */
QScriptValue DBFieldMetadata::toScriptValue(QScriptEngine *engine, DBFieldMetadata * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBFieldMetadata::fromScriptValue(const QScriptValue &object, DBFieldMetadata * &out)
{
    out = qobject_cast<DBFieldMetadata *>(object.toQObject());
}

bool DBFieldMetadata::loadOnBackground() const
{
    if ( calculated() )
    {
        return false;
    }
    return ( memo() || type() == QVariant::Pixmap );
}

QString DBFieldMetadataPrivate::fieldName()
{
    QString result = m_fieldName;
    if ( !m_fieldNameScript.isEmpty() )
    {
        if ( m_calculatedFieldName.isEmpty() )
        {
            BaseBeanMetadata *beanM = qobject_cast<BaseBeanMetadata *>(q_ptr->parent());
            if ( beanM != NULL )
            {
                QString scriptName = QString("%1.%2.alias").arg(beanM->tableName()).arg(m_dbFieldName);
                m_engine->setScript(m_fieldNameScript, scriptName);
                QVariant scriptResult = m_engine->toVariant(m_engine->callQsFunction(QString("aliasScript")));
                if ( m_engine->lastMessage().isEmpty() )
                {
                    result = scriptResult.toString();
                    m_calculatedFieldName = result;
                }
            }
        }
        else
        {
            result = m_calculatedFieldName;
        }
    }
    else if ( m_fieldName.isEmpty() )
    {
        result = m_dbFieldName;
    }
    return result;
}
