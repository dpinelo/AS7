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
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include "configuracion.h"
#include "qlogger.h"
#include "dblineedit.h"
#include "globales.h"
#include "dao/database.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/dbfieldobserver.h"
#include "forms/perpbasedialog.h"
#include "forms/dbrecorddlg.h"
#include "models/dbbasebeanmodel.h"
#include "models/filterbasebeanmodel.h"
#include "models/aerpcompleterhighlightdelegate.h"
#include "models/perpquerymodel.h"
#include "widgets/dbtableview.h"

class DBLineEditPrivate
{
public:
    DBLineEdit *q_ptr;
    bool m_replacePointWithCharacter;
    QChar m_replacePointCharacter;

    /** Permitirá autocompletado de este control. Se supone para ello que este control está enlazado a un DBField, es decir,
      una columna de una tabla de base de datos, y esa columna está relacionada (mediante un DBRelation) con otra tabla.
      Si esta propiedad está a true, cuando el usuario empieza a escribir en ella, se despliega un DBTableView que
      va mostrando el contenido de esa tabla, y ayuda al usuario a introducir el registro.
      Si autoComplete está a true, el control NO podrá ser readOnly (aunque dataEditable esté a false). Eso sí, si dataEditable
      está a false, no se podrá modificar el contenido del registro, sólo escogerlo.
      Si este control no estuviera enlazado a un DBField con relación, es decir, que fuera un DBField libre, la tabla de donde
      buscaría el autocompletado se define en autoCompleteTableName */
    AlephERP::AutoCompleteTypes m_autoComplete;
    QSize m_autoCompletePopupSize;
    QString m_autoCompleteTableName;
    /** Si se especifica una tabla de autocompletado com autoCompleteTableName, será necesario especificar también una columna
     en la que se realizará la búsqueda */
    QString m_autoCompleteColumn;
    /** Se pueden especificar columnas específicas de visualización. Para ello se indican aquí, separadas por ; */
    QString m_autoCompleteVisibleFields;
    /** En el caso en el que se especifique una table, es posible indicar un filtro */
    QString m_autoCompleteTableNameFilter;
    /** El objeto de autocompletado */
    QPointer<QCompleter> m_completer;
    /** Modelo del autocompletado */
    QPointer<QAbstractItemModel> m_completerModel;
    /** Si el modelo es un filtro, este es el origen */
    QPointer<DBBaseBeanModel> m_completerBeanModel;
    /** Widget que presenta el autocompletado. Al ser asignado a un QCompleter, éste se encargará de su borrado. */
    QAbstractItemView * m_completerItemView;
    /** Permitirá destacar la búsqueda actual */
    QPointer<AERPCompleterHighlightDelegate> m_completerDelegate;
    /** Script que se ejecutará cuando el usuario haya escogido un item del completer */
    QString m_scriptAfterChooseFromCompleter;
    /** Si se ha escogido un item del completer, almacena el bean que es... */
    BaseBeanSharedPointer m_completerBaseBean;
    /** Muesta el botón de recálculo de contadores */
    bool m_showRecalculateCounterButton;
    QPointer<QToolButton> m_recalculateCounterButton;
    /** Necesario para el recálculo de los contadores asociados */
    bool m_userModifiedValue;
    QColor m_backgroundColor;
    bool m_focusNextOnEnd;
    bool m_automaticWidthBasedOnFieldMaxLength;
    AERPReturnOnFocus m_return;

    DBLineEditPrivate(DBLineEdit *qq) : q_ptr(qq)
    {
        m_replacePointWithCharacter = false;
        m_replacePointCharacter = '0';
        m_autoComplete = AlephERP::NoCompletition;
        m_completerItemView = NULL;
        m_showRecalculateCounterButton = false;
        m_userModifiedValue = false;
        m_focusNextOnEnd = false;
        m_automaticWidthBasedOnFieldMaxLength = false;
    }

    void initCompleter();
    void initCompleterFromRelation();
    void initCompleterFromFieldValues();
    void initCompleterFromTable();
    int widthForMaxLength();
    void processTextEntry();
    QString setValueFromCompletition(DBField *fld);
};

DBLineEdit::DBLineEdit(QWidget *parent) :
    QLineEdit(parent), DBBaseWidget(), d(new DBLineEditPrivate(this))
{
    // Efecto incómodo de line edit con máscara y vacío, se pincha y se queda el cursor a la mitad.
    d->m_return.installOn(this);
    connect(this, SIGNAL(textChanged(QString)), this, SLOT(emitValueEdited()));
    installEventFilter(this);
}

DBLineEdit::~DBLineEdit()
{
    if ( d->m_completer )
    {
        delete d->m_completer;
    }
    if ( d->m_completerModel )
    {
        delete d->m_completerModel;
    }
    if ( d->m_completerBeanModel )
    {
        delete d->m_completerBeanModel;
    }
    if ( d->m_completerDelegate )
    {
        delete d->m_completerDelegate;
    }
    emit destroyed(this);
    delete d;
}

bool DBLineEdit::replacePointWithCharacter() const
{
    return d->m_replacePointWithCharacter;
}

void DBLineEdit::setReplacePointWithCharacter(bool value)
{
    d->m_replacePointWithCharacter = value;
    if ( !d->m_completerDelegate.isNull() )
    {
        if ( d->m_replacePointWithCharacter )
        {
            d->m_completerDelegate->setReplaceWildCardCharacter(d->m_replacePointCharacter);
        }
        else
        {
            d->m_completerDelegate->setReplaceWildCardCharacter(QChar());
        }
    }
}

QChar DBLineEdit::replacePointCharacter() const
{
    return d->m_replacePointCharacter;
}

