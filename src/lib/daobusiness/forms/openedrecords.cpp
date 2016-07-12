#include "openedrecords.h"

OpenedRecords::OpenedRecords()
{

}

OpenedRecords *OpenedRecords::instance()
{
    if ( singleton == NULL )
    {
        singleton = new OpenedRecords(qApp);
    }
    return singleton;
}
