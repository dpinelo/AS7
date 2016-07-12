#ifndef OPENEDRECORDS_H
#define OPENEDRECORDS_H

#include <QtCore>

class OpenedRecords : QObject
{
    Q_OBJECT
private:
    static OpenedRecords *singleton;

    OpenedRecords();

public:
    OpenedRecords *instance();

};

#endif // OPENEDRECORDS_H
