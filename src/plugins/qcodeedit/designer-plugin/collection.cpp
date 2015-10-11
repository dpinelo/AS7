
#include "collection.h"

#include "editorplugin.h"
#include "colorpickerplugin.h"
#include "editorconfigplugin.h"
#include "formatconfigplugin.h"

Collection::Collection(QObject *p)
    : QObject(p)
{
    widgets << new EditorPlugin(this);
    widgets << new ColorPickerPlugin(this);
    widgets << new EditorConfigPlugin(this);
    widgets << new FormatConfigPlugin(this);
}

QList<QDesignerCustomWidgetInterface*> Collection::customWidgets() const
{
    return widgets;
}

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
Q_EXPORT_PLUGIN2(qcodeeditcollection, Collection);
#endif
