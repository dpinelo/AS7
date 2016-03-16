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
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include "perpbasedialog.h"
#include "perpbasedialog_p.h"
#include "configuracion.h"
#include "globales.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/relatedelement.h"
#include "forms/registereddialogs.h"
#include "widgets/dbbasewidget.h"
#include "widgets/waitwidget.h"
#include "widgets/dbnumberedit.h"
#include "widgets/dblineedit.h"
#include "widgets/dbcombobox.h"
#include "widgets/dbtextedit.h"
#include "widgets/dbdatetimeedit.h"
#include "widgets/dblistview.h"
#include "widgets/dbfileupload.h"
#include "widgets/dbchooserecordbutton.h"
#include "widgets/dbdetailview.h"
#ifdef ALEPHERP_ADVANCED_EDIT
#include "widgets/dbhtmleditor.h"
#include "widgets/dbcodeedit.h"
#ifdef ALEPHERP_QSCISCINTILLA
#include "Qsci/qsciscintilla.h"
#else
#include "qeditor.h"
#endif
#endif
#include "widgets/fademessage.h"
#include "business/aerploggeduser.h"

#define TRANSITION_TIME_MS 250

AERPBaseDialog::AERPBaseDialog(QWidget* parent, Qt::WindowFlags fl) : QDialog(parent, fl), d(new AERPBaseDialogPrivate(this))
{
    RegisteredDialogs::registerDialog(this);
    QMainWindow *main = (QMainWindow *) qApp->property(AlephERP::stMainWindowPointer).value<void *>();
    if ( main != NULL )
    {
        setWindowIcon(main->windowIcon());
        setWindowTitle(main->windowTitle());
    }
    this->installEventFilter(this);
}

AERPBaseDialog::~AERPBaseDialog()
{
    RegisteredDialogs::unRegisterDialog(this);
    delete d;
}

AERPScriptQsObject *AERPBaseDialog::aerpQsEngine()
{
    return d->m_engine;
}

QHash<QString, QObject *> AERPBaseDialog::thisFormObjectProperties()
{
    return d->m_objects;
}

QHash<QString, QVariant> AERPBaseDialog::thisFormValueProperties()
{
    return d->m_data;
}

QString AERPBaseDialog::tableName()
{
    return d->m_tableName;
}

bool AERPBaseDialog::setTableName(const QString &value)
{
    d->m_tableName = value;
    return true;
}

bool AERPBaseDialog::openSuccess()
{
    return d->m_openSuccess;
}

void AERPBaseDialog::setOpenSuccess(bool value)
{
    d->m_openSuccess = value;
}

void AERPBaseDialog::closeEvent(QCloseEvent *event)
{
    // Guardamos las dimensiones del usuario
    alephERPSettings->savePosForm(this);
    alephERPSettings->saveDimensionForm(this);
    event->accept();
}

void AERPBaseDialog::showEvent(QShowEvent *event)
{
    // Cargamos las dimensiones guardadas de la ventana
    alephERPSettings->applyPosForm(this);
    alephERPSettings->applyDimensionForm(this);
    event->accept();
}

/*!
  Slot que redimensiona a los valores guardados por la última acción del usuario
  */
void AERPBaseDialog::resizeToSavedDimension()
{
    alephERPSettings->applyPosForm(this);
    alephERPSettings->applyDimensionForm(this);
}

void AERPBaseDialog::hideEvent(QHideEvent *event)
{
    // Guardamos las dimensiones del usuario
    alephERPSettings->savePosForm(this);
    alephERPSettings->saveDimensionForm(this);
    event->accept();
}

void AERPBaseDialog::showWaitAnimation(bool value, const QString message)
{
    if ( value && d->m_waitWidget == 0 )
    {
        d->m_waitWidget = new WaitWidget(message, this);
        d->m_waitWidget->move(rect().center() - (d->m_waitWidget->rect().center() * 2));
        d->m_waitWidget->setParent(this);
        d->m_waitWidget->show();
        setEnabled(false);
    }
    else if ( !value && d->m_waitWidget != 0 )
    {
        setEnabled(true);
        d->m_waitWidget->closeAnimation();
        d->m_waitWidget = 0;
    }
}

