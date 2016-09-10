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
#include <QtScript>
#include <QtXml>
#include "configuracion.h"
#include <aerpcommon.h>
#include "basebeanmetadata.h"
#include "qlogger.h"
#include "dao/beans/basebean.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/relatedelement.h"
#include "dao/beans/aerpsystemobject.h"
#include "dao/beans/basebeanqsfunction.h"
#include "dao/basedao.h"
#include "dao/database.h"
#include "dao/systemdao.h"
#include "models/envvars.h"
#include "scripts/perpscript.h"
#include "business/aerploggeduser.h"
#ifdef ALEPHERP_FIREBIRD_SUPPORT
#include "dao/aerpfirebirddatabase.h"
#endif
#ifdef ALEPHERP_BARCODE
#include "widgets/dbbarcode.h"
#endif

class BaseBeanMetadataPrivate
{
    Q_DECLARE_PUBLIC(BaseBeanMetadata)
public:
    BaseBeanMetadata *q_ptr;
    /** Configuración en XML del bean: Campos, relaciones... */
    QString m_xml;
    /** Tabla de la base de datos a la que hace referencia */
    QString m_tableName;
    /** Esquema de la base de datos, si aplica, en el que se encuentra la tabla */
    QString m_schema;
    /** Esta es la vista o consulta que se utiliza para obtener los datos que se presentan en el ListView.
    Debe ser una lista o consulta que exista en las definiciones de tablas de AlephERP */
    QString m_viewOnGrid;
    /** Si el metadato es una vista, este valor indica de qué tabla es vista */
    QString m_viewForTable;
    /** AlephERP trabajan intensivamente con el OID de base de datos. Cuando se utilizan vistas de base de datos, es interesante
     * saber si ésta provee el OID de la tabla referencia */
    bool m_viewProvidesOid;
    /** Esta versión permite utilizar vistas en el sistema. (vistas de base de datos). Para el modo de trabajo local,
     necesitamos conocerlas, al igual que necesitaremos la consulta de creación de esa vista, para recrearla. De ahí este campo */
    QHash<QString, QString> m_creationSqlView;
    /** Nombre bonito de la tabla */
    QString m_alias;
    bool m_canNavigate;
    /** Módulo al que pertenece el bean */
    QPointer<AERPSystemModule> m_module;
    /** Listado de los fields que admite este bean */
    QList<DBFieldMetadata *> m_fields;
    /** Listado de relations que se admiten */
    QList<DBRelationMetadata *> m_relations;
    /** Consulta SQL que se utiliza para obtener los datos que será editados. */
    QHash<QString, QString> m_sql;
    /** Indica si el borrado del bean es lógico o físico. Si es lógico, el registro
      se marcará como borrado. Para ello, la tabla deberá tener un campo "is_deleted" */
    bool m_logicalDelete;
    /** En transacciones puede ser interesante indicar el órden de los elementos involucrados en ésta */
    int m_orderOnTransaction;
    /** Cuando este bean se presenta en un DBForm, puede querer filtrarse mediante
    un filtro fuerte, que se deriva de los valores de otra tabla, o de los valores
    de un option list de ese campo. Por ejemplo,
    presupuestos_cabecera tiene un campo, estado, que coge datos de los registros
    marcados en la tabla presupuestos_definicion_estados. El contenido de esta tabla
    se incluirá en el combo. ¿Cómo? se introducirán valores de la siguiente forma:
    <itemsFilterColumn>
        <itemFilter>
            <fieldToFilter>id_tienda</fieldToFilter>
            <relationFieldToShow>nombre</relationFieldToShow>
            <order>another_field_on_relation_to_order</order>
            <!-- Si por ejemplo se crea un registro nuevo, con este filtro activo, y este valor está a true,
            se precargará como valor por defecto en el nuevo registro creado, el del filtro. Por ejemplo,
            si tenemos filtrado los ejercicios para la empresa EMPRESA 1, al crear un nuevo ejercicio, éste
            se creará como hijo de la empresa 1, es decir, su campo codempresa se pondrá al del ID EMPRESA 1 -->
            <setFilterValueOnNewRecords>true</setFilterValueOnNewRecords>
            <relationFilter>compra='true'</relationFilter>
            <relationFilterScript></relationFilterScript>
        </itemFilter>
        <itemFilter>
            <fieldToFilter>codejercicio</fieldToFilter>
            <relationFieldToShow>ejercicio</relationFieldToShow>
            <order>another_field_on_relation_to_order</order>
            <viewAllOption>false</viewAllOption>
        </itemFilter>
    <itemFilter>
        <fieldToFilter>estado</fieldToFilter>
    </itemFilter>
    </itemsFilterColumn>
    Esto sólo tendrá sentido en relaciones M1 y en campos
    */
    QList<QHash<QString, QString> > m_itemsFilterColumn;
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
      y el QStringList quedaría: "idTienda;id_tienda"
      */
    QList<EnvVarDefinition> m_envVars;
    /** Orden inicial de visualización */
    QString m_initOrderSort;
    /** Script a ejecutar tras el borrado en base de datos */
    QString m_afterDeleteScript;
    /** Script a ejecutar tras la inserción en base de datos */
    QString m_afterInsertScript;
    /** Script a ejecutar tras la actualización en base de datos */
    QString m_afterUpdateScript;
    /** Script a ejecutar antes del borrado en base de datos */
    QString m_beforeDeleteScript;
    /** Script que se ejecuta al crear un registro, y permite asociarle valores por defecto complejos (por
     * ejemplo, crear ciertos hijos de relaciones */
    QString m_onCreateScript;
    /** Script a ejecutar antes de la inserción en base de datos */
    QString m_beforeInsertScript;
    /** Script a ejecutar antes de la actualización en base de datos */
    QString m_beforeUpdateScript;
    /** Script a ejectuar antes de guardar en base de datos (esto es, antes de un insert o un update). Están dentro de la transacción */
    QString m_beforeSaveScript;
    /** Script a ejecutar después de guardar en base de datos (tras un insert o un update). Dentro de la transacción */
    QString m_afterSaveScript;
    /** Acciones a ejecutar antes de la copia de un bean */
    QString m_beforeCopyScript;
    /** Acciones a ejecutar tras la copia de un bean */
    QString m_afterCopyScript;
    /** Valida el valor del bean. Se produce antes de la transacción. */
    QString m_validateScript;
    /** Para los scripts asociados */
    AERPScript *m_engine;
    /** Opciones de debug */
    bool m_debug;
    bool m_onInitDebug;
    /** Field de visualización por defecto */
    QString m_defaultVisualizationField;
    /** Hay ciertos beans que no cambian habitualmente. Esos beans pueden ser cacheados en memoria (por ejemplo,
      ejercicios fiscales, definiciones de productos)... Esta propiedad marca esos beans, que por tanto
      quedan residentes en memoria para una mejor ejecución del sistema */
    bool m_isCached;
    bool m_localCache;
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
    bool m_showOnTree;
    /** En algunos modelos es interesante precargar, por eficiencia, todos los registros del modelo en árbol. */
    bool m_showOnTreePreloadRecords;
    /** Interno. Reflejará la estructura anterior de la siguiente forma: Lista de QHash. Esos QHash tendrán los campos del relation antes definidos */
    QVariantList m_treeDefinitions;
    /** Nombre del archivo de systema, de tipo UI, que se utilizará para crear el formulario de Edición de Registros. Si no se especifica,
    se buscará un archivo con el mismo nombre de la tabla seguido de .dbrecord.ui*/
    QHash<QString, QString> m_uiDbRecord;
    /** Nombre del archivo de systema, de tipo UI, que se utilizará para crear el formulario de Inserción de Registros. Si no se especifica,
    se buscará un archivo con el mismo nombre de la tabla seguido de .new.dbrecord.ui*/
    QHash<QString, QString> m_uiNewDbRecord;
    /** Nombre del wizard, si hay, para introducir un nuevo registro */
    QHash<QString, QString> m_uiWizard;
    /** Nombre del archivo de systema, de tipo QML, que se utilizará para crear el formulario de Edición de Registros. Si no se especifica,
    se buscará un archivo con el mismo nombre de la tabla seguido de .dbrecord.qml*/
    QHash<QString, QString> m_qmlDbRecord;
    /** Nombre del archivo de systema, de tipo QML, que se utilizará para crear el formulario de Inserción de Registros. Si no se especifica,
    se buscará un archivo con el mismo nombre de la tabla seguido de .new.dbrecord.qml*/
    QHash<QString, QString> m_qmlNewDbRecord;
    /** Nombre del archivo de systema, de tipo QS, que se utilizará para crear el formulario de Edición de Registros. Si no se especifica,
    se buscará un archivo con el mismo nombre de la tabla seguido de .dbrecord.qs*/
    QHash<QString, QString> m_qsDbRecord;
    /** Nombre del archivo de systema, de tipo QS, que se utilizará para crear el formulario de Inserción de Registros. Si no se especifica,
    se buscará un archivo con el mismo nombre de la tabla seguido de .new.dbrecord.qs*/
    QHash<QString, QString> m_qsNewDbRecord;
    /** Nombre del archivo de systema, de tipo UI, que se utilizará para crear el formulario de Búsqueda de Registros. Si no se especifica,
    se buscará un archivo con el mismo nombre de la tabla seguido de .search.ui*/
    QHash<QString, QString> m_uiDbSearch;
    /** Nombre del archivo de systema, de tipo QS, que se utilizará para crear el formulario de Búsqueda de Registros. Si no se especifica,
    se buscará un archivo con el mismo nombre de la tabla seguido de .search.qs*/
    QHash<QString, QString> m_qsDbSearch;
    /** Nombre del archivo de systema, de tipo QS, que se utilizará para crear el formulario de Listado de Registros. Si no se especifica,
    se buscará un archivo con el mismo nombre de la tabla seguido de .dbform.qs*/
    QHash<QString, QString> m_qsDbForm;
    QString m_qsPrototypeDbForm;
    QString m_qsPrototypeDbSearch;
    QString m_qsPrototypeDbRecord;
    QString m_qsPrototypeDbWizard;
    /** El sistema permitirá a algunas tablas trabajar sin conexión. Para trabajar con ellas, deberá descargarse el contenido
     de las mismas a una base de datos local, desde la base de datos remota. Los cambios se almacenarán en una base de datos
     local hasta que el usuario realice un commit a la base de datos remota, momento en el que se suben todos los datos.
     Esta opción debe utilizarse con especial cuidado y sólo para tablas en las que es seguro que hay pocos usuarios
     trabajando y no se generen conflictos por duplicidad de datos, ya que el sistema no puede comprobar eso
     (por ejemplo, crear dos veces el cliente... etc). Es especialmente delicado los campos calculados estilo contadores
     o los campos seriales.
     Por ejemplo: Para el trabajo con facturas de proveedores: Será necesario descargar previamente todas aquellas tablas,
     que no están marcadas como cacheadas (estas ya de por sí son eficientes), sino aquellas que deben trabajar sin conexión.
     Por ejemplo, clientes.
     Con esta opción marcamos si esa tabla está preparada para trabajar con ella en versión batch
     */
    bool m_allowLocalWork;
    /** Esta función Qs se ejecutará para determinar si la capa de acceso DAO de la aplicación, proporciona acceso
     a un determinado registro. Debe usarse con cuidado, ya que al ejecutarse por cada registro, puede enlentecer en exceso
     la aplicación. Es útil para, por ejemplo, poder poner filtros de visualización de registros por roles de usuario */
    QString m_accessible;
    /** Para los listados (que por ejemplo, presentan los DBForm) es posible determinar un nivel más de filtrado SQL
     a nivel de definición de tabla, que se incluirá en la sentencia SELECT que obtiene los registros. Es útil para, por ejemplo
     filtrados basados en roles o usuarios, y más eficiente que la propiedad accessible. Se suma a los filtros que de por sí establecen
     las variables de entorno. */
    QString m_additionalFilter;
    /** Tipo de objeto de base de datos */
    AlephERP::DBObjectType m_dbObjectType;
    /** ¿Se puede enviar un correo electrónico de este registro? */
    bool m_canSendEmail;
    /** Script para la generación de la plantilla de un correo */
    QString m_emailTemplateScript;
    QString m_emailContactModel;
    QString m_emailContactModelFilter;
    QString m_emailContactDisplayField;
    QString m_emailContactAddressField;
    QString m_emailSubjectScript;
    QString m_pixmapPath;
    QPixmap m_pixmap;
    QString m_rowColorScript;
    QString m_toolTipScript;
    QString m_toolTipStringExpression;
    bool m_canHaveRelatedElements;
    QString m_newRelatedElementScript;
    QString m_modifiedRelatedElementScript;
    QString m_deletedRelatedElementScript;
    /** Cuando se sube un archivo al repositorio de documentos, y está relacionado con un determinado registro, la ruta
     * que este archivo ocupará en el repositorio vendrá determinada por ese registro. Este script deberá devolver la ruta
     * la que almacenar el documento */
    QString m_repositoryPathScript;
    /** Al igual que \a repositoryPathScript, este elemento permitirá determinar las palabras claves asociadas al registro */
    QString m_repositoryKeywordsScript;
    /** Estructura con funciones asociados */
    AssociatedFunctionsPointerList m_associatedFunctions;
    InternalConnectionList m_internalConnections;
    QString m_toStringScript;
    QString m_toStringType;
    StringExpression m_toStringExpression;
    StringExpression m_toolTipExpression;
    HashStringList m_infoSubTotals;
    bool m_filterRowByUser;
    bool m_canHaveRelatedDocuments;
    QList<AlephERP::RelatedElementsContentToBeDeleted> m_relatedElementsContentToBeDelete;
    ScheduleData m_scheduledData;
    QString m_helpUrl;
    QString m_menuEntryPath;
    QString m_toolBarEntryPath;
    QHash<QString, QString> m_metadataVars;
    /** Esta estructura indica qué campos deberán ser recalculados cuando se modifique el campo calculado que hace de key */
    QHash<QString, QStringList> m_fieldsNecessaryToCalculate;
    bool m_readOnly;
    QString m_readOnlyScript;
    bool m_static;

    static bool m_registerFieldsInvolvedOnCalc;
    static QList<DBField *> m_fieldsInvolvedOnCalc;

    BaseBeanMetadataPrivate(BaseBeanMetadata *qq) : q_ptr(qq)
    {
        m_logicalDelete = false;
        m_isCached = false;
        m_showOnTree = false;
        m_showOnTreePreloadRecords = false;
        m_allowLocalWork = false;
        m_dbObjectType = AlephERP::NotValid;
        m_canSendEmail = false;
        m_engine = NULL;
        m_canHaveRelatedElements = false;
        m_viewProvidesOid = false;
        m_filterRowByUser = false;
        m_canHaveRelatedDocuments = false;
        m_scheduledData.canMove = false;
        m_scheduledData.canOverlap = false;
        m_scheduledData.showViewModesFilter = false;
        m_scheduledData.viewMode = AERPScheduleView::WeekView;
        m_debug = false;
        m_onInitDebug = false;
        m_orderOnTransaction = -1;
        m_readOnly = false;
        m_localCache = false;
        m_static = false;
        m_canNavigate = true;
    }

    void importHeritanceElementsOnDom(QDomDocument &document, QDomDocument &otherDocument, QDomNode inheritNode);
    QDomNode importNode(QDomDocument &document, QDomNode &inheritNode, QDomNode &nodeToImport);
    QString checkWildCards(QDomElement &element);
    QString checkWildCards(QDomNode &node);
    void setConfig();
    void readDBRelation(const QDomElement &e, DBFieldMetadata *field);
    BuiltInExpressionDef readBuiltInExpression(const QDomElement &e);
    StringExpression readStringExpression(const QDomElement &e);
    void readAggregate(const QDomElement &e, DBFieldMetadata *field);
    void readDependOnFieldsToCalc(const QDomElement &e, DBFieldMetadata *field);
    void readSql(const QDomElement &e);
    void readItemsFilterColumn(const QDomElement &e);
    void readEnvVarsFilter(const QDomElement &e);
    void readFont(const QDomElement &e, DBFieldMetadata *field);
    void readCounterDefinition(const QDomElement &e, DBFieldMetadata *field);
    void readTreeDefinition(const QDomElement &e);
    void readBehaviourOnInlineEdit(const QDomElement &e, DBFieldMetadata *fld);
    void readRelatedElementsContentToBeDelete(const QDomNode &e);
    void readInfoSubTotal(const QDomElement &e);
    void loadAssociatedScripts();
    void readInternalConnections(const QDomElement &e);
    void readScheduledData(const QDomElement &e);
    void addSqlPart(const QString &clause, const QString &value);
    QList<QObject *> senders(const QString &senderQs, BaseBean *bean);
    void createInternalConnectionForBean(BaseBean *sender, const QString &signalName, BaseBeanQsFunction *qsFunction);
    void createInternalConnectionForRelation(DBRelation *sender, const QString &signalName, BaseBeanQsFunction *qsFunction);
    void createInternalConnectionForField(DBField *sender, const QString &signalName, BaseBeanQsFunction *qsFunction);
    void createInternalConnectionForRelatedElement(RelatedElement *sender, const QString &signalName, BaseBeanQsFunction *qsFunction);
    int countEnvVarForOneField(const QString &fieldName);
    void readFormsConfigNames(const QDomElement &root);
};

bool BaseBeanMetadataPrivate::m_registerFieldsInvolvedOnCalc;
QList<DBField *> BaseBeanMetadataPrivate::m_fieldsInvolvedOnCalc;

void BaseBeanMetadata::consolidateTemp()
{
    foreach (DBFieldMetadata *f, d->m_fields) {
        f->consolidateTemp();
    }
}

BaseBeanMetadata::BaseBeanMetadata(QObject *parent) : QObject(parent), d(new BaseBeanMetadataPrivate(this))
{
    d->m_engine = new AERPScript(this);
    d->m_engine->setDebug(d->m_debug);
    d->m_engine->setOnInitDebug(d->m_onInitDebug);
    connect(d->m_engine->engine(), SIGNAL(signalHandlerException(QScriptValue)), this, SLOT(processScriptSignalException(QScriptValue)));
}

BaseBeanMetadata::~BaseBeanMetadata()
{
    qDeleteAll(d->m_associatedFunctions);
    delete d;
}

QString BaseBeanMetadata::tableName() const
{
    return d->m_tableName;
}

void BaseBeanMetadata::setTableName(const QString &value)
{
    d->m_tableName = value;
    d->m_dbObjectType = BaseDAO::databaseObjectType(this);
}

bool BaseBeanMetadata::canNavigate() const
{
    return d->m_canNavigate;
}

void BaseBeanMetadata::setCanNavigate(bool value)
{
    d->m_canNavigate = value;
}

QString BaseBeanMetadata::sqlTableName(const QString &dialect)
{
    QString dbDialect;
    if ( dialect.isEmpty() )
    {
        dbDialect = Database::driverConnection();
    }
    else
    {
        dbDialect = dialect;
    }
    QString name = d->m_tableName;
    if ( dbDialect == QLatin1String("QIBASE") )
    {
        if ( d->m_tableName.size() > MAX_LENGTH_OBJECT_NAME_FIREBIRD )
        {
            name = d->m_tableName.left(MAX_LENGTH_OBJECT_NAME_FIREBIRD);
        }
    }
    if ( dbDialect == QLatin1String("QPSQL") )
    {
        if ( d->m_tableName.size() > MAX_LENGTH_OBJECT_NAME_PSQL )
        {
            name = d->m_tableName.left(MAX_LENGTH_OBJECT_NAME_PSQL);
        }
    }
    if ( d->m_schema.isEmpty() )
    {
        return name;
    }
    return QString("%1.%2").arg(d->m_schema).arg(name);
}

AlephERP::DBObjectType BaseBeanMetadata::dbObjectType() const
{
    return d->m_dbObjectType;
}

void BaseBeanMetadata::setDbObjectType(AlephERP::DBObjectType arg)
{
    d->m_dbObjectType = arg;
}

QString BaseBeanMetadata::emailContactModel() const
{
    return d->m_emailContactModel;
}

void BaseBeanMetadata::setEmailContactModel(const QString &value)
{
    d->m_emailContactModel = value;
}

QString BaseBeanMetadata::emailContactModelFilter() const
{
    return d->m_emailContactModelFilter;
}

void BaseBeanMetadata::setEmailContactModelFilter(const QString &value)
{
    d->m_emailContactModelFilter = value;
}

QString BaseBeanMetadata::emailContactDisplayField() const
{
    return d->m_emailContactDisplayField;
}

void BaseBeanMetadata::setEmailContactDisplayField(const QString &value)
{
    d->m_emailContactDisplayField = value;
}

QString BaseBeanMetadata::emailContactAddressField() const
{
    return d->m_emailContactAddressField;
}

void BaseBeanMetadata::setEmailContactAddressField(const QString &value)
{
    d->m_emailContactAddressField = value;
}

QString BaseBeanMetadata::rowColorScript() const
{
    return d->m_rowColorScript;
}

void BaseBeanMetadata::setRowColorScript(const QString &value)
{
    d->m_rowColorScript = value;
}

QString BaseBeanMetadata::toolTipScript() const
{
    return d->m_toolTipScript;
}

void BaseBeanMetadata::setToolTipScript(const QString &value)
{
    d->m_toolTipScript = value;
}

QString BaseBeanMetadata::toolTipStringExpression() const
{
    return d->m_toolTipStringExpression;
}

void BaseBeanMetadata::setToolTipStringExpression(const QString &value)
{
    d->m_toolTipStringExpression = value;
}

QString BaseBeanMetadata::metadataVar(const QString &varName) const
{
    return d->m_metadataVars.value(varName, QString());
}

QString BaseBeanMetadata::schema() const
{
    return d->m_schema;
}

void BaseBeanMetadata::setSchema(const QString &value)
{
    d->m_schema = value;
}

void BaseBeanMetadata::setAlias(const QString &alias)
{
    d->m_alias = alias;
}

QString BaseBeanMetadata::alias() const
{
    if ( d->m_alias.isEmpty() )
    {
        return d->m_tableName;
    }
    return d->m_alias;
}

bool BaseBeanMetadata::staticModel() const
{
    return d->m_static;
}

void BaseBeanMetadata::setStaticModel(bool value)
{
    d->m_static = value;
}

bool BaseBeanMetadata::readOnly() const
{
    return d->m_readOnly;
}

void BaseBeanMetadata::setReadOnly(bool value)
{
    d->m_readOnly = value;
}

