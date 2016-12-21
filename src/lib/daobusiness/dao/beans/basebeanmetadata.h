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
#ifndef BASEBEANMETADATA_H
#define BASEBEANMETADATA_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtCore>
#include <QtScript>
#include <alepherpglobal.h>
#include <aerpcommon.h>
#include "dao/beans/stringexpression.h"
#include "widgets/aerpscheduleview/aerpscheduleview.h"

class BaseBean;
class DBFieldMetadata;
class DBRelationMetadata;
class BaseBeanMetadataPrivate;
class RelatedElement;
class AERPSystemModule;
class BeansFactory;

typedef QHash<QString,QString> QVariantString;

struct ConnectionAction
{
    QString signalAction;
    QStringList slotsAction;
};

struct InternalConnection
{
    QString senderQs;
    QList<ConnectionAction> actions;
};

struct AssociatedFunctions
{
    /** Script con funciones asociadas */
    QString scriptFileName;
    /** Programa precompilado */
    QScriptProgram scriptProgram;
    /** Funciones asociadas */
    QStringList functions;
};

struct ScheduleData
{
    /** Indica si el control scheduled mostrará un combo box para que el usuario escoga el tipo de visualización */
    bool showViewModesFilter;
    /** Si no, tipo de visualización por defecto */
    AERPScheduleView::ViewMode viewMode;
    /** Indica si se pueden solapar registros */
    bool canOverlap;
    /** Si está a true, el usuario podrá mover registros en la vistas, y los cambios se almacenarán directamente en base de datos */
    bool canMove;
    /** Precisión, en segundos con el que se podrán presentar los datos */
    int stepWidthInSeconds;
    /** Expresión para el cálculo de la cabecera del item */
    StringExpression titleExpression;
    /** Expresión JS para el cálculo de la cabecera del item */
    QString qsTitleExpression;
};

typedef QList<QHash<QString, QString> > HashStringList;
typedef QList<InternalConnection> InternalConnectionList;
typedef QList<AssociatedFunctions> AssociatedFunctionsList;
typedef QList<AssociatedFunctions *> AssociatedFunctionsPointerList;

typedef struct EnvVarDefinitionStruct
{
    QString varName;
    QString fieldName;
} EnvVarDefinition;

class ALEPHERP_DLL_EXPORT BaseBeanMetadata : public QObject, public QScriptable
{
    Q_OBJECT

    friend class BeansFactory;