/*!
  Para formularios (de búsqueda o edición de datos) crea controles automáticamente para la edición.
  Esta función se utilizará para todas aquellas tablas a editar que no posean ningún archivo .ui
  asociado. Caso de ser un formulario de búsqueda, se indicará mediante searchDlg.
  Devuelve los widgets creados organizados por filas.
  */
QHash<int, QWidgetList> AERPBaseDialog::setupWidgetFromBaseBeanMetadata(BaseBeanMetadata *metadata,
                                                                        QGridLayout *layoutDestiny,
                                                                        bool searchDlg)
{
    QHash<int, QWidgetList> rowWidgets;
    if ( layoutDestiny == NULL )
    {
        return rowWidgets;
    }
    if ( searchDlg )
    {
        rowWidgets = d->setupDBSearchDlg(metadata);
    }
    else
    {
        rowWidgets = d->setupDBRecordDlg(metadata);
    }
    QHashIterator<int, QWidgetList> it(rowWidgets);
    int maxItemsPerRow = 0;
    while (it.hasNext())
    {
        it.next();
        maxItemsPerRow = it.value().size() > maxItemsPerRow ? it.value().size() : maxItemsPerRow;
    }
    it = QHashIterator<int, QWidgetList> (rowWidgets);
    while (it.hasNext())
    {
        it.next();
        QWidgetList rowWid = it.value();
        if ( rowWid.size() > 0 )
        {
            for (int col = 0 ; col < rowWid.size() ; col++)
            {
                layoutDestiny->addWidget(rowWid.at(col), it.key(), col);
            }
        }
    }
    return rowWidgets;
}

QHash<int, QWidgetList> AERPBaseDialogPrivate::setupDBRecordDlg(BaseBeanMetadata *metadata)
{
    QHash<int, QWidgetList> rowWidgets;
    int row = 0;

    foreach (DBFieldMetadata *fld, metadata->fields())
    {
        DBRelationMetadata *rel = NULL;
        foreach (DBRelationMetadata *r, fld->relations(AlephERP::ManyToOne))
        {
            if ( rel == NULL )
            {
                rel = r;
            }
        }
        if ( fld->visibleGrid() || rel != NULL || (fld->includeOnGeneratedRecordDlg().isValid() && fld->includeOnGeneratedRecordDlg().toBool()) )
        {
            QWidgetList widList;

            if ( rel != NULL )
            {
                BaseBeanMetadata *relTable = BeansFactory::metadataBean(rel->tableName());
                if ( !fld->serial() && relTable != NULL )
                {
                    DBChooseRecordButton *cr = new DBChooseRecordButton(q_ptr);
                    cr->setText(relTable->alias());
                    widList.append(cr);
                }
            }
            else if ( !fld->optionsList().isEmpty() )
            {
                DBComboBox *cb = new DBComboBox(q_ptr);
                cb->setFieldName(fld->dbFieldName());
                widList.append(qobject_cast<DBComboBox *>(cb));
            }
            else
            {
                if ( !fld->serial() && (fld->type() == QVariant::Int || fld->type() == QVariant::Double) )
                {
                    DBNumberEdit *ne = new DBNumberEdit(q_ptr);
                    ne->setDecimalPlaces(fld->partD());
                    widList.append(qobject_cast<QWidget *>(ne));
                }
                else if ( fld->type() == QVariant::String && !fld->memo() )
                {
                    DBLineEdit *le = new DBLineEdit(q_ptr);
                    widList.append(qobject_cast<QWidget *>(le));
                }
                else if ( fld->type() == QVariant::String && fld->memo() )
                {
                    DBTextEdit *te = new DBTextEdit(q_ptr);
                    widList.append(qobject_cast<QWidget *>(te));
                }
                else if ( fld->type() == QVariant::Date || fld->type() == QVariant::DateTime )
                {
                    DBDateTimeEdit *de = new DBDateTimeEdit(q_ptr);
                    de->setCalendarPopup(true);
                    widList.append(qobject_cast<QWidget *>(de));
                }
                else if ( fld->type() == QVariant::Pixmap )
                {
                    DBFileUpload *fu = new DBFileUpload(q_ptr);
                    widList.append(qobject_cast<QWidget *>(fu));
                }
            }
            if ( widList.size() > 0 )
            {
                DBBaseWidget *baseWid = dynamic_cast<DBBaseWidget *>(widList.at(0));
                baseWid->setFieldName(fld->dbFieldName());
                if ( !fld->readOnly() )
                {
                    baseWid->setDataEditable(true);
                }
                widList.at(0)->setObjectName(QString("db_%1").arg(fld->dbFieldName()));
                QLabel *lbl = new QLabel(q_ptr);
                lbl->setText(fld->fieldName());
                lbl->setBuddy(widList.at(0));
                widList.prepend(lbl);
                rowWidgets[row] = widList;
                row++;
            }
        }
    }
    return rowWidgets;
}

