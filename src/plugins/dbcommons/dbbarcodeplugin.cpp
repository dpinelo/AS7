#include "dbbarcodeplugin.h"
#include <alepherpdaobusiness.h>
#include <alepherpdbcommons.h>

DBBarCodePlugin::DBBarCodePlugin(QObject *parent) :
    DBBasePlugin(parent)
{
    m_initialized = false;
}

QWidget *DBBarCodePlugin::createWidget(QWidget *parent)
{
    m_widget = new DBBarCode(parent);
    return m_widget;
}

QString DBBarCodePlugin::name() const
{
    return "DBBarCode";
}

QString DBBarCodePlugin::group() const
{
    return "AlephERP";
}

QIcon DBBarCodePlugin::icon() const
{
    return QIcon(":/images/dblabel.png");
}

QString DBBarCodePlugin::toolTip() const
{
    return trUtf8("Código de barras según valor de campo");
}

QString DBBarCodePlugin::whatsThis() const
{
    return trUtf8("Código de barras según valor de campo");
}

bool DBBarCodePlugin::isContainer() const
{
    return false;
}

QString DBBarCodePlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
           " <widget class=\"DBBarCode\" name=\"dbBarCode\">\n"
           "  <property name=\"geometry\">\n"
           "   <rect>\n"
           "    <x>0</x>\n"
           "    <y>0</y>\n"
           "    <width>100</width>\n"
           "    <height>25</height>\n"
           "   </rect>\n"
           "  </property>\n"
           " </widget>\n"
           "</ui>";
}

QString DBBarCodePlugin::includeFile() const
{
    return "widgets/dbbarcode.h";
}
