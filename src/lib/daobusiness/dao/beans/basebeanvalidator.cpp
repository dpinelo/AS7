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
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include "basebeanvalidator.h"
#include "dao/beans/basebean.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/beansfactory.h"
#include "dao/basedao.h"
#include "dao/database.h"
#include "dao/dbfieldobserver.h"

class BaseBeanValidatorPrivate
{
public:
    /** Bean sobre el que se realizarán las validaciones */
    BaseBeanPointer m_bean;
    /** Mensaje que se va escribiendo una vez ejecutado validate */
    QString m_message;
    /** Mismo mensaje, pero en HTML */
    QString m_htmlMessage;
    /** Contiene una referencia al widget al que se le aplicará el foco, caso de que
      el dbfield asociado no pase la validación */
    QWidget *m_widget;
    /** Conexión que se utilizará */
    QString m_connection;

    BaseBeanValidatorPrivate()
    {
        m_widget = NULL;
    }

    bool checkPk();
    bool checkUniqueCompound();
};

BaseBeanValidator::BaseBeanValidator(QObject *parent) :
    QObject(parent), d(new BaseBeanValidatorPrivate)
{
    d->m_connection = Database::databaseConnectionForThisThread();
}

BaseBeanValidator::~BaseBeanValidator()
{
    delete d;
}

void BaseBeanValidator::setBean(BaseBean *bean)
{
    d->m_bean = bean;
}

BaseBeanPointer BaseBeanValidator::bean()
{
    return d->m_bean;
}

void BaseBeanValidator::setConnection(const QString &name)
{
    d->m_connection = name;
}

/*!
  Realiza la validación de los values de los fields del bean d->m_bean, a partir de la definición
  de la tabla dada
*/
bool BaseBeanValidator::validate()
{
    if ( d->m_bean.isNull() )
    {
        return false;
    }

    const QList<DBField *> fields = d->m_bean->fields();
    bool result = true;

    d->m_message.clear();
    d->m_htmlMessage.clear();
    d->m_widget = NULL;

    // Comprobemos si la primary key es única, en operaciones de inserción
    if ( d->m_bean->dbState() == BaseBean::INSERT )
    {
        result = result & d->checkPk();
        result = result & d->checkUniqueCompound();
    }
    for(DBField *fld : fields)
    {
        bool r = fld->validate();
        if ( !r )
        {
            result = false;
            d->m_message.append(fld->validateMessages());
            d->m_htmlMessage.append(fld->validateHtmlMessages());
            if ( d->m_widget == NULL )
            {
                d->m_widget = fld->widgetOnBadValidate();
            }
        }
        result = result & r;
    }
    return result;
}

/*!
  Devuelve el mensaje correspondiente a la última llamada a validate, incluyendo
  los datos por los que no se pasó la validación
*/
QString BaseBeanValidator::validateMessages()
{
    return d->m_message;
}

/*!
  Devuelve el mensaje correspondiente a la última llamada a validate, incluyendo
  los datos por los que no se pasó la validación. Lo hace en HTML
*/
QString BaseBeanValidator::validateHtmlMessages()
{
    return d->m_htmlMessage;
}

/*!
  De una mala validación, devuelve el primer widget que tenía problemas
*/
QWidget *BaseBeanValidator::widgetOnBadValidate()
{
    return d->m_widget;
}

/*!
  Chequea, en operaciones de inserción que la primary key es única
  */
bool BaseBeanValidatorPrivate::checkPk()
{
    if ( m_bean.isNull() )
    {
        return false;
    }

    bool result = true;

    QVariant pk = m_bean->pkValue();
    // Si la primary key es o contiene un valor serial, se pasa la validación automáticamente
    const QList<DBFieldMetadata *> fldPk = m_bean->metadata()->pkFields();
    for (DBFieldMetadata *fld : fldPk)
    {
        if ( fld->serial() )
        {
            return true;
        }
    }
    BaseBeanSharedPointer temp = BaseDAO::selectByPk(pk, m_bean->metadata()->tableName(), m_connection);
    if ( !temp.isNull() )
    {
        result = false;
        m_message = QObject::tr("%1\r\nYa existe un registro con el mismo identificador \303\272nico.").arg(m_message);
        m_htmlMessage = QObject::tr("%1<p>Ya existe un registro con el <strong>mismo identificador \303\272nico.</strong></p>").arg(m_htmlMessage);
    }
    return result;
}

/**
 * @brief BaseBeanValidatorPrivate::checkUniqueCompound
 * Comprueba si la combinación de fields con uniqueCompound es única en la base de datos
 * @return
 */
bool BaseBeanValidatorPrivate::checkUniqueCompound()
{
    if ( m_bean.isNull() )
    {
        return false;
    }

    bool result = true;
    QString where;
    QStringList fieldNames;

    for (DBField *fld : m_bean->fields())
    {
        if ( fld->metadata()->uniqueCompound() )
        {
            if ( !where.isEmpty() )
            {
                where.append(" AND ");
            }
            where.append(fld->sqlWhere("="));
            fieldNames.append(fld->metadata()->fieldName());
        }
    }
    if ( !where.isEmpty() )
    {
        int recordCount = BaseDAO::selectTableRecordCount(m_bean->metadata()->sqlTableName(), where);
        if ( recordCount == -1 )
        {
            result = false;
            m_message = QObject::tr("%1\r\nHa ocurrido un error pasando la validación de campos únicos compuestos: [%2].").
                    arg(m_message, BaseDAO::lastErrorMessage());
            m_htmlMessage = QObject::tr("%1<p>Ha ocurrido un error pasando la validación de campos únicos compuestos: [%2]<</p>").
                    arg(m_htmlMessage, BaseDAO::lastErrorMessage());
        }
        else if ( recordCount > 0 )
        {
            result = false;
            m_message = QObject::tr("%1\r\nYa existe otro registro con los mismos valores para: %2.").
                    arg(m_message, fieldNames.join(", "));
            m_htmlMessage = QObject::tr("%1<p>Ya existe otro registro con los mismos valores para: %2.</p>").
                    arg(m_htmlMessage, fieldNames.join(", "));
        }
    }
    return result;
}