void BaseBeanMetadata::setModule(AERPSystemModule *module)
{
    d->m_module = module;
}

AERPSystemModule *BaseBeanMetadata::module() const
{
    return d->m_module;
}

QString BaseBeanMetadata::viewForTable() const
{
    return d->m_viewForTable;
}

void BaseBeanMetadata::setViewForTable(const QString &query)
{
    d->m_viewForTable = query;
}

void BaseBeanMetadata::setViewOnGrid(const QString &query)
{
    d->m_viewOnGrid = query;
}

QString BaseBeanMetadata::viewOnGrid() const
{
    return d->m_viewOnGrid;
}

void BaseBeanMetadata::setCreationSqlView(const QHash<QString, QString> &value)
{
    d->m_creationSqlView = value;
}

QString BaseBeanMetadata::creationSqlView(const QString &dialect) const
{
    return d->m_creationSqlView[dialect];
}

bool BaseBeanMetadata::logicalDelete() const
{
    return d->m_logicalDelete;
}

void BaseBeanMetadata::setLogicalDelete(bool value)
{
    d->m_logicalDelete = value;
}

void BaseBeanMetadata::setItemsFilterColumn(const QList<QHash<QString, QString> > &alias)
{
    d->m_itemsFilterColumn = alias;
}

QList<QHash<QString, QString> > BaseBeanMetadata::itemsFilterColumn() const
{
    return d->m_itemsFilterColumn;
}

QString BaseBeanMetadata::initOrderSort() const
{
    return d->m_initOrderSort;
}

void BaseBeanMetadata::setInitOrderSort(const QString &value)
{
    d->m_initOrderSort = value;
}

QString BaseBeanMetadata::onCreateScript() const
{
    return d->m_onCreateScript;
}

void BaseBeanMetadata::setOnCreateScript(const QString &value)
{
    d->m_onCreateScript = value;
}

QString BaseBeanMetadata::beforeInsertScript() const
{
    return d->m_beforeInsertScript;
}

void BaseBeanMetadata::setBeforeInsertScript(const QString &value)
{
    d->m_beforeInsertScript = value;
}

QString BaseBeanMetadata::beforeUpdateScript() const
{
    return d->m_beforeUpdateScript;
}

void BaseBeanMetadata::setBeforeUpdateScript(const QString &value)
{
    d->m_beforeUpdateScript = value;
}

QString BaseBeanMetadata::beforeDeleteScript() const
{
    return d->m_beforeDeleteScript;
}

void BaseBeanMetadata::setBeforeDeleteScript(const QString &value)
{
    d->m_beforeDeleteScript = value;
}

QString BaseBeanMetadata::afterInsertScript() const
{
    return d->m_afterInsertScript;
}

void BaseBeanMetadata::setAfterInsertScript(const QString &value)
{
    d->m_afterInsertScript = value;
}

QString BaseBeanMetadata::afterUpdateScript() const
{
    return d->m_afterUpdateScript;
}

void BaseBeanMetadata::setAfterUpdateScript(const QString &value)
{
    d->m_afterUpdateScript = value;
}

QString BaseBeanMetadata::afterDeleteScript() const
{
    return d->m_afterDeleteScript;
}

void BaseBeanMetadata::setAfterDeleteScript(const QString &value)
{
    d->m_afterDeleteScript = value;
}

QString BaseBeanMetadata::beforeCopyScript() const
{
    return d->m_beforeCopyScript;
}

void BaseBeanMetadata::setBeforeCopyScript(const QString &value)
{
    d->m_beforeCopyScript = value;
}

QString BaseBeanMetadata::afterCopyScript() const
{
    return d->m_afterCopyScript;
}

void BaseBeanMetadata::setAfterCopyScript(const QString &value)
{
    d->m_afterCopyScript = value;
}

QString BaseBeanMetadata::emailTemplateScript() const
{
    return d->m_emailTemplateScript;
}

void BaseBeanMetadata::setEmailTemplateScript(const QString &value)
{
    d->m_emailTemplateScript = value;
}

QString BaseBeanMetadata::emailSubjectScript() const
{
    return d->m_emailSubjectScript;
}

void BaseBeanMetadata::setEmailSubjectScript(const QString &value)
{
    d->m_emailSubjectScript = value;
}

bool BaseBeanMetadata::canSendEmail() const
{
    return d->m_canSendEmail;
}

void BaseBeanMetadata::setCanSendEmail(bool value)
{
    d->m_canSendEmail = value;
}

QString BaseBeanMetadata::pixmapPath() const
{
    return d->m_pixmapPath;
}

void BaseBeanMetadata::setPixmapPath(const QString &value)
{
    d->m_pixmapPath = value;
    d->m_pixmap = QPixmap(value);
}

QPixmap BaseBeanMetadata::pixmap() const
{
    return d->m_pixmap;
}

void BaseBeanMetadata::setPixmap(const QPixmap &value)
{
    d->m_pixmap = value;
}

bool BaseBeanMetadata::canHaveRelatedElements() const
{
    return d->m_canHaveRelatedElements;
}

void BaseBeanMetadata::setCanHaveRelatedElements(bool value)
{
    d->m_canHaveRelatedElements = value;
}

QString BaseBeanMetadata::newRelatedElementScript() const
{
    return d->m_newRelatedElementScript;
}

void BaseBeanMetadata::setNewRelatedElementScript(const QString &value)
{
    d->m_newRelatedElementScript = value;
}

QString BaseBeanMetadata::modifiedRelatedElementScript() const
{
    return d->m_modifiedRelatedElementScript;
}

void BaseBeanMetadata::setModifiedRelatedElementScript(const QString &value)
{
    d->m_modifiedRelatedElementScript = value;
}

QString BaseBeanMetadata::deletedRelatedElementScript() const
{
    return d->m_deletedRelatedElementScript;
}

void BaseBeanMetadata::setDeletedRelatedElementScript(const QString &value)
{
    d->m_deletedRelatedElementScript = value;
}

bool BaseBeanMetadata::canHaveRelatedDocuments() const
{
    return d->m_canHaveRelatedDocuments;
}

void BaseBeanMetadata::setCanHaveRelatedDocuments(bool value)
{
    d->m_canHaveRelatedDocuments = value;
}

QList<AlephERP::RelatedElementsContentToBeDeleted> BaseBeanMetadata::relatedElementsContentToBeDelete() const
{
    return d->m_relatedElementsContentToBeDelete;
}

void BaseBeanMetadata::setRelatedElementsContentToBeDelete(const QList<AlephERP::RelatedElementsContentToBeDeleted> &value)
{
    d->m_relatedElementsContentToBeDelete = value;
}

QString BaseBeanMetadata::repositoryPathScript() const
{
    return d->m_repositoryPathScript;
}

void BaseBeanMetadata::setRepositoryPathScript(const QString &value)
{
    d->m_repositoryPathScript = value;
}

QString BaseBeanMetadata::repositoryKeywordsScript() const
{
    return d->m_repositoryKeywordsScript;
}

void BaseBeanMetadata::setRepositoryKeywordsScript(const QString &value)
{
    d->m_repositoryKeywordsScript = value;
}

AssociatedFunctionsPointerList BaseBeanMetadata::associatedScripts() const
{
    return d->m_associatedFunctions;
}

void BaseBeanMetadata::setAssociatedScripts(const AssociatedFunctionsPointerList &value)
{
    d->m_associatedFunctions = value;
}

/**
 * @brief BaseBeanMetadata::associatedScriptProgram
 * Devuelve una versión precompilada del script con funciones asociadas a este bean.
 * @return
 */
QScriptProgram BaseBeanMetadata::associatedScriptProgram(const QString &functionName)
{
    foreach (AssociatedFunctions *associatedScript, d->m_associatedFunctions)
    {
        if ( associatedScript->functions.contains(functionName) )
        {
            QString scriptName = QString("%1.%2.associatedFunctions.js").arg(d->m_tableName).arg(associatedScript->scriptFileName);
            if ( associatedScript->scriptProgram.isNull() )
            {
                if ( !BeansFactory::systemScripts.contains(associatedScript->scriptFileName) )
                {
                    qDebug() << "BaseBeanQsFunction::function: No hay ningún script " << associatedScript->scriptFileName;
                }
                else
                {
                    QString code = BeansFactory::systemScripts[associatedScript->scriptFileName];
                    associatedScript->scriptProgram = QScriptProgram (code, scriptName);
                }
            }
            return associatedScript->scriptProgram;
        }
    }
    return QScriptProgram();
}

InternalConnectionList BaseBeanMetadata::internalConnections() const
{
    return d->m_internalConnections;
}

void BaseBeanMetadata::setInternalConnections(const InternalConnectionList &value)
{
    d->m_internalConnections = value;
}

QString BaseBeanMetadata::toStringScript() const
{
    return d->m_toStringScript;
}

void BaseBeanMetadata::setToStringScript(const QString &value)
{
    d->m_toStringScript = value;
}

QString BaseBeanMetadata::toStringType() const
{
    return d->m_toStringType;
}

void BaseBeanMetadata::setToStringType(const QString &value)
{
    d->m_toStringType = value;
}

HashStringList BaseBeanMetadata::infoSubTotals() const
{
    return d->m_infoSubTotals;
}

void BaseBeanMetadata::setInfoSubTotals(const HashStringList &v)
{
    d->m_infoSubTotals = v;
}

bool BaseBeanMetadata::filterRowByUser() const
{
    return d->m_filterRowByUser;
}

void BaseBeanMetadata::setFilterRowByUser(bool value)
{
    d->m_filterRowByUser = value;
}

const ScheduleData BaseBeanMetadata::scheduledData() const
{
    return d->m_scheduledData;
}

int BaseBeanMetadata::orderOnTransaction() const
{
    return d->m_orderOnTransaction;
}

QString BaseBeanMetadata::helpUrl() const
{
    return d->m_helpUrl;
}

void BaseBeanMetadata::setHelpUrl(const QString &value)
{
    d->m_helpUrl = value;
}

QString BaseBeanMetadata::menuEntryPath() const
{
    return d->m_menuEntryPath;
}

void BaseBeanMetadata::setMenuEntryPath(const QString &value)
{
    d->m_menuEntryPath = value;
}

QString BaseBeanMetadata::moduleToolBarEntryPath() const
{
    return d->m_toolBarEntryPath;
}

void BaseBeanMetadata::setModuleToolBarEntryPath(const QString &value)
{
    d->m_toolBarEntryPath = value;
}

QString BaseBeanMetadata::readOnlyScript() const
{
    return d->m_readOnlyScript;
}

void BaseBeanMetadata::setReadOnlyScript(const QString &value)
{
    d->m_readOnlyScript = value;
}

/**
 * @brief BaseBeanMetadata::showRelatedElementsModel
 * Analiza la estructura de datos de relaciones subyecente, para así determinar si debe presentarse los elementos relacionados
 * @return
 */
bool BaseBeanMetadata::showSomeRelationOnRelatedElementsModel()
{
    foreach (DBRelationMetadata *rel, d->m_relations)
    {
        if ( rel->showOnRelatedModels() )
        {
            return true;
        }
    }
    return false;
}

bool BaseBeanMetadata::debug()
{
    return d->m_debug;
}

void BaseBeanMetadata::setDebug(bool value)
{
    d->m_debug = value;
    d->m_engine->setDebug(d->m_debug);
}

bool BaseBeanMetadata::onInitDebug()
{
    return d->m_onInitDebug;
}

void BaseBeanMetadata::setOnInitDebug(bool value)
{
    d->m_onInitDebug = value;
    d->m_engine->setOnInitDebug(d->m_onInitDebug);
}

QString BaseBeanMetadata::validateScript() const
{
    return d->m_validateScript;
}

void BaseBeanMetadata::setValidateScript(const QString &value)
{
    d->m_validateScript = value;
}

QString BaseBeanMetadata::beforeSaveScript() const
{
    return d->m_beforeSaveScript;
}

void BaseBeanMetadata::setBeforeSaveScript(const QString &value)
{
    d->m_beforeSaveScript = value;
}

QString BaseBeanMetadata::afterSaveScript()
{
    return d->m_afterSaveScript;
}

void BaseBeanMetadata::setAfterSaveScript(const QString &value)
{
    d->m_afterSaveScript = value;
}

QString BaseBeanMetadata::defaultVisualizationField()
{
    if ( d->m_defaultVisualizationField.isEmpty() )
    {
        QList<DBFieldMetadata *> pk = pkFields();
        if ( pk.size() > 0 )
        {
            d->m_defaultVisualizationField = pk.at(0)->dbFieldName();
        }
    }
    return d->m_defaultVisualizationField;
}

void BaseBeanMetadata::setDefaultVisualizationField(const QString &value)
{
    d->m_defaultVisualizationField = value;
}

QList<EnvVarDefinition> BaseBeanMetadata::envVars() const
{
    return d->m_envVars;
}

void BaseBeanMetadataPrivate::addSqlPart(const QString &clause, const QString &value)
{
    m_sql[clause] = value;
}

QHash<QString, QString> BaseBeanMetadata::sql() const
{
    return d->m_sql;
}

QString BaseBeanMetadata::xml() const
{
    return d->m_xml;
}

QList<DBFieldMetadata *> BaseBeanMetadata::fields()
{
    return d->m_fields;
}

QStringList BaseBeanMetadata::dbFieldNames() const
{
    QStringList lst;
    foreach (DBFieldMetadata *fld, d->m_fields)
    {
        lst << fld->dbFieldName();
    }
    return lst;
}

/**
 * @brief BaseBeanMetadata::countSerialFields
 * Devuelve simplemente el número de fields seriales
 * @return
 */
int BaseBeanMetadata::countSerialFields()
{
    int count = 0;
    foreach (DBFieldMetadata *fld, d->m_fields)
    {
        if ( fld->serial() )
        {
            count++;
        }
    }
    return count;
}

