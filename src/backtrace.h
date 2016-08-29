/***************************************************************************
 *   Copyright (C) 2007 by David Pinelo   *
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
#ifndef BACKTRACE_H
#define BACKTRACE_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <alepherpglobal.h>

#ifndef Q_OS_WIN
#include "execinfo.h"
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#include <DbgHelp.h>
#define PACKAGE foo
#include <bfd.h>

struct bfd_ctx {
    bfd *handle;
    asymbol **symbol;
};

struct bfd_set {
    char *name;
    struct bfd_ctx *bc;
    struct bfd_set *next;
};

struct find_info {
    asymbol **symbol;
    bfd_vma counter;
    const char *file;
    const char *func;
    unsigned line;
};
#endif

#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
void startLinuxCrashHandler(int signal);
#endif
#ifdef Q_OS_WIN
LONG WINAPI windowsExceptionFilter(LPEXCEPTION_POINTERS info);
void windowsStackTrace(QFile &output);
#endif

#endif // BACKTRACE_H
