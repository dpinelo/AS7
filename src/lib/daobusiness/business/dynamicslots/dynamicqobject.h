/****************************************************************************
**
** Copyright (C) 2006 Trolltech AS. All rights reserved.
**
** This file is part of the documentation of Qt. It was originally
** published as part of Qt Quarterly.
**
** This file may be used under the terms of the GNU General Public License
** version 2.0 as published by the Free Software Foundation or under the
** terms of the Qt Commercial License Agreement. The respective license
** texts for these are provided with the open source and commercial
** editions of Qt.
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://www.trolltech.com/products/qt/licensing.html or contact the
** sales department at sales@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef DYNAMICQOBJECT_H
#define DYNAMICQOBJECT_H

#include <QtCore>

/**
 * @brief The DynamicSlot class
 * Representa al slot virtual que se creará.
 */
class DynamicSlot
{
public:
    virtual void call(QObject *sender, void **arguments) = 0;
};

/**
 * @brief The DynamicQObject class
 * Cualquier clase que desee poder tener slots virtuales o implementarlos, deberá tener una copia interna
 * de esta clase.
 */
class DynamicQObject: public QObject
{
private:
    QHash<QByteArray, int> m_slotIndices;
    QList<DynamicSlot *> m_slotList;
    QHash<QByteArray, int> m_signalIndices;
    bool m_checkConnectArgs;

public:
    explicit DynamicQObject(QObject *parent = 0);

    virtual int qt_metacall(QMetaObject::Call c, int id, void **arguments);

    bool emitDynamicSignal(char *signal, void **arguments);
    bool connectDynamicSlot(QObject *obj, const char *signal, const char *slot);
    bool connectDynamicSignal(char *signal, QObject *obj, char *slot);

    void setCheckConnectArgs(bool value);

    virtual DynamicSlot *createSlot(char *slot) = 0;

};

#endif