QList<DBRelationMetadata *> BaseBeanMetadata::relations(AlephERP::RelationTypes type)
{
    if ( type == AlephERP::All )
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

bool BaseBeanMetadata::isCached() const
{
    return d->m_isCached;
}

void BaseBeanMetadata::setIsCached(bool value)
{
    d->m_isCached = value;
}

bool BaseBeanMetadata::localCache() const
{
    return d->m_localCache;
}

void BaseBeanMetadata::setLocalCache(bool value)
{
    d->m_localCache = value;
}

bool BaseBeanMetadata::showOnTree()
{
    return d->m_showOnTree;
}

void BaseBeanMetadata::setShowOnTree(bool value)
{
    d->m_showOnTree = value;
}

bool BaseBeanMetadata::showOnTreePreloadRecords() const
{
    return d->m_showOnTreePreloadRecords;
}

void BaseBeanMetadata::setShowOnTreePreloadRecords(bool value)
{
    d->m_showOnTreePreloadRecords = value;
}

QVariantList BaseBeanMetadata::treeDefinitions ()
{
    return d->m_treeDefinitions;
}

void BaseBeanMetadata::setTreeDefinitions (const QVariantList &value)
{
    d->m_treeDefinitions = value;
}

QString BaseBeanMetadata::uiDbRecord() const
{
    if ( AERPLoggedUser::instance()->roles().size() == 1 )
    {
        QString roleName = AERPLoggedUser::instance()->roles().first().roleName;
        if ( d->m_uiDbRecord.contains(roleName) )
        {
            return d->m_uiDbRecord.value(roleName);
        }
        else if ( d->m_uiDbRecord.contains("*") )
        {
            return d->m_uiDbRecord.value("*");
        }
    }
    return QString();
}

QHash<QString, QString> BaseBeanMetadata::uiDbRecordForRoles() const
{
    return d->m_uiDbRecord;
}

void BaseBeanMetadata::setUiDbRecord(const QHash<QString, QString> &value)
{
    d->m_uiDbRecord = value;
}

QString BaseBeanMetadata::uiNewDbRecord() const
{
    if ( AERPLoggedUser::instance()->roles().size() == 1 )
    {
        QString roleName = AERPLoggedUser::instance()->roles().first().roleName;
        if ( d->m_uiNewDbRecord.contains(roleName) )
        {
            return d->m_uiNewDbRecord.value(roleName);
        }
        else if ( d->m_uiNewDbRecord.contains("*") )
        {
            return d->m_uiNewDbRecord.value("*");
        }
    }
    return QString();
}

QHash<QString, QString> BaseBeanMetadata::uiNewDbRecordForRoles() const
{
    return d->m_uiNewDbRecord;
}

void BaseBeanMetadata::setUiNewDbRecord(const QHash<QString, QString> &value)
{
    d->m_uiNewDbRecord = value;
}

QString BaseBeanMetadata::uiWizard() const
{
    if ( AERPLoggedUser::instance()->roles().size() == 1 )
    {
        QString roleName = AERPLoggedUser::instance()->roles().first().roleName;
        if ( d->m_uiWizard.contains(roleName) )
        {
            return d->m_uiWizard.value(roleName);
        }
        else if ( d->m_uiWizard.contains("*") )
        {
            return d->m_uiWizard.value("*");
        }
    }
    return QString();
}

QHash<QString, QString> BaseBeanMetadata::uiWizardForRoles() const
{
    return d->m_uiWizard;
}

void BaseBeanMetadata::setUiWizard(const QHash<QString, QString> &value)
{
    d->m_uiWizard = value;
}

QString BaseBeanMetadata::qmlDbRecord() const
{
    if ( AERPLoggedUser::instance()->roles().size() == 1 )
    {
        QString roleName = AERPLoggedUser::instance()->roles().first().roleName;
        if ( d->m_qmlDbRecord.contains(roleName) )
        {
            return d->m_qmlDbRecord.value(roleName);
        }
        else if ( d->m_qmlDbRecord.contains("*") )
        {
            return d->m_qmlDbRecord.value("*");
        }
    }
    return QString();
}

QHash<QString, QString> BaseBeanMetadata::qmlDbRecordForRoles() const
{
    return d->m_qmlDbRecord;
}

void BaseBeanMetadata::setQmlDbRecord(const QHash<QString, QString> &value)
{
    d->m_qmlDbRecord = value;
}

QString BaseBeanMetadata::qmlNewDbRecord() const
{
    if ( AERPLoggedUser::instance()->roles().size() == 1 )
    {
        QString roleName = AERPLoggedUser::instance()->roles().first().roleName;
        if ( d->m_qmlNewDbRecord.contains(roleName) )
        {
            return d->m_qmlNewDbRecord.value(roleName);
        }
        else if ( d->m_qmlNewDbRecord.contains("*") )
        {
            return d->m_qmlNewDbRecord.value("*");
        }
    }
    return QString();
}

QHash<QString, QString> BaseBeanMetadata::qmlNewDbRecordForRoles() const
{
    return d->m_qmlNewDbRecord;
}

void BaseBeanMetadata::setQmlNewDbRecord(const QHash<QString, QString> &value)
{
    d->m_qmlNewDbRecord = value;
}

QString BaseBeanMetadata::qsDbRecord() const
{
    if ( AERPLoggedUser::instance()->roles().size() == 1 )
    {
        QString roleName = AERPLoggedUser::instance()->roles().first().roleName;
        if ( d->m_qsDbRecord.contains(roleName) )
        {
            return d->m_qsDbRecord.value(roleName);
        }
        else if ( d->m_qsDbRecord.contains("*") )
        {
            return d->m_qsDbRecord.value("*");
        }
    }
    return QString();
}

QHash<QString, QString> BaseBeanMetadata::qsDbRecordForRoles() const
{
    return d->m_qsDbRecord;
}

void BaseBeanMetadata::setQsDbRecord(const QHash<QString, QString> &value)
{
    d->m_qsDbRecord = value;
}

QString BaseBeanMetadata::qsNewDbRecord() const
{
    if ( AERPLoggedUser::instance()->roles().size() == 1 )
    {
        QString roleName = AERPLoggedUser::instance()->roles().first().roleName;
        if ( d->m_qsNewDbRecord.contains(roleName) )
        {
            return d->m_qsNewDbRecord.value(roleName);
        }
        else if ( d->m_qsNewDbRecord.contains("*") )
        {
            return d->m_qsNewDbRecord.value("*");
        }
    }
    return QString();
}

QHash<QString, QString> BaseBeanMetadata::qsNewDbRecordForRoles() const
{
    return d->m_qsNewDbRecord;
}

void BaseBeanMetadata::setQsNewDbRecord(const QHash<QString, QString> &value)
{
    d->m_qsNewDbRecord = value;
}

QString BaseBeanMetadata::qsPrototypeDbRecord() const
{
    return d->m_qsPrototypeDbRecord;
}

void BaseBeanMetadata::setQsPrototypeDbRecord(const QString &value)
{
    d->m_qsPrototypeDbRecord = value;
}

QString BaseBeanMetadata::qsPrototypeDbSearch() const
{
    return d->m_qsPrototypeDbSearch;
}

void BaseBeanMetadata::setQsPrototypeDbSearch(const QString &value)
{
    d->m_qsPrototypeDbSearch = value;
}

QString BaseBeanMetadata::qsPrototypeDbForm() const
{
    return d->m_qsPrototypeDbForm;
}

void BaseBeanMetadata::setQsPrototypeDbForm(const QString &value)
{
    d->m_qsPrototypeDbForm = value;
}

QString BaseBeanMetadata::uiDbSearch() const
{
    if ( AERPLoggedUser::instance()->roles().size() == 1 )
    {
        QString roleName = AERPLoggedUser::instance()->roles().first().roleName;
        if ( d->m_uiDbSearch.contains(roleName) )
        {
            return d->m_uiDbSearch.value(roleName);
        }
        else if ( d->m_uiDbSearch.contains("*") )
        {
            return d->m_uiDbSearch.value("*");
        }
    }
    return QString();
}

QString BaseBeanMetadata::qsPrototypeDbWizard() const
{
    return d->m_qsPrototypeDbWizard;
}

void BaseBeanMetadata::setQsPrototypeDbWizard(const QString &value)
{
    d->m_qsPrototypeDbWizard = value;
}

QHash<QString, QString> BaseBeanMetadata::uiDbSearchForRoles() const
{
    return d->m_uiDbSearch;
}

void BaseBeanMetadata::setUiDbSearch(const QHash<QString, QString> &value)
{
    d->m_uiDbSearch = value;
}

QString BaseBeanMetadata::qsDbSearch() const
{
    if ( AERPLoggedUser::instance()->roles().size() == 1 )
    {
        QString roleName = AERPLoggedUser::instance()->roles().first().roleName;
        if ( d->m_qsDbSearch.contains(roleName) )
        {
            return d->m_qsDbSearch.value(roleName);
        }
        else if ( d->m_qsDbSearch.contains("*") )
        {
            return d->m_qsDbSearch.value("*");
        }
    }
    return QString();
}

QHash<QString, QString> BaseBeanMetadata::qsDbSearchForRoles() const
{
    return d->m_qsDbSearch;
}

void BaseBeanMetadata::setQsDbSearch(const QHash<QString, QString> &value)
{
    d->m_qsDbSearch = value;
}

QString BaseBeanMetadata::qsDbForm() const
{
    if ( AERPLoggedUser::instance()->roles().size() == 1 )
    {
        QString roleName = AERPLoggedUser::instance()->roles().first().roleName;
        if ( d->m_qsDbForm.contains(roleName) )
        {
            return d->m_qsDbForm.value(roleName);
        }
        else if ( d->m_qsDbForm.contains("*") )
        {
            return d->m_qsDbForm.value("*");
        }
    }
    return QString();
}

QHash<QString, QString> BaseBeanMetadata::qsDbFormForRoles() const
{
    return d->m_qsDbForm;
}

void BaseBeanMetadata::setQsDbForm(const QHash<QString, QString> &value)
{
    d->m_qsDbForm = value;
}

QString BaseBeanMetadata::accessibleRule()
{
    return d->m_accessible;
}

void BaseBeanMetadata::setAccessibleRule(const QString &value)
{
    d->m_accessible = value;
}

QString BaseBeanMetadata::additionalFilter()
{
    return d->m_additionalFilter;
}

void BaseBeanMetadata::setAdditionalFilter(const QString &value)
{
    d->m_additionalFilter = value;
}

void BaseBeanMetadata::setXml (const QString &value)
{
    d->m_xml = value;
    qDeleteAll(d->m_fields);
    d->m_fields.clear();
    qDeleteAll(d->m_relations);
    d->m_relations.clear();
    d->setConfig();
}

/**
Lee y establece la configuración existente de campos de este bean a partir del XML
de configuración
*/
void BaseBeanMetadataPrivate::setConfig()
{
    QString defaultValue, optionList, optionValues, optionIcons, errorString;
    int errorLine, errorColumn;

    if ( m_xml.isEmpty() )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBeanMetadataPrivate: setConfig: BaseBean:  [%1]. XML Vacio").arg(m_tableName));
        return;
    }
    qDebug() << "BaseBeanMetadata::setConfig: Analizando [" << m_tableName << "]";

    QDomDocument domDocument;
    // Hay que mantener una referencia abierta de los otros documentos
    QList<QDomDocument> otherDocumentList;
    if ( domDocument.setContent( m_xml, true, &errorString, &errorLine, &errorColumn ) )
    {
        QDomElement root = domDocument.documentElement();
        // Vamos a ver si tenemos que importar elementos heredados, y si se así, se incluyen
        QDomNodeList nodes = root.elementsByTagName("inherits");
        for ( int i = 0 ; i < nodes.size() ; i++ )
        {
            if ( !nodes.at(i).toElement().hasAttribute("name") )
            {
                QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBeanMetadata::setConfig: ATENCION: tabla [%1] contiene inherits mal formados, y sin name.").arg(m_tableName));
            }
            else
            {
                AERPSystemObject *systemObject = SystemDAO::systemObject(nodes.at(i).toElement().attribute("name"), "tableTemp", 0);
                if ( systemObject != NULL )
                {
                    QDomDocument otherDocument;
                    if ( otherDocument.setContent(systemObject->content(), true, &errorString, &errorLine, &errorColumn ) )
                    {
                        otherDocumentList.append(otherDocument);
                        importHeritanceElementsOnDom(domDocument, otherDocument, nodes.at(i));
                    }
                    else
                    {
                        QMessageBox::critical(0, qApp->applicationName(), QObject::trUtf8("El archivo XML de sistema <b>%1</b> no es correcto. "
                                              "El programa no funcionará. Consulte con <i>Aleph Sistemas de Información</i>.").
                                              arg(systemObject->name()),
                                              QMessageBox::Ok);
                        QLogger::QLog_Error(AlephERP::stLogOther, QString("-------------------------------------------------------------------------------------------------------"));
                        QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBeanMetadata::importHeritanceElementsOnDom: Existe un error en : [%1]: LINEA: [%2]: COLUMNA [%3]: ERROR: [%4]").
                                            arg(systemObject->name()).arg(errorLine).arg(errorColumn).arg(errorString));
                        QLogger::QLog_Error(AlephERP::stLogOther, QString("-------------------------------------------------------------------------------------------------------"));
                    }
                }
            }
        }
        QDomNodeList metadataVarNodes = root.elementsByTagName("metadataVar");
        for (int i = 0 ; i < metadataVarNodes.size() ; i++)
        {
            QDomElement el = metadataVarNodes.at(i).toElement();
            m_metadataVars[el.attribute("name", "foo")] = el.text();
        }

        QDomNode n = root.firstChildElement("name");
        if ( m_tableName != n.toElement().text() )
        {
            qDebug() << "BaseBean: setConfig(): No coinciden el nombre de la tabla en registro y en XML. Registro: " << m_tableName << " XML: " << n.toElement().text();
        }

        /** Los metadatos permiten algunas frivolidades. Cuando un elemento del XML contiene una sentencia del estilo ${algo} se sustituye
         * ese "algo" por la propiedad de este metadato. (Generalmente, será el nombre de la tabla, ya que ésto es especialmente útil
         * para reutilizar */
        n = root.firstChildElement("alias");
        if ( !n.isNull() )
        {
            m_alias = QObject::trUtf8(checkWildCards(n).toUtf8());
        }
        n = root.firstChildElement("canNavigate");
        if ( !n.isNull() )
        {
            m_canNavigate = (checkWildCards(n) == QLatin1String("true") ? true : false);
        }
        n = root.firstChildElement("schema");
        if ( !n.isNull() )
        {
            m_schema = QObject::trUtf8(checkWildCards(n).toUtf8());
        }
        n = root.firstChildElement("viewForTable");
        if ( !n.isNull() )
        {
            m_viewForTable = checkWildCards(n);
        }
        n = root.firstChildElement("viewOnGrid");
        if ( !n.isNull() )
        {
            m_viewOnGrid = checkWildCards(n);
        }
        n = root.firstChildElement("viewProvidesOid");
        if ( !n.isNull() )
        {
            m_viewProvidesOid = (checkWildCards(n) == QLatin1String("true") ? true : false);
        }
        n = root.firstChildElement("readOnly");
        if ( !n.isNull() )
        {
            m_readOnly = (checkWildCards(n) == QLatin1String("true") ? true : false);
        }
        n = root.firstChildElement("readOnlyScript");
        if ( !n.isNull() )
        {
            m_readOnlyScript = checkWildCards(n);
        }
        QDomNodeList sqlViewNodes = root.elementsByTagName("creationSqlView");
        for ( int i = 0 ; i < sqlViewNodes.size() ; i++ )
        {
            QDomNode node = sqlViewNodes.at(i);
            if ( node.toElement().attribute("database", "PostgreSQL") == QLatin1String("PostgreSQL") )
            {
                m_creationSqlView["QPSQL"] = checkWildCards(node);
            }
            if ( node.toElement().attribute("database", "Firebird") == QLatin1String("Firebird") )
            {
                m_creationSqlView["QIBASE"] = checkWildCards(node);
            }
            if ( node.toElement().attribute("database", "SQLite") == QLatin1String("SQLite") )
            {
                m_creationSqlView["QSQLITE"] = checkWildCards(node);
            }
        }
        n = root.firstChildElement("logicalDelete");
        if ( !n.isNull() )
        {
            m_logicalDelete = (checkWildCards(n) == QLatin1String("true") ? true : false);
        }
        n = root.firstChildElement("orderOnTransaction");
        if ( !n.isNull() )
        {
            m_orderOnTransaction = checkWildCards(n).toInt();
        }
        n = root.firstChildElement("initOrderSort");
        if ( !n.isNull() )
        {
            m_initOrderSort = checkWildCards(n);
        }
        n = root.firstChildElement("defaultVisualizationField");
        if ( !n.isNull() )
        {
            m_defaultVisualizationField = checkWildCards(n);
        }
        n = root.firstChildElement("isCached");
        if ( !n.isNull() )
        {
            m_isCached = (checkWildCards(n) == QLatin1String("true") ? true : false);
        }
        n = root.firstChildElement("localCache");
        if ( !n.isNull() )
        {
            m_localCache = (checkWildCards(n) == QLatin1String("true") ? true : false);
        }
        n = root.firstChildElement("filterRowByUser");
        if ( !n.isNull() )
        {
            m_filterRowByUser = (checkWildCards(n) == QLatin1String("true") ? true : false);
        }
        n = root.firstChildElement("accesibleRule");
        if ( !n.isNull() )
        {
            m_accessible = checkWildCards(n);
        }
        n = root.firstChildElement("additionalFilter");
        if ( !n.isNull() )
        {
            m_additionalFilter = checkWildCards(n);
        }
        n = root.firstChildElement("menuEntryPath");
        if ( !n.isNull() )
        {
            m_menuEntryPath = checkWildCards(n);
        }
        n = root.firstChildElement("moduleToolBarEntryPath");
        if ( !n.isNull() )
        {
            m_toolBarEntryPath = checkWildCards(n);
        }
        n = root.firstChildElement("static");
        if ( !n.isNull() )
        {
            m_static = checkWildCards(n) == QLatin1String("true") ? true : false;
        }
        n = root.firstChildElement("debug");
        if ( !n.isNull() )
        {
            q_ptr->setDebug(n.toElement().text() == QLatin1String("true") ? true: false);
            if ( n.toElement().hasAttribute("onInitDebug") )
            {
                q_ptr->setOnInitDebug(n.toElement().attribute("onInitDebug", "false") == QLatin1String("true") ? true : false);
            }
        }

        n = root.firstChildElement("validateScript");
        if ( !n.isNull() )
        {
            m_validateScript = checkWildCards(n);
        }
        n = root.firstChildElement("onCreateScript");
        if ( !n.isNull() )
        {
            m_onCreateScript = checkWildCards(n);
        }
        n = root.firstChildElement("beforeInsertScript");
        if ( !n.isNull() )
        {
            m_beforeInsertScript = checkWildCards(n);
        }
        n = root.firstChildElement("beforeUpdateScript");
        if ( !n.isNull() )
        {
            m_beforeUpdateScript = checkWildCards(n);
        }
        n = root.firstChildElement("beforeDeleteScript");
        if ( !n.isNull() )
        {
            m_beforeDeleteScript = checkWildCards(n);
        }
        n = root.firstChildElement("beforeSaveScript");
        if ( !n.isNull() )
        {
            m_beforeSaveScript = checkWildCards(n);
        }
        n = root.firstChildElement("afterInsertScript");
        if ( !n.isNull() )
        {
            m_afterInsertScript = checkWildCards(n);
        }
        n = root.firstChildElement("afterUpdateScript");
        if ( !n.isNull() )
        {
            m_afterUpdateScript = checkWildCards(n);
        }
        n = root.firstChildElement("afterDeleteScript");
        if ( !n.isNull() )
        {
            m_afterDeleteScript = checkWildCards(n);
        }
        n = root.firstChildElement("afterSaveScript");
        if ( !n.isNull() )
        {
            m_afterSaveScript = checkWildCards(n);
        }
        n = root.firstChildElement("canHaveRelatedElements");
        if ( !n.isNull() )
        {
            m_canHaveRelatedElements = checkWildCards(n) == QLatin1String("true") ? true : false;
        }
        n = root.firstChildElement("newRelatedElementScript");
        if ( !n.isNull() )
        {
            m_newRelatedElementScript = checkWildCards(n);
        }
        n = root.firstChildElement("modifiedRelatedElementScript");
        if ( !n.isNull() )
        {
            m_modifiedRelatedElementScript = checkWildCards(n);
        }
        n = root.firstChildElement("deletedRelatedElementScript");
        if ( !n.isNull() )
        {
            m_deletedRelatedElementScript = checkWildCards(n);
        }
        n = root.firstChildElement("relatedElementsContentToBeDelete");
        if ( !n.isNull() )
        {
            readRelatedElementsContentToBeDelete(n);
        }
        n = root.firstChildElement("canSendEmail");
        if ( !n.isNull() )
        {
            m_canSendEmail = checkWildCards(n) == QLatin1String("true") ? true : false;
        }
        n = root.firstChildElement("emailTemplateScript");
        if ( !n.isNull() )
        {
            m_emailTemplateScript = checkWildCards(n);
        }
        n = root.firstChildElement("emailSubjectScript");
        if ( !n.isNull() )
        {
            m_emailSubjectScript = checkWildCards(n);
        }
        n = root.firstChildElement("emailContactModel");
        if ( !n.isNull() )
        {
            m_emailContactModel = checkWildCards(n);
        }
        n = root.firstChildElement("emailContactModelFilter");
        if ( !n.isNull() )
        {
            m_emailContactModelFilter = checkWildCards(n);
        }
        n = root.firstChildElement("emailContactAddressField");
        if ( !n.isNull() )
        {
            m_emailContactAddressField = checkWildCards(n);
        }
        n = root.firstChildElement("emailContactDisplayField");
        if ( !n.isNull() )
        {
            m_emailContactDisplayField = checkWildCards(n);
        }
        n = root.firstChildElement("pixmap");
        if ( !n.isNull() )
        {
            q_ptr->setPixmapPath(checkWildCards(n));
        }
        n = root.firstChildElement("rowColorScript");
        if ( !n.isNull() )
        {
            m_rowColorScript = checkWildCards(n);
        }
        n = root.firstChildElement("toolTipScript");
        if ( !n.isNull() )
        {
            m_toolTipScript = checkWildCards(n);
        }
        n = root.firstChildElement("toolTipStringExpression");
        if ( !n.isNull() )
        {
            m_toolTipStringExpression = checkWildCards(n);
            QDomElement element = n.toElement();
            m_toolTipExpression = readStringExpression(element);
        }
        n = root.firstChildElement("canHaveRelatedDocuments");
        if ( !n.isNull() )
        {
            m_canHaveRelatedDocuments = checkWildCards(n) == QLatin1String("true") ? true : false;
        }
        n = root.firstChildElement("repositoryPathScript");
        if ( !n.isNull() )
        {
            m_repositoryPathScript = checkWildCards(n);
            if ( !m_repositoryPathScript.isEmpty() )
            {
                m_canHaveRelatedDocuments = true;
            }
        }
        n = root.firstChildElement("repositoryKeywordsScript");
        if ( !n.isNull() )
        {
            m_repositoryKeywordsScript = checkWildCards(n);
        }
        n = root.firstChildElement("toStringScript");
        if ( !n.isNull() )
        {
            m_toStringScript = checkWildCards(n);
            m_toStringType = "js";
        }
        n = root.firstChildElement("toStringExpression");
        if ( !n.isNull() )
        {
            QDomElement element = n.toElement();
            m_toStringExpression = readStringExpression(element);
            m_toStringType = "builtIn";
        }

        n = root.firstChildElement("qsPrototypeDbRecord");
        if ( !n.isNull() )
        {
            m_qsPrototypeDbRecord = checkWildCards(n);
        }
        n = root.firstChildElement("qsPrototypeDbWizard");
        if ( !n.isNull() )
        {
            m_qsPrototypeDbWizard = checkWildCards(n);
        }
        n = root.firstChildElement("qsPrototypeDbSearch");
        if ( !n.isNull() )
        {
            m_qsPrototypeDbSearch = checkWildCards(n);
        }
        n = root.firstChildElement("qsPrototypeDbForm");
        if ( !n.isNull() )
        {
            m_qsPrototypeDbForm = checkWildCards(n);
        }
        readFormsConfigNames(root);
        n = root.firstChildElement("helpUrl");
        if ( !n.isNull() )
        {
            m_helpUrl = checkWildCards(n);
        }

        QDomNodeList associatedNodes = root.elementsByTagName("associatedScript");
        for (int i = 0 ; i < associatedNodes.size() ; i++)
        {
            AssociatedFunctions *item = new AssociatedFunctions;
            QDomNode node = associatedNodes.at(i);
            item->scriptFileName = checkWildCards(node);
            m_associatedFunctions.append(item);
        }

        QDomElement e = root.firstChildElement("itemsFilterColumn");
        if ( !e.isNull() )
        {
            readItemsFilterColumn(e);
        }
        e = root.firstChildElement("envVars");
        if ( !e.isNull() )
        {
            readEnvVarsFilter(e);
        }
        e = root.firstChildElement("showOnTree");
        if ( !e.isNull() )
        {
            readTreeDefinition(e);
        }
        e = root.firstChildElement("sql");
        if ( !e.isNull() )
        {
            readSql(e);
        }
        QDomNodeList connectionsVarNodes = root.elementsByTagName("connections");
        for (int i = 0 ; i < connectionsVarNodes.size(); i++)
        {
            QDomElement el = connectionsVarNodes.at(i).toElement();
            if ( !el.isNull() )
            {
                readInternalConnections(el);
            }
        }
        e = root.firstChildElement("infoSubTotals");
        if ( !e.isNull() )
        {
            readInfoSubTotal(e);
        }
        e = root.firstChildElement("scheduleView");
        if ( !e.isNull() )
        {
            readScheduledData(e);
        }

        // Iteramos sobre todos los campos
        int fieldIndex = 0;
        QDomNodeList fieldList = root.elementsByTagName("field");
        for ( int i = 0 ; i < fieldList.size() ; i++ )
        {
            DBFieldMetadata *field = new DBFieldMetadata(q_ptr);
            field->setParent(q_ptr);
            field->setXmlDefinition(fieldList.at(i).toDocument().toString());

            QDomElement element = fieldList.at(i).toElement();
            for ( QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling() )
            {
                QDomElement e = n.toElement();
                QString elementText = checkWildCards(e);
                if ( e.tagName() == QLatin1String("name") )
                {
                    field->setDbFieldName(elementText);
                }
                else if ( e.tagName() == QLatin1String("format") )
                {
                    field->setHasFormat(true);
                    if ( e.hasAttribute("lowerCase") )
                    {
                        field->setLowerCase((e.attribute("lowerCase", "false") == QLatin1String("true") ? true : false));
                    }
                    else if ( e.hasAttribute("upperCase") )
                    {
                        field->setUpperCase((e.attribute("upperCase", "false") == QLatin1String("true") ? true : false));
                    }
                    if ( e.hasAttribute("noSpaces") )
                    {
                        field->setNoSpaces((e.attribute("noSpaces", "false") == QLatin1String("true") ? true : false));
                    }
                }
                else if ( e.tagName() == QLatin1String("expression") )
                {
                    // Esta expresión puede ser una regla con wildcards internos. Requiere un tratamiento posterior cuando
                    // todos los metadatos se han cargado. Aquí se almacena en un elemento intermedio que deberá después se consolidado
                    // con consolidateTemp.
                    field->setTempStringExpression(elementText);
                }
                else if ( e.tagName() == QLatin1String("alias") )
                {
                    field->setFieldName(elementText);
                }
                else if ( e.tagName() == QLatin1String("aliasScript") )
                {
                    field->setFieldNameScript(elementText);
                }
                else if ( e.tagName() == QLatin1String("null") )
                {
                    field->setNull((elementText == QLatin1String("true") ? true : false));
                }
                else if ( e.tagName() == QLatin1String("visiblegrid") )
                {
                    field->setVisibleGrid((elementText == QLatin1String("true") ? true : false));
                }
                else if ( e.tagName() == QLatin1String("showDefault") )
                {
                    field->setShowDefault((elementText == QLatin1String("true") ? true : false));
                }
                else if ( e.tagName() == QLatin1String("pk") )
                {
                    field->setPrimaryKey((elementText == QLatin1String("true") ? true : false));
                }
                else if ( e.tagName() == QLatin1String("unique") )
                {
                    field->setUnique((elementText == QLatin1String("true") ? true : false));
                }
                else if ( e.tagName() == QLatin1String("uniqueCompound") )
                {
                    field->setUniqueCompound((elementText == QLatin1String("true") ? true : false));
                }
                else if ( e.tagName() == QLatin1String("dbIndex") )
                {
                    field->setDbIndex((elementText == QLatin1String("true") ? true : false));
                }
                else if ( e.tagName() == QLatin1String("uniqueOnFilterField") )
                {
                    field->setUniqueOnFilterField(elementText);
                }
                else if ( e.tagName() == QLatin1String("orderField") )
                {
                    field->setOrderField(elementText == QLatin1String("true") ? true : false);
                }
                else if ( e.tagName() == QLatin1String("envDefaultValue") )
                {
                    field->setEnvDefaultValue(elementText);
                }
                else if ( e.tagName() == QLatin1String("type") )
                {
                    field->setMetadataTypeName(elementText);
                    if ( elementText == QLatin1String("string") ||
                         elementText == QLatin1String("stringlist") ||
                         elementText == QLatin1String("password") )
                    {
                        field->setType(QVariant::String);
                        if ( elementText == QLatin1String("stringlist") )
                        {
                            // -1 es longitud ilimitada
                            field->setLength(-1);
                            field->setMemo(true);
                        }
                    }
                    else if ( elementText == QLatin1String("double") )
                    {
                        field->setType(QVariant::Double);
                    }
                    else if ( elementText == QLatin1String("int") || elementText == QLatin1String("integer") )
                    {
                        field->setType(QVariant::Int);
                    }
                    else if ( elementText == QLatin1String("long") )
                    {
                        field->setType(QVariant::LongLong);
                    }
                    else if ( elementText == QLatin1String("serial") )
                    {
                        field->setType(QVariant::Int);
                        field->setSerial(true);
                    }
                    else if ( elementText == QLatin1String("date") )
                    {
                        field->setType(QVariant::Date);
                    }
                    else if ( elementText == QLatin1String("datetime") )
                    {
                        field->setType(QVariant::DateTime);
                    }
                    else if ( elementText == QLatin1String("bool") || elementText == QLatin1String("boolean") )
                    {
                        field->setType(QVariant::Bool);
                    }
                    else if ( elementText == QLatin1String("image") )
                    {
                        field->setType(QVariant::Pixmap);
                        field->setMemo(true);
                    }
                    else
                    {
                        qDebug() << "BaseBeanMetadata::setConfig: No se encuentra el tipo [" << elementText << "] del bean [" << m_tableName << "]";
                    }
                }
                else if ( e.tagName() == QLatin1String("default") )
                {
                    defaultValue = elementText;
                }
                else if ( e.tagName() == QLatin1String("partI") )
                {
                    field->setPartI(elementText.toInt());
                }
                else if ( e.tagName() == QLatin1String("partD") )
                {
                    field->setPartD(elementText.toInt());
                }
                else if ( e.tagName() == QLatin1String("length") )
                {
                    field->setLength(elementText.toInt());
                }
                else if ( e.tagName() == QLatin1String("lengthScript") )
                {
                    field->setLengthScript(elementText);
                }
                else if ( e.tagName() == QLatin1String("counterPrefix") )
                {
                    field->setCounterPrefix(elementText);
                }
                else if ( e.tagName() == QLatin1String("relation") )
                {
                    readDBRelation(e, field);
                }
                else if ( e.tagName() == QLatin1String("optionList") || e.tagName() == QLatin1String("optionsList") )
                {
#ifdef ALEPHERP_BARCODE
                    if ( elementText == QLatin1String("allowedBarCode") )
                    {
                        optionList = DBBarCode::allowedBarCodes().join(",");
                    }
                    else
#endif
                    {
                        optionList = elementText;
                    }
                }
                else if ( e.tagName() == QLatin1String("optionValues") || e.tagName() == QLatin1String("optionsValues") )
                {
                    optionValues = elementText;
                }
                else if ( e.tagName() == QLatin1String("optionIcons") || e.tagName() == QLatin1String("optionsIcons") )
                {
                    optionIcons = elementText;
                }
                else if ( e.tagName() == QLatin1String("displayValueScript") )
                {
                    field->setDisplayValueScript(elementText);
                }
                else if ( e.tagName() == QLatin1String("displayValueExpression") )
                {
                    field->setDisplayValueExpression(readStringExpression(e));
                }
                else if ( e.tagName() == QLatin1String("script") )
                {
                    field->setScript(elementText);
                }
                else if ( e.tagName() == QLatin1String("calculated") )
                {
                    field->setCalculated((elementText == QLatin1String("true") ? true : false));
                    if ( e.hasAttribute("calculatedOneTime") )
                    {
                        field->setCalculatedOneTime((e.attribute("calculatedOneTime", "false") == QLatin1String("true") ? true : false));
                    }
                    else if ( e.hasAttribute("oneTime") )
                    {
                        field->setCalculatedOneTime((e.attribute("oneTime", "false") == QLatin1String("true") ? true : false));
                    }
                    field->setCalculatedSaveOnDb(e.attribute("saveOnDb", "false") == QLatin1String("true") ? true : false);
                    field->setCalculatedOnlyOnInsert(e.attribute("onlyOnInsert", "false") == QLatin1String("true") ? true : false);
                    field->setCalculatedOrder(e.attribute("order").toInt());
                }
                else if ( e.tagName() == QLatin1String("builtInExpression") )
                {
                    field->setCalculated(true);
                    field->setBuiltInExpression(readBuiltInExpression(e));
                }
                else if ( e.tagName() == QLatin1String("builtInStringExpression") )
                {
                    field->setCalculated(true);
                    StringExpression exp = readStringExpression(e);
                    field->setBuiltInStringExpression(exp);
                }
                else if ( e.tagName() == QLatin1String("html") )
                {
                    field->setHtml((elementText == QLatin1String("true") ? true : false));
                }
                else if ( e.tagName() == QLatin1String("coordinates") )
                {
                    field->setCoordinates((elementText == QLatin1String("true") ? true : false));
                }
                else if ( e.tagName() == QLatin1String("email") )
                {
                    field->setEmail(elementText == QLatin1String("true") ? true : false);
                }
                else if ( e.tagName() == QLatin1String("scriptDefaultValue") )
                {
                    field->setScriptDefaultValue(elementText);
                }
                else if ( e.tagName() == QLatin1String("builtInExpressionDefaultValue") )
                {
                    field->setBuiltInExpressionDefaultValue(readBuiltInExpression(e));
                }
                else if ( e.tagName() == QLatin1String("builtInStringExpressionDefaultValue") )
                {
                    field->setBuiltInStringExpressionDefaultValue(readStringExpression(e));
                }
                else if ( e.tagName() == QLatin1String("validationRule") )
                {
                    field->setValidationRuleScript(elementText);
                }
                else if ( e.tagName() == QLatin1String("debug") )
                {
                    field->setDebug(elementText == QLatin1String("true") ? true : false);
                    field->setOnInitDebug(( e.attribute("onInitDebug", "false") == QLatin1String("true") ? true : false));
                }
                else if ( e.tagName() == QLatin1String("aggregateCalc") )
                {
                    readAggregate(e, field);
                }
                else if ( e.tagName() == QLatin1String("font") )
                {
                    readFont(e, field);
                }
                else if ( e.tagName() == QLatin1String("reloadFromDBAfterSave") )
                {
                    field->setReloadFromDBAfterSave((elementText == QLatin1String("true") ? true : false));
                }
                else if ( e.tagName() == QLatin1String("counter") )
                {
                    readCounterDefinition(e, field);
                }
                else if ( e.tagName() == QLatin1String("includeOnGeneratedRecordDlg") )
                {
                    field->setIncludeOnGeneratedRecordDlg((elementText == QLatin1String("true") ? true : false));
                }
                else if ( e.tagName() == QLatin1String("includeOnGeneratedSearchDlg") )
                {
                    field->setIncludeOnGeneratedSearchDlg((elementText == QLatin1String("true") ? true : false));
                }
                else if ( e.tagName() == QLatin1String("behaviourOnInlineEdit") )
                {
                    readBehaviourOnInlineEdit(e, field);
                }
                else if ( e.tagName() == QLatin1String("link") )
                {
                    field->setLink((elementText == QLatin1String("true") ? true : false));
                    field->setLinkOpenReadOnly(( e.attribute("openReadOnly", "false") == QLatin1String("true") ? true : false));
                    field->setLinkRelation(e.attribute(("relation")));
                }
                else if ( e.tagName() == QLatin1String("toolTipScript") )
                {
                    field->setToolTipScript(elementText);
                }
                else if ( e.tagName() == QLatin1String("toolTipStringExpression") )
                {
                    field->setToolTipStringExpression(readStringExpression(e));
                }
                else if ( e.tagName() == QLatin1String("onChangeScript") )
                {
                    field->setOnChangeScript(elementText);
                }
                else if ( e.tagName() == QLatin1String("dependOnFieldsToCalc") )
                {
                    readDependOnFieldsToCalc(e, field);
                }
                else if ( e.tagName() == QLatin1String("scheduleStartTime") )
                {
                    field->setScheduleStartTime(elementText == QLatin1String("true") ? true : false);
                }
                else if ( e.tagName() == QLatin1String("scheduleDuration") )
                {
                    field->setScheduleDuration(elementText == QLatin1String("true") ? true : false);
                }
                else if ( e.tagName() == QLatin1String("scheduleTimeUnit") )
                {
                    AlephERP::DateTimeParts timeUnit;
                    if ( elementText.toLower().contains(QStringLiteral("second")) )
                    {
                        timeUnit = AlephERP::Second;
                    }
                    else if ( elementText.toLower().contains(QStringLiteral("minute")) )
                    {
                        timeUnit = AlephERP::Minute;
                    }
                    else if ( elementText.toLower().contains(QStringLiteral("hour")) )
                    {
                        timeUnit = AlephERP::Hour;
                    }
                    else if ( elementText.toLower().contains(QStringLiteral("day")) )
                    {
                        timeUnit = AlephERP::DayOfMonth;
                    }
                    else if ( elementText.toLower().contains(QStringLiteral("week")) )
                    {
                        timeUnit = AlephERP::Week;
                    }
                    else if ( elementText.toLower().contains(QStringLiteral("month")) )
                    {
                        timeUnit = AlephERP::Month;
                    }
                    else if ( elementText.toLower().contains(QStringLiteral("year")) )
                    {
                        timeUnit = AlephERP::Year;
                    }
                    else
                    {
                        timeUnit = AlephERP::Minute;
                    }
                    field->setScheduleTimeUnit(timeUnit);
                }
            }
            // Ahora que tenemos toda la informacion podemos ajustar el default value
            if ( field->type() == QVariant::Int )
            {
                if ( !defaultValue.isEmpty() )
                {
                    field->setDefaultValue(QVariant(defaultValue.toInt()));
                }
            }
            else if ( field->type() == QVariant::LongLong )
            {
                if ( !defaultValue.isEmpty() )
                {
                    field->setDefaultValue(QVariant(defaultValue.toLongLong()));
                }
            }
            else if ( field->type() == QVariant::Double )
            {
                if ( !defaultValue.isEmpty() )
                {
                    field->setDefaultValue(QVariant(defaultValue.toDouble()));
                }
            }
            else if ( field->type() == QVariant::Bool )
            {
                field->setDefaultValue(defaultValue == QLatin1String("true") ? true : false);
            }
            else if ( field->type() == QVariant::Pixmap )
            {
                /** No hay valores por defecto para los pixmap */
                field->setDefaultValue(QVariant(QPixmap()));
            }
            else if ( field->type() == QVariant::Date )
            {
                if ( defaultValue.startsWith(QStringLiteral("now")) )
                {
                    field->setDefaultValue(QDate::currentDate());
                }
            }
            else if ( field->type() == QVariant::DateTime )
            {
                if ( defaultValue.startsWith(QStringLiteral("now")) )
                {
                    field->setDefaultValue(QDateTime::currentDateTime());
                }
            }
            else
            {
                if ( defaultValue == QLatin1String("username") )
                {
                    field->setDefaultValue(AERPLoggedUser::instance()->userName());
                }
                else
                {
                    field->setDefaultValue(QVariant(defaultValue));
                }
            }
            if ( field->coordinates() )
            {
                field->setType(QVariant::String);
            }
            // También podemos ajustar los options list
            if ( !optionList.isEmpty() )
            {
                QStringList visibleValues = optionList.split(QRegExp(";|,"));
                QStringList dbValues;
                QMap<QString, QString> map;

                if ( optionValues.isEmpty() )
                {
                    optionValues = optionList;
                }
                dbValues = optionValues.split(QRegExp(";|,"));

                // Algunas comprobaciones útiles para el desarrollador
                if ( visibleValues.size() != dbValues.size() && visibleValues.size() > 0 && dbValues.size() > 0 )
                {
                    QLogger::QLog_Error(AlephERP::stLogOther, QString("BaseBeanMetadataPrivate::setConfig: Atención. Tabla [%1], para el field [%2], optionList y optionValues "
                                        "tienen diferente tamaño.").arg(m_tableName).arg(field->dbFieldName()));
                }

                for ( int i = 0 ; i < visibleValues.size() ; i++ )
                {
                    if ( dbValues.size() < visibleValues.size() )
                    {
                        map[visibleValues.at(i).trimmed()] = visibleValues.at(i).trimmed();
                    }
                    else
                    {
                        map[dbValues.at(i).trimmed()] = visibleValues.at(i).trimmed();
                    }
                }
                field->setOptionList(map);

                QStringList iconValues = optionIcons.split(QRegExp(";|,"));
                if ( visibleValues.size() != iconValues.size() && visibleValues.size() > 0 && iconValues.size() > 0 )
                {
                    QLogger::QLog_Info(AlephERP::stLogOther, QString("BaseBeanMetadataPrivate::setConfig: Atención. Tabla [%1], para el field [%2], optionList y optionIcons "
                                        "tienen diferente tamaño.").arg(m_tableName).arg(field->dbFieldName()));
                }

                map.clear();
                for ( int i = 0 ; i < visibleValues.size() ; i++ )
                {
                    if ( iconValues.size() != visibleValues.size() )
                    {
                        map[visibleValues.at(i).trimmed()] = "";
                    }
                    else
                    {
                        map[visibleValues.at(i).trimmed()] = iconValues.at(i).trimmed();
                    }
                }
                field->setOptionIcons(map);
            }
            // Condiciones necesarias: Debemos tener establecido un nombre para el field
            if ( !field->dbFieldName().isEmpty() )
            {
                QVariant pointer = QVariant::fromValue(field);
                QByteArray ba = field->dbFieldName().toUtf8();
                q_ptr->setProperty(ba.constData(), pointer);
                field->setIndex(fieldIndex);
                m_fields.append(field);
                optionList.clear();
                optionValues.clear();
                defaultValue.clear();
                fieldIndex++;
            }
        }
        // Si no se ha definido un orden inicial por defecto, será la primary key
        if ( m_initOrderSort.isEmpty() )
        {
            QString sort;
            QList<DBFieldMetadata *> fldsPkey = q_ptr->pkFields();
            foreach (DBFieldMetadata *fld, fldsPkey)
            {
                if ( !sort.isEmpty() )
                {
                    sort += ", ";
                }
                sort = fld->dbFieldName() + " ASC";
            }
        }

        if ( m_associatedFunctions.size() > 0 )
        {
            loadAssociatedScripts();
        }
    }
    else
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString::fromUtf8("-------------------------------------------------------------------------------------------------------"));
        QLogger::QLog_Error(AlephERP::stLogOther, QString::fromUtf8("BaseBeanMetadata: setConfig(): FILE: [%1]. ERROR: Line: [%2] Column: [%3]. ERROR [%4] ").arg(m_tableName).arg(errorLine).arg(errorColumn).arg(errorString));
        QLogger::QLog_Error(AlephERP::stLogOther, QString::fromUtf8("-------------------------------------------------------------------------------------------------------"));
        QMessageBox::critical(0, qApp->applicationName(), QObject::trUtf8("El archivo XML de sistema <b>%1</b> no es correcto. "
                              "El programa no funcionará. Consulte con <i>Aleph Sistemas de Información</i>.").arg(m_tableName),
                              QMessageBox::Ok);
    }
}

