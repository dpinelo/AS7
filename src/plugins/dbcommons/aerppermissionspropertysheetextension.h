#ifndef AERPPERMISSIONSPROPERTYSHEETEXTENSION_H
#define AERPPERMISSIONSPROPERTYSHEETEXTENSION_H

#include <QObject>
#include <QExtensionFactory>
#include <QDesignerPropertySheetExtension>

class AERPPermissionsPropertySheetExtensionPrivate;

class AERPPermissionsPropertySheetExtension : public QObject,
        public QDesignerPropertySheetExtension
{
    Q_OBJECT
    Q_INTERFACES(QDesignerPropertySheetExtension)

    friend class AERPPermissionsPropertySheetExtensionPrivate;

private:
    AERPPermissionsPropertySheetExtensionPrivate *d;

public:
    AERPPermissionsPropertySheetExtension(QWidget *widget, QObject *parent = 0);
    virtual ~AERPPermissionsPropertySheetExtension();

    virtual int count() const;

    virtual int indexOf(const QString &name) const;

    virtual QString propertyName(int index) const;
    virtual QString propertyGroup(int index) const;
    virtual void setPropertyGroup(int index, const QString &group);

    virtual bool hasReset(int index) const;
    virtual bool reset(int index);

    virtual bool isVisible(int index) const;
    virtual void setVisible(int index, bool b);

    virtual bool isAttribute(int index) const;
    virtual void setAttribute(int index, bool b);

    virtual QVariant property(int index) const;
    virtual void setProperty(int index, const QVariant &value);

    virtual bool isChanged(int index) const;
    virtual void setChanged(int index, bool changed);

};

#endif // AERPPERMISSIONSPROPERTYSHEETEXTENSION_H
