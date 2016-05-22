/***************************************************************************
 *   Copyright (C) 2010 by David Pinelo   *
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
#ifndef AERPBASEDIALOG_P_H
#define AERPBASEDIALOG_P_H

#include <QtGlobal>
#include <QtScript>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include "scripts/perpscript.h"

class WaitWidget;
class AERPBaseDialog;
class BaseBeanMetadata;

class AERPBaseDialogPrivate
{
public:
    Q_DECLARE_PUBLIC(AERPBaseDialog)
    /** Nombre del bean que edita este formulario, si edita alguno */
    QString m_tableName;
    /** Widget para simular la espera */
    WaitWidget *m_waitWidget;
    /** Indicará si el formulario se ha abierto correctamente */
    bool m_openSuccess;
    /** Motor de script para las funciones */
    QPointer<AERPScriptQsObject> m_engine;
    /** Datos pasados a este Diálogo desde QS */
    QHash<QString, QObject *> m_objects;
    QHash<QString, QVariant > m_data;
    /** En las búsquedas, especialmente, si se hacen por SQL, interesa bastante saber el isntante de la última pulsación
     * de una tecla, para así no saturar de peticiones intermedias a la base de datos */
    QDateTime m_lastKeyPressTimeStamp;
    QString m_contextName;

    AERPBaseDialog *q_ptr;

    explicit AERPBaseDialogPrivate(AERPBaseDialog * qq) : q_ptr(qq)
    {
        m_waitWidget = 0;
        m_openSuccess = false;
        m_engine = new AERPScriptQsObject();
    }

    ~AERPBaseDialogPrivate()
    {
        m_engine->deleteLater();
    }

    QHash<int, QWidgetList> setupDBRecordDlg(BaseBeanMetadata *metadata);
    QHash<int, QWidgetList> setupDBSearchDlg(BaseBeanMetadata *metadata);
    QComboBox * createComboOperators(const QString &fld);
};


#endif // AERPBASEDIALOG_P_H
