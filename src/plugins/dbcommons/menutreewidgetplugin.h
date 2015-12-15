#ifndef MENUTREEWIDGETPLUGIN_H
#define MENUTREEWIDGETPLUGIN_H

#include <QtGui>
#if (QT_VERSION < QT_VERSION_CHECK(5,5,0))
#include <QDesignerCustomWidgetInterface>
#else
#include <QtUiPlugin/QDesignerCustomWidgetInterface>
#endif

class MenuTreeWidgetPlugin : public QObject, public QDesignerCustomWidgetInterface
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
    bool m_initialized;

public:
    MenuTreeWidgetPlugin(QObject *parent = 0);

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

#endif // MENUTREEWIDGETPLUGIN_H
