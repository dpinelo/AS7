/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the Graphics Dojo project on Qt Labs.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 or 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/
#include "openrptplugin.h"
#include "openrptscriptobject.h"
#include <QSharedPointer>
#include <QMetaType>
#include <QScriptEngine>
#include <QtPlugin>
#include <QDebug>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif

#define KEY_EXTENSION	"alepherp.openrpt"

OpenRPTPlugin::OpenRPTPlugin(QObject *parent) :
    QScriptExtensionPlugin(parent)
{
}

OpenRPTPlugin::~OpenRPTPlugin()
{
}

QStringList	OpenRPTPlugin::keys () const
{
    QStringList result;
    result.append(KEY_EXTENSION);
    return result;
}

void OpenRPTPlugin::initialize ( const QString & key, QScriptEngine * engine )
{
    if ( key == KEY_EXTENSION )
    {
        Q_INIT_RESOURCE(OpenRPTCommon);
        QScriptValue openRPTValue = engine->scriptValueFromQMetaObject<OpenRPTScriptObject>();
        engine->globalObject().setProperty("OpenRPT", openRPTValue);
        qScriptRegisterMetaType(engine, OpenRPTScriptObject::toScriptValue, OpenRPTScriptObject::fromScriptValue);
        qScriptRegisterMetaType(engine, OpenRPTScriptObject::toScriptValueSharedPointer, OpenRPTScriptObject::fromScriptValueSharedPointer);

        QScriptValue functionJS = engine->newFunction(createOpenRPT);
        engine->globalObject().setProperty("createOpenRPT", functionJS);
    }
}

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
Q_EXPORT_PLUGIN2(openrptplugin, OpenRPTPlugin)
#endif