    /** Configuración en XML del bean: Campos, relaciones... */
    Q_PROPERTY(QString xml READ xml WRITE setXml)
    /** Tabla de la base de datos a la que hace referencia */
    Q_PROPERTY(QString tableName READ tableName WRITE setTableName)
    /** Indica si el formulario de registros que edita estos datos permite la navegación por registros o no */
    Q_PROPERTY(bool canNavigate READ canNavigate WRITE setCanNavigate)
    /** Devuelve el nombre de la tabla para su inclusión en una sentencia SQL, incluyendo el esquema */
    Q_PROPERTY(QString sqlTableName READ sqlTableName)
    /** Esquema de la base de datos, en el que se encuentra la tabla. Si no se especifica nada, se entiende
    que la tabla se encuentra en el predefinido en la base de datos, o en el definido en la configuración del programa. */
    Q_PROPERTY(QString schema READ schema WRITE setSchema)
    /** Esta es la vista o consulta que se utiliza para obtener los datos que
    en los controles que utiliza a modelos derivados de BaseBeanModel */
    Q_PROPERTY(QString viewOnGrid READ viewOnGrid WRITE setViewOnGrid)
    /** Si este metadato representa una vista, se indica a qué tabla hace referencia esta vista. Este valor es
      * muy importante, si se quieren hacer ediciones de datos de vistas. */
    Q_PROPERTY(QString viewForTable READ viewForTable WRITE setViewForTable)
    /** AlephERP trabajan intensivamente con el OID de base de datos. Cuando se utilizan vistas de base de datos, es interesante
     * saber si ésta provee el OID de la tabla referencia */
    Q_PROPERTY(bool viewProvidesOid READ viewProvidesOid WRITE setViewProvidesOid)
    /** Nombre bonito de la tabla */
    Q_PROPERTY(QString alias READ alias WRITE setAlias)
    /** Indica si el borrado es lógico o físico. Si es lógico, el registro
      no podrá ser borrado de base de datos. En su lugar se marca la columna
      is_deleted como true */
    Q_PROPERTY(bool logicalDelete READ logicalDelete WRITE setLogicalDelete)
    /** Un registro marcado en readOnly, indicará que una vez insertado, ya no podrá ser modificado (salvo programáticamente) */
    Q_PROPERTY(bool readOnly READ readOnly WRITE setReadOnly)
    /** Módulo lógico al que pertenece el bean */
    Q_PROPERTY(AERPSystemModule * module READ module WRITE setModule)
    /** Cuando este bean se presenta en un DBForm, puede querer filtrarse mediante
    un filtro fuerte, que se deriva de los valores de otra tabla, o de los valores
    de un option list de ese campo. Por ejemplo,
    presupuestos_cabecera tiene un campo, estado, que coge datos de los registros
    marcados en la tabla presupuestos_definicion_estados. El contenido de esta tabla
    se incluirá en el combo. Ejemplo:
    Para poner un filtro fuerte para los vendís y demás, lo único que tienes es que configurar el
    XML de la tabla así

    <table>
        <name>alepherp_vendi</name>
        <alias>Vendis</alias>
        <itemsFilterColumn>
            <itemFilter>
                <fieldToFilter>id_tienda</fieldToFilter>
                <relationFieldToShow>nombre</relationFieldToShow>
                <!-- Si por ejemplo se crea un registro nuevo, con este filtro activo, y este valor está a true,
                se precargará como valor por defecto en el nuevo registro creado, el del filtro. Por ejemplo,
                si tenemos filtrado los ejercicios para la empresa EMPRESA 1, al crear un nuevo ejercicio, éste
                se creará como hijo de la empresa 1, es decir, su campo codempresa se pondrá al del ID EMPRESA 1 -->
                <setFilterValueOnNewRecords>true</setFilterValueOnNewRecords>
            </itemFilter>
        </itemsFilterColumn>
    ....

    donde fieldToFilter es el campo de los registros del DBFormDlg que estás presentado por el que
    quieres filtrar, y relationFieldToShow es qué campo de la relación M1 que tendría ese campo,
    se mostrará en el combobox de filtro (el name de la tienda).

    Si fieldToFilter no tiene relación definida pero tomas sus valores de un optionList, esos
    valores se agregarán sólos.
    */
    Q_PROPERTY(HashStringList itemsFilterColumn READ itemsFilterColumn WRITE setItemsFilterColumn)
    /** Presentar ciertos datos, puede estar ligado a una serie de variables globales.
    Por ejemplo: queremos mostrar sólo los registros de que pertenecen a un único puesto
    de trabajo. Desde Javascript se configuraría el valor de esa variable global, y al
    mostrarse los datos de esta tabla, se harían teniendo en cuenta el filtro que esa variable
    establece. Aquí se define la lista de variables de entorno que aplican a este bean, y a qué campos
    La definición se haría
    <envVars>
     <pair>
         <field>id_tienda</field>
         <varName>idTienda</varName>
     </pair>
    </envVars>
    y el QStringList quedaría: "idTienda:id_tienda"
      */
    Q_PROPERTY(QList<EnvVarDefinition> envVarsFilter READ envVars)
    /** Para los DBFormDlg, y especialmente los DBFilterT*View, puede ser interesante mostrar subtotales
      del listado de registros que se _pueden editar. Por ejemplo, podría interesarnos ver un sumatorio
      parcial de las facturas actualmente filtradas.
      Aquí pueden definirse esos subtotales que se mostrarán el DBFilterT*View como campos visibles en la misma
      fila de los filtros fuertes. Su definición
      <infoSubTotals>
          <infoSubTotal>
              <alias>Nombre visible con el que aparecerá al usuario (la etiqueta que lo acompañará</alias>
              <name>Nombre identificativo. Se podrá acceder desde el QS del DBForm, de la forma thisForm.nombre</nombre>
              <calc>Puede ser sum, avg, max, min<calc>
              <field>Campo sobre el que se aplicará el cálculo anterior</field>
              <sql>Opcional. Puede definir cómo ha der la SQL para obtener los datos.</sql>
          </infoSubTotal>
      </infoSubTotals>
      */
    Q_PROPERTY(HashStringList infoSubTotals READ infoSubTotals WRITE setInfoSubTotals)
    /** Si se arranca la aplicación por primera vez, éste sera el orden en el que se mostrarán los datos.
    Es una claúsula tal que asi
    id_vendi DESC, fecha ASC...*/
    Q_PROPERTY(QString initOrderSort READ initOrderSort WRITE setInitOrderSort)

    /** Script que se ejecuta al crear un registro, y permite asociarle valores por defecto complejos (por
     * ejemplo, crear ciertos hijos de relaciones */
    Q_PROPERTY(QString onCreateScript READ onCreateScript WRITE setOnCreateScript)

    /** Este código se ejecuta justo antes de insertarse un nuevo bean en base de datos. Si existe como código Qs,
    y no devuelve true, no se realiza la inserción. */
    Q_PROPERTY(QString beforeInsertScript READ beforeInsertScript WRITE setBeforeInsertScript)
    /** Este código se ejecuta justo antes de actualizarse un bean en base de datos. Si existe como código Qs,
    y no devuelve true, no se realiza la inserción. */
    Q_PROPERTY(QString beforeUpdateScript READ beforeUpdateScript WRITE setBeforeUpdateScript)
    /** Este código se ejecuta justo antes de borrarse el bean de base de datos (no al ser marcado para borrar)
    Si existe como código Qs, y no devuelve true, no se realiza la inserción */
    Q_PROPERTY(QString beforeDeleteScript READ beforeDeleteScript WRITE setBeforeDeleteScript)
    /** Valida el valor del bean. Se produce antes de la transacción. */
    Q_PROPERTY(QString validateScript READ validateScript WRITE setValidateScript)
    /** Este código se ejecuta justo después de insertarse un nuevo bean en base de datos. (y antes de un posible commit) */
    Q_PROPERTY(QString afterInsertScript READ afterInsertScript WRITE setAfterInsertScript)
    /** Este código se ejecuta justo después de hacerse una actualización de un bean en base de datos */
    Q_PROPERTY(QString afterUpdateScript READ afterUpdateScript WRITE setAfterUpdateScript)
    /** Este código se ejecuta justo después de borrarse el bean de base de datos (no al ser marcado para borrar)
    Tiene acceso a los datos borrados */
    Q_PROPERTY(QString afterDeleteScript READ afterDeleteScript WRITE setAfterDeleteScript)
    /** Acciones a ejecutar antes de la copia de un bean. Se ejecuta justo antes de guardar en base de datos,
    y cuando el bean destino está creado y con los datos de copia cargados. */
    Q_PROPERTY(QString beforeCopyScript READ beforeCopyScript WRITE setBeforeCopyScript)
    /** Acciones a ejecutar tras la copia de un bean */
    Q_PROPERTY(QString afterCopyScript READ afterCopyScript WRITE setAfterCopyScript)

