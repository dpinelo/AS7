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

#ifndef AERPSCRIPTMESSAGEBOX_H
#define AERPSCRIPTMESSAGEBOX_H

#include <QtCore>
#include <QtScript>
#include <QtGlobal>
#include <QtWidgets>


/**
 * @brief The AERPScriptMessageBox class
 * Interfaz para el motor QS de los QMessageBox.
 */
class AERPScriptMessageBox : public QObject, public QScriptable
{
    Q_OBJECT

public:
    explicit AERPScriptMessageBox(QObject *parent = 0);
    virtual ~AERPScriptMessageBox();

    enum Button
    {
        // keep this in sync with QDialogButtonBox::StandardButton
        NoButton           = 0x00000000,
        Ok                 = 0x00000400,
        Save               = 0x00000800,
        SaveAll            = 0x00001000,
        Open               = 0x00002000,
        Yes                = 0x00004000,
        YesToAll           = 0x00008000,
        No                 = 0x00010000,
        NoToAll            = 0x00020000,
        Abort              = 0x00040000,
        Retry              = 0x00080000,
        Ignore             = 0x00100000,
        Close              = 0x00200000,
        Cancel             = 0x00400000,
        Discard            = 0x00800000,
        Help               = 0x01000000,
        Apply              = 0x02000000,
        Reset              = 0x04000000,
        RestoreDefaults    = 0x08000000
    };
    Q_ENUM(Button)
    Q_DECLARE_FLAGS(Buttons, Button)

    static QScriptValue toScriptValue(QScriptEngine *engine, AERPScriptMessageBox* const &in);
    static void fromScriptValue(const QScriptValue &object, AERPScriptMessageBox* &out);

    static QScriptValue critical(QScriptContext *ctx, QScriptEngine *engine);
    static QScriptValue information (QScriptContext *ctx, QScriptEngine *engine);
    static QScriptValue question (QScriptContext *ctx, QScriptEngine *engine);
    static QScriptValue warning(QScriptContext *ctx, QScriptEngine *engine);

    static QScriptValue registerType(QScriptEngine *engine);

private:
    static bool processScriptArgs(QScriptContext *ctx, QScriptEngine *engine, QWidget* &widget, QString &text, QMessageBox::StandardButtons &buttons, QMessageBox::StandardButton &defaultButton);
    static Button convertToAERPButton(QMessageBox::StandardButton);

signals:

};

Q_DECLARE_OPERATORS_FOR_FLAGS(AERPScriptMessageBox::Buttons)
Q_DECLARE_METATYPE(AERPScriptMessageBox*)
Q_DECLARE_METATYPE(AERPScriptMessageBox::Button)
Q_DECLARE_METATYPE(AERPScriptMessageBox::Buttons)
Q_SCRIPT_DECLARE_QMETAOBJECT(AERPScriptMessageBox, QObject*)

#endif // AERPSCRIPTMESSAGEBOX_H