void BaseBeanMetadataPrivate::readDBRelation(const QDomElement &e, DBFieldMetadata *field)
{
    DBRelationMetadata *relation = new DBRelationMetadata(q_ptr);
    relation->setParent(q_ptr);
    relation->setRootFieldName(field->dbFieldName());

    QDomNodeList n = e.childNodes();
    for ( int i = 0 ; i < n.size() ; i++ )
    {
        QDomElement e = n.at(i).toElement();
        if ( e.tagName() == QLatin1String("table") )
        {
            relation->setTableName(checkWildCards(e));
        }
        else if ( e.tagName() == QLatin1String("name") )
        {
            relation->setName(checkWildCards(e));
        }
        else if ( e.tagName() == QLatin1String("field") )
        {
            relation->setChildFieldName(checkWildCards(e));
        }
        else if ( e.tagName() == QLatin1String("card") )
        {
            if ( checkWildCards(e) == QLatin1String("1M") )
            {
                relation->setType(DBRelationMetadata::ONE_TO_MANY);
            }
            else if ( checkWildCards(e) == QLatin1String("11") )
            {
                relation->setType(DBRelationMetadata::ONE_TO_ONE);
            }
            else if ( checkWildCards(e) == QLatin1String("M1") )
            {
                relation->setType(DBRelationMetadata::MANY_TO_ONE);
            }
        }
        else if ( e.tagName() == QLatin1String("order") )
        {
            relation->setOrder(checkWildCards(e));
        }
        else if ( e.tagName() == QLatin1String("editable") )
        {
            relation->setEditable((checkWildCards(e) == QLatin1String("true") ? true : false));
        }
        else if ( e.tagName() == QLatin1String("delC") )
        {
            relation->setDeleteCascade((checkWildCards(e) == QLatin1String("true") ? true : false));
        }
        else if ( e.tagName() == QLatin1String("readOnly") )
        {
            relation->setReadOnly((checkWildCards(e) == QLatin1String("true") ? true : false));
        }
        else if ( e.tagName() == QLatin1String("referentialIntegrity") )
        {
            relation->setDbReferentialIntegrity((checkWildCards(e) == QLatin1String("true") ? true : false));
        }
        else if ( e.tagName() == QLatin1String("avoidDeleteIfIsReferenced") )
        {
            relation->setAvoidDeleteIfIsReferenced((checkWildCards(e) == QLatin1String("true") ? true : false));
        }
        else if ( e.tagName() == QLatin1String("includeOnCopy") )
        {
            relation->setIncludeOnCopy((checkWildCards(e) == QLatin1String("true") ? true : false));
        }
        else if ( e.tagName() == QLatin1String("linkToFatherOnGrid") )
        {
            relation->setLinkToFather((checkWildCards(e) == QLatin1String("true") ? true : false));
            relation->setLinkDialogReadOnly(e.attribute("readOnlyDialog", "false") == QLatin1String("true") ? true : false);
        }
        else if ( e.tagName() == QLatin1String("showOnRelatedModels") )
        {
            relation->setShowOnRelatedModels((checkWildCards(e) == QLatin1String("true") ? true : false));
        }
        else if ( e.tagName() == QLatin1String("reloadFromDBAfterSave") )
        {
            relation->setReloadFromDBAfterSave((checkWildCards(e) == QLatin1String("true") ? true : false));
        }
    }
    if ( relation->name().isEmpty() )
    {
        relation->setName(relation->tableName());
    }
    field->addRelation(relation);
    m_relations.append(relation);
    QVariant pointer = QVariant::fromValue(relation);
    QByteArray ba = relation->name().toUtf8();
    q_ptr->setProperty(ba.constData(), pointer);
}

/**
 * @brief BaseBeanMetadataPrivate::readStringExpression
 * Leerá una expresión para una presentación o display sencillo.
 * <var>pathAlValor1</var>
 * <var>pathAlValor2</var>
 * <expression>%1 %2</expression>
 * @param e
 * @return
 */
StringExpression BaseBeanMetadataPrivate::readStringExpression(const QDomElement &e)
{
    StringExpression expression;

    QDomNodeList n = e.childNodes();
    for ( int i = 0 ; i < n.size() ; i++ )
    {
        QDomElement e = n.at(i).toElement();
        if ( e.tagName().startsWith("var") )
        {
            QString variable = checkWildCards(e);
            expression.paths.append(variable);
        }
        else if ( e.tagName() == QLatin1String("expression") )
        {
            expression.expression = checkWildCards(e);
        }
    }
    if ( !expression.expression.isEmpty() )
    {
        expression.valid = true;
    }
    return expression;
}

/**
 * @brief BaseBeanMetadataPrivate::readBuiltInExpression
 * Leerá la entrada XML que define una expresión construida internamente para el cálculo de un campo. Por ejemplo
 * <var1 name="x"></var1>
 * <var2 name="y"></var2>
 * <expression>muParser expresion</expression>
 * En esta versión, las variables serán rutas (del estilo de las que se configuran en QtDesigner) que apuntan, mediante propiedades
 * a un elemento de este bean. Por ejemplo: Si este bean tiene un field llamado "importe", simplemente sería
 * importe
 * Si buscamos un elemento de una relación padre (supongamos que tiene una referencia a "divisas"
 * divisas.tasaconv
 * El elemento que debe devolver la variable es un tipo DBField (en esta versión)
 * Si en lugar de "<expression>" se utiliza "<stringExpression>" no se realizara ningun calculo matematico, sino
 * que se utilizan las mismas reglas de expresiones de cadenas.
 * @param e
 * @param field
 */
BuiltInExpressionDef BaseBeanMetadataPrivate::readBuiltInExpression(const QDomElement &e)
{
    BuiltInExpressionDef expression;

    QDomNodeList n = e.childNodes();
    for ( int i = 0 ; i < n.size() ; i++ )
    {
        QDomElement e = n.at(i).toElement();
        if ( e.tagName().startsWith("var") )
        {
            QString varName = e.attribute("name");
            QString variable = checkWildCards(e);
            if ( varName.isEmpty() )
            {
                QStringList path = variable.split(".");
                varName = path.last();
            }
            expression.addVariable(variable);
            expression.addVarName(varName);
        }
        else if ( e.tagName() == QLatin1String("expression") )
        {
            expression.setExpression(checkWildCards(e));
        }
    }
    if ( !expression.expression().isEmpty() )
    {
        expression.setValid(true);
    }
    return expression;
}

void BaseBeanMetadataPrivate::readAggregate(const QDomElement &e, DBFieldMetadata *field)
{
    AggregateCalc calc;
    QString fieldName, relationName, filter, script;

    QDomNodeList aggregateNodes = e.elementsByTagName("aggregate");
    field->setAggregate(true);
    field->setCalculated(true);
    if ( e.hasAttribute("order") )
    {
        field->setCalculatedOrder(e.attribute("order").toInt());
    }
    if ( e.hasAttribute("saveOnDb") )
    {
        field->setCalculatedSaveOnDb(e.attribute("saveOnDb") == QLatin1String("true") ? true : false);
    }
    // El funcionamiento se basa en que todas las listas contienen el mismo número de items.
    for (int i = 0 ; i < aggregateNodes.size() ; i++)
    {
        calc.expression.append(BuiltInExpressionDef());
        calc.field.append("");
        calc.filter.append("");
        calc.relation.append("");
        calc.script.append("");
    }
    for ( int i = 0 ; i < aggregateNodes.size() ; i++ )
    {
        QDomNodeList a = aggregateNodes.at(i).childNodes();
        fieldName = "";
        relationName = "";
        filter = "";
        script = "";
        for ( int j = 0 ; j < a.size() ; j ++ )
        {
            QDomElement element = a.at(j).toElement();
            if ( element.tagName() == QLatin1String("field") )
            {
                calc.field[i] = checkWildCards(element);
            }
            else if ( element.tagName() == QLatin1String("relation") )
            {
                calc.relation[i] = checkWildCards(element);
            }
            else if ( element.tagName() == QLatin1String("filter") )
            {
                calc.filter[i] = checkWildCards(element);
            }
            else if ( element.tagName() == QLatin1String("script") )
            {
                calc.script[i] = checkWildCards(element);
            }
            else if ( element.tagName() == QLatin1String("builtInExpression") )
            {
                calc.expression[i] = readBuiltInExpression(element);
            }
        }
    }
    QDomNodeList op = e.elementsByTagName("operation");
    if ( op.size() > 0 )
    {
        QDomElement operation = op.at(0).toElement();
        calc.operation = operation.text();
    }
    field->setAggregateCalc(calc);
}

void BaseBeanMetadataPrivate::readDependOnFieldsToCalc(const QDomElement &e, DBFieldMetadata *field)
{
    QStringList dependsFields;

    QDomNodeList fields = e.elementsByTagName("dependField");
    // El funcionamiento se basa en que todas las listas contienen el mismo número de items.
    for (int i = 0 ; i < fields.size() ; i++)
    {
        QDomElement childElement = fields.at(i).toElement();
        dependsFields << childElement.text();
    }
    q_ptr->registerRecalculateFields(field->dbFieldName(), dependsFields);
}

