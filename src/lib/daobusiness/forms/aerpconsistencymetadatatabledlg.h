/***************************************************************************
 *   Copyright (C) 2012 by David Pinelo   *
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
#ifndef AERPCONSISTENCYMETADATATABLEDLG_H
#define AERPCONSISTENCYMETADATATABLEDLG_H

#include <QDialog>
#include <QtCore>
#include <alepherpglobal.h>

class BaseBeanMetadata;

namespace Ui
{
class AERPConsistencyMetadataTableDlg;
}

class AERPConsistencyMetadataTableDlgPrivate;

/**
 * @brief The AERPConsistencyMetadataTableDlg class
 * Formulario que muestra los datos de posibles inconsistencia y ofrece algunas soluciones.
 */
class ALEPHERP_DLL_EXPORT AERPConsistencyMetadataTableDlg : public QDialog
{
    Q_OBJECT

    friend class AERPConsistencyMetadataTableDlgPrivate;

private:
    Ui::AERPConsistencyMetadataTableDlg *ui;
    AERPConsistencyMetadataTableDlgPrivate *d;

protected:
    void closeEvent(QCloseEvent *);
    void showEvent(QShowEvent *);

public:
    explicit AERPConsistencyMetadataTableDlg(const QVariantList &err, QWidget *parent = 0);
    ~AERPConsistencyMetadataTableDlg();

public slots:
    void fix();
    bool createTable(BaseBeanMetadata *m);
};

#endif // AERPCONSISTENCYMETADATATABLEDLG_H
