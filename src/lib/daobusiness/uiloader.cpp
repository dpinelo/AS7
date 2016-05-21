/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo                                    *
 *   alepherp@alephsistemas.es                                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "uiloader.h"
#include <QtGui>
#include <alepherpdaobusiness.h>
#include "qlogger.h"

class AERPWidgetFactory
{
private:
    typedef QWidget* (*Constructor)(QWidget* parent);

public:
    template<typename T>
    static void registerClass()
    {
        constructors().insert(QString(T::staticMetaObject.className()), &constructorHelper<T>);
    }

    static bool canCreateObject(const QString &className)
    {
        return constructors().contains(className);
    }

    static QObject* createObject(const QString& className, QWidget* parent = NULL)
    {
        Constructor constructor = constructors().value(className);
        if ( constructor == NULL )
        {
            return NULL;
        }
        return (*constructor)(parent);
    }

    template<typename T>
    static QWidget* constructorHelper(QWidget* parent)
    {
        return new T (parent);
    }


    static QHash<QString, Constructor>& constructors()
    {
        static QHash<QString, Constructor> instance;
        return instance;
    }
};

AERPUiLoader::AERPUiLoader(QObject *parent) :
    QUiLoader(parent)
{
}

QUiLoader *AERPUiLoader::instance()
{
    // Debe ser un puntero. Si fuera directamente un objeto, la aplicación no arrancaría: cascaría
    // justo antes de ejecutar el main, con un mensaje:
    // QPixmap: Must construct a QApplication before a QPaintDevice
    static AERPUiLoader *m_uiLoader;

    if ( m_uiLoader == NULL )
    {
        m_uiLoader = new AERPUiLoader(qApp);
#ifdef Q_OS_ANDROID
        QString pluginDir = QString("%1/plugins/designer").arg(qApp->applicationDirPath());
        m_uiLoader->addPluginPath(pluginDir);
#endif
    }
    m_uiLoader->setTranslationEnabled(true);
    return m_uiLoader;
}

QWidget *AERPUiLoader::createWidget(const QString &className, QWidget *parent, const QString &name)
{
#ifdef Q_OS_ANDROID
    if ( !AERPWidgetFactory::canCreateObject(className) ) {
        return QUiLoader::createWidget(className, parent, name);
    }
    QWidget *widget = static_cast<QWidget*>(AERPWidgetFactory::createObject(className, parent));
    if ( widget != NULL )
    {
        widget->setObjectName(name);
    }
    return widget;
#else
    QLogger::QLog_Debug(AlephERP::stLogOther, QString("AERPUiLoader::createWidget: Creando [%1][%2]").arg(className).arg(name));
    QWidget *w = QUiLoader::createWidget(className, parent, name);
    if ( w != NULL )
    {
        QLogger::QLog_Debug(AlephERP::stLogOther, QString("AERPUiLoader::createWidget: Creado exitosamente [%1][%2]").arg(className).arg(name));
    }
    else
    {
        QLogger::QLog_Debug(AlephERP::stLogOther, QString("AERPUiLoader::createWidget: No se ha podido crear [%1][%2]").arg(className).arg(name));
    }
    return w;
#endif
}

void AERPUiLoader::registerAlephERPMetatypes()
{
    AERPWidgetFactory::registerClass<AERPMainWindow>();
    AERPWidgetFactory::registerClass<DBFormDlg>();
    AERPWidgetFactory::registerClass<DBRecordDlg>();
    AERPWidgetFactory::registerClass<AERPScheduleView>();
    AERPWidgetFactory::registerClass<DBCheckBox>();
    AERPWidgetFactory::registerClass<DBChooseRecordButton>();
    AERPWidgetFactory::registerClass<DBChooseRelatedRecordButton>();
    AERPWidgetFactory::registerClass<DBComboBox>();
    AERPWidgetFactory::registerClass<DBDateTimeEdit>();
    AERPWidgetFactory::registerClass<DBDetailView>();
    AERPWidgetFactory::registerClass<DBFileUpload>();
    AERPWidgetFactory::registerClass<DBFilterScheduleView>();
    AERPWidgetFactory::registerClass<DBLabel>();
    AERPWidgetFactory::registerClass<DBLineEdit>();
    AERPWidgetFactory::registerClass<DBListView>();
    AERPWidgetFactory::registerClass<DBNumberEdit>();
    AERPWidgetFactory::registerClass<DBRelatedElementsView>();
    AERPWidgetFactory::registerClass<DBScheduleView>();
    AERPWidgetFactory::registerClass<DBTableView>();
    AERPWidgetFactory::registerClass<DBTabWidget>();
    AERPWidgetFactory::registerClass<DBTextEdit>();
    AERPWidgetFactory::registerClass<DBTreeView>();
    AERPWidgetFactory::registerClass<AERPStackedWidget>();
    AERPWidgetFactory::registerClass<MenuTreeWidget>();
#ifdef ALEPHERP_BARCODE
    AERPWidgetFactory::registerClass<DBBarCode>();
#endif
#ifdef ALEPHERP_DEVTOOLS
    AERPWidgetFactory::registerClass<AERPSystemObjectEditorWidget>();
#endif
#ifdef ALEPHERP_ADVANCED_EDIT
    AERPWidgetFactory::registerClass<DBCodeEdit>();
    AERPWidgetFactory::registerClass<DBRichTextEdit>();
#endif
}
