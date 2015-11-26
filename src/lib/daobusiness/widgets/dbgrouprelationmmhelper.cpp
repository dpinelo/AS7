#include "dbgrouprelationmmhelper.h"
#include "dao/basedao.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/observerfactory.h"

class DBGroupRelationMMHelperPrivate
{
public:
    DBGroupRelationMMHelper *q_ptr;
    QString m_otherTableName;
    QString m_otherFieldName;
    bool m_inited;
    BaseBeanSharedPointerList m_otherBeans;

    DBGroupRelationMMHelperPrivate(DBGroupRelationMMHelper *qq) : q_ptr(qq)
    {
        m_inited = false;
    }

    void init();
    void clearCheckBoxes();
    void setCheckBoxStates();
};

DBGroupRelationMMHelper::DBGroupRelationMMHelper(QWidget *parent) :
    QGroupBox(parent),
    DBBaseWidget(),
    d(new DBGroupRelationMMHelperPrivate (this))
{

}

DBGroupRelationMMHelper::~DBGroupRelationMMHelper()
{
    delete d;
}

void DBGroupRelationMMHelperPrivate::init()
{
    if ( m_otherTableName.isEmpty() )
    {
        return;
    }
    m_otherBeans.clear();

    if ( q_ptr->layout() )
    {
        delete q_ptr->layout();
    }
    QVBoxLayout *lay = new QVBoxLayout(q_ptr);

    if ( BaseDAO::select(m_otherBeans, m_otherTableName) )
    {
        foreach (BaseBeanSharedPointer bean, m_otherBeans)
        {
            QCheckBox *chk = new QCheckBox();
            chk->setText(bean->fieldValue(m_otherFieldName).toString());
            chk->setProperty("oid", bean->dbOid());
            lay->addWidget(chk);
            QObject::connect(chk, SIGNAL(clicked(bool)), q_ptr, SLOT(checkBoxClicked()));
        }
    }
    q_ptr->setLayout(lay);
    m_inited = true;
}

void DBGroupRelationMMHelperPrivate::clearCheckBoxes()
{
    if ( q_ptr->layout() )
    {
        delete q_ptr->layout();
    }
    m_otherBeans.clear();
}

void DBGroupRelationMMHelperPrivate::setCheckBoxStates()
{
    BaseBeanPointer bean = q_ptr->beanFromContainer();
    BaseBeanPointerList children = bean->relationChildren(q_ptr->relationName());
    QList<QCheckBox *> checks = q_ptr->findChildren<QCheckBox *>();

    foreach (BaseBeanPointer child, children)
    {
        foreach (QCheckBox *chk, checks)
        {
            if ( chk->property("oid").isValid() )
            {
                qlonglong oid = chk->property("oid").toLongLong();
                BaseBeanPointer father = child->father(m_otherTableName);
                if ( !father.isNull() )
                {
                    QSignalBlocker bl(chk);
                    chk->setChecked(oid == father->dbOid());
                }
            }
        }
    }
}

/*!
  Ajusta los parámetros de visualización de este Widget en función de lo definido en DBField m_field
  */
void DBGroupRelationMMHelper::applyFieldProperties()
{
    setEnabled(dataEditable());
    if ( !isEnabled() && qApp->focusWidget() == this )
    {
        focusNextChild();
    }
}

QVariant DBGroupRelationMMHelper::value()
{
    return QVariant();
}

/*!
  Tenemos que decirle al motor de scripts, que DBFormDlg se convierte de esta forma a un valor script
  */
QScriptValue DBGroupRelationMMHelper::toScriptValue(QScriptEngine *engine, DBGroupRelationMMHelper * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBGroupRelationMMHelper::fromScriptValue(const QScriptValue &object, DBGroupRelationMMHelper * &out)
{
    out = qobject_cast<DBGroupRelationMMHelper *>(object.toQObject());
}

QString DBGroupRelationMMHelper::otherTableName() const
{
    return d->m_otherTableName;
}

void DBGroupRelationMMHelper::setOtherTableName(const QString &name)
{
    d->m_otherTableName = name;
}

QString DBGroupRelationMMHelper::otherFieldName() const
{
    return d->m_otherFieldName;
}

void DBGroupRelationMMHelper::setOtherFieldName(const QString &name)
{
    d->m_otherFieldName = name;
}

QScriptValue DBGroupRelationMMHelper::checkedBeans()
{
    return QScriptValue();
}

void DBGroupRelationMMHelper::setCheckedBeans(BaseBeanSharedPointerList list, bool checked)
{
    Q_UNUSED(list)
    Q_UNUSED(checked)
}

void DBGroupRelationMMHelper::setCheckedBeansByPk(QVariantList list, bool checked)
{
    Q_UNUSED(list)
    Q_UNUSED(checked)
}

/**
 * @brief DBGroupRelationMMHelper::checkBoxClicked
 * Actuamos cuando el usuario clickea un checkbox
 */
void DBGroupRelationMMHelper::checkBoxClicked()
{
    QCheckBox *chk = qobject_cast<QCheckBox *>(sender());
    if ( chk != NULL && chk->property("oid").isValid() )
    {
        qlonglong oid = chk->property("oid").toLongLong();
        BaseBeanPointer bean = beanFromContainer();
        BaseBeanSharedPointer otherBean;

        foreach (BaseBeanSharedPointer b, d->m_otherBeans)
        {
            if ( b->dbOid() == oid )
            {
                otherBean = b;
            }
        }
        if ( otherBean.isNull() )
        {
            return;
        }

        BaseBeanPointer child;
        BaseBeanPointerList children = bean->relationChildren(relationName());

        foreach (BaseBeanPointer c, children)
        {
            BaseBeanPointer otherBean = c->father(d->m_otherTableName);
            if ( otherBean != NULL && otherBean->dbOid() == oid )
            {
                child = c;
            }
        }

        // Si se ha chequeado: Agregamos un hijo que relacione ambos beans. Si no se ha chequeado, eliminamos la relación si la hubiese.
        if ( chk->isChecked() )
        {
            if ( child.isNull() )
            {
                DBRelation *relation = bean->relation(relationName());
                if ( relation != NULL )
                {
                    BaseBeanSharedPointer child = relation->newChild();
                    DBRelation *otherRelation = child->relation(d->m_otherTableName);
                    if ( otherRelation != NULL )
                    {
                        child->setFieldValue(otherRelation->metadata()->rootFieldName(),
                                             otherBean->fieldValue(otherRelation->metadata()->childFieldName()));
                        setUserModified(true);
                    }
                }
            }
        }
        else
        {
            if ( !child.isNull() )
            {
                child->setDbState(BaseBean::TO_BE_DELETED);
                setUserModified(true);
            }
        }
    }
}

void DBGroupRelationMMHelper::setValue(const QVariant &value)
{
    Q_UNUSED(value)
}

void DBGroupRelationMMHelper::refresh()
{
    if ( !d->m_inited )
    {
        d->init();
        d->setCheckBoxStates();
    }
}

void DBGroupRelationMMHelper::observerUnregistered()
{
    DBBaseWidget::observerUnregistered();
    bool blockState = blockSignals(true);
    d->clearCheckBoxes();
    d->m_inited = false;
    blockSignals(blockState);
}

void DBGroupRelationMMHelper::showEvent(QShowEvent *event)
{
    DBBaseWidget::showEvent(event);
    QWidget::showEvent(event);
    refresh();
}

