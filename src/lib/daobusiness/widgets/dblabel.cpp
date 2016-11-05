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
#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include "dblabel.h"
#include "globales.h"
#include "forms/perpbasedialog.h"
#include "dao/observerfactory.h"
#include "dao/dbfieldobserver.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/basebean.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/basedao.h"
#include "forms/dbrecorddlg.h"
#include "qlogger.h"

class DBLabelPrivate
{
public:
    DBLabel *q_ptr;
    bool m_link;

    explicit DBLabelPrivate(DBLabel *qq) : q_ptr(qq)
    {
        m_link = false;
    }
};

DBLabel::DBLabel(QWidget *parent) :
    QLabel(parent),
    DBBaseWidget()
{
    d = new DBLabelPrivate(this);
    connect(this, SIGNAL(linkActivated(QString)), this, SLOT(openLink(QString)));
}

DBLabel::~DBLabel()
{
    emit destroyed(this);
    delete d;
}

void DBLabel::setValue(const QVariant &value)
{
    AERPBaseDialog *thisForm = CommonsFunctions::aerpParentDialog(this);
    if ( thisForm )
    {
        QString qsName = QString("%1Text").arg(objectName());
        QScriptValue result = thisForm->callQSMethod(qsName, value);
        if ( result.isValid() && !result.toString().isEmpty() )
        {
            QLabel::setText(result.toString());
            return;
        }
    }

    if ( d->m_link )
    {
        QString link = QString("<a href='%1'>%1</a>").arg(value.toString());
        QLabel::setText(link);
    }
    else
    {
        QLabel::setText(value.toString());
    }
}

/*!
  Ajusta los parámetros de visualización de este Widget en función de lo definido en DBField m_field
  */
void DBLabel::applyFieldProperties()
{

}

QVariant DBLabel::value()
{
    QVariant v;
    return v;
}

bool DBLabel::link() const
{
    return d->m_link;
}

void DBLabel::setLink(bool value)
{
    d->m_link = value;
}

DBField *DBLabel::field()
{
    if ( !observer(false) )
    {
        return NULL;
    }
    DBField *fld = qobject_cast<DBField *>(observer(false)->entity());
    return fld;
}

BaseBeanPointer DBLabel::bean()
{
    if ( field() != NULL )
    {
        return field()->bean();
    }
    return NULL;
}

void DBLabel::observerUnregistered()
{
    DBBaseWidget::observerUnregistered();
    bool blockState = blockSignals(true);
    this->clear();
    blockSignals(blockState);
}

void DBLabel::openLink(const QString &link)
{
    if ( !link.isEmpty() && link.startsWith("#") )
    {
        // Comprobamos si el link tiene el formato adecuado, que será
        // #tablename;OID-o-ID de base de datos
        QStringList parts = link.split(";");
        if ( parts.size() < 2 )
        {
            return;
        }
        QString tableName = parts.at(0);
        tableName = tableName.replace("#", "");
        QString stOidOrWhere = parts.at(1);
        bool ok;
        qlonglong idOrOid = stOidOrWhere.toLongLong(&ok);
        BaseBeanMetadata *m = BeansFactory::instance()->metadataBean(tableName);
        if ( m == NULL || !ok )
        {
            QLogger::QLog_Debug(AlephERP::stLogOther,
                                trUtf8("El link [%] no corresponde a formato válido de apertura."));
            return;
        }
        BaseBeanSharedPointer bean = BeansFactory::instance()->newQBaseBean(m->tableName(), false);
        if ( bean.isNull() )
        {
            return;
        }
        if ( !BaseDAO::selectByOid(idOrOid, bean.data()) )
        {
            if ( !BaseDAO::selectFirst(bean.data(), stOidOrWhere, QString("")) )
            {
                QLogger::QLog_Debug(AlephERP::stLogOther,
                                    trUtf8("El link [%] no corresponde a formato válido de apertura."));
                return;
            }
        }
        if ( bean->dbState() == BaseBean::UPDATE )
        {
            QScopedPointer<DBRecordDlg> dlg (new DBRecordDlg(bean.data(),
                                                             AlephERP::ReadOnly,
                                                             true));
            if ( dlg->openSuccess() && dlg->init() )
            {
                dlg->setModal(true);
                dlg->show();
            }
        }
    }
    if ( observer() == NULL )
    {
        return;
    }
    DBField *fld = qobject_cast<DBField *>(observer()->entity());
    if ( fld != NULL && fld->bean() != beanFromContainer() )
    {
        QScopedPointer<DBRecordDlg> dlg (new DBRecordDlg(fld->bean().data(), AlephERP::ReadOnly, false, this));
        if ( dlg->openSuccess() && dlg->init() )
        {
            dlg->setModal(true);
            dlg->exec();
        }
    }
}

/*!
  Pide un nuevo observador si es necesario
  */
void DBLabel::refresh()
{
    observer();
    if ( m_observer != NULL )
    {
        m_observer->sync();
    }
}

/*!
  Tenemos que decirle al motor de scripts, que DBFormDlg se convierte de esta forma a un valor script
  */
QScriptValue DBLabel::toScriptValue(QScriptEngine *engine, DBLabel * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBLabel::fromScriptValue(const QScriptValue &object, DBLabel * &out)
{
    out = qobject_cast<DBLabel *>(object.toQObject());
}