    /** Habrá un DBField predefinido de visualización. Si no se especifica nada, y quiere mostrarse
      un valor de este DBField, se escogerá este DBField. Se establece en el XML con la cláusula
      defaultVisualizationField */
    Q_PROPERTY(QString defaultVisualizationField READ defaultVisualizationField WRITE setDefaultVisualizationField)
    /** Hay ciertos beans que no cambian habitualmente. Esos beans pueden ser cacheados en memoria (por ejemplo,
      ejercicios fiscales, definiciones de productos)... Esta propiedad marca esos beans, que por tanto
      quedan residentes en memoria para una mejor ejecución del sistema */
    Q_PROPERTY(bool isCached READ isCached WRITE setIsCached)
    Q_PROPERTY(bool localCache READ localCache WRITE setLocalCache)
    /** AlephERP puede proponer un método para realizar un filtrado por usuario. Es decir, será posible especificar
     * si un usuario puede visualizar (=leer) o modificar(incluye también borrar) un registro de una determinada
     * tabla. Esa granularidad se establece por tabla, a través de este flag */
    Q_PROPERTY(bool filterRowByUser READ filterRowByUser WRITE setFilterRowByUser)
    /** Algunas visualizaciones de tablas es más interesantes hacerlas a través de árboles. Se ha escogido el caso
      Familias de Artículos -> Subfamilias de artículos -> Artículos. Se puede definir así
    <showOnTree>
        <relation>
            <!-- Relación a buscar -->
            <name>subfamiliasarticulos</name>
            <!-- Nombre del field que se visualizará -->
            <visibleField>nombre</visibleField>
            <!-- Imagen decorativa a utilizar -->
            <image>:/images/commons/subfamiliasarticulos.png</image>
        </relation>
        <!-- Esta relación familias debe existir en la tabla subfamiliasarticulos -->
        <relation>
            <name>familiasarticulos</name>
            <visibleField>nombre</visibleField>
            <image>:/images/commons/familiasarticulos.png</image>
        </relation>
    </showOnTree>
    */
    Q_PROPERTY(bool showOnTree READ showOnTree WRITE setShowOnTree)
    /** En algunos modelos es interesante precargar, por eficiencia, todos los registros del modelo en árbol. */
    Q_PROPERTY(bool showOnTreePreloadRecords READ showOnTreePreloadRecords WRITE setShowOnTreePreloadRecords)
    /** Interno. Reflejará la estructura anterior de la siguiente forma: Lista de QHash. Esos QHash tendrán los campos del relation antes definidos */
    Q_PROPERTY(QVariantList treeDefinitions READ treeDefinitions WRITE setTreeDefinitions)
    /** Nombre del archivo de systema, de tipo UI, que se utilizará para crear el formulario de Edición de Registros. Si no se especifica,
    se buscará un archivo con el mismo nombre de la tabla seguido de .dbrecord.ui. Podrá definirse por rol. */
    Q_PROPERTY(QString uiDbRecord READ uiDbRecord)
    /** Nombre del archivo de systema, de tipo UI, que se utilizará para crear el formulario de Inserción de Registros. Si no se especifica,
    se buscará un archivo con el mismo nombre de la tabla seguido de .new.dbrecord.ui. Podrá definirse por rol. */
    Q_PROPERTY(QString uiNewDbRecord READ uiNewDbRecord)
    /** Asistente para la introducción de registros. Si no se especifica, se buscarán archivos con el mismo nombre de la tabla seguido de .wizard.1.ui. Podrá definirse por rol. */
    Q_PROPERTY(QString uiWizard READ uiWizard)
    /** Nombre del archivo de systema, de tipo QML, que se utilizará para crear el formulario de Edición de Registros. Si no se especifica,
    se buscará un archivo con el mismo nombre de la tabla seguido de .dbrecord.qml. Podrá definirse por rol. */
    Q_PROPERTY(QString qmlDbRecord READ qmlDbRecord)
    /** Nombre del archivo de systema, de tipo QML, que se utilizará para crear el formulario de Inserción de Registros. Si no se especifica,
    se buscará un archivo con el mismo nombre de la tabla seguido de .new.dbrecord.qml. Podrá definirse por rol. */
    Q_PROPERTY(QString qmlNewDbRecord READ qmlNewDbRecord)
    /** Nombre del archivo de systema, de tipo QS, que se utilizará para crear el formulario de Edición de Registros. Si no se especifica,
    se buscará un archivo con el mismo nombre de la tabla seguido de .dbrecord.qs. Podrá definirse por rol.  */
    Q_PROPERTY(QString qsDbRecord READ qsDbRecord)
    /** Nombre del archivo de systema, de tipo QS, que se utilizará para crear el formulario de Inserción de Registros. Si no se especifica,
    se buscará un archivo con el mismo nombre de la tabla seguido de .new.dbrecord.qs. Podrá definirse por rol. */
    Q_PROPERTY(QString qsNewDbRecord READ qsNewDbRecord)
    /** Puede ser habitual reutilizar código entre formularios, lo que se hace definiendo un prototipo JS para el código contenido a qsDbRecord (herencia
     * de JS). Aquí se puede especificar ese fichero. Podrá definirse por rol.  */
    Q_PROPERTY(QString qsPrototypeDbRecord READ qsPrototypeDbRecord WRITE setQsPrototypeDbRecord)
    Q_PROPERTY(QString qsPrototypeDbWizard READ qsPrototypeDbWizard WRITE setQsPrototypeDbWizard)
    /** Nombre del archivo de systema, de tipo UI, que se utilizará para crear el formulario de Búsqueda de Registros. Si no se especifica,
    se buscará un archivo con el mismo nombre de la tabla seguido de .search.ui. Podrá definirse por rol. */
    Q_PROPERTY(QString uiDbSearch READ uiDbSearch)
    /** Nombre del archivo de systema, de tipo QS, que se utilizará para crear el formulario de Búsqueda de Registros. Si no se especifica,
    se buscará un archivo con el mismo nombre de la tabla seguido de .search.qs. Podrá definirse por rol.  */
    Q_PROPERTY(QString qsDbSearch READ qsDbSearch)
    /** Puede ser habitual reutilizar código entre formularios, lo que se hace definiendo un prototipo JS para el código contenido a qsDbRecord (herencia
     * de JS). Aquí se puede especificar ese fichero. Podrá definirse por rol.  */
    Q_PROPERTY(QString qsPrototypeDbSearch READ qsPrototypeDbSearch WRITE setQsPrototypeDbSearch)
    /** Puede ser habitual reutilizar código entre formularios, lo que se hace definiendo un prototipo JS para el código contenido a qsDbRecord (herencia
     * de JS). Aquí se puede especificar ese fichero */
    Q_PROPERTY(QString qsPrototypeDbForm READ qsPrototypeDbForm WRITE setQsPrototypeDbForm)
    /** Nombre del archivo de systema, de tipo QS, que se utilizará para crear el formulario de Listado Registros. Si no se especifica,
    se buscará un archivo con el mismo nombre de la tabla seguido de .dbform.qs */
    Q_PROPERTY(QString qsDbForm READ qsDbForm)
    /** Esta función Qs se ejecutará para determinar si la capa de acceso DAO de la aplicación, proporciona acceso
     a un determinado registro. Debe usarse con cuidado, ya que al ejecutarse por cada registro, puede enlentecer en exceso
     la aplicación. Es útil para, por ejemplo, poder poner filtros de visualización de registros por roles de usuario */
    Q_PROPERTY(QString accessibleRule READ accessibleRule WRITE setAccessibleRule)
    /** Para los listados (que por ejemplo, presentan los DBForm) es posible determinar un nivel más de filtrado SQL
     a nivel de definición de tabla, que se incluirá en la sentencia SELECT que obtiene los registros. Es útil para, por ejemplo
     filtrados basados en roles o usuarios, y más eficiente que la propiedad accessible. Se suma a los filtros que de por sí establecen
     las variables de entorno. */
    Q_PROPERTY(QString additionalFilter READ additionalFilter WRITE setAdditionalFilter)
    /** Tipo de objeto de base de datos al que representa */
    Q_PROPERTY(AlephERP::DBObjectType dbObjectType READ dbObjectType WRITE setDbObjectType)
    /** Script que permite generar la plantilla o el contenido de lo que se enviará en un posible correo electrónico, relacionado
     * con este registro */
    Q_PROPERTY(QString emailTemplateScript READ emailTemplateScript WRITE setEmailTemplateScript)
    /** ¿Se puede enviar un correo electrónico de este registro? */
    Q_PROPERTY(bool canSendEmail READ canSendEmail WRITE setCanSendEmail)
    /** Para el formulario de búsqueda de direcciones de correo, establece el modelo en el que se buscarán los datos */
    Q_PROPERTY(QString emailContactModel READ emailContactModel WRITE setEmailContactModel)
    /** Es posible además, definir un filtro adicional al modelo anterior. Por ejemplo, tabla terceros es el modelo, y queremos ver
    sólo los terceros marcados como proveedores */
    Q_PROPERTY(QString emailContactModelFilter READ emailContactModelFilter WRITE setEmailContactModelFilter)
    /** Esta será la columna que se visualizará del modelo de contactos */
    Q_PROPERTY(QString emailContactDisplayField READ emailContactDisplayField WRITE setEmailContactDisplayField)
    /** Esta será la columna donde se encuentre el correo electrónico */
    Q_PROPERTY(QString emailContactAddressField READ emailContactAddressField WRITE setEmailContactAddressField)
    /** Ruta al pixmap que representa a este registro */
    Q_PROPERTY(QString pixmapPath READ pixmapPath WRITE setPixmapPath)
    /** Pixmap de este registro */
    Q_PROPERTY(QPixmap pixmap READ pixmap WRITE setPixmap)
    /** Script que permite determinar el color de la fila en la que se presenta el bean, cuando este es listado en un DBTableView. */
    Q_PROPERTY(QString rowColorScript READ rowColorScript WRITE setRowColorScript)
    /** Proporciona un tooltip para este registro, caso de necesitarlo */
    Q_PROPERTY(QString toolTipScript READ toolTipScript WRITE setToolTipScript)
    /** Tooltip para el bean, con expresiones ... más eficiente */
    Q_PROPERTY(QString toolTipStringExpression READ toolTipStringExpression WRITE setToolTipStringExpression)
    /** Indica si este bean puede tener elementos relacionados. Por defecto, será no */
    Q_PROPERTY(bool canHaveRelatedElements READ canHaveRelatedElements WRITE setCanHaveRelatedElements)
    /** Cuando se agrega un nuevo elemento relacionado a este bean, se llama a la función definida en este script.
     * A esta función se le pasa como parámetro el elemento relacionado. Esto permite al bean que va a ser el dueño del elemento
     * relacionado poder definir algunos datos de ese elemento relacionado, por ejemplo.
    function newRelatedElement(var newElement) {
    newElement.id.value = this.id.value;
    }
    */
    Q_PROPERTY(QString newRelatedElementScript READ newRelatedElementScript WRITE setNewRelatedElementScript)
    /** Cuando se modifica el contenido de un elemento relacionado a este bean, se llama a la función definida en este script.
     * A esta función se le pasa como parámetro el elemento relacionado. */
    Q_PROPERTY(QString modifiedRelatedElementScript READ modifiedRelatedElementScript WRITE setModifiedRelatedElementScript)
    /** Cuando se elimina una relación con un elemento relacionado, se llama a la función definida en este script.
     * A esta función se le pasa como parámetro el elemento relacionado. Es función se invoca en el momento en el que
     * el elemento relacionado se marca para ser eliminado (poniendo dbState a TO_BE_DELETE, si el elemento relacionado
     * está en modo edición, o bien justo antes de ser borrado). */
    Q_PROPERTY(QString deletedRelatedElementScript READ deletedRelatedElementScript WRITE setDeletedRelatedElementScript)
    /** Puede ser interesante, que tras borrar este registro, se borren a su vez ciertos registros relacionados, como
     * con una foreign key de una base de datos. Aquí se puede establecer, qué registros relacionados se borrarán.
     * UTILIZAR CON CUIDADO */
    Q_PROPERTY(QList<AlephERP::RelatedElementsContentToBeDeleted> relatedElementsContentToBeDelete READ relatedElementsContentToBeDelete WRITE setRelatedElementsContentToBeDelete)

