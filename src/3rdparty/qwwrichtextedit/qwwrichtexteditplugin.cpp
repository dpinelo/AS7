#include "qwwrichtexteditplugin.h"
#include "qwwrichtextedit.h"
#include <QtPlugin>

QwwRichTextEditPlugin::QwwRichTextEditPlugin(QObject *parent) : QObject (parent)
{
    m_initialized = false;
}

void QwwRichTextEditPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if ( m_initialized )
    {
        return;
    }
    m_initialized = true;
}

bool QwwRichTextEditPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *QwwRichTextEditPlugin::createWidget(QWidget *parent)
{
    return new QwwRichTextEdit(parent);
}

QString QwwRichTextEditPlugin::name() const
{
    return "QwwRichTextEdit";
}

QString QwwRichTextEditPlugin::group() const
{
    return "AlephERP";
}

QIcon QwwRichTextEditPlugin::icon() const
{
    return QIcon(":/trolltech/formeditor/images/widgets/textedit.png");
}

QString QwwRichTextEditPlugin::toolTip() const
{
    return trUtf8("Editor RichText, desarrollado por wysota del proyecto wwWidgets.");
}

QString QwwRichTextEditPlugin::whatsThis() const
{
    return trUtf8("Editor RichText, desarrollado por wysota del proyecto wwWidgets.");
}

bool QwwRichTextEditPlugin::isContainer() const
{
    return false;
}

QString QwwRichTextEditPlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
           " <widget class=\"QwwRichTextEdit\" name=\"wwRichTextEdit\">\n"
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

QString QwwRichTextEditPlugin::includeFile() const
{
    return "qwwrichtextedit.h";
}

Q_EXPORT_PLUGIN2(qwwrichtexteditplugin, QwwRichTextEditPlugin)
