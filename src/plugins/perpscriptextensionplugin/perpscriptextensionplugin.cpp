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
#include "perpscriptextensionplugin.h"
#include "imposition.h"
#include <QSharedPointer>
#include <QMetaType>
#include <QScriptEngine>
#include <QtPlugin>

#define KEY_PERP	"alepherp"

Q_SCRIPT_DECLARE_QMETAOBJECT(Imposition, QObject*)
Q_DECLARE_METATYPE(QSharedPointer<Imposition>)
Q_DECLARE_METATYPE(Imposition::ImpositionTypes)
Q_DECLARE_METATYPE(Imposition::ImpositionMode)

AERPScriptExtensionPlugin::AERPScriptExtensionPlugin(QObject *parent) :
    QScriptExtensionPlugin(parent)
{
}

AERPScriptExtensionPlugin::~AERPScriptExtensionPlugin()
{
}

QStringList	AERPScriptExtensionPlugin::keys () const
{
    QStringList result;
    result.append(KEY_PERP);
    return result;
}

void AERPScriptExtensionPlugin::initialize ( const QString & key, QScriptEngine * engine )
{
    if ( key == KEY_PERP )
    {
        QScriptValue impositionValue = engine->scriptValueFromQMetaObject<Imposition>();
        engine->globalObject().setProperty("Imposition", impositionValue);
        qScriptRegisterMetaType(engine, Imposition::toScriptValueSharedPointer, Imposition::fromScriptValueSharedPointer);
    }
}

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
Q_EXPORT_PLUGIN2(aerpscriptextensionplugin, AERPScriptExtensionPlugin)
#endif