/*!
  Construye un formulario de búsqueda automático
  */
QHash<int, QWidgetList> AERPBaseDialogPrivate::setupDBSearchDlg(BaseBeanMetadata *metadata)
{
    QHash<int, QWidgetList> rowWidgets;
    int row = 0;

    foreach (DBFieldMetadata *fld, metadata->fields())
    {
        if ( fld->includeOnGeneratedSearchDlg().isValid() && !fld->includeOnGeneratedSearchDlg().toBool() )
        {
            continue;
        }
        if ( fld->includeOnGeneratedSearchDlg().toBool() || fld->visibleGrid() )
        {
            QWidgetList widList;
            QList<DBRelationMetadata *> relations = fld->relations(AlephERP::ManyToOne);
            DBRelationMetadata *fatherRelation = NULL;
            foreach ( DBRelationMetadata *tmp, relations )
            {
                fatherRelation = tmp;
            }
            if ( fatherRelation != NULL )
            {
                DBListView	*lv = new DBListView(q_ptr);
                lv->setItemCheckBox(true);
                lv->setKeyField(fld->dbFieldName());
                lv->setFieldName(fld->dbFieldName());
                BaseBeanMetadata *fatherMetadata = BeansFactory::metadataBean(fatherRelation->tableName());
                if ( fatherMetadata != NULL )
                {
                    lv->setVisibleField(fatherMetadata->defaultVisualizationField());
                }
                widList << qobject_cast<DBListView *>(lv);
            }
            else
            {
                if ( !fld->optionsList().isEmpty() )
                {
                    DBComboBox *cb = new DBComboBox(q_ptr);
                    cb->setFieldName(fld->dbFieldName());
                    cb->setObjectName(QString("db_%1").arg(fld->dbFieldName()));
                    widList.append(qobject_cast<DBComboBox *>(cb));
                }
                else
                {
                    if ( fld->type() == QVariant::Int || fld->type() == QVariant::Double )
                    {
                        widList << createComboOperators(fld->dbFieldName());
                        DBNumberEdit *ne = new DBNumberEdit(q_ptr);
                        ne->setDecimalPlaces(fld->partD());
                        ne->setObjectName(QString("db_%1_1").arg(fld->dbFieldName()));
                        ne->setProperty(AlephERP::stFieldName, fld->dbFieldName());
                        ne->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
                        widList << qobject_cast<QWidget *>(ne);
                        ne = new DBNumberEdit(q_ptr);
                        ne->setDecimalPlaces(fld->partD());
                        ne->setObjectName(QString("db_%1_2").arg(fld->dbFieldName()));
                        ne->setVisible(false);
                        ne->setProperty(AlephERP::stFieldName, fld->dbFieldName());
                        ne->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
                        widList << qobject_cast<QWidget *>(ne);
                    }
                    else if ( fld->type() == QVariant::String )
                    {
                        DBLineEdit *le = new DBLineEdit(q_ptr);
                        le->setObjectName(QString("db_%1").arg(fld->dbFieldName()));
                        le->setProperty(AlephERP::stFieldName, fld->dbFieldName());
                        le->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
                        widList << qobject_cast<QWidget *>(le);
                    }
                    else if ( fld->type() == QVariant::Date || fld->type() == QVariant::DateTime )
                    {
                        widList << createComboOperators(fld->dbFieldName());
                        DBDateTimeEdit *de = new DBDateTimeEdit(q_ptr);
                        de->setCalendarPopup(true);
                        de->setObjectName(QString("db_%1_1").arg(fld->dbFieldName()));
                        if ( fld->type() == QVariant::Date )
                        {
                            de->setDisplayFormat(CommonsFunctions::dateFormat());
                            de->setDate(alephERPSettings->minimumDate());
                        }
                        else
                        {
                            de->setDisplayFormat(CommonsFunctions::dateTimeFormat());
                            de->setDateTime(alephERPSettings->minimumDateTime());
                        }
                        de->setProperty(AlephERP::stFieldName, fld->dbFieldName());
                        de->setSpecialValueText(QObject::trUtf8("Seleccione fecha"));
                        de->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
                        widList << qobject_cast<QWidget *>(de);
                        de = new DBDateTimeEdit(q_ptr);
                        de->setCalendarPopup(true);
                        de->setObjectName(QString("db_%1_2").arg(fld->dbFieldName()));
                        if ( fld->type() == QVariant::Date )
                        {
                            de->setDisplayFormat(CommonsFunctions::dateFormat());
                            de->setDate(alephERPSettings->minimumDate());
                        }
                        else
                        {
                            de->setDisplayFormat(CommonsFunctions::dateTimeFormat());
                            de->setDateTime(alephERPSettings->minimumDateTime());
                        }
                        de->setVisible(false);
                        de->setProperty(AlephERP::stFieldName, fld->dbFieldName());
                        de->setSpecialValueText(QObject::trUtf8("Seleccione fecha"));
                        de->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
                        widList << qobject_cast<QWidget *>(de);
                    }
                }
            }
            if ( widList.size() > 0 )
            {
                QLabel *lbl = new QLabel(q_ptr);
                lbl->setText(fld->fieldName());
                lbl->setBuddy(widList.at(0));
                widList.prepend(lbl);
                rowWidgets[row] = widList;
                row++;
            }
        }
    }
    return rowWidgets;
}

