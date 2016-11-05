#include "perpbasedialog_p.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/basebean.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/beansfactory.h"
#include "widgets/dbchooserecordbutton.h"
#include "widgets/dblineedit.h"
#include "widgets/dbcombobox.h"
#include "widgets/dbnumberedit.h"
#include "widgets/dbtextedit.h"
#include "widgets/dbdatetimeedit.h"
#include "widgets/dbfileupload.h"
#include "widgets/dblistview.h"
#include "globales.h"
#include "aerpcommon.h"
#include "configuracion.h"

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
                if ( !fld->serial() && (fld->type() == QVariant::Int || fld->type() == QVariant::Double || fld->type() == QVariant::LongLong) )
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
                if ( m_firstFocusWidget.isNull() )
                {
                    m_firstFocusWidget = widList.at(0);
                }
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
                    if ( fld->type() == QVariant::Int || fld->type() == QVariant::Double || fld->type() == QVariant::LongLong )
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
                if ( m_firstFocusWidget.isNull() )
                {
                    m_firstFocusWidget = widList.at(0);
                }
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

bool AERPBaseDialogPrivate::connectPushButtonToQsFunction(QPushButton *button)
{
    QString methodNameToInvokeOnClicked = button->property(AlephERP::stQsFunction).toString();
    if ( methodNameToInvokeOnClicked.isEmpty() )
    {
        return false;
    }
    QScriptValue thisObject = m_engine->qsThisObject();
    if ( thisObject.isValid() )
    {
        QScriptValue function = thisObject.property(methodNameToInvokeOnClicked);
        QScriptValue functionOnProtoype = thisObject.prototype().property(methodNameToInvokeOnClicked);
        if ( function.isValid() && function.isFunction() )
        {
            qScriptConnect(button, SIGNAL(clicked(bool)), thisObject, function);
        }
        else if ( functionOnProtoype.isValid() && functionOnProtoype.isFunction() )
        {
            qScriptConnect(button, SIGNAL(clicked(bool)), thisObject, functionOnProtoype);
        }
    }
    return true;
}
