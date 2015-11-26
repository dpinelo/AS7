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

#include "bugreportform.h"
#include <QApplication>
#include <QTranslator>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    // Cargamos el archivo con las traducciones
    QTranslator translator;
    QString translationFile = QString("qt_es");
    translator.load(translationFile, ":/translations/5", "_", ".qm");
    app.installTranslator(&translator);

    QString stackTraceFile;
    for (int i = 1; i < argc; ++i)
    {
        if ( qstrcmp(argv[i], "-stackFile") )
        {
            if ( (i+1) < argc )
            {
                stackTraceFile = QString(argv[i+1]);
            }
        }
    }

    if ( !stackTraceFile.isEmpty() )
    {
        BugReportForm dlg(stackTraceFile);
        dlg.setModal(true);
        dlg.show();
        app.exec();
    }

    return(0);
}