QComboBox * AERPBaseDialogPrivate::createComboOperators(const QString &fld)
{
    QComboBox *cb = new QComboBox(q_ptr);
    QStringList list;
    list << QObject::trUtf8("Menor que") << QObject::trUtf8("Igual") <<
         QObject::trUtf8("Mayor que") << QObject::trUtf8("Entre los valores");
    cb->addItems(list);
    cb->setObjectName(QString("cb_%1").arg(fld));
    cb->setProperty(AlephERP::stFieldName, fld);
    QObject::connect(cb, SIGNAL(currentIndexChanged(int)), q_ptr, SLOT(searchComboChanged(int)));
    return cb;
}

/*!
  Cuando el combobox cambia, puede ser necesario que algunos controles aparezcan o desaparezcan
  */
void AERPBaseDialog::searchComboChanged(int index)
{
    QString fieldName = QObject::sender()->property(AlephERP::stFieldName).toString();
    QString controlName = QString("db_%1_2").arg(fieldName);
    QWidget *secondWidget = findChild<QWidget *>(controlName);
    if ( secondWidget != NULL )
    {
        if ( index != BETWEEN_ELECTION_INDEX )
        {
            secondWidget->setVisible(false);
        }
        else
        {
            secondWidget->setVisible(true);
        }
    }
}

void AERPBaseDialog::executeJsMethod()
{
    QString method = QInputDialog::getText(this, qApp->applicationName(), trUtf8("Indique el nombre del método JavaScript de este formulario a ejecutar"));
    if ( !method.isEmpty() )
    {
        callQSMethod(method);
    }
}