void DBLineEdit::setReplacePointCharacter(const QChar &character)
{
    d->m_replacePointCharacter = character;
    if ( !d->m_completerDelegate.isNull() )
    {
        if ( d->m_replacePointWithCharacter )
        {
            d->m_completerDelegate->setReplaceWildCardCharacter(d->m_replacePointCharacter);
        }
        else
        {
            d->m_completerDelegate->setReplaceWildCardCharacter(QChar());
        }
    }
}

void DBLineEdit::setRelationFilter(const QString &name)
{
    if ( name != relationFilter() && !d->m_completer.isNull() )
    {
        delete d->m_completer;
    }
    DBBaseWidget::setRelationFilter(name);
}

AlephERP::AutoCompleteTypes DBLineEdit::autoComplete() const
{
    return d->m_autoComplete;
}

void DBLineEdit::setAutoComplete(AlephERP::AutoCompleteTypes value)
{
    d->m_autoComplete = value;
}

QString DBLineEdit::autoCompleteTableName()
{
    return d->m_autoCompleteTableName;
}

void DBLineEdit::setAutoCompleteTableName(const QString &tableName)
{
    d->m_autoCompleteTableName = tableName;
}

QString DBLineEdit::autoCompleteColumn() const
{
    return d->m_autoCompleteColumn;
}

void DBLineEdit::setAutoCompleteColumn(const QString &column)
{
    d->m_autoCompleteColumn = column;
}

QSize DBLineEdit::autoCompletePopupSize() const
{
    return d->m_autoCompletePopupSize;
}

void DBLineEdit::setAutoCompletePopupSize(const QSize &size)
{
    d->m_autoCompletePopupSize = size;
}

QString DBLineEdit::autoCompleteTableNameFilter() const
{
    return d->m_autoCompleteTableNameFilter;
}

void DBLineEdit::setAutoCompleteTableNameFilter(const QString &value)
{
    d->m_autoCompleteTableNameFilter = value;
}

QString DBLineEdit::autoCompleteVisibleFields() const
{
    return d->m_autoCompleteVisibleFields;
}

void DBLineEdit::setAutoCompleteVisibleFields(const QString &value)
{
    d->m_autoCompleteVisibleFields = value;
}

QString DBLineEdit::scriptAfterChooseFromCompleter() const
{
    return d->m_scriptAfterChooseFromCompleter;
}

void DBLineEdit::setScriptAfterChooseFromCompleter(const QString &value)
{
    d->m_scriptAfterChooseFromCompleter = value;
}

BaseBeanSharedPointer DBLineEdit::completerSelectedBean() const
{
    return d->m_completerBaseBean;
}

void DBLineEdit::setCompleterSelectedBean(const BaseBeanSharedPointer &bean)
{
    d->m_completerBaseBean = bean;
}

bool DBLineEdit::showRecalculateCounterButton() const
{
    return d->m_showRecalculateCounterButton;
}

void DBLineEdit::setShowRecalculateCounterButton(bool value)
{
    d->m_showRecalculateCounterButton = value;
}

QColor DBLineEdit::backgroundColor() const
{
    return d->m_backgroundColor;
}

void DBLineEdit::setBackgroundColor(const QColor &color)
{
    d->m_backgroundColor = color;
    QPalette pal(palette());
    pal.setColor(QPalette::Window, d->m_backgroundColor);
    setPalette(pal);
}

bool DBLineEdit::focusNextOnEnd() const
{
    return d->m_focusNextOnEnd;
}

void DBLineEdit::setFocusNextOnEnd(bool value)
{
    d->m_focusNextOnEnd = value;
}

bool DBLineEdit::automaticWidthBasedOnFieldMaxLength() const
{
    return d->m_automaticWidthBasedOnFieldMaxLength;
}

void DBLineEdit::setAutomaticWidthBasedOnFieldMaxLength(bool value)
{
    d->m_automaticWidthBasedOnFieldMaxLength = value;
}

void DBLineEdit::executeScriptAfterChooseFromCompleter()
{
    QWidget *obj = dynamic_cast<QWidget *>(this);
    AERPBaseDialog *thisForm = CommonsFunctions::aerpParentDialog(obj);
    if ( d->m_scriptAfterChooseFromCompleter.isEmpty() )
    {
        QString scriptName = QString("%1AfterChooseFromCompleter").arg(obj->objectName());
        thisForm->callQSMethod(scriptName);
    }
    else
    {
        thisForm->callMethod(d->m_scriptAfterChooseFromCompleter);
    }
}

void DBLineEdit::setValue(const QVariant &value)
{
    QString newText = value.toString();
    if ( text() != newText )
    {
        if ( observer() != NULL )
        {
            DBField *fld = qobject_cast<DBField *>(observer()->entity());
            if ( fld != NULL )
            {
                // Aquí se controla el caso de almacenar un ID (idtercero) pero presentar un valor del bean padre, para
                // ser utilizado por un completer, como es referencia o codtercero.
                if ( d->m_autoComplete.testFlag(AlephERP::ValuesFromRelation) && !d->m_autoCompleteColumn.isEmpty() && !newText.isEmpty() )
                {
                    newText = d->setValueFromCompletition(fld);
                }
                else
                {
                    if ( fld->metadata()->type() != QVariant::String && newText == "0" )
                    {
                        newText = "";
                    }
                }
                // Si el valor del line edit se cambia desde fuera (por ser un valor calculado, por
                if ( !hasFocus() && fld->metadata()->hasCounterDefinition() )
                {
                    QPropertyAnimation animation(this, "backgroundColor");
                    animation.setKeyValueAt(0, palette().color(QPalette::Window));
                    animation.setKeyValueAt(1, QColor(255,0,0));
                    animation.setDuration(500);
                    animation.setLoopCount(1);
                    animation.setEasingCurve(QEasingCurve::OutInExpo);
                    animation.start();
                }
            }
        }
        AbstractObserver *obs = qobject_cast<AbstractObserver *>(sender());
        if ( obs != NULL )
        {
            blockSignals(true);
        }
        QLineEdit::setText(newText);
        if ( obs != NULL )
        {
            blockSignals(false);
        }
    }
}

