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
#ifndef DBRECORDDLG_H
#define DBRECORDDLG_H

#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <alepherpglobal.h>
#include <aerpcommon.h>
#include "forms/perpbasedialog.h"
#include "dao/beans/basebean.h"

class BaseBean;
class BaseBeanObserver;
class DBRecordDlgPrivate;
class FilterBaseBeanModel;

namespace Ui
{
class DBRecordDlg;
}

/**
  Clase base que controlará la edición de registros. Para componer un formulario
  de edición de registros son necesarios tres entradas en la tabla alepherp_system:
  <ul>
  <li>nombre_de_la_tabla archivo de tipo table</li>
  <li>nombre_de_la_tabla.dbrecord.qs archivo con código JavaScript que lo controlará</li>
  <li>nombre_de_la_tabla.dbrecord.ui archivo con la interfaz de usuario.</li>
  </ul>
  A esta interfaz de usuario esta clase le agregará los botones para salir, guardar registros...
  El código Javascript deberá ser una clase con el nombre DBRecordDlg. Podrá tener
  algunos métodos que serán llamados por DBRecordDlg en determinados eventos como:
  -beanSaved Se llama justo después de guardar el registro
  -beforeNavigate Se llama justo antes de navegar a un nuevo registro por los botones de navegación.
    Si devuelve false, no se hace nada.
  -afterNavigate Se llama justo después de navegar a un nuevo registro
  */
class ALEPHERP_DLL_EXPORT DBRecordDlg : public AERPBaseDialog
{
    Q_OBJECT
    Q_FLAGS (DBRecordButtons)

    Q_PROPERTY(bool windowModified READ isWindowModified WRITE setWindowModified)
    /** Esta propiedad permite determinar la funcionalidad al pulsar el botón de cerrar en la esquina superior
      derecha de la ventana:
      -Si está a true, el formulario preguntará si se guardan los datos, que será el comportamiento por defecto.
      -El desarrollador puede poner esta propiedad a false, y al pinchar el botón cerrar se cierra el formulario
       sin preguntar si hay datos guardados */
    Q_PROPERTY(bool closeButtonAskForSave READ closeButtonAskForSave WRITE setCloseButtonAskForSave)
    Q_PROPERTY(DBRecordButtons visibleButtons READ visibleButtons WRITE setVisibleButtons)
    /** Qué código Qs utilizar */
    Q_PROPERTY(QString qsCode READ qsCode WRITE setQsCode)
    /** Qué ui importar */
    Q_PROPERTY(QString uiCode READ uiCode WRITE setUiCode)
    Q_PROPERTY(QString qmlCode READ qmlCode WRITE setQmlCode)
    Q_PROPERTY(bool userSaveData READ userSaveData)
    Q_PROPERTY(QString helpUrl READ helpUrl WRITE setHelpUrl)
    /** Establece un modo de trabajo especial (utilizado para rellenar rápidamente formulario, especialmente cuando se trabaja
     * con pistolas de códigos de barra, por ejemplo). Cuando todos los campos del registro están validados (el bean está validado)
     * guarda automáticamente el registro y crea uno nuevo */
    Q_PROPERTY(bool saveAndNewWithAllFieldsValidated READ saveAndNewWithAllFieldsValidated WRITE setSaveAndNewWithAllFieldsValidated)
    /** Indica en qué modo se ha abierto el formulario */
    Q_PROPERTY(AlephERP::FormOpenType openType READ openType WRITE setOpenType)
    /** Con esta propiedad a true, se pone el formulario en modo de edición diferente. Si se navega a un registro, el formulario
     * se recarga y muestra el formulario adecuado para ese registro */
    Q_PROPERTY(bool advancedNavigation READ advancedNavigation WRITE setAdvancedNavigation)
    /** Si esta propiedad se pone a true, fuerza la persistencia a base de datos. UTILIZAR CON CUIDADO. */
    Q_PROPERTY(bool forceSaveToDb READ forceSaveToDb WRITE setForceSaveToDb)
    /** Indica si este registro presenta los botones de navegación */
    Q_PROPERTY(bool canNavigate READ canNavigate WRITE setCanNavigate)

public:

