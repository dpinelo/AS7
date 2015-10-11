/***************************************************************************
 *   http://qt-project.org/wiki/Qt_thread-safe_singleton                   *
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
#ifndef CALL_ONCE_H
#define CALL_ONCE_H

#include <QtCore>

namespace CallOnce
{
enum ECallOnce
{
    CO_Request,
    CO_InProgress,
    CO_Finished
};

Q_GLOBAL_STATIC(QThreadStorage<QAtomicInt*>, once_flag)
}

template <class Function> inline static void qCallOnce(Function func, QBasicAtomicInt& flag)
{
    using namespace CallOnce;

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
    int protectFlag = flag.fetchAndStoreAcquire(flag);
#elif (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
    int protectFlag = flag.fetchAndStoreAcquire(flag.load());
#endif

    if (protectFlag == CO_Finished)
    {
        return;
    }
    if (protectFlag == CO_Request && flag.testAndSetRelaxed(protectFlag, CO_InProgress))
    {
        func();
        flag.fetchAndStoreRelease(CO_Finished);
    }
    else
    {
        do
        {
            QThread::yieldCurrentThread();
        }
        while (!flag.testAndSetAcquire(CO_Finished, CO_Finished));
    }
}

template <class Function> inline static void qCallOncePerThread(Function func)
{
    using namespace CallOnce;
    if (!once_flag()->hasLocalData())
    {
        once_flag()->setLocalData(new QAtomicInt(CO_Request));
        qCallOnce(func, *once_flag()->localData());
    }
}

#endif // CALL_ONCE_H