void BaseBeanMetadataPrivate::readSql(const QDomElement &e)
{
    QDomNodeList n = e.childNodes();
    for ( int i = 0 ; i < n.size() ; i++ )
    {
        QDomElement e = n.at(i).toElement();
        if ( e.tagName() == QLatin1String("from") )
        {
            addSqlPart("FROM", checkWildCards(e));
        }
        else if ( e.tagName() == QLatin1String("where") )
        {
            addSqlPart("WHERE", checkWildCards(e));
        }
        else if ( e.tagName() == QLatin1String("order") )
        {
            addSqlPart("ORDER", checkWildCards(e));
        }
        else if ( e.tagName() == QLatin1String("selectCount") )
        {
            addSqlPart("SELECTCOUNT", checkWildCards(e));
        }
    }
}

void BaseBeanMetadataPrivate::readTreeDefinition(const QDomElement &e)
{
    QDomNodeList n = e.childNodes();
    m_showOnTree = true;
    QDomElement loadAll = e.firstChildElement("preloadAllRecords");
    if ( !loadAll.isNull() )
    {
        m_showOnTreePreloadRecords = checkWildCards(loadAll) == QLatin1String("true") ? true : false;
    }
    for ( int i = 0 ; i < n.size() ; i++ )
    {
        if ( n.at(i).toElement().tagName() == QLatin1String("relation") )
        {
            QDomNodeList p = n.at(i).childNodes();
            QHash<QString, QVariant> hash;
            for ( int j = 0 ; j < p.size() ; j++ )
            {
                QDomElement final = p.at(j).toElement();
                if ( final.tagName() == QLatin1String("name") )
                {
                    hash["name"] = checkWildCards(final);
                }
                else if ( final.tagName() == QLatin1String("visibleField") )
                {
                    hash["visibleField"] = checkWildCards(final);
                }
                else if ( final.tagName() == QLatin1String("image") )
                {
                    hash["image"] = checkWildCards(final);
                }
                else if ( final.tagName() == QLatin1String("allowInsert") )
                {
                    hash["allowInsert"] = checkWildCards(final);
                }
                else if ( final.tagName() == QLatin1String("allowEdit") )
                {
                    hash["allowEdit"] = checkWildCards(final);
                }
                else if ( final.tagName() == QLatin1String("allowDelete") )
                {
                    hash["allowDelete"] = checkWildCards(final);
                }
            }
            m_treeDefinitions.append(hash);
        }
    }
}

void BaseBeanMetadataPrivate::readBehaviourOnInlineEdit(const QDomElement &e, DBFieldMetadata *fld)
{
    QHash<QString, QVariant> hash;
    QDomNodeList n = e.childNodes();
    for ( int i = 0 ; i < n.size() ; i++ )
    {
        QDomElement element = n.at(i).toElement();
        hash[n.at(i).toElement().tagName()] = checkWildCards(element);
    }
    fld->setBehaviourOnInlineEdit(hash);
}

void BaseBeanMetadataPrivate::readRelatedElementsContentToBeDelete(const QDomNode &e)
{
    QDomNodeList n = e.childNodes();

    for ( int i = 0 ; i < n.size() ; i++ )
    {
        if ( n.at(i).toElement().tagName() == QLatin1String("item") )
        {
            QDomNodeList children = n.at(i).childNodes();
            AlephERP::RelatedElementsContentToBeDeleted data;
            for ( int j = 0 ; j < children.size() ; j++ )
            {
                if ( children.at(j).toElement().tagName() == QLatin1String("tableName") )
                {
                    data.tableName = children.at(j).toElement().text();
                }
                else if ( children.at(j).toElement().tagName() == QLatin1String("category") )
                {
                    data.category = children.at(j).toElement().text();
                }
                else if ( children.at(j).toElement().tagName() == QLatin1String("type") )
                {
                    QString stringType = children.at(j).toElement().text();
                    if ( stringType == QLatin1String("record") )
                    {
                        data.type = AlephERP::Record;
                    }
                    else if ( stringType == QLatin1String("document") )
                    {
                        data.type = AlephERP::Document;
                    }
                    else if ( stringType == QLatin1String("email") )
                    {
                        data.type = AlephERP::Email;
                    }
                    else if ( stringType == QLatin1String("emailAttachment") )
                    {
                        data.type = AlephERP::EmailAttachment;
                    }
                    else
                    {
                        data.type = AlephERP::NoneAll;
                    }
                }
                else if ( children.at(j).toElement().tagName() == QLatin1String("cardinality") )
                {
                    QString stringType = children.at(j).toElement().text();
                    if ( stringType == QLatin1String("child") )
                    {
                        data.cardinality = AlephERP::PointToChild;
                    }
                    else if ( stringType == QLatin1String("master") )
                    {
                        data.cardinality = AlephERP::PointToMaster;
                    }
                    else
                    {
                        data.cardinality = AlephERP::PointToAll;
                    }
                }
            }
            m_relatedElementsContentToBeDelete.append(data);
        }
    }
}

void BaseBeanMetadataPrivate::readInfoSubTotal(const QDomElement &e)
{
    QDomNodeList nInfoSubTotal = e.childNodes();
    for ( int i = 0 ; i < nInfoSubTotal.size() ; i++ )
    {
        QDomNodeList n = nInfoSubTotal.at(i).childNodes();
        QHash<QString, QString> hash;
        for ( int j = 0 ; j < n.size() ; j++ )
        {
            QDomElement element = n.at(j).toElement();
            if ( element.tagName() == QLatin1String("alias") )
            {
                hash["alias"] = checkWildCards(element);
            }
            else if ( element.tagName() == QLatin1String("name") )
            {
                hash["name"] = checkWildCards(element);
            }
            else if ( element.tagName() == QLatin1String("calc") )
            {
                hash["calc"] = checkWildCards(element);
            }
            else if ( element.tagName() == QLatin1String("field") )
            {
                hash["field"] = checkWildCards(element);
            }
            else if ( element.tagName() == QLatin1String("sql") )
            {
                hash["sql"] = checkWildCards(element);
            }
            else if ( element.tagName() == QLatin1String("lineEditStyleSheet") )
            {
                hash["lineEditStyleSheet"] = checkWildCards(element);
            }
            else if ( element.tagName() == QLatin1String("labelStyleSheet") )
            {
                hash["labelStyleSheet"] = checkWildCards(element);
            }
            else if ( element.tagName() == QLatin1String("lineEditWidth") )
            {
                hash["lineEditWidth"] = checkWildCards(element);
            }
            else if ( element.tagName() == QLatin1String("lineEditAlign") )
            {
                hash["lineEditAlign"] = checkWildCards(element);
            }
        }
        if ( !hash.isEmpty() )
        {
            m_infoSubTotals.append(hash);
        }
    }
}

void BaseBeanMetadataPrivate::readFont(const QDomElement &e, DBFieldMetadata *field)
{
    QDomNodeList n = e.childNodes();
    QFont font;
    for ( int i = 0 ; i < n.size() ; i++ )
    {
        QDomElement e = n.at(i).toElement();
        QString elementText = checkWildCards(e);
        if ( e.tagName() == QLatin1String("name") )
        {
            font.setFamily(elementText);
        }
        else if ( e.tagName() == QLatin1String("size") )
        {
            font.setPointSize(elementText.toInt());
        }
        else if ( e.tagName() == QLatin1String("weight") )
        {
            if ( elementText == QLatin1String("light") )
            {
                font.setWeight(QFont::Light);
            }
            else if ( elementText == QLatin1String("normal") )
            {
                font.setWeight(QFont::Normal);
            }
            else if ( elementText == QLatin1String("demiBold") )
            {
                font.setWeight(QFont::DemiBold);
            }
            else if ( elementText == QLatin1String("bold") )
            {
                font.setWeight(QFont::Bold);
            }
            else if ( elementText == QLatin1String("black") )
            {
                font.setWeight(QFont::Black);
            }
            else
            {
                font.setWeight(QFont::Normal);
            }
        }
        else if ( e.tagName() == QLatin1String("italic") )
        {
            font.setItalic(elementText == QLatin1String("true") ? true : false);
        }
        else if ( e.tagName() == QLatin1String("color") )
        {
            QColor color;
            color.setNamedColor(elementText);
            field->setColor(color);
        }
        else if ( e.tagName() == QLatin1String("backgroundColor") )
        {
            QColor color;
            color.setNamedColor(elementText);
            field->setBackgroundColor(color);
        }
    }
    field->setFontOnGrid(font);
}

/*!
  Lee del XML los fields que se mostrarán en el DBFormDlg como filtros fuertas. Se cargará en m_itemsFilterColumn
  un conjunto de cadenas separadas por ; con los valores leidos
  */
void BaseBeanMetadataPrivate::readItemsFilterColumn(const QDomElement &e)
{
    QDomNodeList n = e.childNodes();
    for ( int i = 0 ; i < n.size() ; i++ )
    {
        if ( n.at(i).toElement().tagName() == QLatin1String("itemFilter") )
        {
            QDomNodeList p = n.at(i).childNodes();
            QHash<QString, QString> hash;
            for ( int j = 0 ; j < p.size() ; j++ )
            {
                QDomElement final = p.at(j).toElement();
                if ( final.tagName() == QLatin1String(AlephERP::stFieldToFilter) )
                {
                    hash[AlephERP::stFieldToFilter] = checkWildCards(final);
                    if ( final.hasAttribute(AlephERP::stShowTextLine) )
                    {
                        hash[AlephERP::stShowTextLine] = final.attribute(AlephERP::stShowTextLine);
                    }
                    if ( final.hasAttribute(AlephERP::stShowTextLineExactlySearch) )
                    {
                        hash[AlephERP::stShowTextLineExactlySearch] = final.attribute(AlephERP::stShowTextLineExactlySearch);
                    }
                    if ( final.hasAttribute(AlephERP::stShowTextLineAutocomplete) )
                    {
                        hash[AlephERP::stShowTextLineAutocomplete] = final.attribute(AlephERP::stShowTextLineAutocomplete);
                    }
                }
                else if ( final.tagName() == QLatin1String(AlephERP::stRelationFieldToShow) )
                {
                    hash[AlephERP::stRelationFieldToShow] = checkWildCards(final);
                }
                else if ( final.tagName() == QLatin1String(AlephERP::stOrder) )
                {
                    hash[AlephERP::stOrder] = checkWildCards(final);
                }
                else if ( final.tagName() == QLatin1String(AlephERP::stSetFilterValueOnNewRecords) )
                {
                    hash[AlephERP::stSetFilterValueOnNewRecords] = checkWildCards(final);
                }
                else if ( final.tagName() == QLatin1String(AlephERP::stRelationFilter) )
                {
                    QString filter = checkWildCards(final);
                    hash[AlephERP::stRelationFilter] = filter.replace("${user}", AERPLoggedUser::instance()->userName());
                }
                else if ( final.tagName() == QLatin1String(AlephERP::stRelationFilterScript) )
                {
                    hash[AlephERP::stRelationFilterScript] = checkWildCards(final);
                }
                else if ( final.tagName() == QLatin1String(AlephERP::stViewAllOption) )
                {
                    hash[AlephERP::stViewAllOption] = checkWildCards(final);
                }
            }

            if ( n.at(i).toElement().hasAttribute(AlephERP::stRow) )
            {
                hash[AlephERP::stRow] = n.at(i).toElement().attribute(AlephERP::stRow);
            }
            else
            {
                hash[AlephERP::stRow] = "0";
            }


            QString idFilter = QString("%1;%2;%3;%4;%5;%6;%7").arg(hash[AlephERP::stFieldToFilter]).arg(hash[AlephERP::stRelationFieldToShow]).arg(hash[AlephERP::stOrder])
                               .arg(hash[AlephERP::stSetFilterValueOnNewRecords]).arg(hash[AlephERP::stRelationFilter]).arg(hash[AlephERP::stRelationFilterScript])
                               .arg(hash[AlephERP::stViewAllOption]);
            hash["idFilter"] = QCryptographicHash::hash(idFilter.toUtf8(), QCryptographicHash::Md5);
            m_itemsFilterColumn.append(hash);
        }
    }
}

void BaseBeanMetadataPrivate::readEnvVarsFilter(const QDomElement &e)
{
    QString field, varName;
    QDomNodeList n = e.childNodes();
    for ( int i = 0 ; i < n.size() ; i++ )
    {
        if ( n.at(i).toElement().tagName() == QLatin1String("pair") )
        {
            QDomNodeList p = n.at(i).childNodes();
            for ( int j = 0 ; j < p.size() ; j++ )
            {
                QDomElement final = p.at(j).toElement();
                if ( final.tagName() == QLatin1String("field") )
                {
                    field = checkWildCards(final);
                }
                else if ( final.tagName() == QLatin1String("varName") )
                {
                    varName = checkWildCards(final);
                }
            }
            if ( !field.isEmpty() && !varName.isEmpty() )
            {
                EnvVarDefinition def;
                def.fieldName = field;
                def.varName = varName;
                m_envVars << def;
            }
        }
    }
}

/**
  Devuelve todos aquellos campos que componen la primaryKey
  */
QList<DBFieldMetadata *> BaseBeanMetadata::pkFields ()
{
    QList<DBFieldMetadata *> fields;
    for ( int i = 0 ; i < d->m_fields.size() ; i++ )
    {
        if ( d->m_fields.at(i)->primaryKey() )
        {
            fields.append(d->m_fields.at(i));
        }
    }
    if ( fields.isEmpty() )
    {
        qCritical() << "pkFields: BaseBean: " << tableName() << ". No existe primary key.";
    }
    return fields;
}

int BaseBeanMetadata::fieldCount()
{
    return d->m_fields.size();
}

DBFieldMetadata * BaseBeanMetadata::field(const QString &dbFieldName)
{
    DBFieldMetadata *field = NULL;
    // Veamos si el campo se refiere a un objeto de un padre o de una relación
    if ( dbFieldName.contains(QStringLiteral(".")) )
    {
        field = qobject_cast<DBFieldMetadata *>(navigateThroughProperties(dbFieldName));
    }
    else
    {
        for ( int i = 0 ; i < d->m_fields.size() ; i++ )
        {
            if ( d->m_fields.at(i)->dbFieldName() == dbFieldName )
            {
                field = d->m_fields.at(i);
            }
        }
    }
    if ( field == NULL && !dbFieldName.isEmpty() )
    {
        qCritical() << "field: BaseBean: " << tableName() << ". No existe el campo: " << dbFieldName;
    }
    return field;
}

DBFieldMetadata * BaseBeanMetadata::field(int index)
{
    if ( index < 0 || index >= d->m_fields.size() )
    {
        return NULL;
    }
    return d->m_fields.at(index);
}

int BaseBeanMetadata::fieldIndex(const QString &dbFieldName)
{
    int index = -1;
    for ( int i = 0 ; i < d->m_fields.size() ; i++ )
    {
        if ( d->m_fields.at(i)->dbFieldName() == dbFieldName )
        {
            index = i;
        }
    }
    if ( index == -1 )
    {
        qCritical() << "fieldIndex: BaseBean: " << tableName() << ". No existe el campo: " << dbFieldName;
    }
    return index;
}

/*!
 Proporciona una claúsula where para la primary key de este bean. En list van los valores
*/
QString BaseBeanMetadata::pkWhere(const QVariantMap &map)
{
    QString where;

    QList<DBFieldMetadata *> pk = pkFields();
    if ( pk.isEmpty() )
    {
        return where;
    }
    foreach ( DBFieldMetadata *fld, pk )
    {
        if ( where.isEmpty() )
        {
            where = fld->sqlWhere("=", map[fld->dbFieldName()].toString());
        }
        else
        {
            where = where + " AND " + fld->sqlWhere("=", map[fld->dbFieldName()].toString());
        }
    }
    return where;
}

/**
 * @brief BaseBeanMetadata::pkOrder
 * Devuelve la parte ORDER a incluir en una sentencia SQL con esta primary key
 * @return
 */
QString BaseBeanMetadata::pkOrder()
{
    QString order;
    QList<DBFieldMetadata *> pk = pkFields();
    if ( pk.isEmpty() )
    {
        return order;
    }
    foreach ( DBFieldMetadata *fld, pk )
    {
        if ( order.isEmpty() )
        {
            order = fld->dbFieldName() + " ASC";
        }
        else
        {
            order = order + ", " + fld->dbFieldName() + " ASC";
        }
    }
    return order;
}

/**
 * @brief BaseBeanMetadata::pkSelect Devuelve un string, con los campos que componen la primary key separados por ,
 * @return
 */
QString BaseBeanMetadata::sqlSelectPk()
{
    QString sql;

    QList<DBFieldMetadata *> pk = pkFields();
    if ( pk.isEmpty() )
    {
        return sql;
    }
    foreach ( DBFieldMetadata *fld, pk )
    {
        if ( sql.isEmpty() )
        {
            sql = fld->dbFieldName();
        }
        else
        {
            sql = sql + ", " + fld->dbFieldName();
        }
    }
    return sql;
}

/**
 * @brief BaseBeanMetadata::isScheduleValid
 * Indica si este bean es un registro
 * @return
 */
bool BaseBeanMetadata::isScheduleValid()
{
    bool foundStartTime = false;
    bool foundDuration = false;
    foreach (DBFieldMetadata *fld, d->m_fields)
    {
        if ( fld->scheduleStartTime() )
        {
            foundStartTime = true;
        }
        if ( fld->scheduleDuration() )
        {
            foundDuration = true;
        }
    }
    return foundStartTime & foundDuration;
}

DBRelationMetadata * BaseBeanMetadata::relation(const QString &relationName)
{
    DBRelationMetadata *rel = NULL;
    for ( int i = 0 ; i < d->m_relations.size() ; i++ )
    {
        if ( d->m_relations.at(i)->name() == relationName )
        {
            rel = d->m_relations.at(i);
        }
    }
    if ( rel == NULL )
    {
        QLogger::QLog_Debug(AlephERP::stLogOther, QString("relation: BaseBean: ") + tableName() + QString(". No existe la relacion: ") + relationName);
    }
    return rel;
}

/**
 * @brief BaseBeanMetadata::processToIncludePrefix
 * @param where
 * @param prefix
 * @return
 * Útil para agregar un prefijo a las campos en SQL
 */
QString BaseBeanMetadata::processToIncludePrefix(const QString &initialWhere, const QString &prefixTemplate)
{
    if ( prefixTemplate.isEmpty() )
    {
        return initialWhere;
    }

    QString prefix = prefixTemplate;
    QString where = initialWhere;

    if ( !prefix.isEmpty() && !prefix.endsWith(".") )
    {
        prefix.append(".");
    }
    foreach (DBFieldMetadata *f, fields())
    {
        QRegExp exp(QString("%1[\\s=)]").arg(f->dbFieldName()));
        int pos = 0;
        while ( (pos = exp.indexIn(where, pos)) != -1 )
        {
            where.replace(pos, f->dbFieldName().length(), f->dbFieldName().prepend(prefix));
            pos += exp.matchedLength();
        }
    }
    return where;
}

/*!
  Obtiene la cláusula where para obtener los registros filtrados según las variables de entorno.
  Debemos modelar un caso particular: Si en el initialWhere existe ya una variable de entorno establecida
  no agregaremos otra adicional a través de un AND. La idea es:
  1.- initialWhere = "idtienda = 2".
  2.- EnvVar contiene una variable de entorno idtienda con valor 3.
  No podemos construir un nuevo where así: idtienda = 2 AND idtienda = 3 ...
  así que debemos primero identificar que en initialWhere no hay ninguna variable de entorno
  */
QString BaseBeanMetadata::processWhereSqlToIncludeEnvVars(const QString &initialWhere, const QString &prefixTemplate)
{
    QHash<QString, QVariant> envVars = EnvVars::instance()->vars();
    QString where;
    QStringList initialWhereParts = initialWhere.toLower().split(" and ");
    QString prefix = prefixTemplate;

    if ( !prefix.isEmpty() && !prefix.endsWith(".") )
    {
        prefix.append(".");
    }

    // Se comprueba si los metadatos de este bean, contienen algún field
    // relacionado con alguna variable global con datos establecidos

    // Pudiese ocurrir que se introduzcan varias variables de entorno para un mismo campo
    // <envVars>
    //    <pair>
    //        <varName>id_tienda</varName>
    //        <field>id_tienda</field>
    //    </pair>
    //    <pair>
    //        <varName>id_tienda_empeños</varName>
    //        <field>id_tienda</field>
    //    </pair>
    //</envVars>
    // En este caso, no se hace un "=", sino que se haría una sql

    QMultiMap<QString, QVariant> multipleFieldsEntry;

    foreach ( const EnvVarDefinition &mEnvVar, d->m_envVars )
    {
        QVariant envVarValue = envVars.contains(mEnvVar.varName) ? envVars.value(mEnvVar.varName) : QVariant(QVariant::Invalid);
        if ( envVarValue.isValid() && !envVarValue.isNull() && (envVarValue.type() != QVariant::String || !envVarValue.toString().isEmpty()) )
        {
            DBFieldMetadata *fld = field(mEnvVar.fieldName);
            if ( fld != NULL )
            {
                bool found = false;
                foreach (const QString &initialWherePart, initialWhereParts)
                {
                    if ( initialWherePart.toLower().contains(fld->dbFieldName()) )
                    {
                        found = true;
                    }
                }
                if ( !found )
                {
                    int count = d->countEnvVarForOneField(fld->dbFieldName());
                    if ( count > 1 )
                    {
                        multipleFieldsEntry.insertMulti(fld->dbFieldName(), envVarValue);
                    }
                    else
                    {
                        if ( !where.isEmpty() )
                        {
                            where = QString("%1 AND ").arg(where);
                        }
                        where.append(prefix).append(fld->sqlWhere("=", envVarValue));
                    }
                }
            }
        }
    }

    QString tempSql;
    QString dbField;
    DBFieldMetadata *fld = NULL;
    QMapIterator<QString, QVariant> itMult(multipleFieldsEntry);
    while (itMult.hasNext())
    {
        itMult.next();
        if ( dbField.isEmpty() || dbField != itMult.key() )
        {
            dbField = itMult.key();
            fld = field(itMult.key());
            if ( !tempSql.isEmpty() )
            {
                if ( !where.isEmpty() )
                {
                    where.append(" AND ");
                }
                where = QString("%1(%2)").arg(where).arg(tempSql);
                tempSql.clear();
            }
        }
        if ( fld != NULL )
        {
            if (!tempSql.isEmpty())
            {
                tempSql = QString("%1 OR ").arg(tempSql);
            }
            tempSql = QString("%1%2").arg(tempSql).append(prefix).arg(fld->sqlWhere("=", itMult.value()));
        }
    }
    if ( !tempSql.isEmpty() )
    {
        if ( !where.isEmpty() )
        {
            where.append(" AND ");
        }
        where = QString("%1(%2)").arg(where).arg(tempSql);
    }

    if ( !initialWhere.isEmpty() )
    {
        if ( !where.isEmpty() )
        {
            where.append(" AND ");
        }
        where.append(processToIncludePrefix(initialWhere, prefix));
    }
    return where;
}

