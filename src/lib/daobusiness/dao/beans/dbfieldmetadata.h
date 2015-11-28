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
#ifndef DBFIELDMETADATA_H
#define DBFIELDMETADATA_H

#include <QtCore>
#include <QtScript>
#include <alepherpglobal.h>
#include <aerpcommon.h>
#include "dao/beans/aggregatecalc.h"
#include "dao/beans/builtinexpressiondef.h"
#include "dao/beans/stringexpression.h"

class BaseBean;
class DBRelationMetadata;
class DBFieldMetadataPrivate;
class BaseBeanMetadata;
class BaseBeanMetadataPrivate;
class DBField;

class ALEPHERP_DLL_EXPORT DBFieldMetadata : public QObject, public QScriptable
{
    Q_OBJECT

    friend class BaseBeanMetadata;
    friend class BaseBeanMetadataPrivate;

    /** Número cardinal que identifica al field */
    Q_PROPERTY(int index READ index WRITE setIndex)
    /** Nombre en base de datos de la columna donde se almacena el dato. Es el nombre interno con el que se accede al campo.
    Puede ser también una estructura compleja que haga referencia a un campo de otra tabla relacionada. En ese caso

   <field>
        <name>nombrearticulo</name>
        <expression>{stocks.father.articulos.father.nombre}</expression>
    </field>

    sería equivalente a

   <field>
        <name>nombrearticulo</name>
        <alias>Artículo</alias>
        <null>false</null>
        <pk>false</pk>
        <type>string</type>
        <length>250</length>
        <visiblegrid>true</visiblegrid>
        <calculated>true</calculated>
        <builtInStringExpression>
            <expression>{stocks.father.articulos.father.nombre}</expression>
        </builtInStringExpression>
    </field>

*/
    Q_PROPERTY(QString dbFieldName READ dbFieldName WRITE setDbFieldName)
    /** Nombre a mostrar al usuario de esta columna */
    Q_PROPERTY(QString fieldName READ fieldName WRITE setFieldName)
    /** Script para determinar el nombre a mostrar al usuario de esta columna */
    Q_PROPERTY(QString fieldNameScript READ fieldNameScript WRITE setFieldNameScript)
    /** Indica si es un campo de sólo lectura */
    Q_PROPERTY(bool readOnly READ readOnly)
    /** Indica si este campo se creará como un campo de índice en la base de datos */
    Q_PROPERTY(bool dbIndex READ dbIndex WRITE setDbIndex)
    /** ¿Puede este campo ser nulo ? True si puede serlo */
    Q_PROPERTY(bool null READ canBeNull WRITE setNull)
    /** ¿Es este campo una primary key? */
    Q_PROPERTY(bool primaryKey READ primaryKey WRITE setPrimaryKey)
    /** ¿Este campo es único? No tiene porqué ser primary key para ser único. */
    Q_PROPERTY(bool unique READ unique WRITE setUnique)
    /** Puede ser que un campo deba ser único sólo en el ámbito que marca otra/s columna/s. Por ejemplo,
      supongamos que el registro tiene un campo "código_de_empresa". El valor que tenga este field será
      único para cada valor de código_de_empresa. Así, puede haber un valor de este campo '01' para
      código_de_empresa=1 y otro valor '01' para código_de_empresa=2.
      Se pueden incluir varios fields simplemente separándolos por comas: codempresa,codejercicio.
      Ojo, si se utiliza esta propiedad, hay que poner unique a false. */
    Q_PROPERTY(QString uniqueOnFilterField READ uniqueOnFilterField WRITE setUniqueOnFilterField)
    /** Es posible definir reglas para registros únicos más complejos, por ejemplo: El stock debe ser único para
     * un mismo artículo y un mismo almacén. En la tabla stocks, se definirá los fields idarticulo e idalmacen
     * con uniqueCompound a true */
    Q_PROPERTY(bool uniqueCompound READ uniqueCompound WRITE setUniqueCompound)
    /** Longitud del campo */
    Q_PROPERTY(int length READ length WRITE setLength)
    Q_PROPERTY(QString lengthScript READ lengthScript WRITE setLengthScript)
    /** Longitud de la parte entera si es un numero */
    Q_PROPERTY(int partI READ partI WRITE setPartI)
    /** Longitud de la parte decimal si es un numero */
    Q_PROPERTY(int partD READ partD WRITE setPartD)
    /** Indica si es autonumerico */
    Q_PROPERTY(bool serial READ serial WRITE setSerial)
    /** Indica si el valor del campo contiene coordenadas geográficas */
    Q_PROPERTY(bool coordinates READ coordinates WRITE setCoordinates)
    /** Indica si se visualiza en los modelos como columna */
    Q_PROPERTY(bool visibleGrid READ visibleGrid WRITE setVisibleGrid)
    /** Algunos campos pueden ser calculados. La definición de estos campos implica muchas variables. Por un lado,
    puede determinarse, si por eficiencia, el campo se calcula úna única vez en su vida. Por otro lado, si éste campo debe almacenarse
    en base de datos. Y después el cálculo en sí. Siembre que se tenga un campo con calculated a true, se debe tener una fórmula de cálculo
    El cálculo en sí puede hacerse de dos formas:
    1.- Con un script JS. Para ello se crea una entrada <script> que debe contener una función global de nombre value, de esta forma

    <script>
    <![CDATA[
    function value() {
    // Cálculo a realizar
    return 0;
    }
    ]]>
    </script>

    Esta función tendrá definido en el contexto dos objetos: "bean" y "dbField". El primero hacer referencia
    al registro para el que se está calculando el campo, y dbField, al propio campo en sí.
    2.- Con una expresión interna. Mucho más eficiente, pero más limitada. Su definición se hace de la siguiente forma

    <builtInExpression>
    <var name="var1">ruta.Var1</var>
    <var name="var2">ruta.Var2</var>
    <var name="var3">ruta.Var3</var>
    <expression>var1 * var2 / var3</expression>
    </builtInExpression>

    La definición de las variables es optativa, si se utilizan propiedades directas del bean en ejecución. Por ejemplo,
    supongamos que se está calculando el valor del campo "total" dentro de un bean que representa una factura y tiene campos
    "neto", "iva"... Esta expresión puede simplificarse así

    <builtInExpression>
    <expression> (neto * iva / 100)  + neto </expression>
    </builtInExpression>

    Y el motor buscará automáticamente dentro del bean, los fields neto e iva. Para casos más complejos, donde por ejemplo
    queramos escoger el campo tasaconv de la relación M1 que facturas tiene con divisas, tendríamos que ajustar <var> de la siguiente forma:
    name es el nombre que se utilizará en la expresión
    El contenido del nodo <var> es la ruta, en propiedades para llegar al elemento dbField.

    <var name="tasaconv">father.tasaconv</var>

    Igualmente, podemos recurrir a una variable de tipo string:
    <builtInStringExpression>
        <expression>tabla_padre.father.field_father.value</expression>
    </builtInStringExpression>
    */
    Q_PROPERTY(bool calculated READ calculated WRITE setCalculated)
    /** Los campos calculados pueden calcularse sólo una vez para mejorar el rendimiento */
    Q_PROPERTY(bool calculatedOneTime READ calculatedOneTime WRITE setCalculatedOneTime)
    /** Pueden existir campos calculados cuyo valor se almacena en base de datos */
    Q_PROPERTY(bool calculatedSaveOnDb READ calculatedSaveOnDb WRITE setCalculatedSaveOnDb)
    /** Con determinados campos calculados, puede ser deseable estar conectados a las modificaciones
      que se producen en campos de beans de relaciones dependientes. Esta opción puede causar
      un determinado proceso recursivo que puede producirse cuando hay vinculaciones entre campos calculados
      de los beans hijos y de este field, que se invocan de forma recursiva. Por ello, esta propiedad
      hay que utilizarla con suma cautela */
    Q_PROPERTY(bool calculatedConnectToChildModifications READ calculatedConnectToChildModifications WRITE setCalculatedConnectToChildModifications)
    /** A veces es interesante que los cálculos se hagan sólo mientras el bean sea INSERT. Después ya no será
     * modificado */
    Q_PROPERTY(bool calculatedOnlyOnInsert READ calculatedOnlyOnInsert WRITE setCalculatedOnlyOnInsert)
    /** Algunos campos podrán ser calculados, a partir de los datos del propio bean.
      El cálculo de esos campos se realiza mediante el script que almacena esta variable
      */
    Q_PROPERTY(QString script READ script WRITE setScript)
    /** Si el campo es un String, indicará si almacena código HTML */
    Q_PROPERTY(bool html READ html WRITE setHtml)
    /** Indica si el campo almacena una dirección de correo electrónico. */
    Q_PROPERTY(bool email READ email WRITE setEmail)
    /** Valor por defecto del campo (en casos numéricos, directamente un número) o expresión (en caso de fechas,
     * si defaultValue es igual a "now" se aplicará el instante de tiempo o fecha actual */
    Q_PROPERTY(QVariant defaultValue READ defaultValue WRITE setDefaultValue)
    /** Script a ejecutar para obtener el default value */
    Q_PROPERTY(QString scriptDefaultValue READ scriptDefaultValue WRITE setScriptDefaultValue)
    /** Es posible definir un valor por defecto, a partir de un cálculo simple de este tipo
     * <builtInExpressionDefaultValue>
     *      <expression>{importeunidad} * {horas}</expression>
     * </builtInExpressionDefaultValue>
     * donde importeunidad es una propiedad directa del bean al que pertenece el field (de modo que puede accederse por propiedades
     * a otros elementos. Por ejemplo: cuentasbanco.father.cuenta
     * donde cuentasbanco es una propiedad (DBRelation) del bean, father es una propiedad de cuentasbanco (BaseBean) y cuenta es una propiedad
     * de father (DBField) */
    Q_PROPERTY(BuiltInExpressionDef builtInExpressionDefaultValue READ builtInExpressionDefaultValue WRITE setBuiltInExpressionDefaultValue)
    /** Misma regla anterior, pero para cadenas de caracteres */
    Q_PROPERTY(StringExpression builtInStringExpressionDefaultValue READ builtInStringExpressionDefaultValue WRITE setBuiltInStringExpressionDefaultValue)
    /** El valor por defecto que tomará este field, será el de la variable de entorno definida aquí. Este tag espera el nombre de la
     * variable de entorno */
    Q_PROPERTY(QString envDefaultValue READ envDefaultValue WRITE setEnvDefaultValue)
    /** Flag para activar el debug de los scripts de cálculo de este field */
    Q_PROPERTY(bool debug READ debug WRITE setDebug)
    /** Indica si el script se ejecuta desde el debugger, por ejemplo paso a paso */
    Q_PROPERTY(bool onInitDebug READ onInitDebug WRITE setOnInitDebug)
    /** El campo calculado puede ser, un aggregate, esto es, su valor depende de otros beans como él
      que pertenecen al bean referencia. Por ejemplo, una suma de un determinado campo. Este campo
      marca si lo es, y sobre qué field realiza el cálculo */
    Q_PROPERTY(bool aggregate READ aggregate WRITE setAggregate)
    /** Definición de cómo se realiza el cálculo del campo agregado */
    Q_PROPERTY(AggregateCalc aggregateCalc READ aggregateCalc WRITE setAggregateCalc)
    /** Indica si el campo almacena un campo MEMO. Hay que distinguirlos, porque obtenerlos de base
      de datos es muy gravoso. */
    Q_PROPERTY(bool memo READ memo WRITE setMemo)
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
    Q_PROPERTY(QString validationRuleScript READ validationRuleScript WRITE setValidationRuleScript)
    /** En la visualización en grid, permite determinar la fuente, que aparece */
    Q_PROPERTY(QFont fontOnGrid READ fontOnGrid WRITE setFontOnGrid)
    /** Color de la fuente en el grid */
    Q_PROPERTY(QColor color READ color WRITE setColor)
    /** Background Color de la fuente en el grid */
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)
    /** Tras un INSERT, o un UPDATE exitoso en base de datos, puede ser necesario, reobtener el valor
      de un field, porque, por ejemplo, se ha asociado un trigger al mismo a ejecutar en el proceso de actualización,
      con lo que el valor del que dispone AlephERP puede no ser el actual. Esta propiedad permite marcar
      ese tipo de campos. Un ejemplo clásico, es un contador único que surge de una tabla de contadores, y que
      se asigna a una tabla (por ejemplo, facturas) */
    Q_PROPERTY(bool reloadFromDBAfterSave READ reloadFromDBAfterSave WRITE setReloadFromDBAfterSave)
    /** Indica si este campo se crea como widget en los DBSearchDlg generados automáticamente y no mediante
      widgets de tipo .search.ui */
    Q_PROPERTY(bool includeOnGeneratedSearchDlg READ includeOnGeneratedSearchDlg WRITE setIncludeOnGeneratedSearchDlg)
    /** El programador QS puede describir un formato específico de visualización de los field. Lo hará mediante
      una función QS definida en los metadatos de la tabla y que se encuentra aquí. Debe definirse asi:
    <displayValueScript>
    <![CDATA[
    function displayValueScript(value, displayValue) {
    // Value contiene el valor a formatear, y displayValue el valor que formatería el sistema.
    }
    ]]>
    </displayValueScript>
    */
    Q_PROPERTY(QString displayValueScript READ displayValueScript WRITE setDisplayValueScript)
    /**
     * Leerá una expresión para una presentación o display sencillo.
     * <var>pathAlValor1</var>
     * <var>pathAlValor2</var>
     * <expression>{1} {2} {fieldDelBean} {relacion.father.fieldDeLaRelacion}</expression>
    */
    Q_PROPERTY(StringExpression displayValueExpression READ displayValueExpression WRITE setDisplayValueExpression)
    /** Un field, definido dentro de un BaseBean puede no ser visible en un abstractview normal, pero su edición en abstractview con inlineEdit sí es relevante.
      Aquí se introduce esa definición. Se le define un comportamiento en paralelo
    <behaviourOnInlineEdit>
        <!-- ¿Qué se presenta en esa celda del abstract view cuando no se está editando? -->
        <viewOnRead>actividadesproductivas.nombre</viewOnRead>
        <!-- ¿Qué se presenta en esa celda del abstract view cuando se está editando? -->
        <viewOnEdit>DBChooseButtonRecord</viewOnEdit>
    </behaviourOnInlineEdit>
    */
    // Q_PROPERTY(QVariant behaviourOnInlineEdit READ behaviourOnInlineEdit WRITE setBehaviourOnInlineEdit)
    /** Este field se presenta como un enlace que permite abrir, en un diálogo asociado, el registro. */
    Q_PROPERTY(bool link READ link WRITE setLink)
    /** La anterior apertura puede hacerse en modo lectura o escritura */
    Q_PROPERTY(bool linkOpenReadOnly READ linkOpenReadOnly WRITE setLinkOpenReadOnly)
    /** Para la presentación de los registros en abstracts views, puede ser interesante proporcionar un Tooltip adicional, que permita al usuario
      obtener datos interesantes. Este tooltip se calcula mediante una función de script, que se define aquí */
    Q_PROPERTY(QString toolTipScript READ toolTipScript WRITE setToolTipScript)
    /** Tooltip con expresiones de cadena... más eficientes */
    Q_PROPERTY(StringExpression toolTipStringExpression READ toolTipStringExpression WRITE setToolTipStringExpression)
    /** Nombre del tipo de campo especificado en los metadatos */
    Q_PROPERTY(QString metadataTypeName READ metadataTypeName WRITE setMetadataTypeName)
    /** En ciertas ocasiones, es necesario obtener una representación del bean (por ejemplo, entre los items
     de un combobox, sin que un se haya definido ninguna información adicional. Se hace con esta propiedad.
     Se proporcionará una representación de tipo: campos marcados como showDefault separados por "-" */
    Q_PROPERTY(bool showDefault READ showDefault WRITE setShowDefault)
    /** Script que se ejecuta cuando el valor del campo cambia */
    Q_PROPERTY(QString onChangeScript READ onChangeScript WRITE setOnChangeScript)
    /** Para cálculos sencillos es posible definir expresiones matemáticas construidas internamente.
     * Aquí se accede a la definición de la misma */
    Q_PROPERTY(BuiltInExpressionDef builtInExpression READ builtInExpression WRITE setBuiltInExpression)
    /** Para campos calculados con cadenas de caracteres, se utilizan expresiones sencillas */
    Q_PROPERTY(StringExpression builtInStringExpression READ builtInStringExpression WRITE setBuiltInStringExpression)
    /** A veces es interesante tener un campo de orden con el que se presentan los campos, especialmente
     * en los DBTableView. Esta propiedad lo indicará. Esta propiedad permitirá al usuario ordenar registros
     * que se presentan en DBTableView con arrastrar y soltar, actualizando el valor de este campo. */
    Q_PROPERTY(bool orderField READ orderField WRITE setOrderField)
    /** Para registros que son susceptibles de representar su información en un widget de tipo Schedule View, es necesario
     * identificar qué campo indica la marca temporal de incio (ItemStartTimeRole). */
    Q_PROPERTY(bool scheduleStartTime READ scheduleStartTime WRITE setScheduleStartTime)
    /** En línea con el registro anterior, indica la duración */
    Q_PROPERTY(bool scheduleDuration READ scheduleDuration WRITE setScheduleDuration)
    Q_PROPERTY(QString xmlDefinition READ xmlDefinition)
    /** La carga de estos campos se hará siempre en background */
    Q_PROPERTY(bool loadOnBackground READ loadOnBackground)
    /** Si este campo está involucrado en un contador y se proporciona un valor de counterPrefix, este
     * será utilizado para calcular el valor en lugar de value */
    Q_PROPERTY(QString counterPrefix READ counterPrefix)
    /** Hay ocasiones que determinadas reglas de cálculo en campos calculados, especialmente en JS hacen que el sistema
     * no sea capaz de detectar todos los fields involucrados en el cálculo. Por ejemplo, este field calculado puede tener un if
     * dentro que quizás no ejecute su interior donde se acceden a otros campos. Sin embargo, la modificación en estos debe recalcular
     * este campo. Es posible registrar estos cambios a través de código JS con bean.registerRecalculateFields, pero también podemos
     * definirlo en los metadatos, con esta propiedad */
    Q_PROPERTY(QStringList dependOnFieldsToCalc READ dependOnFieldsToCalc WRITE setDependOnFieldsToCalc)

