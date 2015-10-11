/***************************************************************************
 *   Copyright (C) 2011 by David Pinelo   *
 *   alepherp@alephsistemas.es   *
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

#include "aerpscriptmessagebox.h"
#include <aerpcommon.h>
#include <QtGui>

AERPScriptMessageBox::AERPScriptMessageBox(QObject *parent) :
    QObject(parent)
{
}

AERPScriptMessageBox::~AERPScriptMessageBox()
{
}

QScriptValue AERPScriptMessageBox::toScriptValue(QScriptEngine *engine, AERPScriptMessageBox * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void AERPScriptMessageBox::fromScriptValue(const QScriptValue &object, AERPScriptMessageBox * &out)
{
    out = qobject_cast<AERPScriptMessageBox *>(object.toQObject());
}

QScriptValue AERPScriptMessageBox::critical(QScriptContext *ctx, QScriptEngine *engine)
{
    QMessageBox::StandardButtons buttons;
    QMessageBox::StandardButton defaultButton;
    QString text;
    QWidget *parent;
    if ( processScriptArgs(ctx, engine, parent, text, buttons, defaultButton) )
    {
        return AERPScriptMessageBox::convertToAERPButton(QMessageBox::critical(parent, qApp->applicationName(), text, buttons, defaultButton));
    }
    return 0;
}

QScriptValue AERPScriptMessageBox::information(QScriptContext *ctx, QScriptEngine *engine)
{
    QMessageBox::StandardButtons buttons;
    QMessageBox::StandardButton defaultButton;
    QString text;
    QWidget *parent;
    if ( processScriptArgs(ctx, engine, parent, text, buttons, defaultButton) )
    {
        return AERPScriptMessageBox::convertToAERPButton(QMessageBox::information(parent, qApp->applicationName(), text, buttons, defaultButton));
    }
    return 0;
}

QScriptValue AERPScriptMessageBox::question(QScriptContext *ctx, QScriptEngine *engine)
{
    QMessageBox::StandardButtons buttons;
    QMessageBox::StandardButton defaultButton;
    QString text;
    QWidget *parent;
    if ( processScriptArgs(ctx, engine, parent, text, buttons, defaultButton) )
    {
        return AERPScriptMessageBox::convertToAERPButton(QMessageBox::question(parent, qApp->applicationName(), text, buttons, defaultButton));
    }
    return 0;
}

QScriptValue AERPScriptMessageBox::warning(QScriptContext *ctx, QScriptEngine *engine)
{
    QMessageBox::StandardButtons buttons;
    QMessageBox::StandardButton defaultButton;
    QString text;
    QWidget *parent;
    if ( processScriptArgs(ctx, engine, parent, text, buttons, defaultButton) )
    {
        return AERPScriptMessageBox::convertToAERPButton(QMessageBox::warning(parent, qApp->applicationName(), text, buttons, defaultButton));
    }
    return 0;
}

bool AERPScriptMessageBox::processScriptArgs(QScriptContext *ctx, QScriptEngine *engine, QWidget* &widget, QString &text, QMessageBox::StandardButtons &buttons, QMessageBox::StandardButton &defaultButton)
{
    if ( ctx == NULL )
    {
        return false;
    }
    text.clear();
    buttons = QMessageBox::NoButton;
    defaultButton = QMessageBox::NoButton;
    widget = NULL;

    // Debe tener al menos un argumento: el mensaje
    if ( ctx->argumentCount() < 1 )
    {
        return false;
    }
    int nextArg = 0;

    QObject *thisFormObject = engine->globalObject().property(AlephERP::stThisForm).toQObject();
    widget = qobject_cast<QWidget *>(thisFormObject);

    if ( ctx->argumentCount() > nextArg )
    {
        if ( ctx->argument(nextArg).isString() )
        {
            text = ctx->argument(nextArg).toString();
            nextArg++;
        }
    }

    if ( ctx->argumentCount() > nextArg )
    {
        if ( ctx->argument(nextArg).isNumber() )
        {
            AERPScriptMessageBox::Buttons temp;
            temp = static_cast<AERPScriptMessageBox::Buttons>(ctx->argument(nextArg).toInt32());
            if ( temp.testFlag(AERPScriptMessageBox::Ok) )
            {
                buttons |= QMessageBox::Ok;
            }
            if ( temp.testFlag(AERPScriptMessageBox::Save) )
            {
                buttons |= QMessageBox::Save;
            }
            if ( temp.testFlag(AERPScriptMessageBox::SaveAll) )
            {
                buttons |= QMessageBox::SaveAll;
            }
            if ( temp.testFlag(AERPScriptMessageBox::Open) )
            {
                buttons |= QMessageBox::Open;
            }
            if ( temp.testFlag(AERPScriptMessageBox::Yes) )
            {
                buttons |= QMessageBox::Yes;
            }
            if ( temp.testFlag(AERPScriptMessageBox::YesToAll) )
            {
                buttons |= QMessageBox::YesToAll;
            }
            if ( temp.testFlag(AERPScriptMessageBox::No) )
            {
                buttons |= QMessageBox::No;
            }
            if ( temp.testFlag(AERPScriptMessageBox::NoToAll) )
            {
                buttons |= QMessageBox::NoToAll;
            }
            if ( temp.testFlag(AERPScriptMessageBox::Abort) )
            {
                buttons |= QMessageBox::Abort;
            }
            if ( temp.testFlag(AERPScriptMessageBox::Retry) )
            {
                buttons |= QMessageBox::Retry;
            }
            if ( temp.testFlag(AERPScriptMessageBox::Ignore) )
            {
                buttons |= QMessageBox::Ignore;
            }
            if ( temp.testFlag(AERPScriptMessageBox::Close) )
            {
                buttons |= QMessageBox::Close;
            }
            if ( temp.testFlag(AERPScriptMessageBox::Cancel) )
            {
                buttons |= QMessageBox::Cancel;
            }
            if ( temp.testFlag(AERPScriptMessageBox::Discard) )
            {
                buttons |= QMessageBox::Discard;
            }
            if ( temp.testFlag(AERPScriptMessageBox::Help) )
            {
                buttons |= QMessageBox::Help;
            }
            if ( temp.testFlag(AERPScriptMessageBox::Apply) )
            {
                buttons |= QMessageBox::Apply;
            }
            if ( temp.testFlag(AERPScriptMessageBox::Reset) )
            {
                buttons |= QMessageBox::Reset;
            }
            if ( temp.testFlag(AERPScriptMessageBox::RestoreDefaults) )
            {
                buttons |= QMessageBox::RestoreDefaults;
            }
            nextArg++;
        }
    }

    if ( ctx->argumentCount() > nextArg )
    {
        if ( ctx->argument(nextArg).isNumber() )
        {
            AERPScriptMessageBox::Button temp = static_cast<AERPScriptMessageBox::Button>(static_cast<int>(ctx->argument(nextArg).toInteger()));
            if ( temp == AERPScriptMessageBox::Ok )
            {
                defaultButton = QMessageBox::Ok;
            }
            if ( temp == AERPScriptMessageBox::Save )
            {
                defaultButton = QMessageBox::Save;
            }
            if ( temp == AERPScriptMessageBox::SaveAll )
            {
                defaultButton = QMessageBox::SaveAll;
            }
            if ( temp == AERPScriptMessageBox::Open )
            {
                defaultButton = QMessageBox::Open;
            }
            if ( temp == AERPScriptMessageBox::Yes )
            {
                defaultButton = QMessageBox::Yes;
            }
            if ( temp == AERPScriptMessageBox::YesToAll )
            {
                defaultButton = QMessageBox::YesToAll;
            }
            if ( temp == AERPScriptMessageBox::No )
            {
                defaultButton = QMessageBox::No;
            }
            if ( temp == AERPScriptMessageBox::NoToAll )
            {
                defaultButton = QMessageBox::NoToAll;
            }
            if ( temp == AERPScriptMessageBox::Abort )
            {
                defaultButton = QMessageBox::Abort;
            }
            if ( temp == AERPScriptMessageBox::Retry )
            {
                defaultButton = QMessageBox::Retry;
            }
            if ( temp == AERPScriptMessageBox::Ignore )
            {
                defaultButton = QMessageBox::Ignore;
            }
            if ( temp == AERPScriptMessageBox::Close )
            {
                defaultButton = QMessageBox::Close;
            }
            if ( temp == AERPScriptMessageBox::Cancel )
            {
                defaultButton = QMessageBox::Cancel;
            }
            if ( temp == AERPScriptMessageBox::Discard )
            {
                defaultButton = QMessageBox::Discard;
            }
            if ( temp == AERPScriptMessageBox::Help )
            {
                defaultButton = QMessageBox::Help;
            }
            if ( temp == AERPScriptMessageBox::Apply )
            {
                defaultButton = QMessageBox::Apply;
            }
            if ( temp == AERPScriptMessageBox::Reset )
            {
                defaultButton = QMessageBox::Reset;
            }
            if ( temp == AERPScriptMessageBox::RestoreDefaults )
            {
                defaultButton = QMessageBox::RestoreDefaults;
            }
        }
    }
    return true;
}

AERPScriptMessageBox::Button AERPScriptMessageBox::convertToAERPButton(QMessageBox::StandardButton temp)
{
    if ( temp == QMessageBox::NoButton )
    {
        return AERPScriptMessageBox::NoButton;
    }
    if ( temp == QMessageBox::Ok )
    {
        return AERPScriptMessageBox::Ok;
    }
    if ( temp == QMessageBox::Save )
    {
        return AERPScriptMessageBox::Save;
    }
    if ( temp == QMessageBox::SaveAll )
    {
        return AERPScriptMessageBox::SaveAll;
    }
    if ( temp == QMessageBox::Open )
    {
        return AERPScriptMessageBox::Open;
    }
    if ( temp == QMessageBox::Yes )
    {
        return AERPScriptMessageBox::Yes;
    }
    if ( temp == QMessageBox::YesToAll )
    {
        return AERPScriptMessageBox::YesToAll;
    }
    if ( temp == QMessageBox::No )
    {
        return AERPScriptMessageBox::No;
    }
    if ( temp == QMessageBox::NoToAll )
    {
        return AERPScriptMessageBox::NoToAll;
    }
    if ( temp == QMessageBox::Abort )
    {
        return AERPScriptMessageBox::Abort;
    }
    if ( temp == QMessageBox::Retry )
    {
        return AERPScriptMessageBox::Retry;
    }
    if ( temp == QMessageBox::Ignore )
    {
        return AERPScriptMessageBox::Ignore;
    }
    if ( temp == QMessageBox::Close )
    {
        return AERPScriptMessageBox::Close;
    }
    if ( temp == QMessageBox::Cancel )
    {
        return AERPScriptMessageBox::Cancel;
    }
    if ( temp == QMessageBox::Discard )
    {
        return AERPScriptMessageBox::Discard;
    }
    if ( temp == QMessageBox::Help )
    {
        return AERPScriptMessageBox::Help;
    }
    if ( temp == QMessageBox::Apply )
    {
        return AERPScriptMessageBox::Apply;
    }
    if ( temp == QMessageBox::Reset )
    {
        return AERPScriptMessageBox::Reset;
    }
    if ( temp == QMessageBox::RestoreDefaults )
    {
        return AERPScriptMessageBox::RestoreDefaults;
    }
    return AERPScriptMessageBox::NoButton;
}

QScriptValue AERPScriptMessageBox::registerType(QScriptEngine *engine)
{
    engine->setDefaultPrototype(qMetaTypeId<AERPScriptMessageBox*>(), QScriptValue());
    QScriptValue proto = engine->scriptValueFromQMetaObject<AERPScriptMessageBox>();
    proto.setPrototype(engine->defaultPrototype(qMetaTypeId<QObject*>()));

    QScriptValue funCritical = engine->newFunction(AERPScriptMessageBox::critical);
    proto.setProperty("critical", funCritical);

    QScriptValue funInformaction = engine->newFunction(AERPScriptMessageBox::information);
    proto.setProperty("information", funInformaction);

    QScriptValue funQuestion = engine->newFunction(AERPScriptMessageBox::question);
    proto.setProperty("question", funQuestion);

    QScriptValue funWarning = engine->newFunction(AERPScriptMessageBox::warning);
    proto.setProperty("warning", funWarning);

    qScriptRegisterMetaType<AERPScriptMessageBox*>(engine, toScriptValue, fromScriptValue, proto);
    return proto;
}
