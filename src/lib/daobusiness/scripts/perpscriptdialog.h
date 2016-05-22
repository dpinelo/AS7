/***************************************************************************
 *   Copyright (C) 2010 by David Pinelo   *
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
#ifndef AERPSCRIPTDIALOG_H
#define AERPSCRIPTDIALOG_H

#include <QtCore>
#include <QtScript>
#include "dao/beans/basebean.h"

class AERPScriptDialog;
class AERPScriptDialogPrivate;

typedef QSharedPointer<AERPScriptDialog> AERPScriptDialogPointer;

/**
  Esta clase permitirá crear o invocar formularios directamente desde código Qs.
  Esto permite programar la interacción con ventanas. Se identifican tres tipos de formularios
  <ul>
  <li>DBRecordDlg: Formulario para la edición de un registro de base de datos</li>
  <li>DBSearchDlg: Formulario que permite realizar una búsqueda en base de datos</li>
  <li>ScriptDlg: Formulario que no está ligado a base de datos, y ejecuta código arbitrario en Qs</li>
  La forma de crear un formulario es la siguiente:

  var dlg = new DBDialog;
  dlg.type = "search";
  dlg.tableName = "clientes";
  dlg.fieldToSearch = "codcliente";
  dlg.show();
  if ( dlg.userClickOk && oldCodCliente != dlg.fieldSearched ) {
	bean.setFieldValue("codcliente", dlg.fieldSearched);
	bean.setFieldValue("iddircliente", -1);
  }
*/

class AERPScriptDialog : public QObject, public QScriptable
{
    Q_OBJECT
    /** Indica el tipo de formulario que se abrirá. Los posibles valores son, un tipo string con los valores
     * - "search": Formulario de búsqueda
     * - "record": Abre un registro en su formulario de edición o visualización
     * - "script": Abre un formulario de tipo script
     * - "report": Abre un formulario para la ejecución de un informe
    */
    Q_PROPERTY(QString type READ type WRITE setType)
    /** Si se abre un formulario de búsqueda, o ejecución de un informe, o un tipo "script" debemos indicar la tabla
     * o el metadato que se abrirá */
    Q_PROPERTY(QString tableName READ tableName WRITE setTableName)
    /** Cuando se realiza una búsqueda puede ser interesante no obtener todo un registro, sino el valor de un field
     * de ese registro. Aquí se proporciona el valor de ese field. El nombre del field se especifica en
     * \a fieldToSearch */
    Q_PROPERTY(QScriptValue fieldSearched READ fieldSearched)
    /** Estableciendo esta propiedad a un field determinado de un metadato (tableName) nos permite obtener el valor
     * buscado directamente en un formulario de búsqueda. Si el usuario no ha pinchado nada, contendrá "undefined". Null
     * podrá ser un valor de la búsqueda. */
    Q_PROPERTY(QString fieldToSearch READ fieldToSearch WRITE setFieldToSearch)
    /** Al abrir el formulario de búsqueda es posible determinar un filtro fuerte a los datos que se presentan (Filtro SQL).
     * Por tanto, esta será una claúsula sql de filtrado */
    Q_PROPERTY(QString searchFilter READ searchFilter WRITE setSearchFilter)
    /** Es posible determinar el código Qs del formulario que se abre */
    Q_PROPERTY(QString qsName READ qsName WRITE setQsName)
    /** También es posible determinar la interfaz UI del formulario que se abre */
    Q_PROPERTY(QString uiName READ uiName WRITE setUiName)
    /** Indica si el usuario pinchó en Ok en el formulario y por tanto, realizó una acción */
    Q_PROPERTY(bool userClickOk READ userClickOk)
    /** Si se abre un formulario de edición de registros, es posible pasarle el bean a editar */
    Q_PROPERTY(BaseBean* beanToEdit READ beanToEdit WRITE setBeanToEdit)
    /** En el caso de ejecutar un informe, aquí se indica el nombre */
    Q_PROPERTY(QString reportName READ reportName WRITE setReportName)
    /** Indica si el formulario que se abre deberá estar siempre sobre cualquier otra ventana */
    Q_PROPERTY(bool staysOnTop READ staysOnTop WRITE setStaysOnTop)
    /** Indica si el formulario es modal (aplicable sólo en algunos casos) */
    Q_PROPERTY(bool modal READ modal WRITE setModal)
    /** Contexto de la transacción (Si vacío, crea una nueva) */
    Q_PROPERTY(QString contextName READ contextName WRITE setContextName)

private:
    AERPScriptDialogPrivate *d;

    void newSearchDlg();
    void newRecordDlg();
    void newScriptDlg();
    void newReportDlg();

public:
    explicit AERPScriptDialog(QObject *parent = 0);
    ~AERPScriptDialog();

    void setType(const QString &type);
    QString type();
    void setTableName(const QString &type);
    QString tableName();

    void setQsName(const QString &name);
    QString qsName();
    void setUiName(const QString &name);
    QString uiName();
    void setReportName(const QString &name);
    QString reportName() const;
    void setContextName(const QString &name);
    QString contextName() const;

    QString fieldToSearch();
    void setFieldToSearch(const QString &value);
    QScriptValue fieldSearched();
    QString searchFilter();
    void setSearchFilter(const QString &value);
    bool userClickOk();
    bool staysOnTop() const;
    void setStaysOnTop(bool value);
    bool modal() const;
    void setModal(bool value);

    BaseBeanPointer beanToEdit();
    void setBeanToEdit(BaseBeanPointer bean);

    void setParentWidget(QWidget *parent);

    Q_INVOKABLE void addPkValue(const QString &field, const QVariant &data);
    Q_INVOKABLE QScriptValue selectedBean();
    Q_INVOKABLE void addPropertyObjectToThisForm(const QString &name, QObject *obj);
    Q_INVOKABLE void addPropertyValueToThisForm(const QString &name, QVariant data);
    Q_INVOKABLE QScriptValue readPropertyFromThisForm(const QString &name);
    Q_INVOKABLE void addFieldValue(const QString &dbFieldName, const QVariant &value);

    static QScriptValue toScriptValue(QScriptEngine *engine, AERPScriptDialog * const &in);
    static void fromScriptValue(const QScriptValue &object, AERPScriptDialog * &out);
    static QScriptValue toScriptValueSharedPointer(QScriptEngine *engine, const QSharedPointer<AERPScriptDialog> &in);
    static void fromScriptValueSharedPointer(const QScriptValue &object, QSharedPointer<AERPScriptDialog> &out);
    static QScriptValue specialAERPScriptDialogConstructor(QScriptContext *context, QScriptEngine *engine);

signals:

public slots:
    void show();
    void openScriptDialog(const QString &ui, const QString &qs = "");
};

Q_DECLARE_METATYPE(QSharedPointer<AERPScriptDialog>)
Q_DECLARE_METATYPE(AERPScriptDialog*)
Q_SCRIPT_DECLARE_QMETAOBJECT(AERPScriptDialog, QObject*)

#endif // AERPSCRIPTDIALOG_H
