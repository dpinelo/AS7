#ifndef WWRICHTEXTEDITPLUGIN_H
#define WWRICHTEXTEDITPLUGIN_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,5,0))
#include <QDesignerCustomWidgetInterface>
#else
#include <QtUiPlugin/QDesignerCustomWidgetInterface>
#endif

class QwwRichTextEditPlugin: public QObject, public QDesignerCustomWidgetInterface
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)

private:
    bool m_initialized;

public:
    QwwRichTextEditPlugin(QObject *parent = 0);

    bool isContainer() const;
    bool isInitialized() const;
    QIcon icon() const;
    QString domXml() const;
    QString group() const;
    QString includeFile() const;
    QString name() const;
    QString toolTip() const;
    QString whatsThis() const;
    QWidget *createWidget(QWidget *parent);
    void initialize(QDesignerFormEditorInterface *core);

};

#endif // WWRICHTEXTEDITPLUGIN_H