/**
 * @brief BaseBeanMetadata::fieldsOnSqlClausule
 * @param clausule
 * @return
 * Extrae los campos de este bean que se encuentran en la sentencia pasada
 */
QStringList BaseBeanMetadata::fieldsOnSqlClausule(const QString &clausule)
{
    QStringList result;
    foreach (DBFieldMetadata *fld, fields())
    {
        QString exp = QString("%1[\\s=)]").arg(fld->dbFieldName());
        if ( clausule.toLower().contains(QRegExp(exp)) )
        {
            result.append(fld->dbFieldName());
        }
    }
    return result;
}

int BaseBeanMetadataPrivate::countEnvVarForOneField(const QString &fieldName)
{
    int count = 0;

    foreach (const EnvVarDefinition &mEnvVar, m_envVars)
    {
        if ( mEnvVar.fieldName == fieldName )
        {
            DBFieldMetadata *fld = q_ptr->field(mEnvVar.fieldName);
            if ( fld != NULL )
            {
                count++;
            }
        }
    }
    return count;
}

void BaseBeanMetadataPrivate::readFormsConfigNames(const QDomElement &root)
{
    QDomNodeList nodes = root.elementsByTagName("uiDbRecord");
    for ( int i = 0 ; i < nodes.size() ; i++ )
    {
        if ( nodes.at(i).toElement().hasAttribute("role") )
        {
            m_uiDbRecord[nodes.at(i).toElement().attribute("role")] = nodes.at(i).toElement().text();
        }
        else
        {
            m_uiDbRecord["*"] = nodes.at(i).toElement().text();
        }
    }

    nodes = root.elementsByTagName("uiNewDbRecord");
    for ( int i = 0 ; i < nodes.size() ; i++ )
    {
        if ( nodes.at(i).toElement().hasAttribute("role") )
        {
            m_uiNewDbRecord[nodes.at(i).toElement().attribute("role")] = nodes.at(i).toElement().text();
        }
        else
        {
            m_uiNewDbRecord["*"] = nodes.at(i).toElement().text();
        }
    }

    nodes = root.elementsByTagName("uiWizard");
    for ( int i = 0 ; i < nodes.size() ; i++ )
    {
        if ( nodes.at(i).toElement().hasAttribute("role") )
        {
            m_uiWizard[nodes.at(i).toElement().attribute("role")] = nodes.at(i).toElement().text();
        }
        else
        {
            m_uiWizard["*"] = nodes.at(i).toElement().text();
        }
    }

    nodes = root.elementsByTagName("qmlDbRecord");
    for ( int i = 0 ; i < nodes.size() ; i++ )
    {
        if ( nodes.at(i).toElement().hasAttribute("role") )
        {
            m_qmlDbRecord[nodes.at(i).toElement().attribute("role")] = nodes.at(i).toElement().text();
        }
        else
        {
            m_qmlDbRecord["*"] = nodes.at(i).toElement().text();
        }
    }

    nodes = root.elementsByTagName("qmlNewDbRecord");
    for ( int i = 0 ; i < nodes.size() ; i++ )
    {
        if ( nodes.at(i).toElement().hasAttribute("role") )
        {
            m_qmlNewDbRecord[nodes.at(i).toElement().attribute("role")] = nodes.at(i).toElement().text();
        }
        else
        {
            m_qmlNewDbRecord["*"] = nodes.at(i).toElement().text();
        }
    }

    nodes = root.elementsByTagName("qsDbRecord");
    for ( int i = 0 ; i < nodes.size() ; i++ )
    {
        if ( nodes.at(i).toElement().hasAttribute("role") )
        {
            m_qsDbRecord[nodes.at(i).toElement().attribute("role")] = nodes.at(i).toElement().text();
        }
        else
        {
            m_qsDbRecord["*"] = nodes.at(i).toElement().text();
        }
    }

    nodes = root.elementsByTagName("qsNewDbRecord");
    for ( int i = 0 ; i < nodes.size() ; i++ )
    {
        if ( nodes.at(i).toElement().hasAttribute("role") )
        {
            m_qsNewDbRecord[nodes.at(i).toElement().attribute("role")] = nodes.at(i).toElement().text();
        }
        else
        {
            m_qsNewDbRecord["*"] = nodes.at(i).toElement().text();
        }
    }

    nodes = root.elementsByTagName("uiDbSearch");
    for ( int i = 0 ; i < nodes.size() ; i++ )
    {
        if ( nodes.at(i).toElement().hasAttribute("role") )
        {
            m_uiDbSearch[nodes.at(i).toElement().attribute("role")] = nodes.at(i).toElement().text();
        }
        else
        {
            m_uiDbSearch["*"] = nodes.at(i).toElement().text();
        }
    }

    nodes = root.elementsByTagName("qsDbSearch");
    for ( int i = 0 ; i < nodes.size() ; i++ )
    {
        if ( nodes.at(i).toElement().hasAttribute("role") )
        {
            m_qsDbSearch[nodes.at(i).toElement().attribute("role")] = nodes.at(i).toElement().text();
        }
        else
        {
            m_qsDbSearch["*"] = nodes.at(i).toElement().text();
        }
    }

    nodes = root.elementsByTagName("qsDbForm");
    for ( int i = 0 ; i < nodes.size() ; i++ )
    {
        if ( nodes.at(i).toElement().hasAttribute("role") )
        {
            m_qsDbForm[nodes.at(i).toElement().attribute("role")] = nodes.at(i).toElement().text();
        }
        else
        {
            m_qsDbForm["*"] = nodes.at(i).toElement().text();
        }
    }
}

/**
 * @brief BaseBeanMetadata::whereFromAdditionalFilter
 * Le permite a un usuario, establecer un filtro adicional más para los modelos
 * DBBaseBeanModel y TreeBaseBeanModel
 * @return
 */
QString BaseBeanMetadata::whereFromAdditionalFilter()
{
    if ( !d->m_additionalFilter.isEmpty() )
    {
        d->m_engine->setScript(d->m_additionalFilter, QString("%1.additionalFilter.js").arg(d->m_tableName));
        QVariant r = d->m_engine->toVariant(d->m_engine->callQsFunction(QString("additionalFilter")));
        return r.toString();
    }
    return QString();
}

/**
 * @brief BaseBeanMetadata::emailTemplateScriptExecute
 * Genera el template, que podrá enviarse en un correo electrónico
 * @param bean
 */
QString BaseBeanMetadata::emailTemplateScriptExecute(BaseBean *b)
{
    if ( b != NULL && !d->m_emailTemplateScript.isEmpty() )
    {
        d->m_engine->addFunctionArgument("bean", b);
        d->m_engine->setScript(d->m_emailTemplateScript, QString("%1.emailTemplate.js").arg(d->m_tableName));
        QVariant r = d->m_engine->toVariant(d->m_engine->callQsFunction(QString("emailTemplate")));
        return r.toString();
    }
    return QString();
}

/**
 * @brief BaseBeanMetadata::emailSubjectScriptExecute
 * Genera el asunto del correo electrónico para enviar datos de este bean
 * @param bean
 * @return
 */
QString BaseBeanMetadata::emailSubjectScriptExecute(BaseBean *b)
{
    if ( b != NULL && !d->m_emailSubjectScript.isEmpty() )
    {
        d->m_engine->addFunctionArgument("bean", b);
        d->m_engine->setScript(d->m_emailSubjectScript, QString("%1.emailSubject.js").arg(d->m_tableName));
        QVariant r = d->m_engine->toVariant(d->m_engine->callQsFunction(QString("emailSubject")));
        return r.toString();
    }
    return QString();
}

QString BaseBeanMetadata::repositoryPathScriptExecute(BaseBean *b)
{
    if ( b != NULL && !d->m_repositoryPathScript.isEmpty() )
    {
        d->m_engine->addFunctionArgument("bean", b);
        d->m_engine->setScript(d->m_repositoryPathScript, QString("%1.repositoryPath.js").arg(d->m_tableName));
        QVariant r = d->m_engine->toVariant(d->m_engine->callQsFunction(QString("repositoryPath")));
        return r.toString();
    }
    return QString();
}

QString BaseBeanMetadata::toStringExecute(BaseBean *b)
{
    QString result;
    if ( b != NULL && d->m_toStringType == QLatin1String("js") && !d->m_toStringScript.isEmpty() )
    {
        d->m_engine->addFunctionArgument("bean", b);
        d->m_engine->setScript(d->m_toStringScript, QString("%1.toString.js").arg(d->m_tableName));
        QVariant r = d->m_engine->toVariant(d->m_engine->callQsFunction(QString("toString")));
        return r.toString();
    }
    else if ( d->m_toStringType == QLatin1String("builtIn") && d->m_toStringExpression.valid )
    {
        return d->m_toStringExpression.applyExpressionRules(b);
    }
    else
    {
        foreach(DBField *fld, b->fields())
        {
            if ( fld->metadata()->showDefault() )
            {
                if (!result.isEmpty())
                {
                    result = QString("%1 - ").arg(result);
                }
                result = QString("%1%2").arg(result).arg(fld->displayValue());
            }
        }
        if ( result.isEmpty() )
        {
            result = trUtf8("Registro de tipo: %1").arg(d->m_alias.isEmpty() ? d->m_tableName : d->m_alias);
        }
    }
    return result;
}

QStringList BaseBeanMetadata::repositoryKeywordsScriptExecute(BaseBean *b)
{
    QStringList list;
    if ( b != NULL && !d->m_repositoryKeywordsScript.isEmpty() )
    {
        d->m_engine->addFunctionArgument("bean", b);
        d->m_engine->setScript(d->m_repositoryKeywordsScript, QString("%1.repositoryKeywords.js").arg(d->m_tableName));
        QVariant r = d->m_engine->toVariant(d->m_engine->callQsFunction(QString("repositoryKeywords")));
        QString result = r.toString();
        list = result.split(';');
    }
    return list;
}

/*!
Tras borrarse un registro de base de datos (antes de un posible commit), se llama a esta función, que ejecutará
el codigo asociado
*/
void BaseBeanMetadata::afterDeleteScriptExecute(BaseBean *b)
{
    if ( b != NULL && !d->m_afterDeleteScript.isEmpty() )
    {
        d->m_engine->addFunctionArgument("bean", b);
        d->m_engine->setScript(d->m_afterDeleteScript, QString("%1.afterDelete.js").arg(d->m_tableName));
        d->m_engine->callQsFunction(QString("afterDelete"));
    }
}

/*!
  Función que invoca un posible método a ejecutar tras guardar (insert o update) un nuevo registro en base de datos.
  El motor se encargara de llamarla sólo cuando el registro se ha guardado en base de datos
  */
void BaseBeanMetadata::afterSaveScriptExecute(BaseBean *b)
{
    if ( b != NULL && !d->m_afterSaveScript.isEmpty() )
    {
        d->m_engine->addFunctionArgument("bean", b);
        d->m_engine->setScript(d->m_afterSaveScript, QString("%1.afterSave.js").arg(d->m_tableName));
        d->m_engine->callQsFunction(QString("afterSave"));
    }
}

void BaseBeanMetadata::afterCopyScriptExecute(BaseBean *beanOriginal, BaseBean *beanDest)
{
    if ( !d->m_afterCopyScript.isEmpty() )
    {
        d->m_engine->addFunctionArgument("beanOriginal", beanOriginal);
        d->m_engine->addFunctionArgument("beanDest", beanDest);
        d->m_engine->setScript(d->m_afterCopyScript, QString("%1.afterCopy.js").arg(d->m_tableName));
        d->m_engine->callQsFunction(QString("afterCopy"));
    }
}

void BaseBeanMetadata::onCreateScriptExecute(BaseBean *bean)
{
    if ( !d->m_onCreateScript.isEmpty() )
    {
        d->m_engine->addFunctionArgument("bean", bean);
        d->m_engine->setScript(d->m_onCreateScript, QString("%1.onCreate.js").arg(d->m_tableName));
        d->m_engine->callQsFunction(QString("onCreate"));
    }
}

QString BaseBeanMetadata::validateScriptExecute(BaseBean *bean)
{
    if ( !d->m_validateScript.isEmpty() )
    {
        d->m_engine->addFunctionArgument("bean", bean);
        d->m_engine->setScript(d->m_validateScript, QString("%1.validateScript.js").arg(d->m_tableName));
        QVariant r = d->m_engine->toVariant(d->m_engine->callQsFunction(QString("validate")));
        if ( !r.isValid() )
        {
            return QString(trUtf8("Undefined error"));
        }
        return r.toString();
    }
    return QString();
}

/*!
  Función que invoca un posible método a ejecutar tras insertar un nuevo registro en base de datos.
  El motor se encargara de llamarla sólo cuando el registro se ha guardado en base de datos
  */
void BaseBeanMetadata::afterInsertScriptExecute(BaseBean *b)
{
    if ( !d->m_afterInsertScript.isEmpty() )
    {
        d->m_engine->addFunctionArgument("bean", b);
        d->m_engine->setScript(d->m_afterInsertScript, QString("%1.afterInsert.js").arg(d->m_tableName));
        d->m_engine->callQsFunction(QString("afterInsert"));
    }
}

void BaseBeanMetadata::afterUpdateScriptExecute(BaseBean *b)
{
    if ( !d->m_afterUpdateScript.isEmpty() )
    {
        d->m_engine->addFunctionArgument("bean", b);
        d->m_engine->setScript(d->m_afterUpdateScript, QString("%1.afterUpdate.js").arg(d->m_tableName));
        d->m_engine->callQsFunction(QString("afterUpdate"));
    }
}

bool BaseBeanMetadata::beforeSaveScriptExecute(BaseBean *b)
{
    if ( !d->m_beforeSaveScript.isEmpty() )
    {
        d->m_engine->addFunctionArgument("bean", b);
        d->m_engine->setScript(d->m_beforeSaveScript, QString("%1.beforeSave.js").arg(d->m_tableName));
        QVariant r = d->m_engine->toVariant(d->m_engine->callQsFunction(QString("beforeSave")));
        if ( !r.isValid() )
        {
            return false;
        }
        return r.toBool();
    }
    return true;
}

bool BaseBeanMetadata::beforeCopyScriptExecute(BaseBean *beanOriginal, BaseBean *beanDest)
{
    if ( !d->m_beforeCopyScript.isEmpty() )
    {
        d->m_engine->addFunctionArgument("beanOriginal", beanOriginal);
        d->m_engine->addFunctionArgument("beanDest", beanDest);
        d->m_engine->setScript(d->m_beforeCopyScript, QString("%1.beforeCopy.js").arg(d->m_tableName));
        QVariant r = d->m_engine->toVariant(d->m_engine->callQsFunction(QString("beforeCopy")));
        if ( !r.isValid() )
        {
            return false;
        }
        return r.toBool();
    }
    return true;
}

bool BaseBeanMetadata::isAccessible(BaseBean *b)
{
    if ( !d->m_accessible.isEmpty() )
    {
        d->m_engine->addFunctionArgument("bean", b);
        d->m_engine->setScript(d->m_accessible, QString("%1.accesibleRule.js").arg(d->m_tableName));
        QVariant r = d->m_engine->toVariant(d->m_engine->callQsFunction(QString("accesibleRule")));
        if ( !r.isValid() )
        {
            return false;
        }
        return r.toBool();
    }
    return true;
}

bool BaseBeanMetadata::beforeInsertScriptExecute(BaseBean *b)
{
    if ( !d->m_beforeInsertScript.isEmpty() )
    {
        d->m_engine->addFunctionArgument("bean", b);
        d->m_engine->setScript(d->m_beforeInsertScript, QString("%1.beforeInsert.js").arg(d->m_tableName));
        QVariant r = d->m_engine->toVariant(d->m_engine->callQsFunction(QString("beforeInsert")));
        if ( !r.isValid() )
        {
            return false;
        }
        return r.toBool();
    }
    return true;
}

bool BaseBeanMetadata::beforeUpdateScriptExecute(BaseBean *b)
{
    if ( !d->m_beforeUpdateScript.isEmpty() )
    {
        d->m_engine->addFunctionArgument("bean", b);
        d->m_engine->setScript(d->m_beforeUpdateScript, QString("%1.beforeUpdate.js").arg(d->m_tableName));
        QVariant r = d->m_engine->toVariant(d->m_engine->callQsFunction(QString("beforeUpdate")));
        if ( !r.isValid() )
        {
            return false;
        }
        return r.toBool();
    }
    return true;
}

bool BaseBeanMetadata::beforeDeleteScriptExecute(BaseBean *b)
{
    if ( !d->m_beforeDeleteScript.isEmpty() )
    {
        d->m_engine->addFunctionArgument("bean", b);
        d->m_engine->setScript(d->m_beforeDeleteScript, QString("%1.beforeDelete.js").arg(d->m_tableName));
        QVariant r = d->m_engine->toVariant(d->m_engine->callQsFunction(QString("beforeDelete")));
        if ( !r.isValid() )
        {
            return false;
        }
        return r.toBool();
    }
    return true;
}

QColor BaseBeanMetadata::rowColorScriptExecute(BaseBean *b)
{
    QColor color;
    if ( !d->m_rowColorScript.isEmpty() )
    {
        d->m_engine->addFunctionArgument("bean", b);
        d->m_engine->setScript(d->m_rowColorScript, QString("%1.rowColor.js").arg(d->m_tableName));
        QVariant r = d->m_engine->toVariant(d->m_engine->callQsFunction(QString("rowColor")));
        if ( !r.toString().isEmpty() )
        {
            color = QColor(r.toString());
        }
    }
    return color;
}

QString BaseBeanMetadata::toolTipExecute(BaseBean *b)
{
    if ( !d->m_toolTipStringExpression.isEmpty() )
    {
        return d->m_toolTipExpression.applyExpressionRules(b);
    }
    if ( !d->m_toolTipScript.isEmpty() )
    {
        d->m_engine->addFunctionArgument("bean", b);
        d->m_engine->setScript(d->m_toolTipScript, QString("%1.toolTip.js").arg(d->m_tableName));
        QVariant r = d->m_engine->toVariant(d->m_engine->callQsFunction(QString("toolTip")));
        return r.toString();
    }
    return QString();
}

void BaseBeanMetadata::newRelatedElementScriptExecute(BaseBean *bean, RelatedElement *element)
{
    if ( !d->m_newRelatedElementScript.isEmpty() )
    {
        d->m_engine->addFunctionArgument("bean", bean);
        d->m_engine->addFunctionArgument("element", element);
        d->m_engine->setScript(d->m_newRelatedElementScript, QString("%1.newRelatedElement.js").arg(d->m_tableName));
        d->m_engine->callQsFunction(QString("newRelatedElement"));
    }
}

void BaseBeanMetadata::modifiedRelatedElementScriptExecute(BaseBean *bean, RelatedElement *element)
{
    if ( !d->m_modifiedRelatedElementScript.isEmpty() )
    {
        d->m_engine->addFunctionArgument("bean", bean);
        d->m_engine->addFunctionArgument("element", element);
        d->m_engine->setScript(d->m_newRelatedElementScript, QString("%1.modifiedRelatedElement.js").arg(d->m_tableName));
        d->m_engine->callQsFunction(QString("modifiedRelatedElement"));
    }
}

void BaseBeanMetadata::deletedRelatedElementScriptExecute(BaseBean *bean, RelatedElement *element)
{
    if ( !d->m_deletedRelatedElementScript.isEmpty() )
    {
        d->m_engine->addFunctionArgument("bean", bean);
        d->m_engine->addFunctionArgument("element", element);
        d->m_engine->setScript(d->m_deletedRelatedElementScript, QString("%1.deletedRelatedElement.js").arg(d->m_tableName));
        d->m_engine->callQsFunction(QString("deletedRelatedElement"));
    }
}

bool BaseBeanMetadata::readOnlyScriptExecute(BaseBean *bean)
{
    if ( !d->m_readOnlyScript.isEmpty() )
    {
        d->m_engine->addFunctionArgument("bean", bean);
        d->m_engine->setScript(d->m_readOnlyScript, QString("%1.readOnly.js").arg(d->m_tableName));
        QVariant r = d->m_engine->toVariant(d->m_engine->callQsFunction(QString("readOnly")));
        if ( !r.isValid() )
        {
            return true;
        }
        return r.toBool();
    }
    return false;
}

void BaseBeanMetadataPrivate::loadAssociatedScripts()
{
    bool debugBackup, onInitDebugBackup;
    debugBackup = m_engine->debug();
    onInitDebugBackup = m_engine->onInitDebug();
    foreach (AssociatedFunctions *item, m_associatedFunctions)
    {
        if ( BeansFactory::systemScripts.contains(item->scriptFileName) )
        {
            QString script = BeansFactory::systemScripts.value(item->scriptFileName).trimmed();
            m_engine->setScript(script, item->scriptFileName);
            m_engine->setDebug(BeansFactory::systemScriptsDebug.value(item->scriptFileName));
            m_engine->setOnInitDebug(BeansFactory::systemScriptsDebugOnInit.value(item->scriptFileName));
            QScriptValue associatedScriptValue = m_engine->evaluateObjectScript();
            if ( associatedScriptValue.isValid() && !associatedScriptValue.isError() )
            {
                QScriptValueIterator it(associatedScriptValue);
                while (it.hasNext())
                {
                    it.next();
                    QString propertyName = it.name();
                    QScriptValue prop = it.value();
                    if ( prop.isValid() && prop.isFunction() )
                    {
                        item->functions.append(propertyName);
                    }
                }
            }
            else
            {
                QLogger::QLog_Error(AlephERP::stLogScript, "BaseBeanMetadata::loadAssociatedScript: No se pudieron cargar las funciones asociadas");
            }
        }
        else
        {
            QLogger::QLog_Error(AlephERP::stLogScript, QString("BaseBeanMetadata::loadAssociatedScript: NO EXISTE NINGÚN SCRIPT QS DE NOMBRE [%1]").arg(item->scriptFileName));
        }
    }
    m_engine->setDebug(debugBackup);
    m_engine->setOnInitDebug(onInitDebugBackup);
}

