#ifndef DBGROUPRELATIONMMHELPER_H
#define DBGROUPRELATIONMMHELPER_H

#include <QtWidgets>
#include <QtScript>
#include <alepherpglobal.h>
#include <aerpcommon.h>
#include "widgets/dbabstractview.h"
#include "dao/beans/basebean.h"

class DBGroupRelationMMHelperPrivate;

/**
 * @brief The DBGroupRelationMMHelper class
 * Para relaciones de tipo MM, este widget ayuda a crear, editar o modificar los registros
 * de la relación intermedia que hace MM. Para ello, presenta un conjunto de "CheckBoxes"
 * seleccionables por el usuario
 */
class ALEPHERP_DLL_EXPORT DBGroupRelationMMHelper : public QGroupBox, public DBBaseWidget, public QScriptable
{
    Q_OBJECT
    /** De esta relación se crearán los hijos de forma automática cuando se marquen los checks */
    Q_PROPERTY (QString relationName READ relationName WRITE setRelationName)
    /** Nombre del otro extremo de la relación */
    Q_PROPERTY (QString otherTableName READ otherTableName WRITE setOtherTableName)
    /** De la otra tabla, en el otro extremo de la relación: ¿Qué fieldName se utilizará en los checkboxes? */
    Q_PROPERTY (QString otherFieldName READ otherFieldName WRITE setOtherFieldName)
    /** La propiedad \relationFilter tiene en este control un aspecto específico: Se utiliza para el "completador" que se activa con \a autoComplete.
      El valor viene de un conjunto de valores de otra tabla. Al usuario puede mostrársele toda esa tabla o sólo los registros filtrados por relationFilter.
      Es un filtro SQL */
    Q_PROPERTY (QString relationFilter READ relationFilter WRITE setRelationFilter)
    /** Indica si el control está en modo readOnly o no */
    Q_PROPERTY (bool dataEditable READ dataEditable WRITE setDataEditable)
    Q_PROPERTY (bool aerpControl READ aerpControl)
    /** Útil desde Qs, para conocer si el usuario ha modificado el valor que contiene */
    Q_PROPERTY (bool userModified READ userModified WRITE setUserModified)
    /** Acceso al valor del field asociado (la columna de base de datos */
    Q_PROPERTY (QVariant value READ value WRITE setValue)
    /** Un control puede estar dentro de un AERPScriptWidget. ¿De dónde lee los datos? Los puede leer
      del bean asignado al propio AERPScriptWidget, o bien, leerlos del bean del formulario base
      que lo contiene. Esta propiedad marca esto */
    Q_PROPERTY (bool dataFromParentDialog READ dataFromParentDialog WRITE setDataFromParentDialog)

    friend class DBGroupRelationMMHelperPrivate;

private:
    DBGroupRelationMMHelperPrivate *d;

protected:
    virtual void showEvent(QShowEvent *event)
    {
        DBBaseWidget::showEvent(event);
        QWidget::showEvent(event);
    }
    virtual void hideEvent(QHideEvent *event)
    {
        DBBaseWidget::hideEvent(event);
        QWidget::hideEvent(event);
    }

public:
    DBGroupRelationMMHelper(QWidget *parent = 0);
    virtual ~DBGroupRelationMMHelper();

    AlephERP::ObserverType observerType(BaseBean *)
    {
        return AlephERP::DbRelation;
    }
    virtual void applyFieldProperties();
    virtual QVariant value();

    static QScriptValue toScriptValue(QScriptEngine *engine, DBGroupRelationMMHelper * const &in);
    static void fromScriptValue(const QScriptValue &object, DBGroupRelationMMHelper * &out);

    QString otherTableName() const;
    void setOtherTableName(const QString &name);
    QString otherFieldName() const;
    void setOtherFieldName(const QString &name);

    virtual QScriptValue checkedBeans();
    virtual void setCheckedBeans(BaseBeanSharedPointerList list, bool checked = true);
    virtual void setCheckedBeansByPk(QVariantList list, bool checked = true);

    virtual void orderColumns(const QStringList &) {}
    virtual void sortByColumn(const QString &, Qt::SortOrder) {}
    virtual void saveColumnsOrder(const QStringList &, const QStringList &) {}

signals:
    void valueEdited(const QVariant &value);
    void destroyed(QWidget *widget);

private slots:
    void checkBoxClicked();

public slots:
    virtual void setValue(const QVariant &value);
    virtual void refresh();
    virtual void observerUnregistered();
    void setFocus()
    {
        QWidget::setFocus();
    }

};

Q_DECLARE_METATYPE(DBGroupRelationMMHelper*)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBGroupRelationMMHelper, DBGroupRelationMMHelper*)


#endif // DBGROUPRELATIONMMHELPER_H