    enum DBRecordButtonsFlag
    {
        First = 0x001,
        Previous = 0x002,
        Next = 0x004,
        Last = 0x008,
        Save = 0x010,
        SaveAndClose = 0x020,
        SaveAndNew = 0x040,
        Print = 0x080,
        History = 0x100,
        Email = 0x200,
        Script = 0x400,
        RelatedItems = 0x800,
        Documents = 0x1000,
        Help = 0x2000
    };
    Q_DECLARE_FLAGS (DBRecordButtons, DBRecordButtonsFlag)

protected:
    Ui::DBRecordDlg *ui;
    DBRecordDlgPrivate *d;

    void closeEvent (QCloseEvent * event);
    bool eventFilter (QObject *target, QEvent *event);
    void keyPressEvent (QKeyEvent * e);

    virtual void setupMainWidget();
    virtual void execQs();
    virtual bool lock();

public:
    explicit DBRecordDlg(QWidget *parent = 0, Qt::WindowFlags fl = 0);
    DBRecordDlg(BaseBeanPointer bean,
                AlephERP::FormOpenType openType,
                bool initTransaction,
                QWidget* parent = 0,
                Qt::WindowFlags fl = 0);
    ~DBRecordDlg();

    bool isWindowModified();
    virtual bool closeButtonAskForSave();
    virtual void setCloseButtonAskForSave(bool value);
    QString qsCode() const;
    QString uiCode() const;
    QString qmlCode() const;
    bool userSaveData() const;
    void setCanChangeModality(bool value);
    QString helpUrl() const;
    void setHelpUrl(const QString &value);
    QString helpFile() const;
    void setHelpFile(const QString &value);
    bool saveAndNewWithAllFieldsValidated() const;
    void setSaveAndNewWithAllFieldsValidated(bool value);
    AlephERP::FormOpenType openType() const;
    void setOpenType(AlephERP::FormOpenType &type);
    bool advancedNavigation() const;
    void setAdvancedNavigation(bool value);
    virtual DBRecordButtons visibleButtons() const;
    virtual void setVisibleButtons(DBRecordButtons buttons);
    bool forceSaveToDb() const;
    void setForceSaveToDb(bool value);
    bool canNavigate() const;
    void setCanNavigate(bool value);

    QWidget *contentWidget() const;

    Q_INVOKABLE virtual BaseBean *bean();
    Q_INVOKABLE virtual QString parentType();

    virtual void navigateBean(BaseBeanPointer bean, AlephERP::FormOpenType openType);

    static QScriptValue toScriptValue(QScriptEngine *aerpQsEngine, DBRecordDlg * const &in);
    static void fromScriptValue(const QScriptValue &object, DBRecordDlg * &out);

protected slots:
    virtual void lockBreaked(const QString &notificacion);
    virtual void navigate(const QString &direction);
    virtual void possibleRecordToSave(const QString &contextName, BaseBean *bean);
    virtual void advancedNavigationListRowChanged(int row);

public slots:
    virtual void accept();
    virtual void reject();
    virtual bool init();
    virtual bool save();
    virtual bool validate();
    virtual bool beforeSave();
    void first()
    {
        navigate("first");
    }
    void next()
    {
        navigate("next");
    }
    void previous()
    {
        navigate("previous");
    }
    void last()
    {
        navigate("last");
    }
    void setWindowModified(bool value);
    void setWindowModified(BaseBean *bean, bool value);
    virtual void cancel();
    virtual void showHistory();
    virtual void saveAndNew();
    virtual void saveAndClose();
    virtual void saveRecord();
    virtual void setReadOnly(bool value = true);
    virtual void printRecord();
    virtual void emailRecord();
    virtual void showOrHideRelatedElements();
    virtual void setQsCode(const QString &objectName);
    virtual void setUiCode(const QString &objectName);
    virtual void setQmlCode(const QString &objectName);
    virtual void setUserAccess();
    virtual void hideDBButtons();
    virtual void showDBButtons();
    virtual void showOrHideHelp();
    virtual void restoreContext();
#ifdef ALEPHERP_DOC_MANAGEMENT
    virtual void showOrHideDocuments();
#endif
#ifdef ALEPHERP_DEVTOOLS
    virtual void inspectBean();
#endif

signals:
    void accepted(BaseBeanPointer bean, bool userSaveData);
    void rejected(BaseBeanPointer bean, bool userSaveData);
};

Q_DECLARE_METATYPE(DBRecordDlg*)
Q_DECLARE_OPERATORS_FOR_FLAGS(DBRecordDlg::DBRecordButtons)
Q_DECLARE_METATYPE(DBRecordDlg::DBRecordButtons)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBRecordDlg, DBRecordDlg*)


#endif // DBRECORDDLG_H
