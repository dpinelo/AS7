/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo                                    *
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
#ifndef IMPORTSYSTEMITEMS_H
#define IMPORTSYSTEMITEMS_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <alepherpglobal.h>

namespace Ui
{
class SystemItemsDlg;
}

class SystemItemsPrivate;
class BaseBeanMetadata;

class ALEPHERP_DLL_EXPORT SystemItemsDlg : public QDialog
{
    Q_OBJECT

    friend class SystemItemsPrivate;

private:
    Ui::SystemItemsDlg *ui;
    SystemItemsPrivate *d;

protected:
    void closeEvent(QCloseEvent *event);
    void showEvent(QShowEvent *event);
    void resizeToSavedDimension();
    void hideEvent(QHideEvent *event);

public:
    explicit SystemItemsDlg(const QString &moduleName, const QList<QHash<QString, QString> > &items,
                            const QString &moduleDirPath, QWidget *parent = 0);
    explicit SystemItemsDlg(const QString &module, const QString &type, QWidget *parent = 0);
    ~SystemItemsDlg();

    QList<QHash<QString, QString> > selectedItems();
    QStringList selectedTables();
    void setCanShowDiff(bool value);
    void hideModuleSelecion();
    void setLabel(const QString &message);

protected slots:
    void showTables(const QString &moduleId);

public slots:
    void ok();
    void cancel();
    void showDiff();
    void checkAll();
    void uncheckAll();
    void backDiff();
    void applyIndividualDiff(QTableWidgetItem *selectedItem);
};

#endif // IMPORTSYSTEMITEMS_H