    /** Si está habilitado el módulo de gestión documental, indica si se pueden asociar documentos a este registro */
    Q_PROPERTY(bool canHaveRelatedDocuments READ canHaveRelatedDocuments WRITE setCanHaveRelatedDocuments)
    /** Cuando se sube un archivo al repositorio de documentos, y está relacionado con un determinado registro, la ruta
     * que este archivo ocupará en el repositorio vendrá determinada por ese registro. Este script deberá devolver la ruta
     * la que almacenar el documento */
    Q_PROPERTY(QString repositoryPathScript READ repositoryPathScript WRITE setRepositoryPathScript)
    /** Al igual que \a repositoryPathScript, este elemento permitirá determinar las palabras claves asociadas al registro */
    Q_PROPERTY(QString repositoryKeywordsScript READ repositoryKeywordsScript WRITE setRepositoryKeywordsScript)
    /** El número de scripts o funciones que puede tener un bean es elevado. Para una mejor organización del código,
     * se puede asociar un fichero .qs con funciones exclusivas para este registro. Ese fichero será un script JS, que definirá
     * un objeto que contendrá funciones (como propiedades) de la siguiente forma
     *
     * ({ variableDefinidaParaEsteJS: "ValorQueQueramosDar",
     * function1: function (posiblesArgumentos) {
     *    Hace Algo y además su objeto this, es el propio bean.
     * },
     * function2: function () {
     * }
     *
     * Podrá invocarse una de estas funciones de la siguiente forma
     * var valorQueDevuelve = bean.function1.call(posiblesArgumentos);
     */
    Q_PROPERTY(AssociatedFunctionsPointerList associatedScripts READ associatedScripts)
    /** Con la anterior funcionalidad, es posible además, definir algunas reglas internas en los datos.
     * Esto se hará con reglas de esta forma
     * <connections>
     *      <connection>
     *          <sender>expresión sencilla en QS que apunta directamente al objeto que genera la señal. Por ejemplo: bean.lineasserviciosfacturasprov</sender>
     *          <action>
     *              <signal>childModified</signal>
     *              <!-- test será un función definida en un associated Script, y le llegarán los parámetros a través de variables -->
     *              <slot>test</slot>
     *              <slot>test2</slot>
     *          </action>
     *          <action>
     *              <signal>childInserted</signal>
     *              <!-- test será un función definida en un associated Script, y le llegarán los parámetros a través de variables -->
     *              <slot>test3</slot>
     *              <slot>test2</slot>
     *          </action>
     *      </connection>
     * </connections>
     * Para coger de forma dinámica los nombres de las señales, la forma que he encontrado depende de modificar el funcionamiento de Qt (redefiniendo
     * las macros SIGNAL/SLOT) lo que puede generar problemas de compatibilidades. Aparte, complica el asunto. Por ello, he decidido definir un conjunto
     * de señales preestablecidas que los objetos que derivan de DBObject (los que tienen interés para esta funcionalidad) pueden disparar. Son:
     * Para emisores de tipo BaseBean:
     * <signal>modified</signal>
     * Invocará al a función QS pasándole además una variable indicando si se ha marcado como modificado o no
     * <signal>fieldModified</signal>
     * Enviará el field que se ha modificado y una variable, con la indicación de si se ha modificado o no
     * <signal>dbStateModified</signal>
     * <signal>beforeSave</signal>
     * <signal>saved</signal>
     * <signal>committed</signal>
     * <signal>endEdit</signal>
     * <signal>relatedElementAdded</signal>
     * <signal>relatedElementDeleted</signal>
     * <signal>relatedElementModified</signal>
     *
     * Para los emisores de tipo DBField
     * <signal>valueModified</signal>
     * <signal>calculatedValueInit</signal>
     * <signal>calculateValueEnd</signal>
     *
     * Para los emisores de tipo DBRelation
     * <signal>childModified</signal>
     * <signal>childInserted</signal>
     * <signal>childSaved</signal>
     * <signal>childCommitted</signal>
     * <signal>childEndEdit</signal>
     * <signal>childDbStateModified</signal>
     * <signal>fieldChildModified</signal>
     * <signal>rootFieldChanged</signal>
     * <signal>fatherLoaded</signal>
     * <signal>fatherUnloaded</signal>
     *
     * Para los emisores tipo RelatedElement
     * <signal>modified</signal>
     * <signal>relatedBeanModified</signal>
     */
    Q_PROPERTY(InternalConnectionList internalConnections READ internalConnections WRITE setInternalConnections)
    /** Proporciona una representación (preferiblemente en HTML) adecuada para el usuario de los datos del bean. Será muy útil
     * para toolTips (cuando no esté definido uno por defecto) o para mostrar elementos relacionados */
    Q_PROPERTY(QString toStringScript READ toStringScript WRITE setToStringScript)
    /** La anterior expresión puede ser o bien una expresión js, o bien una expresión internal... aquí determinamos el tipo */
    Q_PROPERTY(QString toStringType READ toStringType WRITE setToStringType)
    /** En transacciones puede ser interesante indicar el órden de los elementos involucrados en ésta */
    Q_PROPERTY(int orderOnTransaction READ orderOnTransaction)
    /** Url al archivo de ayuda */
    Q_PROPERTY(QString helpUrl READ helpUrl WRITE setHelpUrl)
    Q_PROPERTY(QString pkOrder READ pkOrder)
    Q_PROPERTY(QString sqlSelectPk READ sqlSelectPk)
    Q_PROPERTY(QList<DBFieldMetadata *> fields READ fields)
    Q_PROPERTY(QList<DBRelationMetadata *> relations READ relations)
    /** Entrada en el menú de la ventana principal, a modo de ruta */
    Q_PROPERTY(QString menuEntryPath READ menuEntryPath WRITE setMenuEntryPath)
    /** Lo mismo que la entrada de menú pero para la barra de herramientas */
    Q_PROPERTY(QString moduleToolBarEntryPath READ moduleToolBarEntryPath WRITE setModuleToolBarEntryPath)
    /** Lo mismo que la entrada de menú pero para la barra de herramientas */
    Q_PROPERTY(QString readOnlyScript READ readOnlyScript WRITE setReadOnlyScript)
    /** Los modelos que muestran todos los registros (DBBaseBeanModel, TreeBaseBeanModel), recargan constantemente
     * los datos en segundo plano... Esto podría incrementar mucho la carga en base de datos. Es posible que el modelo
     * permanezca estático (no produzca la recarga). Se debe poner esta variable a true */
    Q_PROPERTY(bool staticModel READ staticModel WRITE setStaticModel)
    /** Indica si algún campo es editable en los DBFormDlg */
    Q_PROPERTY(bool editOnDbForm READ editOnDbForm)

private:
    Q_DISABLE_COPY(BaseBeanMetadata)
    BaseBeanMetadataPrivate *d;

protected:
    void consolidateTemp();

