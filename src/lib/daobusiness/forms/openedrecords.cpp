#include "openedrecords.h"
#include "dao/beans/basebeanmetadata.h"

OpenedRecords::OpenedRecords(QObject *parent) :
    QObject(parent)
{

}

bool OpenedRecords::containsBean(BaseBeanPointer bean)
{
    foreach (OpenedRecordsData d, m_recordBeans)
    {
        if ( !d.bean.isNull() &&
             !bean.isNull() &&
             bean->dbOid() == d.bean->dbOid() &&
             bean->metadata()->tableName() == d.bean->metadata()->tableName() )
        {
            return true;
        }
    }
    return false;
}

BaseBeanPointerList OpenedRecords::beans()
{
    BaseBeanPointerList list;
    foreach (OpenedRecordsData d, m_recordBeans)
    {
        if ( !d.bean.isNull() )
        {
            list.append(d.bean);
        }
    }
    return list;
}

QList<QPointer<DBRecordDlg> > OpenedRecords::dialogs()
{
    QList<QPointer<DBRecordDlg> > list;
    foreach (OpenedRecordsData d, m_recordBeans)
    {
        if ( !d.dialog.isNull() )
        {
            list.append(d.dialog);
        }
    }
    return list;
}

QPointer<DBRecordDlg> OpenedRecords::dialog(BaseBeanPointer bean)
{
    foreach (OpenedRecordsData d, m_recordBeans)
    {
        if ( !d.bean.isNull() &&
             !bean.isNull() &&
             bean->dbOid() == d.bean->dbOid() &&
             bean->metadata()->tableName() == d.bean->metadata()->tableName() )
        {
            return d.dialog;
        }
    }
    return NULL;
}

OpenedRecords *OpenedRecords::instance()
{
    static OpenedRecords *singleton;
    if ( singleton == NULL )
    {
        singleton = new OpenedRecords(qApp);
    }
    return singleton;
}

void OpenedRecords::registerRecord(BaseBeanPointer bean, QPointer<DBRecordDlg> dlg)
{
    if ( !containsBean(bean) )
    {
        connect(dlg, SIGNAL(destroyed(QObject*)), this, SLOT(recordClosed()));
        OpenedRecordsData d;
        d.bean = bean;
        d.dialog = dlg;
        m_recordBeans.append(d);
    }
}

bool OpenedRecords::isBeanOpened(BaseBeanPointer bean)
{
    foreach (BaseBeanPointer b, beans())
    {
        if ( !b.isNull() &&
             !bean.isNull() &&
             b->dbOid() == bean->dbOid() &&
             b->metadata()->tableName() == bean->metadata()->tableName() )
        {
            return true;
        }
    }
    return false;
}

QPointer<DBRecordDlg> OpenedRecords::dialogForBean(BaseBeanPointer bean)
{
    if ( containsBean(bean) )
    {
        return dialog(bean);
    }
    return NULL;
}

void OpenedRecords::recordClosed()
{
    DBRecordDlg *dlg = qobject_cast<DBRecordDlg *>(sender());
    QList<QPointer<DBRecordDlg> > list = dialogs();
    for (int i = 0 ; i < list.size() ; i++)
    {
        QPointer<DBRecordDlg> registerdDlg = list.at(i);
        if ( registerdDlg.data() == dlg )
        {
            m_recordBeans.removeAt(i);
        }
    }
}