private:
    Q_DISABLE_COPY(DBFieldMetadata)
    DBFieldMetadataPrivate *d;
    //Q_DECLARE_PRIVATE(DBFieldMetadata)

protected:
    void setTempStringExpression(const QString &value);
    void consolidateTemp();
    void setXmlDefinition(const QString &value);

public:
    explicit DBFieldMetadata(QObject *parent = NULL);
    ~DBFieldMetadata();

    static int desiredLengthForType(QVariant::Type type);

    int index() const;
    void setIndex(int value);
    QString dbFieldName() const;
    void setDbFieldName(const QString &name);
    QString fieldName() const;
    void setFieldName(const QString &name);
    QString fieldNameScript() const;
    void setFieldNameScript(const QString &name);
    bool readOnly() const;
    bool dbIndex() const;
    void setDbIndex(bool value);
    bool html() const;
    void setHtml(bool html);
    bool coordinates() const;
    void setCoordinates(bool value);
    bool email() const;
    void setEmail(bool email);
    bool debug() const;
    void setDebug(bool value);
    bool onInitDebug();
    void setOnInitDebug(bool value);
    bool calculated() const;
    void setCalculated(bool value);
    bool calculatedOneTime() const;
    void setCalculatedOneTime(bool value);
    void setCalculatedSaveOnDb(bool vlaue);
    bool calculatedSaveOnDb() const;
    void setCalculatedConnectToChildModifications(bool value);
    bool calculatedConnectToChildModifications() const;
    void setCalculatedOnlyOnInsert(bool value);
    bool calculatedOnlyOnInsert() const;
    QString script() const;
    void setScript(const QString &value);
    bool canBeNull() const;
    void setNull(bool value);
    bool primaryKey() const;
    void setPrimaryKey(bool value);
    bool unique() const;
    void setUnique(bool value);
    QString uniqueOnFilterField() const;
    void setUniqueOnFilterField(const QString &value);
    bool uniqueCompound() const;
    void setUniqueCompound(bool value);
    enum QVariant::Type type() const;
    void setType(enum QVariant::Type value);
    int length() const;
    void setLength(int value);
    QString lengthScript();
    void setLengthScript(const QString &value);
    int partI() const;
    void setPartI(int value);
    int partD() const;
    void setPartD(int value);
    bool serial() const;
    void setSerial(bool value);
    bool visibleGrid() const;
    void setVisibleGrid(bool value);
    bool memo() const;
    void setMemo(bool value);
    QMap<QString, QString> optionsList() const;
    void setOptionList(const QMap<QString, QString> &option);
    QMap<QString, QString> optionsIcons() const;
    void setOptionIcons(const QMap<QString, QString> &optionList);
    QString scriptDefaultValue() const;
    void setScriptDefaultValue(const QString &string);
    BuiltInExpressionDef builtInExpressionDefaultValue() const;
    void setBuiltInExpressionDefaultValue(const BuiltInExpressionDef &string);
    StringExpression builtInStringExpressionDefaultValue() const;
    void setBuiltInStringExpressionDefaultValue(const StringExpression &string);
    QString envDefaultValue() const;
    void setEnvDefaultValue(const QString &value);
    bool aggregate() const;
    void setAggregate(bool value);
    AggregateCalc aggregateCalc() const;
    void setAggregateCalc(const AggregateCalc &calc);
    QString validationRuleScript() const;
    void setValidationRuleScript(const QString &value);
    QFont fontOnGrid() const;
    void setFontOnGrid(const QFont &font);
    QColor color() const;
    void setColor(const QColor &color);
    QColor backgroundColor() const;
    void setBackgroundColor(const QColor &color);
    bool reloadFromDBAfterSave() const;
    void setReloadFromDBAfterSave(bool v);
    bool includeOnGeneratedRecordDlg() const;
    void setIncludeOnGeneratedRecordDlg(bool v);
    bool includeOnGeneratedSearchDlg() const;
    void setIncludeOnGeneratedSearchDlg(bool v);
    QString displayValueScript() const;
    void setDisplayValueScript(QString v);
    StringExpression displayValueExpression() const;
    void setDisplayValueExpression(const StringExpression &exp);
    QHash<QString, QVariant> behaviourOnInlineEdit() const;
    void setBehaviourOnInlineEdit(const QHash<QString, QVariant> &value);
    bool link() const;
    void setLink(bool value);
    bool linkOpenReadOnly() const;
    void setLinkOpenReadOnly(bool value);
    QString toolTipScript() const;
    void setToolTipScript(const QString &value);
    StringExpression toolTipStringExpression() const;
    void setToolTipStringExpression(const StringExpression &string);
    QString metadataTypeName() const;
    void setMetadataTypeName(const QString &value);
    QString databaseType(const QString &dialect) const;
    bool checkDatabaseType(const QString &databaseColumnType, const QString &dialect, QString &err);
    bool showDefault() const;
    void setShowDefault(bool value);
    bool hasDefaultValue() const;
    QString onChangeScript() const;
    void setOnChangeScript(const QString &value);
    BuiltInExpressionDef builtInExpression() const;
    void setBuiltInExpression(const BuiltInExpressionDef &def);
    StringExpression builtInStringExpression() const;
    void setBuiltInStringExpression(const StringExpression &def);
    bool orderField() const;
    void setOrderField(bool value);
    bool scheduleStartTime() const;
    void setScheduleStartTime(bool value);
    bool scheduleDuration() const;
    void setScheduleDuration(bool value);
    AlephERP::DateTimeParts scheduleTimeUnit () const;
    void setScheduleTimeUnit(AlephERP::DateTimeParts value);
    QString xmlDefinition() const;
    Qt::Alignment alignment() const;
    QStringList dependOnFieldsToCalc() const;
    void setDependOnFieldsToCalc(const QStringList &data);

    QString ddlCreationTable(const AlephERP::CreationTableSqlOptions &options, const QString &dialect);

    /**
     * @brief counterDefinition
     * Es posible definir una estructura para la construcción de campos calculados. La versión clásica de AlephERP
     * consta de
     * <counter>
     *       <dependsOnField prefixOnRelation="counter_prefix">idserie</dependsOnField>
     *       <dependsOnField prefixOnRelation="counter_prefix">idejercicio</dependsOnField>
     *       <userCanModify>true</userCanModify>
     *       <calculateOnlyOnInsert>true</calculateOnlyOnInsert>
     *       <separator>/</separator>
     *       <useTrailingZeros>true</useTrailingZeros>
     * </counter>
     * La versión moderna de AlephERP propone la siguiente estructura para la definición
     * <counter>
     *       <expression>{empresa.father.prefix.trimmed}/{ejercicios.father.prefix.toUpper}/{value.trailingZeros}</expression>
     *       <userCanModify>true</userCanModify>
     *       <calculateOnlyOnInsert>true</calculateOnlyOnInsert>
     * </counter>
     * En las versiones modernas de AlephERP, cuando se encuentra una claúsula <expression> se ignoran
     * automáticamente las claúsulas <dependsOnField>, <separator> y <useTrailingZeros>
     * @return
     */
    bool hasCounterDefinition() const;
    AlephERP::DbFieldCounterDefinition * counterDefinition();
    void setCounterDefinitionFields(const QStringList &fields);
    void setCounterDefinitionPrefixOnRelation(const QStringList &prefix);
    void setCounterDefinitionUserCanModified(bool value);
    void setCounterDefinitionSeparator(const QString &value);
    void setCounterDefinitionUseTrailingZeros(bool value);
    void setCounterDefinitionExpression(const QString &value);
    void setCounterDefinitionCalculateOnlyOnInsert(bool value);
    void setCounterPrefix(const QString &value);
    QString counterPrefix() const;

    QVariant calculateDefaultValue(DBField *parent);
    QVariant defaultValue() const;
    void setDefaultValue(const QVariant &value);
    QVariant calculateValue(DBField *fld);
    QVariant calculateAggregateScript(DBField *fld, const QString &script, BaseBean *relationChildRecord = NULL);

    QString toolTip(DBField *parent);

    QList<DBRelationMetadata *> relations(const AlephERP::RelationTypes &type = AlephERP::All);
    void addRelation(DBRelationMetadata *rel);

    BaseBeanMetadata *beanMetadata() const;
    QString displayValue(QVariant v, DBField *parent);
    int calculateLength(DBField *parent);

    QString validateRule(DBField *parent, bool &validate);

    void onChangeEvent(DBField *parent);

    QString sqlWhere(const QString &op, const QVariant &value, const QString &dialect = "");
    QString sqlValue(const QVariant &value, bool includeQuotes = true, const QString &dialect = "");
    QString sqlNullCondition(const QString &dialect = "");

    QVariant parseValue(const QString &v);

    int nextSerial(DBField *fld);

    QVariant variantValueFromSqlRawData(const QString &stringValue);

    static QScriptValue toScriptValue(QScriptEngine *engine, DBFieldMetadata * const &in);
    static void fromScriptValue(const QScriptValue &object, DBFieldMetadata * &out);

    bool loadOnBackground() const;
};

Q_DECLARE_METATYPE(DBFieldMetadata*)
Q_DECLARE_METATYPE(QList<DBFieldMetadata *>)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBFieldMetadata, QObject*)

#endif // DBFIELDMETADATA_H
