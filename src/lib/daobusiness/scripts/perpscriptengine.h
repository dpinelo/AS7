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
#ifndef AERPSCRIPTENGINE_H
#define AERPSCRIPTENGINE_H

#include <QtScript>
#include <QtGlobal>
#include <alepherpglobal.h>

class AERPScriptCommon;
class AERPScriptMessageBox;
class AERPScriptEnginePrivate;
class QScriptEngine;
class QScriptEngineDebugger;
class QMainWindow;

/**
  Envoltorio del motor de Javascript para la creación de código a medida.
  AlephERP se basa en gran medida en la existencia de código Javascript. Éste se asocia a:
  <ul>
  <li>Campos calculados en las definiciones de tablas de sistema</li>
  <li>Formularios de interacción con el usuario, ya sean de navegación, edición de datos o búsqueda</li>
  <li>Widgets reutilizables dentro de cada formulario</li>
  </ul>
  Tanto formularios como widgets, al crearse, serán instanciados con un objeto Javascript asociado,
  creado automáticamente por este Engine, y que se asocia a los primeros para darles funcionalidad y control.
  Los campos calculados utilizarán un motor común para su ejecución de cara a mejorar rendimiento.
  Todo objeto Javascript asociado a formulario o widget tiene a su disposición una serie de objetos
  ya instanciados y otro que el desarrollador puede crear. Éstos son
  <ul>
  <li><i>debug</i> Función que presenta por salida estándar un mensaje. debug("Mensaje por salida estándar");</li>
  <li><i>loadExtension</i> Permite cargar componentes externos al motor de Javascript para dotarlo de más funcionalidad.
  Por ejemplo, permite la carga de la interfaces Javascript para las Qt: loadExtension("qt.core");</li>
  <li><i>loadJS</i> Permite cargar código javascript externo, de manera que pueda reutilizarse código. Este código
  javascript externo se encuentra en un fichero en disco duro: loadJS("/home/user/micodigo.js");</li>
  <li><i>copyBeanFields</i>Permite copiar los valores de un bean de un tipo a otro bean de otra tabla, siempre que los nombres
  de los fields sean coincidentes. Ej:</li>
  copyBeanFields(beanOriginal, beanDestino, "field1", "field2");
  <li><i>Config</i> Objeto ya instanciado en el motor que permite el acceso al objeto que contiene la configuración de AlephERP
	\a Configuracion. Aquí hay definidos una serie de variables disponibles para el desarrolador:
	Config.tempPath() devuelve el directorio temporal configurado</li>
  <li><i>AERPSqlQuery</i> Clase que puede ser instanciada en código Javascript para ejecutar una consulta en base de datos. Ej:<li>
	 var query = new AERPSqlQuery;
	 var sql = "select id_papel, formatocmsancho, formatocmslargo from papeles where id_papel>1";
	 query.prepare(sql);
	 if ( query.exec() ) {
		 while ( query.next() ) {
			 var formato = query.value(1) + "x" + query.value(2);
		 }
	 }
  <li><i>DBDialog</i> Clase para instanciar un nuevo formulario: por ejemplo de búsqueda, o de edición de un registro. Ej:</li>
  var dlg = new DBDialog;
  dlg.type = "search";
  dlg.tableName = "clientes";
  dlg.fieldToSearch = "codcliente";
  dlg.show();
  if ( dlg.userClickOk && oldCodCliente != dlg.fieldSearched ) {
	bean.setFieldValue("codcliente", dlg.fieldSearched);
	bean.setFieldValue("iddircliente", -1);
  }

  Este código abre un formulario de búsqueda (utilizando clientes.dbsearch.ui) sobre la tabla "clientes". Indica que el resultado
  que devolverá este objeto, será el field "codcliente" del registro seleccionado por el usuario (fieldToSearch). El resultado
  está almacenado en la propiedad .fieldSearched.
  Es un envoltorio de la clase AERPScriptDialog.

  <li><i>DBField</i> Clase para instanciar que representa una columna de un registro de base de datos. ej: var field = bean.field(0)
  field es un objeto de tipo DBField </li>

  <li><i>DBRelation</i> Clase para instanciar que representa una relación de un bean. ej: var relation = bean.relation("direcciones")
  relation es un objeto de tipo DBRelarion </li>

  <li><i>BaseBean</i> Clase para instanciar que representa un registro de base de datos. </li>

  <li><i>perpWidget</i> Para widgets sólo: Es un objeto ya instanciado que hace referencia al propio widget (y por tanto a sus QWidgets
  internos)</li>
  <li><i>bean</i> Objeto ya instanciado que hace referencia al registro de base de datos que el formulario está editando. Sólo presente
  en formularios de edición y widgets. Ejemplo: var value = bean.fieldValue("codcliente") devuelve el valor de la columna
  codcliente del formulario de edición</li>
  <li><i>thisForm</i> Objeto ya instanciado que hace referencia al formulario de edición.</li>

  @author David Pinelo <alepherp@alephsistemas.es>
  */
class ALEPHERP_DLL_EXPORT AERPScriptEngine
{

    friend class Database;

private:
    static void destroyEngineSingleton();

    /** Lo ponemos privado para crearlo como singleton */
    explicit AERPScriptEngine();
    ~AERPScriptEngine();
    Q_DISABLE_COPY(AERPScriptEngine)

public:

    static QScriptEngine *instance();
    static AERPScriptCommon *scriptCommon();
    static void registerScriptsTypes(QScriptEngine *instance);
    static void importExtensions(QScriptEngine *instance);

    static bool checkForErrorOnQS(const QString &script, QString &error, int &line);

    static void registerConsoleFunc(QScriptEngine *engine, void (*function)(const QString &));

#ifdef ALEPHERP_DEVTOOLS
    static QScriptEngineDebugger *debugger();
    static QPointer<QMainWindow> debugWindow();
#endif
};

#endif // AERPSCRIPTENGINE_H
