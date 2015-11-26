#ifndef DBNUMBEREDITPLUGIN_H
#define DBNUMBEREDITPLUGIN_H

#include <QtUiPlugin/QDesignerCustomWidgetInterface>
#include "dbbaseplugin.h"

class DBNumberEditPlugin: public DBBasePlugin
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)

private:

public:
    DBNumberEditPlugin(QObject *parent = 0);

    bool isContainer() const;
    QIcon icon() const;
    QString domXml() const;
    QString group() const;
    QString includeFile() const;
    QString name() const;
    QString toolTip() const;
    QString whatsThis() const;
    QWidget *createWidget(QWidget *parent);

};

#endif // DBNUMBEREDITPLUGIN_H