void BaseBeanMetadataPrivate::readInternalConnections(const QDomElement &e)
{
    QDomNodeList n = e.childNodes();
    for ( int i = 0 ; i < n.size() ; i++ )
    {
        if ( n.at(i).toElement().tagName() == QLatin1String("connection") )
        {
            QDomNodeList p = n.at(i).childNodes();
            InternalConnection conn;
            for ( int j = 0 ; j < p.size() ; j++ )
            {
                QDomElement final = p.at(j).toElement();
                if ( final.tagName() == QLatin1String("sender") )
                {
                    conn.senderQs = checkWildCards(final);
                }
                else if ( final.tagName() == QLatin1String("action") )
                {
                    ConnectionAction connAction;
                    QDomNodeList actions = final.childNodes();
                    for ( int iAction = 0 ; iAction < actions.size() ; ++iAction )
                    {
                        QDomElement actionElement = actions.at(iAction).toElement();
                        if ( actionElement.tagName() == QLatin1String("signal") )
                        {
                            connAction.signalAction = checkWildCards(actionElement);
                        }
                        else if ( actionElement.tagName() == QLatin1String("slot") )
                        {
                            connAction.slotsAction.append(checkWildCards(actionElement));
                        }
                    }
                    conn.actions.append(connAction);
                }
            }
            m_internalConnections.append(conn);
        }
    }
}

void BaseBeanMetadataPrivate::readScheduledData(const QDomElement &e)
{
    QDomNodeList n = e.childNodes();
    for ( int i = 0 ; i < n.size() ; i++ )
    {
        if ( n.at(i).toElement().tagName() == QLatin1String("showModesFilter") )
        {
            m_scheduledData.showViewModesFilter = (n.at(i).toElement().text() == QLatin1String("true") ? true : false);
        }
        if ( n.at(i).toElement().tagName() == QLatin1String("viewMode") )
        {
            QString itemText = n.at(i).toElement().text();
            if ( itemText == QLatin1String("week") )
            {
                m_scheduledData.viewMode = AERPScheduleView::WeekView;
            }
            else if ( itemText == QLatin1String("month") )
            {
                m_scheduledData.viewMode = AERPScheduleView::MonthView;
            }
            else if ( itemText == QLatin1String("day") )
            {
                m_scheduledData.viewMode = AERPScheduleView::DayView;
            }
            else
            {
                m_scheduledData.viewMode = AERPScheduleView::WeekView;
            }
        }
        if ( n.at(i).toElement().tagName() == QLatin1String("canOverlap") )
        {
            m_scheduledData.canOverlap = (n.at(i).toElement().text() == QLatin1String("true") ? true : false);
        }
        if ( n.at(i).toElement().tagName() == QLatin1String("canMove") )
        {
            m_scheduledData.canMove = (n.at(i).toElement().text() == QLatin1String("true") ? true : false);
        }
        if ( n.at(i).toElement().tagName() == QLatin1String("stepWidthInSeconds") )
        {
            bool ok;
            m_scheduledData.stepWidthInSeconds = n.at(i).toElement().text().toInt(&ok);
            if ( !ok )
            {
                m_scheduledData.stepWidthInSeconds = 5 * 60;
            }
        }
        if ( n.at(i).toElement().tagName() == QLatin1String("qsTitleExpression") )
        {
            m_scheduledData.qsTitleExpression = n.at(i).toElement().text();
        }
        if ( n.at(i).toElement().tagName() == QLatin1String("titleExpression") )
        {
            QDomElement element = n.at(i).toElement();
            m_scheduledData.titleExpression = readStringExpression(element);
        }
    }
}

/**
 * @brief BaseBeanMetadata::associatedScriptFunctions
 * Devuelve el listado de funciones (los nombres) del script asociado al bean
 * @return
 */
QStringList BaseBeanMetadata::associatedScriptFunctions()
{
    QStringList functions;
    foreach (AssociatedFunctions *item, d->m_associatedFunctions)
    {
        functions.append(item->functions);
    }
    return functions;
}

QString BaseBeanMetadata::associatedScript(const QString &functionName)
{
    foreach (AssociatedFunctions *item, d->m_associatedFunctions)
    {
        if ( item->functions.contains(functionName) )
        {
            return item->scriptFileName;
        }
    }
    return QString();
}

void BaseBeanMetadataPrivate::readCounterDefinition(const QDomElement &e, DBFieldMetadata *field)
{
    QStringList fields, prefix;
    QDomNodeList n = e.childNodes();
    for ( int i = 0 ; i < n.size() ; i++ )
    {
        if ( n.at(i).toElement().tagName() == QLatin1String("dependsOnField") )
        {
            fields.append(n.at(i).toElement().text());
            prefix.append(n.at(i).toElement().attribute("prefixOnRelation", ""));
        }
        if ( n.at(i).toElement().tagName() == QLatin1String("userCanModify") )
        {
            field->setCounterDefinitionUserCanModified((n.at(i).toElement().text() == QLatin1String("true") ? true : false));
        }
        if ( n.at(i).toElement().tagName() == QLatin1String("separator") )
        {
            field->setCounterDefinitionSeparator(n.at(i).toElement().text());
        }
        if ( n.at(i).toElement().tagName() == QLatin1String("calculateOnlyOnInsert") )
        {
            field->setCounterDefinitionCalculateOnlyOnInsert((n.at(i).toElement().text() == QLatin1String("true") ? true : false));
        }
        if ( n.at(i).toElement().tagName() == QLatin1String("fixedString") )
        {
            fields.append(n.at(i).toElement().text());
            prefix.append("");
        }
        if ( n.at(i).toElement().tagName() == QLatin1String("useTrailingZeros") )
        {
            field->setCounterDefinitionUseTrailingZeros((n.at(i).toElement().text() == QLatin1String("true") ? true : false));
        }
        if ( n.at(i).toElement().tagName() == QLatin1String("expression") )
        {
            field->setCounterDefinitionExpression(n.at(i).toElement().text());
        }
    }
    field->setCounterDefinitionFields(fields);
    field->setCounterDefinitionPrefixOnRelation(prefix);
    if ( field->counterDefinition()->fields.size() == 0 && field->counterDefinition()->expression.isEmpty() )
    {
        field->setCounterDefinitionExpression("{value}");
    }
}

/*!
  Tenemos que decirle al motor de scripts, que BaseBean se convierte de esta forma a un valor script
  */