    static void initRegisterFieldsInvolvedOnCalc();
    static void endRegisterFieldsInvolvedOnCalc();
    static QList<DBField *> fieldsInvolvedOnCalc();


public:
    explicit BaseBeanMetadata(QObject *parent = NULL);
    virtual ~BaseBeanMetadata();

    QString xml() const;
    void setXml (const QString &value);
    QString tableName() const;
    void setTableName(const QString &value);
    bool canNavigate() const;
    void setCanNavigate(bool value);
    QString sqlTableName(const QString &dialect = "");
    QString schema() const;
    void setSchema(const QString &value);
    void setAlias(const QString &alias);
    QString alias() const;
    bool staticModel() const;
    void setStaticModel(bool value);
    bool readOnly() const;
    void setReadOnly(bool value);
    void setModule(AERPSystemModule *module);
    AERPSystemModule *module() const;
    QString viewForTable() const;
    void setViewForTable(const QString &query);
    void setViewOnGrid(const QString &query);
    QString viewOnGrid() const;
    bool viewProvidesOid() const;
    void setViewProvidesOid(bool value);
    void setCreationSqlView(const QHash<QString, QString> &value);
    QString creationSqlView(const QString &dialect) const;
    bool logicalDelete() const;
    void setLogicalDelete(bool value);
    void setItemsFilterColumn(const QList<QHash<QString, QString> > &alias);
    QList<QHash<QString, QString> > itemsFilterColumn() const;
    QList<EnvVarDefinition> envVars() const;
    QString initOrderSort() const;
    void setInitOrderSort(const QString &value);
    QString onCreateScript() const;
    void setOnCreateScript(const QString &value);
    QString beforeInsertScript() const;
    void setBeforeInsertScript(const QString &value);
    QString beforeUpdateScript() const;
    void setBeforeUpdateScript(const QString &value);
    QString beforeDeleteScript() const;
    void setBeforeDeleteScript(const QString &value);
    QString validateScript() const;
    void setValidateScript(const QString &value);
    QString beforeSaveScript() const;
    void setBeforeSaveScript(const QString &value);
    QString afterSaveScript();
    void setAfterSaveScript(const QString &value);
    QString afterInsertScript() const;
    void setAfterInsertScript(const QString &value);
    QString afterUpdateScript() const;
    void setAfterUpdateScript(const QString &value);
    QString afterDeleteScript() const;
    void setAfterDeleteScript(const QString &value);
    QString beforeCopyScript() const;
    void setBeforeCopyScript(const QString &value);
    QString afterCopyScript() const;
    void setAfterCopyScript(const QString &value);
    QString emailTemplateScript() const;
    void setEmailTemplateScript(const QString &value);
    QString emailSubjectScript() const;
    void setEmailSubjectScript(const QString &value);
    bool canSendEmail() const;
    void setCanSendEmail(bool value);
    QString pixmapPath() const;
    void setPixmapPath(const QString &value);
    QPixmap pixmap() const;
    void setPixmap(const QPixmap &value);
    bool canHaveRelatedElements() const;
    void setCanHaveRelatedElements(bool value);
    QString newRelatedElementScript() const;
    void setNewRelatedElementScript(const QString &value);
    QString modifiedRelatedElementScript() const;
    void setModifiedRelatedElementScript(const QString &value);
    QString deletedRelatedElementScript() const;
    void setDeletedRelatedElementScript(const QString &value);
    bool canHaveRelatedDocuments() const;
    void setCanHaveRelatedDocuments(bool value);
    QList<AlephERP::RelatedElementsContentToBeDeleted> relatedElementsContentToBeDelete() const;
    void setRelatedElementsContentToBeDelete(const QList<AlephERP::RelatedElementsContentToBeDeleted> &value);
    QString repositoryPathScript() const;
    void setRepositoryPathScript(const QString &value);
    QString repositoryKeywordsScript() const;
    void setRepositoryKeywordsScript(const QString &value);
    AssociatedFunctionsPointerList associatedScripts() const;
    void setAssociatedScripts(const AssociatedFunctionsPointerList &value);
    QScriptProgram associatedScriptProgram(const QString &functionName);
    InternalConnectionList internalConnections() const;
    void setInternalConnections(const InternalConnectionList &value);
    QString toStringScript () const;
    void setToStringScript(const QString &value);
    QString toStringType () const;
    void setToStringType(const QString &value);
    HashStringList infoSubTotals() const;
    void setInfoSubTotals(const HashStringList &v);
    bool filterRowByUser() const;
    void setFilterRowByUser(bool value);
    const ScheduleData scheduledData() const;
    int orderOnTransaction() const;
    QString helpUrl() const;
    void setHelpUrl(const QString &value);
    QString menuEntryPath() const;
    void setMenuEntryPath(const QString &value);
    QString moduleToolBarEntryPath() const;
    void setModuleToolBarEntryPath(const QString &value);
    QString readOnlyScript() const;
    void setReadOnlyScript(const QString &value);
    bool editOnDbForm() const;

