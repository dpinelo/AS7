/***************************************************************************
 *   Copyright (C) 2014 by David Pinelo                                    *
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
#include "aerpapplication.h"
#include "muParser.h"
#include "qlogger.h"
#include "aerpcommon.h"

AERPApplication::AERPApplication(int &argc, char **argv) :
    QApplication(argc, argv)
{
}

AERPApplication::~AERPApplication()
{

}

/**
 * @brief AERPApplication::notify
 * Necesario porque muParser puede lanzar excepciones, y necesitamos capturarlas aquí por requerimientos de Qt
 * @param receiver
 * @param event
 * @return
 */
bool AERPApplication::notify(QObject *receiver, QEvent *event)
{
    try {
        return QApplication::notify(receiver, event);
    } catch (mu::Parser::exception_type &e) {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("AERPApplication::notify: Mu Parser Exception thrown: [%1]").arg(QString::fromStdString(e.GetMsg())));
    } catch (...) {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("AERPApplication::notify: Error General"));
    }

    return false;
}