#ifdef ALEPHERP_DEVTOOLS
void AERPBaseDialog::showDebugger()
{
    if ( d->m_engine.isNull() )
    {
        return;
    }

    d->m_engine->showDebugger();
}
#endif

/*!
  Permite llamar a un método de la clase que controla al formulario. Muy interesante
  para obtener valores determinados del formulario.
  */
QScriptValue AERPBaseDialog::callQSMethod(const QString &method)
{
    QScriptValue result;
    if ( d->m_engine )
    {
        d->m_engine->callQsObjectFunction(result, method);
    }
    return result;
}

QScriptValue AERPBaseDialog::callQSMethod(const QString &method, const QScriptValueList &args)
{
    QScriptValue result;
    if ( d->m_engine )
    {
        d->m_engine->callQsObjectFunction(result, method, args);
    }
    return result;
}

/**
 * @brief AERPBaseDialog::callQSMethod
 * Permite invocar a una función con una lista de parámetros. Si args contiene "elemento1", "elemento2", "elemento3"... se invocará
 * a la función JS de la forma = function(elemento1, elemento2, elemento3)
 * @param method
 * @param args
 * @return
 */
QScriptValue AERPBaseDialog::callQSMethod(const QString &method, const QObjectList &args)
{
    QScriptValueList argList;
    if ( d->m_engine.isNull() )
    {
        return QScriptValue();
    }
    foreach (QObject *object, args)
    {
        QScriptValue val = d->m_engine->createScriptValue(object);
        argList.append(val);
    }
    QScriptValue result;
    d->m_engine->callQsObjectFunction(result, method, argList);
    return result;
}

QScriptValue AERPBaseDialog::callQSMethod(const QString &method, const QVariant &args)
{
    if ( d->m_engine.isNull() )
    {
        return QScriptValue();
    }

    QScriptValueList argList;    
    QScriptValue val = d->m_engine->createScriptValue(args);
    argList.append(val);
    QScriptValue result;
    d->m_engine->callQsObjectFunction(result, method, argList);
    return result;
}

QScriptValue AERPBaseDialog::callQSMethod(const QString &method, QObject *obj)
{
    if ( d->m_engine.isNull() )
    {
        return QScriptValue();
    }

    QScriptValueList argList;
    QScriptValue val = d->m_engine->createScriptValue(obj);
    argList.append(val);
    QScriptValue result;
    d->m_engine->callQsObjectFunction(result, method, argList);
    return result;
}

/**
 * @brief AERPBaseDialog::callQSMethod
 * Invoca a una función pasando un listado de beans, en un array
 * @param method
 * @param beans
 * @return
 */
QScriptValue AERPBaseDialog::callQSMethod(const QString &method, const BaseBeanSharedPointerList &beans)
{
    if ( d->m_engine.isNull() )
    {
        return QScriptValue();
    }
    QScriptValueList argList;
    QScriptValue array = d->m_engine->newArray();
    int i = 0;
    foreach (BaseBeanSharedPointer bean, beans)
    {
        array.setProperty(i, d->m_engine->createScriptValue(bean.data()));
        i++;
    }
    argList.append(array);
    QScriptValue result;
    d->m_engine->callQsObjectFunction(result, method, argList);
    return result;
}

QScriptValue AERPBaseDialog::callQSMethod(const QString &method, const BaseBeanPointerList &beans)
{
    if ( d->m_engine.isNull() )
    {
        return QScriptValue();
    }

    QScriptValueList argList;
    QScriptValue array = d->m_engine->newArray();
    int i = 0;
    foreach (BaseBeanPointer bean, beans)
    {
        array.setProperty(i, d->m_engine->createScriptValue(bean.data()));
        i++;
    }
    argList.append(array);
    QScriptValue result;
    d->m_engine->callQsObjectFunction(result, method, argList);
    return result;
}