void DBLineEdit::emitValueEdited()
{
    if ( d->m_autoComplete.testFlag(AlephERP::ValuesFromRelation) )
    {
        return;
    }
    QVariant v (QLineEdit::text());
    emit valueEdited(v);
    d->m_userModifiedValue = true;
}

/**
 * @brief DBLineEdit::itemActivatedOnCompleterModel
 * Este evento se produce cuando el usuario escoge uno de los valores de la lista de autocompletado que se despliega.
 * @param idx
 */
void DBLineEdit::itemActivatedOnCompleterModel(const QModelIndex &idx)
{
    if ( idx.isValid() )
    {
        if ( (d->m_autoComplete.testFlag(AlephERP::ValuesFromRelation) || d->m_autoComplete.testFlag(AlephERP::ValuesFromTableWithNoRelation)) && !d->m_completerModel.isNull() )
        {
            FilterBaseBeanModel *mdl = qobject_cast<FilterBaseBeanModel *>(d->m_completerModel);
            QAbstractProxyModel *proxyModel = qobject_cast<QAbstractProxyModel *>(d->m_completer->completionModel());
            if ( mdl != NULL && proxyModel != NULL)
            {
                QModelIndex sourceIdx = proxyModel->mapToSource(idx);
                d->m_completerBaseBean = mdl->bean(sourceIdx);
                if ( d->m_autoComplete.testFlag(AlephERP::ValuesFromRelation) )
                {
                    BaseBean *formBean = beanFromContainer();
                    QAbstractProxyModel *completionModel = qobject_cast<QAbstractProxyModel *>(d->m_completer->completionModel());
                    if ( mdl != NULL && formBean != NULL && completionModel != NULL )
                    {
                        QModelIndex sourceIdx = completionModel->mapToSource(idx);
                        BaseBeanSharedPointer beanSelectedOnCompleter = mdl->bean(sourceIdx);
                        if ( !beanSelectedOnCompleter.isNull() && observer() != NULL )
                        {
                            setCompleterSelectedBean(beanSelectedOnCompleter);
                            DBField *fld = qobject_cast<DBField *> (observer()->entity());
                            if ( fld != NULL )
                            {
                                if ( d->m_autoComplete.testFlag(AlephERP::UpdateOwnerFieldBean) )
                                {
                                    if ( fld != NULL && fld->bean() != NULL )
                                    {
                                        QStringList parts = m_fieldName.split(".");
                                        if ( parts.size() == 2 )
                                        {
                                            DBRelation *rel = formBean->relation(parts.at(0));
                                            if ( rel != NULL )
                                            {
                                                DBField *destFld = formBean->field(rel->metadata()->rootFieldName());
                                                if ( destFld != NULL )
                                                {
                                                    bool blockSignalsState = destFld->blockSignals(true);
                                                    destFld->setValue(beanSelectedOnCompleter->fieldValue(rel->metadata()->childFieldName()));
                                                    destFld->blockSignals(blockSignalsState);
                                                }
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    QList<DBRelation *> rels = fld->relations(AlephERP::ManyToOne);
                                    if ( rels.size() > 0 )
                                    {
                                        QVariant selectedValue = beanSelectedOnCompleter->fieldValue(rels.at(0)->metadata()->childFieldName());
                                        emit valueEdited(selectedValue);
                                    }
                                    else
                                    {
                                        QLogger::QLog_Debug(AlephERP::stLogOther,
                                                            QString("DBLineEdit::itemActivatedOnCompleterModel: El field [%1] ] no tiene relaciones definidas. No se puede autocompletar").arg(fld->metadata()->dbFieldName()));
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        executeScriptAfterChooseFromCompleter();
    }
}

void DBLineEdit::showRecalculateButton()
{
    if ( observer() == NULL )
    {
        return;
    }
    DBField *fld = qobject_cast<DBField *>(observer()->entity());
    if ( fld == NULL || !fld->metadata()->hasCounterDefinition() )
    {
        return;
    }
    DBRecordDlg *dlg = qobject_cast<DBRecordDlg *>(CommonsFunctions::aerpParentDialog(this));
    if ( fld->bean()->readOnly() || ( dlg != NULL && dlg->openType() == AlephERP::ReadOnly) )
    {
        return;
    }
    if ( d->m_recalculateCounterButton.isNull() )
    {
        int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, this);
        QPixmap pixmap(":/generales/images/recalculatecounter.png");
        d->m_recalculateCounterButton = new QToolButton(this);
        d->m_recalculateCounterButton->setIcon(QIcon(pixmap));
        d->m_recalculateCounterButton->setIconSize(QSize(sizeHint().height() - (4 * frameWidth),
                sizeHint().height() - (4 * frameWidth)));
        d->m_recalculateCounterButton->setCursor(Qt::ArrowCursor);
        d->m_recalculateCounterButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
        d->m_recalculateCounterButton->setToolTip(trUtf8("Recalcula el valor del campo"));
        // Add extra padding to the right of the line edit. It looks better.
        setStyleSheet(QString("QLineEdit { padding-right: %1px; } ").arg(d->m_recalculateCounterButton->sizeHint().width() + frameWidth + 1));
        QSize msz = minimumSizeHint();
        setMinimumSize(qMax(msz.width(), d->m_recalculateCounterButton->sizeHint().height() + (frameWidth * 4) + 2),
                       qMax(msz.height(), d->m_recalculateCounterButton->sizeHint().height() + (frameWidth * 4) + 2));
        connect(d->m_recalculateCounterButton.data(), SIGNAL(clicked()), this, SLOT(recalculateCounterField()));
    }
    d->m_recalculateCounterButton->setVisible(true);
}

void DBLineEdit::hideRecalculateButton()
{
    if ( d->m_recalculateCounterButton )
    {
        d->m_recalculateCounterButton->setVisible(false);
    }
}

void DBLineEdit::recalculateCounterField()
{
    if ( observer() == NULL )
    {
        return;
    }
    DBField *fld = qobject_cast<DBField *>(observer()->entity());
    if ( fld == NULL )
    {
        return;
    }
    DBRecordDlg *dlg = qobject_cast<DBRecordDlg *>(CommonsFunctions::aerpParentDialog(this));
    if ( fld->bean()->readOnly() || ( dlg != NULL && dlg->openType() == AlephERP::ReadOnly) )
    {
        return;
    }
    QVariant v = fld->calculateCounter();
    fld->setValue(v);
}

void DBLineEdit::checkBarCode(const QString &txt)
{
    // Para ver si quien introduce los datos es un lector de código de barras, vamos a medir el tiempo entre pulsaciones
    if ( barCodeReaderAllowed() )
    {
        QLineEdit *obj = dynamic_cast<QLineEdit *>(this);
        AERPBaseDialog *thisForm = CommonsFunctions::aerpParentDialog(obj);
        // Al leer un código de barras podemos invocar esta función.
        if ( scriptAfterBarCodeRead().isEmpty() )
        {
            QString scriptName = QString("%1AfterCodeBarRead").arg(obj->objectName());
            thisForm->callQSMethod(scriptName, obj->text());
        }
        else
        {
            thisForm->callQSMethod(scriptAfterBarCodeRead(), obj->text());
        }

        qDebug() << Q_FUNC_INFO << "código de barras detectado. se va a emitir señal " << txt;
        emit barCodeRead(txt);
        if ( onBarCodeReadNextFocus() )
        {
            focusNextChild();
        }
    }
}

/*!
  Ajusta los parámetros de visualización de este Widget en función de lo definido en DBField m_field
  */
void DBLineEdit::applyFieldProperties()
{
    DBField *fld = NULL;
    DBFieldObserver *obs = qobject_cast<DBFieldObserver *>(observer());
    if ( obs != NULL )
    {
        fld = qobject_cast<DBField *>(obs->entity());
    }
    if ( fld != NULL && d->m_autoComplete != AlephERP::NoCompletition && d->m_autoComplete != AlephERP::ValuesFromThisField )
    {
        if ( d->m_autoComplete.testFlag(AlephERP::ValuesFromRelation) )
        {
            QList<DBRelation *> rels = fld->relations(AlephERP::ManyToOne);
            if ( rels.size() > 0 )
            {
                BaseBean *father = rels.first()->father();
                if ( father != NULL )
                {
                    fld = father->field(d->m_autoCompleteColumn);
                }
            }
            else
            {
                rels = fld->relations(AlephERP::OneToOne);
                if ( rels.size() > 0 )
                {
                    BaseBean *brother = rels.first()->brother();
                    if ( brother != NULL )
                    {
                        fld = brother->field(d->m_autoCompleteColumn);
                    }
                }
            }
        }
    }
    if ( fld != NULL )
    {
        if ( fld->metadata()->type() == QVariant::String && fld->length() > 0 )
        {
            setMaxLength(fld->length());
        }
    }
    else
    {
        BaseBeanPointer b = beanFromContainer();
        if ( b )
        {
            DBFieldMetadata *f = qobject_cast<DBFieldMetadata*> (b->metadata()->navigateThroughProperties(m_fieldName));
            if ( f )
            {
                setMaxLength(f->length());
            }
        }
    }
    setFontAndColor();
    setReadOnly(!dataEditable());
    if ( !dataEditable() )
    {
        setFocusPolicy(Qt::NoFocus);
        if ( qApp->focusWidget() == this )
        {
            focusNextChild();
        }
    }
    else
    {
        setFocusPolicy(Qt::StrongFocus);
    }
}

QVariant DBLineEdit::value()
{
    QVariant v;
    if ( text().isEmpty() )
    {
        return v;
    }
    QString txt = text();
    return QVariant(txt);
}

/**
 * @brief DBLineEdit::finalValue
 * @return
 * Valor tras posible procesamiento con los reemplazos por puntos.
 */
QVariant DBLineEdit::finalValue()
{
    d->processTextEntry();
    processAutocompletion();
    return value();
}

QSize DBLineEdit::sizeHint() const
{
    if ( d->m_automaticWidthBasedOnFieldMaxLength && maxLength() != 32767 )
    {
        QSize sz;
        sz.setWidth(d->widthForMaxLength());
        sz.setHeight(QLineEdit::sizeHint().height());
        return sz;
    }
    return QLineEdit::sizeHint();
}

void DBLineEdit::observerUnregistered()
{
    DBBaseWidget::observerUnregistered();
    bool blockState = blockSignals(true);
    this->clear();
    blockSignals(blockState);
}

/*!
  Pide un nuevo observador si es necesario
  */
void DBLineEdit::refresh()
{
    observer();
    if ( m_observer != NULL )
    {
        m_observer->sync();
    }
}

/*!
  Tenemos que decirle al motor de scripts, que DBFormDlg se convierte de esta forma a un valor script
  */
QScriptValue DBLineEdit::toScriptValue(QScriptEngine *engine, DBLineEdit * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBLineEdit::fromScriptValue(const QScriptValue &object, DBLineEdit * &out)
{
    out = qobject_cast<DBLineEdit *>(object.toQObject());
}

void DBLineEdit::showEvent(QShowEvent *event)
{
    if ( d->m_showRecalculateCounterButton )
    {
        showRecalculateButton();
    }
    if ( !d->m_autoComplete.testFlag(AlephERP::NoCompletition) && d->m_completer.isNull() )
    {
        d->initCompleter();
        if ( !d->m_completer.isNull() )
        {
            QLineEdit::setCompleter(d->m_completer);
            if ( d->m_replacePointWithCharacter && !d->m_completerDelegate.isNull() )
            {
                d->m_completerDelegate->setReplaceWildCardCharacter(d->m_replacePointCharacter);
            }
        }
    }
    DBBaseWidget::showEvent(event);
    // Si la longitud del field está determinada, aplicaremos un tamaño máximo
    if ( d->m_automaticWidthBasedOnFieldMaxLength && maxLength() != 32767 )
    {
        setFixedWidth(d->widthForMaxLength());
        setSizePolicy(QSizePolicy::Fixed, sizePolicy().verticalPolicy());
        updateGeometry();
    }
}

void DBLineEdit::keyPressEvent(QKeyEvent *event)
{
    if ( !d->m_completer.isNull() )
    {
        // ¿Debemos empezar a filtrar con el wildcard? Si es así, informémosle al modelo
        if ( !d->m_replacePointCharacter.isNull() && !d->m_completerBeanModel.isNull() )
        {
            if ( event->text() == "." )
            {
                d->m_completerBeanModel->setWildCard(d->m_replacePointCharacter);
            }
            else
            {
                if ( !text().contains('.') )
                {
                    d->m_completerBeanModel->setWildCard(QChar());
                }
            }
        }
    }
    // Se puede desplegar el popup con esta combinación de teclas.
    if ( (event->modifiers() == Qt::ShiftModifier ||
          event->modifiers() == Qt::AltModifier ||
          event->modifiers() == Qt::ControlModifier)
         && event->key() == Qt::Key_Down )
    {
        if ( !d->m_completer.isNull() )
        {
            d->m_completer->complete();
        }
    }
    QLineEdit::keyPressEvent(event);
    if ( !d->m_completer.isNull() )
    {
        if ( d->m_completer->popup()->isVisible() )
        {
            QRect rect = d->m_completer->popup()->geometry();
            if ( d->m_autoCompletePopupSize.width() > 0 )
            {
                rect.setWidth(d->m_autoCompletePopupSize.width());
            }
            if ( d->m_autoCompletePopupSize.height() > 0 )
            {
                rect.setHeight(d->m_autoCompletePopupSize.height());
            }
            d->m_completer->popup()->setGeometry(rect);
        }
    }
    if ( event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter )
    {
        d->processTextEntry();
        processAutocompletion();
    }
    if ( d->m_focusNextOnEnd )
    {
        // ¿Estamos al final del control?
        DBFieldObserver *obs = qobject_cast<DBFieldObserver *>(observer());
        if ( obs != NULL )
        {
            DBField *fld = qobject_cast<DBField *>(obs->entity());
            if ( fld != NULL && fld->length() <= text().length() )
            {
                focusNextChild();
            }
        }
    }
}

bool DBLineEdit::eventFilter(QObject *watched, QEvent *event)
{
    if ( event->type() == QEvent::Show )
    {
        QTableView *tv = qobject_cast<QTableView *>(watched);
        if ( tv != NULL )
        {
            tv->resizeColumnsToContents();
        }
    }
    if ( event->type() == QEvent::KeyPress )
    {
        if ( watched == this )
        {
            // Si es un código de barras, debemos capturar la pulsación de la tecla finalizadora y tratarla...
            // Vaya a ser que sea el terminador. Y además, evitamos que se propage
            if ( m_barCodeReaderAllowed && !m_barCodeEndString.isEmpty() )
            {
                QKeyEvent *ev = static_cast<QKeyEvent *>(event);
                if ( m_barCodeEndString == QLatin1Literal("\\t") )
                {
                    if ( ev->key() == Qt::Key_Tab )
                    {
                        checkBarCode(text());
                        return true;
                    }
                }
                else if ( m_barCodeEndString == QLatin1Literal("\\n") || m_barCodeEndString == QLatin1Literal("\\r") )
                {
                    if ( ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return )
                    {
                        checkBarCode(text());
                        return true;
                    }
                }
            }
        }
        if ( watched == d->m_completerItemView )
        {
            QKeyEvent *ev = static_cast<QKeyEvent *>(event);
            if ( ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return )
            {
                d->m_completer->popup()->hide();
                d->processTextEntry();
                processAutocompletion();
                // Nos vamos al siguiente foco, pero si esto no es parte de un abstract item view
                QAbstractItemView *itemView = qobject_cast<QAbstractItemView *>(parent());
                if ( itemView == NULL )
                {
                    focusNextChild();
                }
                // Repintamos (si no, se queda el resto del cursor)
                update();
            }
        }
    }
    return QLineEdit::eventFilter(watched, event);
}

void DBLineEdit::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    if ( d->m_recalculateCounterButton.isNull() )
    {
        return;
    }
    QSize sz = d->m_recalculateCounterButton->sizeHint();
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    d->m_recalculateCounterButton->move(rect().right() - frameWidth - sz.width(),
                                        (rect().bottom() + 1 - sz.height())/2);
}

void DBLineEdit::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    // Si cuando pinchamos en un line edit que tiene una máscara, este está vacío, ubicamos
    // el cursor en el inicio.
    if ( !inputMask().isEmpty() && displayText().trimmed().isEmpty() )
    {
        setCursorPosition(0);
    }
    if ( observer() )
    {
        DBField *fld = qobject_cast<DBField *>(observer()->entity());
        if ( fld )
        {
            QLineEdit::setText(fld->displayValue());
        }
    }
}

/**
 * @brief DBLineEdit::focusInEvent
 * Conectamos el completer. Es importante hacerlo aquí, ya que focusOutEvent desconecta las señales del completer.
 * @param event
 */
void DBLineEdit::focusInEvent(QFocusEvent *event)
{
    d->m_userModifiedValue = false;
    if ( event->gotFocus() && !d->m_completer.isNull() && !d->m_completerDelegate.isNull() )
    {
        connect(d->m_completer, SIGNAL(highlighted(QModelIndex)), this, SLOT(itemActivatedOnCompleterModel(QModelIndex)), Qt::UniqueConnection);
        connect(this, SIGNAL(textChanged(QString)), d->m_completerDelegate, SLOT(setCurrentSearch(QString)), Qt::UniqueConnection);
    }
    QLineEdit::focusInEvent(event);
}

/**
     Cuando pierde el foco, se comprueba si hay que aplicar patrones: 43.1 -> 4300001
*/
void DBLineEdit::focusOutEvent(QFocusEvent * event)
{
    if ( d->m_userModifiedValue )
    {
        DBBaseWidget::askToRecalculateCounterField();
    }
    if ( event->lostFocus() )
    {
        d->processTextEntry();
    }
    if ( !d->m_autoComplete.testFlag(AlephERP::NoCompletition) )
    {
        processAutocompletion();
    }
    // Hay que llamar a este evento obligatoriamente.
    QLineEdit::focusOutEvent(event);
}

void DBLineEdit::processAutocompletion()
{
    // La causística es diversa: Por ejemplo: los valores disponible son "Serie A", "Serie B" y "Serie C":
    // 1.- El usuario introduce "ser" y sale del campo, en ese caso, se borra el campo, y además se pone el field del bean a vacio
    // 2.- El usuario escribe "serie a" pero no lo ha escogido del modelo. Debemos entonces hacer la búsqueda por él
    // 3.- El usuario ha borrado lo que hubiera antes... en ese caso, hay que limpiar el valor del padre.
    // Si esta columna es de autocompletado total no podemos dejar el campo libre... se escoge el del padre
    BaseBean *formBean = beanFromContainer();
    DBField *fld = NULL;
    FilterBaseBeanModel *mdl = qobject_cast<FilterBaseBeanModel *>(d->m_completerModel);
    QModelIndexList items;
    if ( mdl != NULL && !text().isEmpty() )
    {
        items = mdl->match(mdl->index(0, 0), Qt::DisplayRole, text(), 1, Qt::MatchStartsWith);
    }

    if ( observer() != NULL )
    {
        fld = qobject_cast<DBField *>(observer()->entity());
        if ( fld != NULL )
        {
            if ( QLineEdit::text().isEmpty() )
            {
                fld->setValue(QVariant());
                return;
            }
        }
    }

    if ( d->m_autoComplete.testFlag(AlephERP::ValuesFromThisField) )
    {
        if ( d->m_autoComplete.testFlag(AlephERP::RestrictValueToItemFromList) && items.isEmpty() )
        {
            QLineEdit::clear();
            if ( fld != NULL )
            {
                fld->setValue(QVariant());
            }
            if ( d->m_autoComplete.testFlag(AlephERP::UpdateOwnerFieldBean) )
            {
                if ( formBean != NULL )
                {
                    QStringList parts = m_fieldName.split(".");
                    if ( parts.size() == 2 )
                    {
                        DBRelation *rel = formBean->relation(parts.at(0));
                        if ( rel != NULL )
                        {
                            DBField *destFld = formBean->field(rel->metadata()->rootFieldName());
                            if ( destFld != NULL )
                            {
                                bool blockState = destFld->blockSignals(true);
                                destFld->setValue(QVariant());
                                destFld->blockSignals(blockState);
                            }
                        }
                    }
                }
            }
        }
    }
    else if ( d->m_autoComplete.testFlag(AlephERP::ValuesFromTableWithNoRelation) )
    {
        if ( d->m_autoComplete.testFlag(AlephERP::RestrictValueToItemFromList) )
        {
            if ( !text().isEmpty() )
            {
                if ( items.isEmpty() )
                {
                    QLineEdit::clear();
                    if ( fld != NULL )
                    {
                        fld->setValue(QVariant());
                    }
                }
                else
                {
                    d->m_completerBaseBean = mdl->bean(items.at(0));
                    if ( !d->m_completerBaseBean.isNull() && text() != d->m_completerBaseBean->fieldValue(d->m_autoCompleteColumn) )
                    {
                        // Caso en el que el usuario ha introducido parcialmente el texto de un item. Ponemos el texto completo.
                        setText(d->m_completerBaseBean->fieldValue(d->m_autoCompleteColumn).toString());
                    }
                    executeScriptAfterChooseFromCompleter();
                }
            }
            else
            {
                if ( !d->m_completerBaseBean.isNull() )
                {
                    d->m_completerBaseBean.clear();
                    executeScriptAfterChooseFromCompleter();
                }
            }
        }
    }
    else if ( d->m_autoComplete.testFlag(AlephERP::ValuesFromRelation) )
    {
        if ( formBean == NULL || observer() == NULL || fld == NULL )
        {
            return;
        }
        QList<DBRelation *> rels = fld->relations(AlephERP::ManyToOne);
        if ( rels.size() == 0 )
        {
            return;
        }
        DBRelation *rel = rels.first();

        // Caso 3
        if ( text().isEmpty() )
        {
            QLineEdit::clear();
            formBean->setFieldValue(rel->metadata()->rootFieldName(), QVariant());
            return;
        }
        if ( items.size() == 0 )
        {
            // Caso 1
            QLineEdit::clear();
            formBean->setFieldValue(rel->metadata()->rootFieldName(), QVariant());
            return;
        }
        // Caso 2
        BaseBeanSharedPointer beanToBeSelected = mdl->bean(items.at(0));
        if ( !beanToBeSelected.isNull() )
        {
            formBean->setFieldValue(rel->metadata()->rootFieldName(), beanToBeSelected->fieldValue(rel->metadata()->childFieldName()));
            setCompleterSelectedBean(beanToBeSelected);
            executeScriptAfterChooseFromCompleter();
        }
    }
}

void DBLineEditPrivate::initCompleter ()
{
    if ( !m_autoComplete.testFlag(AlephERP::NoCompletition) && m_completer.isNull() )
    {
        m_completer = new QCompleter();
        if ( m_autoComplete.testFlag(AlephERP::ValuesFromRelation) )
        {
            initCompleterFromRelation();
        }
        else if ( m_autoComplete.testFlag(AlephERP::ValuesFromThisField) )
        {
            initCompleterFromFieldValues();
        }
        else if ( m_autoComplete.testFlag(AlephERP::ValuesFromTableWithNoRelation) )
        {
            initCompleterFromTable();
        }
        if ( m_completerItemView != NULL )
        {
            // Se hace porque así garantizamos que setModel borrará un modelo previo.
            m_completerModel->setParent(m_completer);
            m_completer->setModel(m_completerModel);
            m_completer->setCaseSensitivity(Qt::CaseInsensitive);
            m_completer->setWrapAround(true);
            m_completer->setCompletionMode(QCompleter::PopupCompletion);
            m_completer->setPopup(m_completerItemView);
            m_completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
            m_completerItemView->installEventFilter(q_ptr);
        }
    }
}

void DBLineEditPrivate::initCompleterFromRelation()
{
    if ( q_ptr->observer() == NULL )
    {
        return;
    }
    QString order = m_autoCompleteColumn + QString(" ASC");
    DBField *fld = qobject_cast<DBField *>(q_ptr->observer()->entity());
    if ( fld == NULL )
    {
        return;
    }
    QList<DBRelation *> relations = fld->relations(AlephERP::ManyToOne | AlephERP::OneToOne);
    if ( relations.size() > 1 )
    {
        QLogger::QLog_Debug(AlephERP::stLogOther,
                            QString("DBBaseWidget::initCompleter:  [%1] ] Mal formado. Tiene varias referencias M a 1. Se escoge la primera relación.").arg(fld->metadata()->dbFieldName()));
    }
    if ( relations.size() == 0 )
    {
        return;
    }
    BaseBeanMetadata *fatherMetadata = BeansFactory::metadataBean(relations.first()->metadata()->tableName());
    if ( fatherMetadata == NULL )
    {
        return;
    }
    relations.first()->setFilter(q_ptr->relationFilter());
    // No asignamos padre, porque después, con el completer se generar el borrado del objeto.
    FilterBaseBeanModel *filter = new FilterBaseBeanModel();
    DBBaseBeanModel *mdl = new DBBaseBeanModel(fatherMetadata->tableName(),
                                               relations.first()->sqlRelationWhere(),
                                               order,
                                               true,
                                               true,
                                               false,
                                               q_ptr);
    if ( !m_autoCompleteVisibleFields.isEmpty() )
    {
        mdl->setVisibleFields(m_autoCompleteVisibleFields.split(QRegExp(";|,")));
    }
    m_completerBeanModel = mdl;
    filter->setSourceModel(m_completerBeanModel);
    m_completerModel = filter;
    if ( filter->visibleFields().size() == 1 )
    {
        m_completerItemView = new QListView;
    }
    else
    {
        m_completerItemView = new DBTableView;
        (qobject_cast<DBTableView *>(m_completerItemView))->horizontalHeader()->setVisible(false);
    }
    int idxColumn = filter->dbFieldColumnIndex(m_autoCompleteColumn);
    m_completerDelegate = new AERPCompleterHighlightDelegate();
    m_completerItemView->setItemDelegateForColumn(idxColumn, m_completerDelegate);
    m_completer->setCompletionColumn(idxColumn);
    m_completer->setCompletionRole(AlephERP::ReplaceWildCards);
}

void DBLineEditPrivate::initCompleterFromFieldValues()
{
    if ( q_ptr->observer() == NULL )
    {
        return;
    }
    QString order = m_autoCompleteColumn + QString(" ASC");
    DBField *fld = qobject_cast<DBField *>(q_ptr->observer()->entity());
    if ( fld == NULL )
    {
        return;
    }
    // Si no hay relación en el campo, se completa con el conjunto de valores del propio campo
    AERPQueryModel *model = new AERPQueryModel(true);
    model->freezeModel();
    // La consulta que se configura aquí es importante: Debe estar ordenada correctamente si queremos que
    // el completer funcione. Esto signfica que Completer no funcionará si se encuentra algo así como esto
    // "Aa", "Ba", ""
    // Esa última cadena "" hace que no funcione.
    QString sql = QString("SELECT DISTINCT %1 FROM %2 WHERE %1 <> '' AND %1 IS NOT NULL ORDER BY %3 ASC").arg(fld->metadata()->dbFieldName()).
                  arg(fld->bean()->metadata()->tableName()).arg(fld->metadata()->dbFieldName());
    model->setQuery(sql);
    m_completerModel = model;
    m_completerItemView = new QListView;
    m_completer->setCompletionColumn(0);
    m_completerDelegate = new AERPCompleterHighlightDelegate();
    m_completerItemView->setItemDelegateForColumn(0, m_completerDelegate);
}

void DBLineEditPrivate::initCompleterFromTable()
{
    QString order = m_autoCompleteColumn + QString(" ASC");
    BaseBeanMetadata *m = BeansFactory::metadataBean(m_autoCompleteTableName);
    if ( m == NULL )
    {
        QLogger::QLog_Debug(AlephERP::stLogOther,
                            QString("DBBaseWidget::initCompleter: Tabla de autocompletado [%1] no valida para campo [%2]").
                            arg(m_autoCompleteTableName).arg(q_ptr->fieldName()));
        return;
    }
    FilterBaseBeanModel *filter = new FilterBaseBeanModel();
    DBBaseBeanModel *mdl = new DBBaseBeanModel(m_autoCompleteTableName, m_autoCompleteTableNameFilter, order, true, true, false, filter);
    if ( !m_autoCompleteVisibleFields.isEmpty() )
    {
        mdl->setVisibleFields(m_autoCompleteVisibleFields.split(QRegExp(";|,")));
    }
    m_completerBeanModel = mdl;
    filter->setSourceModel(m_completerBeanModel);
    m_completerModel = filter;
    if ( filter->visibleFields().size() == 1 )
    {
        m_completerItemView = new QListView;
    }
    else
    {
        m_completerItemView = new DBTableView;
        (qobject_cast<DBTableView *>(m_completerItemView))->horizontalHeader()->setVisible(false);
    }
    int idxColumn = filter->dbFieldColumnIndex(m_autoCompleteColumn);
    m_completerDelegate = new AERPCompleterHighlightDelegate();
    m_completerItemView->setItemDelegateForColumn(idxColumn, m_completerDelegate);
    m_completer->setCompletionColumn(idxColumn);
    m_completer->setCompletionRole(AlephERP::ReplaceWildCards);
}

int DBLineEditPrivate::widthForMaxLength()
{
    QString str = QString("0").repeated(q_ptr->maxLength() + 1);
    QFontMetrics fm(q_ptr->font());
    QStyleOptionFrameV2 opt;
    q_ptr->initStyleOption(&opt);

    QSize computedSize(fm.boundingRect(str).size());
    computedSize.setWidth(computedSize.width() + q_ptr->textMargins().left() + q_ptr->textMargins().bottom());
    QSize sz = q_ptr->style()->sizeFromContents(QStyle::CT_LineEdit, &opt, computedSize.
                                      expandedTo(QApplication::globalStrut()), q_ptr);
    return sz.width();
}

/**
 * @brief DBLineEditPrivate::processTextEntry
 * Procesa el texto introducido para aplicar la sustitución de caracteres en caso de que el control
 * esté así configurado.
 */
void DBLineEditPrivate::processTextEntry()
{
    DBFieldObserver *obs = qobject_cast<DBFieldObserver *>(q_ptr->observer());
    if ( obs != NULL )
    {
        DBField *fld = qobject_cast<DBField *>(obs->entity());
        if ( m_replacePointWithCharacter && fld != NULL )
        {
            // Hay que tener en cuenta el caso en el que se presente un completer
            int maxLength = obs->maxLength();
            if ( m_autoComplete.testFlag(AlephERP::ValuesFromRelation) && !m_autoCompleteColumn.isEmpty() )
            {
                QList<DBRelation *> rels = fld->relations(AlephERP::ManyToOne);
                if ( rels.size() > 0 )
                {
                    BaseBean *father = rels.first()->father();
                    DBField *fldM = father->field(m_autoCompleteColumn);
                    if ( fldM != NULL )
                    {
                        maxLength = fldM->length();
                    }
                }
                else
                {
                    rels = fld->relations(AlephERP::OneToOne);
                    if ( rels.size() > 0 )
                    {
                        BaseBean *brother = rels.first()->brother();
                        if ( brother )
                        {
                            DBField *fldM = brother->field(m_autoCompleteColumn);
                            if ( fldM != NULL )
                            {
                                maxLength = fldM->length();
                            }
                        }
                    }
                }
            }
            int numChars = maxLength - q_ptr->text().size() + 1;
            QString chars = QString("").fill(m_replacePointCharacter, numChars);
            QString actualText = q_ptr->text();
            QString finalText = actualText.replace('.', chars);
            q_ptr->setText(finalText);
        }
    }
}

QString DBLineEditPrivate::setValueFromCompletition(DBField *fld)
{
    QString newText;

    QList<DBRelation *> rels = fld->relations(AlephERP::ManyToOne);
    if ( rels.size() > 0 )
    {
        BaseBean *father = rels.first()->father();
        // Los padres "vírgenes" no muestran datos...
        if ( father->dbState() != BaseBean::INSERT || father->modified() )
        {
            newText = father->fieldValue(m_autoCompleteColumn).toString();
        }
    }
    else
    {
        rels = fld->relations(AlephERP::OneToOne);
        BaseBean *brother = rels.first()->brother();
        if ( brother )
        {
            newText = brother->fieldValue(m_autoCompleteColumn).toString();
        }
    }
    return newText;
}

bool DBLineEdit::enableAutoCompleteOnDesigner()
{
    // No se van a permitir autocompletados en campos con más de una relación
    return (m_fieldName.split(".").size() <= 2);
}

bool DBLineEdit::enableAutoCompleteTableNameOnDesigner()
{
    if ( d->m_autoComplete.testFlag(AlephERP::NoCompletition) )
    {
        return false;
    }
    if ( d->m_autoComplete.testFlag(AlephERP::ValuesFromThisField) || d->m_autoComplete.testFlag(AlephERP::ValuesFromRelation) )
    {
        return false;
    }
    return true;
}

bool DBLineEdit::enableAutoCompleteColumnOnDesigner()
{
    if ( d->m_autoComplete.testFlag(AlephERP::NoCompletition) )
    {
        return false;
    }
    if ( d->m_autoComplete.testFlag(AlephERP::ValuesFromThisField) )
    {
        return false;
    }
    return true;
}

bool DBLineEdit::enableAutoCompletePopupSizeOnDesigner()
{
    if ( d->m_autoComplete.testFlag(AlephERP::NoCompletition) )
    {
        return false;
    }
    return true;
}

bool DBLineEdit::enableAutoCompleteVisibleFieldsOnDesigner()
{
    if ( d->m_autoComplete.testFlag(AlephERP::NoCompletition) || d->m_autoComplete.testFlag(AlephERP::ValuesFromThisField) )
    {
        return false;
    }
    return true;
}
