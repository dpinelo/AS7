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

#ifndef globalesH
#define globalesH

#include <math.h>
#include <vector>
#include <string>
#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <alepherpglobal.h>
#ifdef ALEPHERP_DEVTOOLS
#include <diff/diff_match_patch.h>
#endif

// Precisión de los campos DOUBLE en base de datos
#define DB_DOUBLE_PRECISION	5

#define LOOK_FUSION		0
#define LOOK_CLEANLOOK	1
#define LOOK_MAC		2
#define LOOK_PLASTIQUE	4
#define LOOK_WINDOWS	5
#define LOOK_VISTA		6
#define LOOK_XP			8
#define LOOK_GTK		9

class AERPBaseDialog;
#ifdef ALEPHERP_SMTP_SUPPORT
class AERPSMTPIface;
#endif
#ifdef ALEPHERP_DOC_MANAGEMENT
class AERPDocumentDAOPluginIface;
class AERPScannerIface;
class AERPExtractPlainContentIface;
#endif
#ifdef ALEPHERP_QTDESIGNERBUILTIN
class QDesignerWorkbench;
#endif


#define AERP_CHECK_INDEX_OK(index, vector) (index >= 0 && index < vector.size())

/**
 * @brief The CommonsFunctions class
 * Funciones transversales o comunes a la aplicación.
 */
class ALEPHERP_DLL_EXPORT CommonsFunctions : public QObject
{

    Q_OBJECT

private:

public:
    explicit CommonsFunctions(QObject *parent = 0);

    static void processEvents();

    static bool cifValid (const QString &cif);
    static bool nifValid (const QString &nif);
    static bool creditCardValidLuhnTest(const QString &creditCard);

    static QString dateFormat();
    static QString dateTimeFormat();

    static bool openPDF (QUrl urlFile);

    static void centerWindow (QDialog *dlg);
    static QDialog *parentDialog(QWidget *wid);
    static AERPBaseDialog * aerpParentDialog(QObject *widget);
    static QWidget *firstFocusWidget(QWidget *widget);

    static QString sizeHuman(double sizeOnBytes);

    static void removeContentsFromDir(const QString &dirName, const QStringList &exceptions);
    static bool capsOn();

    static QString mimeType(const QString &absolutePath);
    static QIcon iconFromMimeType(const QString &mimeType);
    static QPixmap pixmapFromMimeType(const QString &mimeType);
    static QString prefixFromMimeType(const QString &type);

    static QStringList recursiveDirEntryList(const QString &path);

    static double round(double value, int Digits);

    static void setOverrideCursor(const QCursor & cursor);
    static void restoreOverrideCursor();

    static int countDirsAndFiles(const QString &absolutePath);
    static bool removeDir(const QString &dirName);

    static QString removeAccents(const QString &s);

    static QString processToHtml(const QString &s);

#ifdef ALEPHERP_DEVTOOLS
    static QString prettyDiff(const QList<Diff> &diffs);
    static QString prettyDiffResult(const QList<Diff> &diffs);
#ifdef ALEPHERP_QTDESIGNERBUILTIN
    static QDesignerWorkbench *openQtDesigner(const QString &file);
#endif
#endif

    static QString timeToFormat(int seconds);

    static QColor contrastedColor(const QColor &bg, const QColor &fg);

#ifdef ALEPHERP_SMTP_SUPPORT
    static AERPSMTPIface * loadSMTPPlugin(const QString &pluginName, QString &error);
#endif

#ifdef ALEPHERP_DOC_MANAGEMENT
    static AERPDocumentDAOPluginIface * documentDAOPluginInstance(QString &error);
    static bool initRepo(QString &error);
    static bool isRepoInit(QString &error);
    static bool connectDocRepo(QString &error);
    static bool disconnectDocRepo(QString &error);

    static AERPScannerIface * loadScannerPlugin(QString &error);
    static AERPExtractPlainContentIface * loadPlainContentPlugin(QString &error);
#endif

};


#endif