    bool showSomeRelationOnRelatedElementsModel();

    bool debug();
    void setDebug(bool value);
    bool onInitDebug();
    void setOnInitDebug(bool value);

    QString defaultVisualizationField();
    void setDefaultVisualizationField(const QString &value);
    bool isCached() const;
    void setIsCached(bool value);
    bool localCache() const;
    void setLocalCache(bool value);
    bool showOnTree();
    void setShowOnTree(bool value);
    bool showOnTreePreloadRecords() const;
    void setShowOnTreePreloadRecords(bool value);
    QVariantList treeDefinitions ();
    void setTreeDefinitions (const QVariantList &value);

    QHash<QString, QString> uiDbRecordForRoles() const;
    QString uiDbRecord() const;
    void setUiDbRecord(const QHash<QString, QString> &value);

    QHash<QString, QString> uiNewDbRecordForRoles() const;
    QString uiNewDbRecord() const;
    void setUiNewDbRecord(const QHash<QString, QString> &value);

    QHash<QString, QString> uiWizardForRoles() const;
    QString uiWizard() const;
    void setUiWizard(const QHash<QString, QString> &value);

    QHash<QString, QString> qmlDbRecordForRoles() const;
    QString qmlDbRecord() const;
    void setQmlDbRecord(const QHash<QString, QString> &value);