QScriptValue AERPBaseDialog::callQSMethod(const QString &method, const RelatedElementPointerList &relElements)
{
    if ( d->m_engine.isNull() )
    {
        return QScriptValue();
    }

    QScriptValueList argList;
    QScriptValue array = d->m_engine->newArray();
    int i = 0;
    foreach (RelatedElementPointer relElement, relElements)
    {
        array.setProperty(i, d->m_engine->createScriptValue(relElement.data()));
        i++;
    }
    argList.append(array);
    QScriptValue result;
    d->m_engine->callQsObjectFunction(result, method, argList);
    return result;
}

bool AERPBaseDialog::hasQSMethod(const QString &methodName)
{
    if ( d->m_engine.isNull() )
    {
        return false;
    }

    return d->m_engine->existQsFunction(methodName);
}

QScriptValue AERPBaseDialog::qsThisObject()
{
    if ( d->m_engine.isNull() )
    {
        return QScriptValue();
    }
    return d->m_engine->qsThisObject();
}

void AERPBaseDialog::installEventFilters()
{
    QList<QWidget *> children = findChildren<QWidget *>();
    foreach ( QWidget *child, children )
    {
        if ( child->property(AlephERP::stAerpControl).toBool() )
        {
            child->installEventFilter(this);
        }
    }
}

/**
  Hace accesible a todos los objetos de tipo AERPControl en el motor de javascript directamente
  desde thisForm, de la forma: thisForm.db_nombre donde db_nombre es el objectName dado en el Qt Designer
  */
void AERPBaseDialog::exposeAERPControlToQsEngine(QDialog *dlg, AERPScriptQsObject *engine)
{
    QList<QWidget *> list = dlg->findChildren<QWidget *>();
    foreach ( QWidget *widget, list )
    {
        if ( widget->property(AlephERP::stAerpControl).toBool() )
        {
            engine->addPropertyToThisForm(widget->objectName(), widget);
            dlg->setProperty(widget->objectName().toUtf8(), QVariant::fromValue((void *) widget));
        }
        else if ( QString(widget->metaObject()->className()) == "DBChooseRelatedRecordButton" )
        {
            engine->addPropertyToThisForm(widget->objectName(), widget);
            dlg->setProperty(widget->objectName().toUtf8(), QVariant::fromValue((void *) widget));
        }
    }
}