QScriptValue BaseBeanMetadata::toScriptValue(QScriptEngine *engine, BaseBeanMetadata * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void BaseBeanMetadata::fromScriptValue(const QScriptValue &object, BaseBeanMetadata * &out)
{
    out = qobject_cast<BaseBeanMetadata *>(object.toQObject());
}

void BaseBeanMetadata::processScriptSignalException(const QScriptValue &exception)
{
    qDebug() << "BaseBeanMetadata::processScriptSignalException: " << exception.toString();
}

/**
 * @brief BeansFactory::createTableSql Devuelve la sentencia DDL de creación de la tabla a partir de la información pasada en los metadatos
 * @param tableName
 * @param dialect Base de datos para la que se creará el DDL (SQLITE o PSQL)
 * @param alternativeName Especifica otro nombre para la tabla (no tableName);
 * @return
 */
QString BaseBeanMetadata::sqlCreateTable(AlephERP::CreationTableSqlOptions options, const QString &dialect, const QString &alternativeName)
{
    QString sql = QString("CREATE TABLE %1 (").arg(alternativeName.isEmpty() ? sqlTableName(dialect) : alternativeName);
    QString fieldsSql;
    AlephERP::CreationTableSqlOptions optionsForColumn = options;

    foreach ( DBFieldMetadata *field, fields() )
    {
        if ( field->isOnDb() )
        {
            if ( !fieldsSql.isEmpty() )
            {
                fieldsSql = QString("%1, %2").arg(fieldsSql).arg(field->ddlCreationTable(optionsForColumn, dialect));
            }
            else
            {
                fieldsSql = field->ddlCreationTable(optionsForColumn, dialect);
            }
        }
    }
    if ( options.testFlag(AlephERP::WithCommitColumnToLocalWork) )
    {
        if ( dialect == QLatin1String("QIBASE") )
        {
            fieldsSql = QString("%1, committed boolean DEFAULT 0").arg(fieldsSql);
        }
        else
        {
            fieldsSql = QString("%1, committed boolean DEFAULT false").arg(fieldsSql);
        }
    }
    if ( options.testFlag(AlephERP::WithSimulateOID) )
    {
        fieldsSql = QString("%1, oid integer").arg(fieldsSql);
    }
    if ( options.testFlag(AlephERP::WithRemoteOID) )
    {
        fieldsSql = QString("%1, remoteoid integer").arg(fieldsSql);
    }
    if ( options.testFlag(AlephERP::WithHashRowColumn) )
    {
        fieldsSql = QString("%1, hash character varying(100)").arg(fieldsSql);
    }
    if ( options.testFlag(AlephERP::LogicalDeleteColumn) )
    {
        if ( dialect == QLatin1String("QIBASE") )
        {
            fieldsSql = QString("%1, is_deleted boolean DEFAULT 0").arg(fieldsSql);
        }
        else
        {
            fieldsSql = QString("%1, is_deleted boolean DEFAULT false").arg(fieldsSql);
        }
    }
    sql = QString("%1%2").arg(sql).arg(fieldsSql);
    if ( pkFields().size() > 0 )
    {
        QString pkeySql;
        bool isSerial = false;
        foreach ( DBFieldMetadata *field, pkFields() )
        {
            isSerial = isSerial | field->serial();
            if ( !pkeySql.isEmpty() )
            {
                pkeySql = QString("%1, %2").arg(pkeySql).arg(field->dbFieldName());
            }
            else
            {
                pkeySql = field->dbFieldName();
            }
        }
        QString constraintName = QString("%1_pkey").arg(d->m_tableName);
        if ( dialect == QLatin1String("QIBASE") && constraintName.size() > 30 )
        {
            constraintName = QString("pkey_%1").arg(alephERPSettings->uniqueId());
        }
        if ( dialect == QLatin1String("QSQLITE") )
        {
            if ( !isSerial )
            {
                QString constraintPkey = QString("CONSTRAINT %1 PRIMARY KEY(%2))").arg(constraintName).arg(pkeySql);
                sql = QString("%1, %2").arg(sql).arg(constraintPkey);
            }
            else
            {
                sql = sql.append(")");
            }
        }
        else
        {
            QString constraintPkey = QString("CONSTRAINT %1 PRIMARY KEY(%2))").arg(constraintName).arg(pkeySql);
            sql = QString("%1, %2").arg(sql).arg(constraintPkey);
        }
    }
    else
    {
        sql = QString("%1)").arg(sql);
    }
    if ( dialect == QLatin1String("QPSQL") )
    {
        sql = QString("%1 WITH (OIDS=TRUE)").arg(sql);
    }
    return sql;
}

/**
 * @brief BaseBeanMetadata::sqlForeignKeys Para las relaciones de este bean, obtiene las foreign keys
 * @param options
 * @param dialect
 * @return
 */
QStringList BaseBeanMetadata::sqlForeignKeys(AlephERP::CreationTableSqlOptions options, const QString &dialect)
{
    QStringList ddl;
    foreach ( DBRelationMetadata *rel, d->m_relations )
    {
        ddl.append(rel->sqlForeignKey(options, dialect));
    }
    return ddl;
}

/**
 * @brief BaseBeanMetadata::sqlCreateIndex Devolverá las sentencias para la creación de los índices necesarios
 * @param dialect
 * @return
 */
QString BaseBeanMetadata::sqlCreateIndex(AlephERP::CreationTableSqlOptions options, const QString &dialect)
{
    Q_UNUSED(dialect)
    QString sql;
    foreach ( DBFieldMetadata *fld, d->m_fields )
    {
        bool createIndex = false;
        if ( options.testFlag(AlephERP::CreateIndexOnRelationColumns) )
        {
            QList<DBRelationMetadata *> relations = fld->relations(AlephERP::ManyToOne);
            createIndex = relations.size() > 0;
        }
        createIndex = createIndex | fld->dbIndex();
        // Creamos índices si el campo está marcado como tal, y si contiene una columna que referencia a una tabla padre.
        if ( createIndex )
        {
            QString unique = fld->unique() ? " UNIQUE " : " ";
            QString idxName = QString("idx_%1_%2").arg(tableName()).arg(fld->dbFieldName());
            if ( dialect == QLatin1String("QIBASE") && idxName.size() > MAX_LENGTH_OBJECT_NAME_FIREBIRD )
            {
                idxName = QString("idx_%1").arg(alephERPSettings->uniqueId());
            }
            if ( dialect == QLatin1String("QPSQL") && idxName.size() > MAX_LENGTH_OBJECT_NAME_PSQL )
            {
                idxName = QString("idx_%1").arg(alephERPSettings->uniqueId());
            }
            sql = QString("%1CREATE%2INDEX %3 ON %4(%5);").
                  arg(sql).
                  arg(unique).
                  arg(idxName).
                  arg(sqlTableName(dialect)).
                  arg(fld->dbFieldName());
        }
    }
    if ( options.testFlag(AlephERP::WithHashRowColumn) )
    {
        QString idxName = QString("idx_%1_hash").arg(tableName());
        if ( dialect == QLatin1String("QIBASE") && idxName.size() > MAX_LENGTH_OBJECT_NAME_FIREBIRD )
        {
            idxName = QString("idx_%1").arg(alephERPSettings->uniqueId());
        }
        if ( dialect == QLatin1String("QPSQL") && idxName.size() > MAX_LENGTH_OBJECT_NAME_PSQL )
        {
            idxName = QString("idx_%1").arg(alephERPSettings->uniqueId());
        }
        sql = QString("%1CREATE UNIQUE INDEX %2 ON %3(hash);").
              arg(sql).
              arg(idxName).
              arg(sqlTableName(dialect));
    }
    return sql;
}

QString BaseBeanMetadata::sqlAddColumn(AlephERP::CreationTableSqlOptions options, const QString &dbFieldName, const QString &dialect)
{
    DBFieldMetadata *fld = field(dbFieldName);
    if ( fld == NULL )
    {
        return QString();
    }
    QString partialSql = fld->ddlCreationTable(options, dialect);
    QString sql = QString("ALTER TABLE %1 ADD COLUMN %2").arg(sqlTableName(dialect)).arg(partialSql);
    return sql;
}

QString BaseBeanMetadata::sqlDropColumn(AlephERP::CreationTableSqlOptions options, const QString &dbFieldName, const QString &dialect)
{
    Q_UNUSED(options)
    QString sql = QString("ALTER TABLE %1 DROP COLUMN %2").arg(sqlTableName(dialect)).arg(dbFieldName);
    return sql;
}

QString BaseBeanMetadata::sqlMakeNotNull(AlephERP::CreationTableSqlOptions options, const QString &dbFieldName, const QString &dialect)
{
    Q_UNUSED(options)
    QString sql = QString("ALTER TABLE %1 ALTER COLUMN %2 SET NOT NULL").arg(sqlTableName(dialect)).arg(dbFieldName);
    return sql;
}

QString BaseBeanMetadata::sqlAlterColumnSetLength(AlephERP::CreationTableSqlOptions options, const QString &dbFieldName, const QString &dialect)
{
    Q_UNUSED(options)
    DBFieldMetadata *f = field(dbFieldName);
    QString sql;
    if ( f->type() == QVariant::String && f->length() > 0 )
    {
        sql = QString("ALTER TABLE %1 ALTER COLUMN %2 TYPE character varying(%3)").
                arg(sqlTableName(dialect)).
                arg(dbFieldName).
                arg(f->length());
    }
    return sql;
}

/**
 * @brief BaseBeanMetadata::sqlAditional
 * Tras la sentencia de creación de la tabla es probable que sea necesaria ejecutar otras adicionales: Por ejemplo,
 * Firebird no soporta secuencias. Y se simulan con triggers. Aquí se hace eso.
 * @param options
 * @param dialect
 * @return
 */
QStringList BaseBeanMetadata::sqlAditional(AlephERP::CreationTableSqlOptions options, const QString &dialect)
{
    Q_UNUSED(options)
    QStringList sqls;
    if ( dialect == QLatin1String("QIBASE") )
    {
#ifdef ALEPHERP_FIREBIRD_SUPPORT
        return AERPFirebirdDatabase::additionalSqlCreateTable(this, options);
#endif
    }
    return sqls;
}

/**
 * @brief BaseBeanMetadataPrivate::importHeritanceElementsOnDom
 * Se podrá realizar una importación de elementos de esta forma
 * <inherits name="templateDelQueImportamos">
 *  <element>envVars</element>
 *  <element>qsDbForm</element>
 *  <element type="field">tipoiva</element>
 *  <!-- Importa todos los fields del elemento padre -->
 *  <element type="allFields"/>
 * </inherits>
 *
 * </inherits>
 * @param document
 */
void BaseBeanMetadataPrivate::importHeritanceElementsOnDom(QDomDocument &document, QDomDocument &otherDocument, QDomNode inheritNode)
{
    QDomElement otherDocumentRoot = otherDocument.documentElement();
    QDomNodeList whatToImportNodes = inheritNode.toElement().elementsByTagName("element");
    for ( int importNodeIdx = 0 ; importNodeIdx < whatToImportNodes.size() ; importNodeIdx ++ )
    {
        QDomNode otherDocumentNodeToImport;
        if ( whatToImportNodes.at(importNodeIdx).toElement().attribute("type") == QLatin1String("field") )
        {
            qDebug() << "BaseBeanMetadataPrivate::importHeritanceElementsOnDom: Importando definicion para campo [" << whatToImportNodes.at(importNodeIdx).toElement().text() << "]";
            QDomNodeList fieldsNodes = otherDocumentRoot.elementsByTagName("field");
            bool found = false;
            for ( int fieldsIdx = 0 ; fieldsIdx < fieldsNodes.size() ; fieldsIdx++ )
            {
                QDomNode fieldName = fieldsNodes.at(fieldsIdx).toElement().firstChildElement("name");
                if (fieldName.toElement().text() == whatToImportNodes.at(importNodeIdx).toElement().text())
                {
                    otherDocumentNodeToImport = fieldsNodes.at(fieldsIdx);
                    importNode(document, inheritNode, otherDocumentNodeToImport);
                    found = true;
                }
            }
            if ( !found )
            {
                qDebug() << "----------------------------------------------------------------------------------------------------------------------------------------------------------";
                qDebug() << "BaseBeanMetadataPrivate::importHeritanceElementsOnDom: ATENCION: No existe ninguna definicion en el elemento padre para el campo [" << whatToImportNodes.at(importNodeIdx).toElement().text() << "]";
                qDebug() << "Campos remotos disponibles:";
                for ( int fieldsIdx = 0 ; fieldsIdx < fieldsNodes.size() ; fieldsIdx++ )
                {
                    QDomNode fieldName = fieldsNodes.at(fieldsIdx).toElement().firstChildElement("name");
                    qDebug() << fieldName.toElement().text();
                }
                qDebug() << "----------------------------------------------------------------------------------------------------------------------------------------------------------";
            }
        }
        else if ( whatToImportNodes.at(importNodeIdx).toElement().attribute("type") == QLatin1String("allFields") )
        {
            QDomNodeList fieldsNodes = otherDocumentRoot.elementsByTagName("field");
            QDomNode placeToInsert = inheritNode;
            for ( int fieldsIdx = 0 ; fieldsIdx < fieldsNodes.size() ; fieldsIdx++ )
            {
                otherDocumentNodeToImport = fieldsNodes.at(fieldsIdx);
                placeToInsert = importNode(document, placeToInsert, otherDocumentNodeToImport);
            }
        }
        else if ( whatToImportNodes.at(importNodeIdx).toElement().attribute("type") == QLatin1String("connections") )
        {
            QDomNodeList connectionsNodes = otherDocumentRoot.elementsByTagName("connections");
            QDomNode placeToInsert = inheritNode;
            for ( int connectionIdx = 0 ; connectionIdx < connectionsNodes.size() ; connectionIdx++ )
            {
                otherDocumentNodeToImport = connectionsNodes.at(connectionIdx);
                placeToInsert = importNode(document, placeToInsert, otherDocumentNodeToImport);
            }
        }
        else
        {
            otherDocumentNodeToImport = otherDocumentRoot.firstChildElement(whatToImportNodes.at(importNodeIdx).toElement().text());
            importNode(document, inheritNode, otherDocumentNodeToImport);
        }
    }
}

QDomNode BaseBeanMetadataPrivate::importNode(QDomDocument &document, QDomNode &inheritNode, QDomNode &nodeToImport)
{
    QDomNode result;
    QDomElement rootElement = document.documentElement();
    if ( !nodeToImport.isNull() )
    {
        QDomNode importedNode = document.importNode(nodeToImport, true);
        if ( !importedNode.isNull() )
        {
            result = rootElement.insertAfter(importedNode, inheritNode);
            if ( result.isNull() )
            {
                qDebug() << "BaseBeanMetadataPrivate::importHeritanceElementsOnDom: No se ha podido mover el nodo.";
            }
        }
        else
        {
            qDebug() << "BaseBeanMetadataPrivate::importHeritanceElementsOnDom: No se ha podido importar el nodo.";
        }
    }
    else
    {
        qDebug() << "BaseBeanMetadataPrivate::importHeritanceElementsOnDom: No se ha encontrado en " << inheritNode.toElement().attribute("name") << " " << nodeToImport.toElement().text();
    }
    return result;
}

/**
 * @brief BaseBeanMetadataPrivate::checkWildCards
 * Buscará ocurrencias del tipo ${algo} siendo algo una propiedad de los metadatos
 * @param element
 */
QString BaseBeanMetadataPrivate::checkWildCards(QDomNode &node)
{
    QDomElement element = node.toElement();
    return checkWildCards(element);
}

QString BaseBeanMetadataPrivate::checkWildCards(QDomElement &element)
{
    QString text = element.text();
    QByteArray ba;
    while ( text.contains(QStringLiteral("${")) )
    {
        QString property;
        int originalIdx = text.indexOf(QStringLiteral("${"));
        int idx = originalIdx + 2;
        while (text.at(idx) != '}')
        {
            property.append(text.at(idx));
            idx++;
        }
        ba = property.toLatin1();
        const char *cProperty = ba.constData();
        QString propertyValue;
        if ( q_ptr->property(cProperty).isValid() )
        {
            propertyValue = q_ptr->property(cProperty).toString();
        }
        else
        {
            propertyValue = m_metadataVars[property];
        }
        text.remove(originalIdx, property.size() + 3);
        text.insert(originalIdx, propertyValue);
    }
    ba.clear();
    return text;
}

void BaseBeanMetadata::createInternalConnections(BaseBean *bean)
{
    foreach (const InternalConnection &conn, d->m_internalConnections)
    {
        QList<QObject *> senders;
        QString tmp = conn.senderQs.trimmed();
        if ( tmp == QLatin1String("bean") )
        {
            senders.append(bean);
        }
        if ( tmp.startsWith(QStringLiteral("bean.")) )
        {
            tmp = tmp.replace("bean.", "");
            DBObject *obj = bean->navigateThroughProperties(tmp);
            if ( obj != NULL )
            {
                senders.append(obj);
            }
        }
        if ( senders.isEmpty() )
        {
            DBObject *obj = bean->navigateThroughProperties(conn.senderQs);
            if ( obj != NULL )
            {
                senders.append(obj);
            }
        }
        if ( senders.isEmpty() )
        {
            senders = d->senders(conn.senderQs, bean);
        }
        foreach (QObject *obj, senders)
        {
            foreach (const ConnectionAction &action, conn.actions)
            {
                if ( !action.signalAction.isEmpty() )
                {
                    foreach (const QString &slotName, action.slotsAction )
                    {
                        QByteArray baSlotName = slotName.toLocal8Bit();
                        QVariant qsFunctionPointer = bean->property(baSlotName.constData());
                        if ( qsFunctionPointer.isValid() )
                        {
                            BaseBeanQsFunction *qsFunction = qsFunctionPointer.value<BaseBeanQsFunction *>();
                            if ( obj->inherits("BaseBean") )
                            {
                                d->createInternalConnectionForBean(qobject_cast<BaseBean *> (obj), action.signalAction, qsFunction);
                            }
                            else if ( obj->inherits("DBRelation") )
                            {
                                d->createInternalConnectionForRelation(qobject_cast<DBRelation *>(obj), action.signalAction, qsFunction);
                            }
                            else if ( obj->inherits("DBField") )
                            {
                                d->createInternalConnectionForField(qobject_cast<DBField *>(obj), action.signalAction, qsFunction);
                            }
                            else if ( obj->inherits("RelatedElement") )
                            {
                                d->createInternalConnectionForRelatedElement(qobject_cast<RelatedElement *>(obj), action.signalAction, qsFunction);
                            }
                            else
                            {
                                qDebug() << "BaseBeanMetadata::createInternalConnections: El tipo " << obj->metaObject()->className() << " no está contemplado";
                            }
                        }
                        else
                        {
                            qDebug() << "BaseBeanMetadata::createInternalConnections: No se ha encontrado ninguna función con nombre: " << slotName;
                        }
                    }
                }
            }
        }
    }
}

/**
 * @brief BaseBeanMetadata::fieldCounter
 * Devuelve fields que son campos calculados, y en los que está involucrado en el cálculo @param fld
 * @param field
 * @return
 */
QList<DBFieldMetadata *> BaseBeanMetadata::counterFields(const QString &field)
{
    QList<DBFieldMetadata *> list;
    foreach (DBFieldMetadata *fld, d->m_fields)
    {
        if ( fld->hasCounterDefinition() && fld->dbFieldName() != field )
        {
            if ( fld->counterDefinition()->fields.contains(field) )
            {
                list << fld;
            }
        }
    }
    return list;
}

QList<QObject *> BaseBeanMetadataPrivate::senders(const QString &senderQs, BaseBean *bean)
{
    QList<QObject *> result;
    if ( senderQs.isEmpty() )
    {
        return result;
    }
    m_engine->setScript(senderQs, QString("%1.senderQs.js").arg(m_tableName));
    m_engine->addToEnviroment("bean", bean);
    QScriptValue r = m_engine->executeScript();
    m_engine->removeFromEnviroment("bean");
    if ( !r.isError() )
    {
        if ( !r.isNull() && !r.isUndefined() )
        {
            if ( !r.isArray() )
            {
                if ( r.isQObject() )
                {
                    QObject *obj = r.toQObject();
                    if ( obj )
                    {
                        result.append(obj);
                    }
                    else
                    {
                        QLogger::QLog_Error(AlephERP::stLogScript, QString("BaseBeanMetadataPrivate::senders: TABLA: ").append(m_tableName));
                        QLogger::QLog_Error(AlephERP::stLogScript, QString("BaseBeanMetadataPrivate::senders: conn.senderQs: [%1] El objeto QObject devuelto es nulo.").arg(senderQs));
                    }
                }
                else
                {
                    QLogger::QLog_Error(AlephERP::stLogScript, QString("BaseBeanMetadataPrivate::senders: TABLA: ").append(m_tableName));
                    QLogger::QLog_Error(AlephERP::stLogScript, QString("BaseBeanMetadataPrivate::senders: conn.senderQs: [%1] El resultado no es un QObject.").arg(senderQs));
                }
            }
            else
            {
                int length = r.property("length").toInteger();
                for ( int index = 0 ; index < length ; ++index )
                {
                    QScriptValue item = r.property(index);
                    if ( !item.isNull() && item.isValid() && !item.isUndefined() )
                    {
                        if ( item.isQObject() )
                        {
                            QObject *obj = item.toQObject();
                            if ( obj )
                            {
                                result.append(obj);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            QLogger::QLog_Error(AlephERP::stLogScript, QString("BaseBeanMetadataPrivate::senders: TABLA: ").append(m_tableName));
            QLogger::QLog_Error(AlephERP::stLogScript, QString("BaseBeanMetadataPrivate::senders: conn.senderQs: [%1] ha devuelto undefined o null").arg(senderQs));
        }
    }
    return result;
}

void BaseBeanMetadataPrivate::createInternalConnectionForBean(BaseBean *sender, const QString &signalName, BaseBeanQsFunction *qsFunction)
{
    if ( sender == NULL )
    {
        return;
    }
    if ( signalName == QLatin1String("modified") )
    {
        QObject::connect(sender, SIGNAL(beanModified(bool)), qsFunction, SLOT(call(bool)));
    }
    else if ( signalName == QLatin1String("fieldModified") )
    {
        QObject::connect(sender, SIGNAL(fieldModified(QString,QVariant)), qsFunction, SLOT(call(QString,QVariant)));
    }
    else if ( signalName == QLatin1String("defaultValueFieldCalculated") )
    {
        QObject::connect(sender, SIGNAL(defaultValueCalculated(QString,QVariant)), qsFunction, SLOT(call(QString,QVariant)));
    }
    else if ( signalName == QLatin1String("dbStateModified") )
    {
        QObject::connect(sender, SIGNAL(dbStateModified(int)), qsFunction, SLOT(call(int)));
    }
    else if ( signalName == QLatin1String("beforeSave") )
    {
        QObject::connect(sender, SIGNAL(beforeSave()), qsFunction, SLOT(call()));
    }
    else if ( signalName == QLatin1String("beforeInsert") )
    {
        QObject::connect(sender, SIGNAL(beforeInsert()), qsFunction, SLOT(call()));
    }
    else if ( signalName == QLatin1String("beforeUpdate") )
    {
        QObject::connect(sender, SIGNAL(beforeUpdate()), qsFunction, SLOT(call()));
    }
    else if ( signalName == QLatin1String("beforeDelete") )
    {
        QObject::connect(sender, SIGNAL(beforeDelete()), qsFunction, SLOT(call()));
    }
    else if ( signalName == QLatin1String("saved") )
    {
        QObject::connect(sender, SIGNAL(beanSaved()), qsFunction, SLOT(call()));
    }
    else if ( signalName == QLatin1String("committed") )
    {
        QObject::connect(sender, SIGNAL(beanCommitted()), qsFunction, SLOT(call()));
    }
    else if ( signalName == QLatin1String("endEdit") )
    {
        QObject::connect(sender, SIGNAL(endEdit()), qsFunction, SLOT(call()));
    }
    else if ( signalName == QLatin1String("relatedElementAdded") )
    {
        QObject::connect(sender, SIGNAL(relatedElementAdded(RelatedElement*)), qsFunction, SLOT(call(RelatedElement*)));
    }
    else if ( signalName == QLatin1String("relatedElementDeleted") )
    {
        QObject::connect(sender, SIGNAL(relatedElementDeleted(RelatedElement*)), qsFunction, SLOT(call(RelatedElement*)));
    }
    else if ( signalName == QLatin1String("relatedElementModified") )
    {
        QObject::connect(sender, SIGNAL(relatedElementModified(RelatedElement*)), qsFunction, SLOT(call(RelatedElement*)));
    }
}

void BaseBeanMetadataPrivate::createInternalConnectionForRelation(DBRelation *sender, const QString &signalName, BaseBeanQsFunction *qsFunction)
{
    if ( sender == NULL )
    {
        return;
    }
    if ( signalName == QLatin1String("childModified") )
    {
        QObject::connect(sender, SIGNAL(childModified(BaseBean*,bool)), qsFunction, SLOT(call(BaseBean*,bool)));
    }
    else if ( signalName == QLatin1String("childInserted") )
    {
        QObject::connect(sender, SIGNAL(childInserted(BaseBean*,int)), qsFunction, SLOT(call(BaseBean*,int)));
    }
    else if ( signalName == QLatin1String("childDeleted") )
    {
        QObject::connect(sender, SIGNAL(childDeleted(BaseBean*,int)), qsFunction, SLOT(call(BaseBean*,int)));
    }
    else if ( signalName == QLatin1String("childSaved") )
    {
        QObject::connect(sender, SIGNAL(childSaved(BaseBean*)), qsFunction, SLOT(call(BaseBean*)));
    }
    else if ( signalName == QLatin1String("childCommitted") )
    {
        QObject::connect(sender, SIGNAL(childComitted(BaseBean*)), qsFunction, SLOT(call(BaseBean*)));
    }
    else if ( signalName == QLatin1String("childEndEdit") )
    {
        QObject::connect(sender, SIGNAL(childEndEdit(BaseBean*)), qsFunction, SLOT(call(BaseBean*)));
    }
    else if ( signalName == QLatin1String("childDbStateModified") )
    {
        QObject::connect(sender, SIGNAL(childDbStateModified(BaseBean*,int)), qsFunction, SLOT(call(BaseBean*,int)));
    }
    else if ( signalName == QLatin1String("fieldChildModified") )
    {
        QObject::connect(sender, SIGNAL(fieldChildModified(BaseBean*,QString,QVariant)), qsFunction, SLOT(call(BaseBean*,QString,QVariant)));
    }
    else if ( signalName == QLatin1String("rootFieldChanged") )
    {
        QObject::connect(sender, SIGNAL(rootFieldChanged()), qsFunction, SLOT(call()));
    }
    else if ( signalName == QLatin1String("fatherLoaded") )
    {
        QObject::connect(sender, SIGNAL(fatherLoaded(BaseBean*)), qsFunction, SLOT(call(BaseBean*)));
    }
    else if ( signalName == QLatin1String("fatherUnloaded") )
    {
        QObject::connect(sender, SIGNAL(fatherUnloaded()), qsFunction, SLOT(call()));
    }
}

void BaseBeanMetadataPrivate::createInternalConnectionForField(DBField *sender, const QString &signalName, BaseBeanQsFunction *qsFunction)
{
    if ( sender == NULL )
    {
        return;
    }
    if ( signalName == QLatin1String("valueModified") )
    {
        QObject::connect(sender, SIGNAL(valueModified(QVariant)), qsFunction, SLOT(call(QVariant)));
    }
    else if ( signalName == QLatin1String("calculatedValueInit") )
    {
        QObject::connect(sender, SIGNAL(calculatedValueInit(DBField*)), qsFunction, SLOT(call(DBField*)));
    }
    else if ( signalName == QLatin1String("calculatedValueEnd") )
    {
        QObject::connect(sender, SIGNAL(calculateValueEnd(DBField*)), qsFunction, SLOT(call(DBField*)));
    }
}

void BaseBeanMetadataPrivate::createInternalConnectionForRelatedElement(RelatedElement *sender, const QString &signalName, BaseBeanQsFunction *qsFunction)
{
    if ( sender == NULL )
    {
        return;
    }

    if ( signalName == QLatin1String("modified") )
    {
        QObject::connect(sender, SIGNAL(hasBeenModified(RelatedElement *)), qsFunction, SLOT(call(RelatedElement *)));
    }
    else if ( signalName == QLatin1String("relatedBeanModified") )
    {
        QObject::connect(sender, SIGNAL(relatedBeanModified(BaseBean *)), qsFunction, SLOT(call(BaseBean*)));
    }
}


bool BaseBeanMetadata::viewProvidesOid() const
{
    return d->m_viewProvidesOid;
}


void BaseBeanMetadata::setViewProvidesOid(bool value)
{
    d->m_viewProvidesOid = value;
}

QString BaseBeanMetadata::calculateScheduleTitleExpression(BaseBean *bean)
{
    // Al ser un campo calculado, ejecutamos el script asociado. Este se ejecuta
    // sobre el bean, de modo, que parent debe estar visible en el script
    QVariant data;
    if ( !bean->metadata()->isScheduleValid() )
    {
        return QString();
    }
    if ( !bean->metadata()->scheduledData().titleExpression.valid )
    {
        const ScheduleData schData = bean->metadata()->scheduledData();
        StringExpression *exp = const_cast<StringExpression *> (& schData.titleExpression);
        data = exp->applyExpressionRules(bean);
    }
    else
    {
        d->m_engine->addFunctionArgument("bean", bean);
        d->m_engine->setScript(bean->metadata()->scheduledData().qsTitleExpression,
                               QString("%1.scheduleTitle.js").arg(bean->metadata()->tableName()));
        data = d->m_engine->toVariant(d->m_engine->callQsFunction(QString("scheduleTitle")));
        return data.toString();
    }
    return QString();
}

QObject *BaseBeanMetadata::navigateThroughProperties(const QString &path, bool returnIntermediate)
{
    QStringList items = path.split(".");
    QObject *obj = this, *backupObj = this;
    for (int i = 0 ; i < items.size() ; i++)
    {
        QByteArray ba = items.at(i).toUtf8();
        QVariant v = obj->property(ba.constData());
        if ( !v.isValid() )
        {
            QLogger::QLog_Debug(AlephERP::stLogOther, QString("BaseBeanMetadata::navigateThroughProperties: La ruta [%1] no conduce a ningún lugar. Item: [%2]").arg(path).arg(items.at(i)));
            return NULL;
        }
        // http://liveblue.wordpress.com/2012/10/29/qobject-multiple-inheritance-and-smart-delegators/
        // Ahí está la razón para estas cosas...
        // TODO: The sad thing is: in Qt4, values can be extracted from QVariant’s only by using the same
        // type used when constructing the QVariant. Qt5 introduces a nifty feature where you can safely
        // extract a QObject * from any QVariant built from a QObjectDerived *.
        obj = static_cast<DBFieldMetadata *>(v.value<DBFieldMetadata *>());
        if ( obj == NULL )
        {
            obj = static_cast<DBRelationMetadata *>(v.value<DBRelationMetadata *>());
            if ( obj == NULL )
            {
                obj = static_cast<BaseBeanMetadata *>(v.value<BaseBeanMetadata *>());
                if ( obj == NULL )
                {
                    if ( returnIntermediate )
                    {
                        return backupObj;
                    }
                    else
                    {
                        QLogger::QLog_Debug(AlephERP::stLogOther, QString("BaseBean::navigateThroughProperties: La ruta [%1] no ha podido ser convertida a un objeto valido..").arg(path));
                        return NULL;
                    }
                }
                else
                {
                    backupObj = obj;
                }
            }
            else
            {
                backupObj = obj;
            }
        }
        else
        {
            backupObj = obj;
        }
    }
    return obj;
}

QStringList BaseBeanMetadata::fieldsNecessaryToCalculate(const QString &dbFieldName)
{
    if ( d->m_fieldsNecessaryToCalculate.contains(dbFieldName) )
    {
        return d->m_fieldsNecessaryToCalculate[dbFieldName];
    }
    return QStringList();
}

void BaseBeanMetadata::registerRecalculateFields()
{
    if ( engine() == NULL || context() == NULL )
    {
        return;
    }
    if ( context()->argumentCount() < 2 )
    {
        return;
    }
    QString fieldToRecalc = context()->argument(0).toString();
    QStringList fieldsOnCalc;
    for (int i = 1 ; i < context()->argumentCount() ; i++)
    {
        QString fieldOnCalc = context()->argument(i).toString();
        if ( !fieldsOnCalc.contains(fieldOnCalc) )
        {
            fieldsOnCalc.append(fieldOnCalc);
        }
    }
    registerRecalculateFields(fieldToRecalc, fieldsOnCalc);
}

void BaseBeanMetadata::registerRecalculateFields(const QString &fieldToRecalculate, const QStringList &fieldsOnCalc)
{
    if ( d->m_fieldsNecessaryToCalculate.contains(fieldToRecalculate) )
    {
        foreach (const QString &fieldOnCalc, fieldsOnCalc)
        {
            if ( !d->m_fieldsNecessaryToCalculate.value(fieldToRecalculate).contains(fieldOnCalc) )
            {
                d->m_fieldsNecessaryToCalculate[fieldToRecalculate].append(fieldOnCalc);
            }
        }
    }
    else
    {
        d->m_fieldsNecessaryToCalculate[fieldToRecalculate] = fieldsOnCalc;
    }
}

/**
 * @brief BaseBeanMetadataPrivate::buildFieldsCalculatedRelations
 * Analiza los campos involucrados en el cálculo de un campo.
 */
void BaseBeanMetadata::buildFieldsCalculatedRelations()
{
    QScopedPointer<BaseBean> b (BeansFactory::instance()->newBaseBean(d->m_tableName, false, false));

    QLogger::QLog_Debug(AlephERP::stLogScript, QString("BaseBeanMetadata::buildFieldsCalculatedRelations: Generando estructura de relaciones de: [%1]").arg(d->m_tableName));

    foreach (DBField *fld, b->fields())
    {
        if ( fld->metadata()->calculated() )
        {
            // Vamos a registrar todos los Fields que se acceden (a su value) cuando se calcula el valor del actual FLD
            BaseBeanMetadata::initRegisterFieldsInvolvedOnCalc();
            fld->recalculate();
            QList<DBField *> fieldsInvolved = BaseBeanMetadata::fieldsInvolvedOnCalc();
            BaseBeanMetadata::endRegisterFieldsInvolvedOnCalc();

            QStringList fieldsInvolvedNames;
            foreach (DBField *fieldInvolved, fieldsInvolved)
            {
                if ( fieldInvolved->metadata()->dbFieldName() != fld->metadata()->dbFieldName() || fieldInvolved->bean()->metadata()->tableName() != d->m_tableName )
                {
                    if ( fieldInvolved->bean() == b.data() )
                    {
                        fieldsInvolvedNames.append(fieldInvolved->metadata()->dbFieldName());
                        QLogger::QLog_Debug(AlephERP::stLogOther, QString("BaseBeanMetadata::buildFieldsCalculatedRelations: Tabla: [%1] Field [%2] involucra a: [%3]").
                                            arg(d->m_tableName).
                                            arg(fld->metadata()->dbFieldName()).
                                            arg(fieldInvolved->metadata()->dbFieldName()));
                    }
                    else
                    {
                        DBObject *dbObject = fieldInvolved;
                        QString path;
                        while ( dbObject != NULL && dbObject->parent() != NULL )
                        {
                            DBRelation *rel = qobject_cast<DBRelation *>(dbObject->parent());
                            if ( rel != NULL )
                            {
                                if ( !path.isEmpty() )
                                {
                                    path.append(".");
                                }
                                path.append(rel->metadata()->tableName());
                                if ( rel->metadata()->type() == DBRelationMetadata::MANY_TO_ONE )
                                {
                                    if ( rel->father() )
                                    {
                                        if ( rel->father().data() == b.data() )
                                        {
                                            path.append("father");
                                            fieldsInvolvedNames.append(path);
                                            QLogger::QLog_Debug(AlephERP::stLogOther, QString("BaseBeanMetadata::buildFieldsCalculatedRelations: Tabla: [%1] Field [%2] involucra a: [%3]").
                                                                arg(d->m_tableName).
                                                                arg(fld->metadata()->dbFieldName()).
                                                                arg(path));
                                        }
                                        else
                                        {
                                            dbObject = rel->father();
                                        }
                                    }
                                }
                                else
                                {
                                    fieldsInvolvedNames.append(path);
                                    QLogger::QLog_Debug(AlephERP::stLogOther, QString("BaseBeanMetadata::buildFieldsCalculatedRelations: Tabla: [%1] Field [%2] involucra a: [%3]").
                                                        arg(d->m_tableName).
                                                        arg(fld->metadata()->dbFieldName()).
                                                        arg(path));
                                }
                            }
                            dbObject = qobject_cast<DBObject *>(dbObject->parent());
                        }
                    }
                }
            }
            registerRecalculateFields(fld->metadata()->dbFieldName(), fieldsInvolvedNames);
        }
    }
}

void BaseBeanMetadata::initRegisterFieldsInvolvedOnCalc()
{
    BaseBeanMetadataPrivate::m_registerFieldsInvolvedOnCalc = true;
    BaseBeanMetadataPrivate::m_fieldsInvolvedOnCalc.clear();
}

void BaseBeanMetadata::endRegisterFieldsInvolvedOnCalc()
{
    BaseBeanMetadataPrivate::m_registerFieldsInvolvedOnCalc = false;
}

void BaseBeanMetadata::fieldIsAccesed(DBField *fld)
{
    if ( BaseBeanMetadataPrivate::m_registerFieldsInvolvedOnCalc && !BaseBeanMetadataPrivate::m_fieldsInvolvedOnCalc.contains(fld) )
    {
        BaseBeanMetadataPrivate::m_fieldsInvolvedOnCalc.append(fld);
    }
}

QList<DBField *> BaseBeanMetadata::fieldsInvolvedOnCalc()
{
    return BaseBeanMetadataPrivate::m_fieldsInvolvedOnCalc;
}

