#ifndef OPENEDRECORDS_H
#define OPENEDRECORDS_H

#include <QtCore>
#include <dao/beans/basebean.h>
#include <forms/dbrecorddlg.h>
#include <alepherpglobal.h>

typedef struct stOpenedRecordsData {
    BaseBeanPointer bean;
    BaseBeanSharedPointer sharedBean;
    QPointer<DBRecordDlg> dialog;
} OpenedRecordsData;

/**
 * @brief The OpenedRecords class
 * Registro de DBRecord abiertos.
 */
class ALEPHERP_DLL_EXPORT OpenedRecords : QObject
{
    Q_OBJECT

private:
    QList<OpenedRecordsData> m_recordBeans;

    OpenedRecords(QObject *parent = NULL);

    bool containsBean(BaseBeanPointer bean);
    BaseBeanPointerList beans();
    QList<QPointer<DBRecordDlg> > dialogs();
    QPointer<DBRecordDlg> dialog(BaseBeanPointer bean);

public:
    static OpenedRecords *instance();

    void registerRecord(BaseBeanSharedPointer bean, QPointer<DBRecordDlg> dlg);
    void registerRecord(BaseBeanPointer bean, QPointer<DBRecordDlg> dlg);

    bool isBeanOpened(BaseBeanPointer bean);
    QPointer<DBRecordDlg> dialogForBean(BaseBeanPointer bean);
    bool dialogOpenedForRecord(const QString &tableName);

protected slots:
    void recordClosed(QObject *obj);
};

#endif // OPENEDRECORDS_H
