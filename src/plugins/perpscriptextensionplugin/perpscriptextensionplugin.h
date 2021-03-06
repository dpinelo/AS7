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
#ifndef AERPSCRIPTEXTENSIONPLUGIN_H
#define AERPSCRIPTEXTENSIONPLUGIN_H

#include <QScriptExtensionPlugin>

class AERPScriptExtensionPlugin : public QScriptExtensionPlugin
{
    Q_OBJECT

#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
    Q_PLUGIN_METADATA(IID "es.alephsistemas.alepherp.AERPScriptExtensionPluginInterface" FILE "aerpscriptextensionplugin.json")
#endif

public:
    AERPScriptExtensionPlugin(QObject * parent = 0);
    ~AERPScriptExtensionPlugin();

    void initialize ( const QString & key, QScriptEngine * engine );
    QStringList	keys () const;

};

#endif // AERPSCRIPTEXTENSIONPLUGIN_H