    QHash<QString, QString> qmlNewDbRecordForRoles() const;
    QString qmlNewDbRecord() const;
    void setQmlNewDbRecord(const QHash<QString, QString> &value);

    QHash<QString, QString> qsDbRecordForRoles() const;
    QString qsDbRecord() const;
    void setQsDbRecord(const QHash<QString, QString> &value);

    QHash<QString, QString> qsNewDbRecordForRoles() const;
    QString qsNewDbRecord() const;
    void setQsNewDbRecord(const QHash<QString, QString> &value);

    QString qsPrototypeDbRecord() const;
    void setQsPrototypeDbRecord(const QString &value);

    QString qsPrototypeDbSearch() const;
    void setQsPrototypeDbSearch(const QString &value);

    QString qsPrototypeDbWizard() const;
    void setQsPrototypeDbWizard(const QString &value);

    QString qsPrototypeDbForm() const;
    void setQsPrototypeDbForm(const QString &value);

    QHash<QString, QString> uiDbSearchForRoles() const;
    QString uiDbSearch() const;
    void setUiDbSearch(const QHash<QString, QString> &value);

    QHash<QString, QString> qsDbSearchForRoles() const;
    QString qsDbSearch() const;
    void setQsDbSearch(const QHash<QString, QString> &value);

    QHash<QString, QString> qsDbFormForRoles() const;
    QString qsDbForm() const;
    void setQsDbForm(const QHash<QString, QString> &value);

    QString accessibleRule();
    void setAccessibleRule(const QString &value);
    QString additionalFilter();
    void setAdditionalFilter(const QString &value);
    AlephERP::DBObjectType dbObjectType() const;
    void setDbObjectType(AlephERP::DBObjectType arg);
    QString emailContactModel() const;
    void setEmailContactModel(const QString &value);
    QString emailContactModelFilter() const;
    void setEmailContactModelFilter(const QString &value);
    QString emailContactDisplayField() const;
    void setEmailContactDisplayField(const QString &value);
    QString emailContactAddressField() const;
    void setEmailContactAddressField(const QString &value);
    QString rowColorScript() const;
    void setRowColorScript(const QString &value);
    QString toolTipScript() const;
    void setToolTipScript(const QString &value);
    QString toolTipStringExpression() const;
    void setToolTipStringExpression(const QString &value);

