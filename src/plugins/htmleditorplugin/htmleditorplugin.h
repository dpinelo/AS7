#ifndef HTMLEDITORPLUGIN_H
#define HTMLEDITORPLUGIN_H

#include <QDesignerCustomWidgetInterface>

class HtmlEditorPlugin: public QObject, public QDesignerCustomWidgetInterface
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)

#if (QT_VERSION > 0x050000)
    Q_PLUGIN_METADATA(IID "es.alephsistemas.alepherp.HtmlEditorPluginInterface" FILE "htmleditorplugin.json")
#endif

private:
    bool m_initialized;

public:
    HtmlEditorPlugin(QObject *parent = 0);

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

#endif // HTMLEDITORPLUGIN_H
