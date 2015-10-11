#include "htmleditorplugin.h"
#include "htmleditor.h"
#include <QtPlugin>

HtmlEditorPlugin::HtmlEditorPlugin(QObject *parent) : QObject (parent)
{
    m_initialized = false;
}

void HtmlEditorPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if ( m_initialized )
    {
        return;
    }
    m_initialized = true;
}

bool HtmlEditorPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *HtmlEditorPlugin::createWidget(QWidget *parent)
{
    return new HtmlEditor(parent);
}

QString HtmlEditorPlugin::name() const
{
    return "HtmlEditor";
}

QString HtmlEditorPlugin::group() const
{
    return "AlephERP";
}

QIcon HtmlEditorPlugin::icon() const
{
    return QIcon(":/images/htmleditor.png");
}

QString HtmlEditorPlugin::toolTip() const
{
    return trUtf8("Editor WYSIWYG de código HTML");
}

QString HtmlEditorPlugin::whatsThis() const
{
    return trUtf8("Editor WYSIWYG de código HTML");
}

bool HtmlEditorPlugin::isContainer() const
{
    return false;
}

QString HtmlEditorPlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
           " <widget class=\"HtmlEditor\" name=\"htmlEditor\">\n"
           "  <property name=\"geometry\">\n"
           "   <rect>\n"
           "    <x>0</x>\n"
           "    <y>0</y>\n"
           "    <width>100</width>\n"
           "    <height>100</height>\n"
           "   </rect>\n"
           "  </property>\n"
           " </widget>\n"
           "</ui>";
}

QString HtmlEditorPlugin::includeFile() const
{
    return "htmleditor.h";
}

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
Q_EXPORT_PLUGIN2(htmleditorplugin, HtmlEditorPlugin)
#endif

