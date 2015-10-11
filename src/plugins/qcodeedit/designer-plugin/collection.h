
#ifndef _COLLECTION_H_
#define _COLLECTION_H_

#include <QtCore/qplugin.h>
#include <QtDesigner/QtDesigner>

class Collection : public QObject, public QDesignerCustomWidgetCollectionInterface
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetCollectionInterface)

#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
    Q_PLUGIN_METADATA(IID "es.alephsistemas.alepherp.QCodeEditDesignerPluginInterface" FILE "qcodeedit.json")
#endif

public:
    Collection(QObject *p = 0);

    virtual QList<QDesignerCustomWidgetInterface*> customWidgets() const;

private:
    QList<QDesignerCustomWidgetInterface*> widgets;
};

Q_DECLARE_METATYPE(QList<QDesignerCustomWidgetInterface*>)

#endif // _COLLECTION_H_
