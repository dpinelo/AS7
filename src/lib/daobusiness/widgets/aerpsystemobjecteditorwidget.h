/***************************************************************************
 *   Copyright (C) 2011 by David Pinelo                                    *
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
#ifndef AERPSYSTEMOBJECTEDITORDLG_H
#define AERPSYSTEMOBJECTEDITORDLG_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <alepherpglobal.h>

namespace Ui
{
class AERPSystemObjectEditorWidget;
}

class AERPSystemObjectEditorDlgPrivate;

class ALEPHERP_DLL_EXPORT AERPSystemObjectEditorWidget : public QWidget
{
    Q_OBJECT

private:
    Ui::AERPSystemObjectEditorWidget *ui;
    AERPSystemObjectEditorDlgPrivate *d;

protected:
    void xmlValidationMoveCursor(int line, int column);
    bool eventFilter(QObject *, QEvent *);

public:
    explicit AERPSystemObjectEditorWidget(QWidget *parent = 0);
    ~AERPSystemObjectEditorWidget();

    bool replaceTabs();
    int numSpaces();

public slots:
    void init();
    void openDesigner();
    void openCreator();
    void importBinary();
    void editReport();
    void calculatePatch();
    void readContent();
    void stackedWidgetView();
    void setTemplate();
    bool validateXML(const QByteArray &xmlSchema);
    void validateOkClicked();
    void selectOrigin();
};

Q_DECLARE_METATYPE(AERPSystemObjectEditorWidget*)

#endif // AERPSYSTEMOBJECTEDITORDLG_H
