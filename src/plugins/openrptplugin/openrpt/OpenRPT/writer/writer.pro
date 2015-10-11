#
# OpenRPT report writer and rendering engine
# Copyright (C) 2001-2012 by OpenMFG, LLC
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
# Please contact info@openmfg.com with any questions on this license.
#

include( ../../../../../../config.pri )

TEMPLATE = lib
TARGET   = writer

CONFIG += qt warn_on staticlib

INCLUDEPATH += ../../common ../common ../images ../../../../../
LIBS += -lwrtembed -lcommon -lrenderer -lDmtx_Library

HEADERS += reportwriterwindow.h \
           ../common/builtinSqlFunctions.h

SOURCES += ../common/builtinSqlFunctions.cpp \
           reportwriterwindow.cpp

QT += xml sql network
contains(QT_VERSION, ^4\\.[0-8]\\..*) {
    QT += gui
}
!contains(QT_VERSION, ^4\\.[0-8]\\..*) {
    QT += widgets printsupport
}

RESOURCES += writer.qrc

TRANSLATIONS = writer_fr.ts writer_it.ts writer_ru.ts writer_es.ts
