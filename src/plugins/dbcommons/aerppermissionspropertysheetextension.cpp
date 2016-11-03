#include "aerppermissionspropertysheetextension.h"
#include "aerpcommon.h"

class AERPPermissionsPropertySheetExtensionPrivate
{
public:
    AERPPermissionsPropertySheetExtension *q_ptr;
    QStringList m_properties;
    QPointer<QWidget> m_widget;

    AERPPermissionsPropertySheetExtensionPrivate(AERPPermissionsPropertySheetExtension *qq) : q_ptr(qq)
    {
        m_properties.append(AlephERP::stEnabledForRoles);
        m_properties.append(AlephERP::stVisibleForRoles);
        m_properties.append(AlephERP::stEnabledForUsers);
        m_properties.append(AlephERP::stVisibleForUsers);
    }
};

AERPPermissionsPropertySheetExtension::AERPPermissionsPropertySheetExtension(QWidget *widget, QObject *parent) :
    QObject(parent),
    d(new AERPPermissionsPropertySheetExtensionPrivate(this))
{
    d->m_widget = widget;
}

AERPPermissionsPropertySheetExtension::~AERPPermissionsPropertySheetExtension()
{
    delete d;
}

int AERPPermissionsPropertySheetExtension::count() const
{
    return 6;
}

int AERPPermissionsPropertySheetExtension::indexOf(const QString &name) const
{
    return d->m_properties.indexOf(name);
}

QString AERPPermissionsPropertySheetExtension::propertyName(int index) const
{
    return d->m_properties.at(index);
}

QString AERPPermissionsPropertySheetExtension::propertyGroup(int index) const
{
    return "Default";
}

void AERPPermissionsPropertySheetExtension::setPropertyGroup(int index, const QString &group)
{

}

bool AERPPermissionsPropertySheetExtension::hasReset(int index) const
{
    return true;
}

bool AERPPermissionsPropertySheetExtension::reset(int index)
{
    return true;
}

bool AERPPermissionsPropertySheetExtension::isVisible(int index) const
{
    return true;
}

void AERPPermissionsPropertySheetExtension::setVisible(int index, bool b)
{

}

bool AERPPermissionsPropertySheetExtension::isAttribute(int index) const
{
    return true;

}

void AERPPermissionsPropertySheetExtension::setAttribute(int index, bool b)
{

}

QVariant AERPPermissionsPropertySheetExtension::property(int index) const
{
    return QVariant();
}

void AERPPermissionsPropertySheetExtension::setProperty(int index, const QVariant &value)
{

}

bool AERPPermissionsPropertySheetExtension::isChanged(int index) const
{
    return false;
}

void AERPPermissionsPropertySheetExtension::setChanged(int index, bool changed)
{

}