    Q_INVOKABLE QString metadataVar(const QString &varName) const;

    QHash<QString, QString> sql() const;

    QList<DBFieldMetadata *> fields();
    QStringList dbFieldNames() const;

    int countSerialFields();

    QList<DBRelationMetadata *> relations(AlephERP::RelationTypes type = AlephERP::All);

    QList<DBFieldMetadata *> pkFields ();
    int fieldCount();
    Q_INVOKABLE DBFieldMetadata * field(const QString &dbFieldName);
    Q_INVOKABLE DBFieldMetadata * field(int index);
    int fieldIndex(const QString &dbFieldName);
    QString pkWhere(const QVariantMap &list);
    QString pkOrder();
    QString sqlSelectPk();

    bool isScheduleValid();

    DBRelationMetadata * relation(const QString &table);

    QString sqlCreateTable(AlephERP::CreationTableSqlOptions options, const QString &dialect, const QString &alternativeName = "");
    QStringList sqlForeignKeys(AlephERP::CreationTableSqlOptions options, const QString &dialec);
    QString sqlCreateIndexes(AlephERP::CreationTableSqlOptions options, const QString &dialect);
    QString sqlAddColumn(AlephERP::CreationTableSqlOptions options, const QString &dbFieldName, const QString &dialect);
    QString sqlDropColumn(AlephERP::CreationTableSqlOptions options, const QString &dbFieldName, const QString &dialect);
    QString sqlMakeNotNull(AlephERP::CreationTableSqlOptions options, const QString &dbFieldName, const QString &dialect);
    QString sqlAlterColumnSetLength(AlephERP::CreationTableSqlOptions options, const QString &dbFieldName, const QString &dialect);
    QStringList sqlAditional(AlephERP::CreationTableSqlOptions options, const QString &dialect);

    QString processToIncludePrefix(const QString &where, const QString &prefix);
    QString processWhereSqlToIncludeEnvVars(const QString &initialWhere = "", const QString &prefix = "");
    QStringList fieldsOnSqlClausule(const QString &clausule);
    QString whereFromAdditionalFilter();
    QString emailTemplateScriptExecute(BaseBean *bean);
    QString emailSubjectScriptExecute(BaseBean *b);
    QString repositoryPathScriptExecute(BaseBean *b);
    QString toStringExecute(BaseBean *b);
    QStringList repositoryKeywordsScriptExecute(BaseBean *b);
    void afterInsertScriptExecute(BaseBean *bean);
    void afterUpdateScriptExecute(BaseBean *bean);
    void afterDeleteScriptExecute(BaseBean *bean);
    void afterSaveScriptExecute(BaseBean *bean);
    void afterCopyScriptExecute(BaseBean *beanOriginal, BaseBean *beanDest);
    void onCreateScriptExecute(BaseBean *bean);
    QString validateScriptExecute(BaseBean *bean);
    bool beforeInsertScriptExecute(BaseBean *bean);
    bool beforeUpdateScriptExecute(BaseBean *bean);
    bool beforeDeleteScriptExecute(BaseBean *bean);
    bool beforeSaveScriptExecute(BaseBean *bean);
    bool beforeCopyScriptExecute(BaseBean *beanOriginal, BaseBean *beanDest);
    bool isAccessible(BaseBean *bean);
    QColor rowColorScriptExecute(BaseBean *b);
    QString toolTipExecute(BaseBean *b);
    void newRelatedElementScriptExecute(BaseBean *bean, RelatedElement *element);
    void modifiedRelatedElementScriptExecute(BaseBean *bean, RelatedElement *element);
    void deletedRelatedElementScriptExecute(BaseBean *bean, RelatedElement *element);
    bool readOnlyScriptExecute(BaseBean *bean);

    QStringList associatedScriptFunctions();
    QString associatedScript(const QString &functionName);

    void createInternalConnections(BaseBean *bean);

    const QList<DBFieldMetadata *> counterFields(const QString &field) const;

    static QScriptValue toScriptValue(QScriptEngine *engine, BaseBeanMetadata * const &in);
    static void fromScriptValue(const QScriptValue &object, BaseBeanMetadata * &out);

    QString calculateScheduleHtmlForeground(BaseBean *bean);
    QString calculateScheduleHtmlBackground(BaseBean *bean);
    QString calculateScheduleTitleExpression(BaseBean *fld);

    QObject *navigateThroughProperties(const QString &path, bool returnIntermediate = false);

    QStringList fieldsNecessaryToCalculate(const QString &dbFieldName);
    void registerRecalculateFields();
    void registerRecalculateFields(const QString &fieldToRecalculate, const QStringList &fielsOnCalc);

    static void fieldIsAccesed(DBField *fld);

protected slots:
    void processScriptSignalException(const QScriptValue &exception);
    void buildFieldsCalculatedRelations();
};

Q_DECLARE_METATYPE(BaseBeanMetadata*)
Q_DECLARE_METATYPE(QList<BaseBeanMetadata *>)
Q_SCRIPT_DECLARE_QMETAOBJECT(BaseBeanMetadata, QObject*)
Q_DECLARE_METATYPE(EnvVarDefinition)

#endif // BASEBEANMETADATA_H