QScriptValue AERPBaseDialog::parentDialog()
{
    QScriptValue obj(QScriptValue::NullValue);

    QDialog *dlg = CommonsFunctions::parentDialog(this);
    if ( engine() != NULL )
    {
        obj = engine()->newQObject(dlg, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
    }
    return obj;
}

QScriptValue AERPBaseDialog::thisForm()
{
    return aerpQsEngine()->qsThisForm();
}

QWidget *AERPBaseDialog::contentWidget() const
{
    return NULL;
}

/*!
  Vamos a obtener y guardar cuándo el usuario ha modificado un control
  */
bool AERPBaseDialog::eventFilter (QObject *target, QEvent *event)
{
    QTextEdit *tmp1 = qobject_cast<QTextEdit *>(target);
#ifdef ALEPHERP_ADVANCED_EDIT
    DBHtmlEditor *tmp2 = qobject_cast<DBHtmlEditor *>(target);
    DBCodeEdit *tmp3 = qobject_cast<DBCodeEdit *>(target);
#ifndef ALEPHERP_QSCISCINTILLA
    QEditor *tmp7 = qobject_cast<QEditor *> (target);
#else
    QsciScintilla *tmp7 = qobject_cast<QsciScintilla *> (target);;
#endif
#else
    QObject *tmp2 = NULL;
    QObject *tmp3 = NULL;
    QObject *tmp7 = NULL;
#endif
    QAbstractItemView *tmp4 = qobject_cast<QAbstractItemView *>(target);
    QPlainTextEdit *tmp5 = qobject_cast<QPlainTextEdit *>(target);
    DBChooseRecordButton *tmp6 = qobject_cast<DBChooseRecordButton *> (target);
    if ( event->type() == QEvent::KeyPress )
    {
        QKeyEvent *ev = static_cast<QKeyEvent *>(event);
    if ( ev->key() == Qt::Key_Escape /*&& !ev->isAccepted()*/ )
        {
            if ( tmp4 == NULL )
            {
                close();
                ev->accept();
                return true;
            }
        }
        // Almacenamos siempre la última pulsación de una tecla...
        d->m_lastKeyPressTimeStamp = QDateTime::currentDateTime();
        if ( tmp1 == NULL && tmp2 == NULL && tmp3 == NULL && tmp4 == NULL && tmp5 == NULL && tmp6 == NULL && tmp7 == NULL )
        {
            if ( ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return )
            {
                focusNextChild();
                if ( qApp->focusWidget() != NULL )
                {
                    qDebug() << qApp->focusWidget()->metaObject()->className();
                    // Los datetimeedit tienen varios controles anidados... hay que hacer este hack.
                    DBDateTimeEdit *de = qobject_cast<DBDateTimeEdit *>(target);
                    if ( de != NULL && QString(qApp->focusWidget()->metaObject()->className()) == "QToolButton" )
                    {
                        focusNextChild();
                    }
                }
                ev->accept();
                return true;
            }
        }
    }
    else if ( event->type() == QEvent::FocusOut )
    {
        if ( target->property(AlephERP::stAerpControl).toBool() && !target->objectName().isEmpty() )
        {
            QString scriptName = QString("%1LostFocus").arg(target->objectName());
            if (hasQSMethod(scriptName))
            {
                callQSMethod(scriptName);
            }
        }
        event->ignore();
    }
    return QDialog::eventFilter(target, event);
}

/**
  Agrega el objeto \a obj, como una propiedad de nombre \a name del objeto disponible para el formulario
  thisForm
*/
void AERPBaseDialog::addPropertyToThisForm(const QString &name, QObject *obj)
{
    if ( d->m_engine.isNull() )
    {
        return;
    }

    d->m_objects[name] = obj;
    d->m_engine->replaceThisFormProperty(name, obj);
}

/**
  Agrega el dato \a data, como una propiedad de nombre \a name del objeto disponible para el formulario
  thisForm
*/
void AERPBaseDialog::addPropertyToThisForm(const QString &name, QVariant data)
{
    if ( d->m_engine.isNull() )
    {
        return;
    }

    d->m_data[name] = data;
    d->m_engine->replaceThisFormProperty(name, data);
}

void AERPBaseDialog::setFocusOnFirstWidget()
{
    if ( contentWidget() != NULL )
    {
        // Establecer el foco en el widget hace que este en sí tenga el foco, pero como es creado
        // por un QUILoader no va a ningún lado
        setFocusProxy(contentWidget());
        if ( contentWidget()->focusProxy() )
        {
            contentWidget()->focusProxy()->setFocus();
        }
        else
        {
            setTabOrder(0, contentWidget());
        }
    }
}

/**
  Permite desde código QS leer propiedades del objeto thisForm establecidas en otro formulario.
  Muy útil para el intercambio de datos entre diálogos
  */
QScriptValue AERPBaseDialog::readPropertyFromThisForm(const QString &name)
{
    if ( d->m_engine.isNull() )
    {
        return QScriptValue();
    }

    return d->m_engine->qsThisFormProperty(name);
}

QDateTime AERPBaseDialog::lastKeyPressTimeStamp() const
{
    return d->m_lastKeyPressTimeStamp;
}

/**
 * @brief DBRecordDlg::showFadeMessage
 * Permite mostrar un mensaje en una ventana translúcida
 * @param message
 */
void AERPBaseDialog::showFadeMessage(const QString &message, int msecsToHide)
{
    AERPFadeMessage *msgDlg = new AERPFadeMessage(message, this);
    msgDlg->move(this->rect().center());
    msgDlg->show();
    if ( msecsToHide != -1 )
    {
        QTimer::singleShot(msecsToHide, msgDlg, SLOT(closeAnimation()));
    }
}

void AERPBaseDialog::hideFadeMessage()
{
    AERPFadeMessage *msgDlg = findChild<AERPFadeMessage *>();
    if ( msgDlg != NULL )
    {
        msgDlg->closeAnimation();
    }
}
