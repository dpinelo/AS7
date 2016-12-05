/***************************************************************************
 *   Copyright (C) 2007 by David Pinelo                                    *
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
#include "relatedtreeitem.h"
#include <aerpcommon.h>
#include "business/smtp/emailobject.h"
#include "dao/beans/basebean.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/relatedelement.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/relateddao.h"
#include "configuracion.h"
#include "globales.h"

class RelatedTreeItemPrivate
{
public:
    RelatedTreeItem::ItemType m_type;
    BaseBeanPointer m_rootBean;
    QPointer<RelatedElement> m_element;
    QPointer<DBRelation> m_dbRelation;
    RelatedTreeItem *m_parent;
    QList<RelatedTreeItem> m_childItems;
    QPixmap m_pixmap;
    int m_attachmentRow;

    RelatedTreeItemPrivate()
    {
        m_attachmentRow = 0;
        m_type = RelatedTreeItem::Document;
        m_parent = NULL;
    }

};

/**
 * @brief RelatedTreeItem::RelatedTreeItem
 * Este será el constructor que se usará para los items que aglutinan a otros. Como las "cabeceras".
 * @param type
 * @param bean
 * @param parent
 */
RelatedTreeItem::RelatedTreeItem(RelatedTreeItem::ItemType type, int childCount, RelatedTreeItem *parent) :
    BeanTreeItem(BaseBeanSharedPointer(), static_cast<BeanTreeItem *>(parent)), d(new RelatedTreeItemPrivate)
{
    d->m_type = type;
    setChildCount(childCount);
    d->m_parent = parent;
}

RelatedTreeItem::RelatedTreeItem(RelatedTreeItem::ItemType type, BaseBeanPointer bean, RelatedTreeItem *parent) :
    BeanTreeItem(BaseBeanSharedPointer(), static_cast<BeanTreeItem *>(parent)), d(new RelatedTreeItemPrivate)
{
    d->m_type = type;
    d->m_parent = parent;
    d->m_rootBean = bean;
}

/**
 * @brief RelatedTreeItem::RelatedTreeItem
 * Este será el item que se utilizará para representar a un elemetnto que proviene de registro relacionado
 * @param element
 * @param childCount
 * @param parent
 */
RelatedTreeItem::RelatedTreeItem(RelatedElement *element, int childCount, RelatedTreeItem *parent) :
    BeanTreeItem(BaseBeanSharedPointer(), static_cast<BeanTreeItem *>(parent)), d(new RelatedTreeItemPrivate)
{
    setChildCount(childCount);
    if ( element != NULL )
    {
        d->m_element = QPointer<RelatedElement> (element);
        if ( element->type() == AlephERP::Record )
        {
            d->m_type = RelatedTreeItem::Record;
        }
        else if ( element->type() == AlephERP::Email )
        {
            d->m_type = RelatedTreeItem::Email;
        }
        else if ( element->type() == AlephERP::Document )
        {
            d->m_type = RelatedTreeItem::Document;
        }
    }
}

RelatedTreeItem::RelatedTreeItem(ItemType type, DBRelation *relation, int childCount, RelatedTreeItem *parent) :
    BeanTreeItem(BaseBeanSharedPointer(), static_cast<BeanTreeItem *>(parent)), d(new RelatedTreeItemPrivate)
{
    d->m_rootBean = qobject_cast<BaseBean *> (relation->parent());
    d->m_dbRelation = relation;
    d->m_type = type;
    setChildCount(childCount);
}

RelatedTreeItem::RelatedTreeItem(ItemType type, BaseBeanPointer bean, int childCount, RelatedTreeItem *parent) :
    BeanTreeItem(BaseBeanSharedPointer(), static_cast<BeanTreeItem *>(parent)), d(new RelatedTreeItemPrivate)
{
    setChildCount(childCount);
    d->m_rootBean = bean;
    d->m_type = type;
}

RelatedTreeItem::~RelatedTreeItem()
{
    delete d;
}

int RelatedTreeItem::columnCount() const
{
    return 1;
}

int RelatedTreeItem::childCount() const
{
    if ( !d->m_rootBean.isNull() )
    {
        if ( d->m_type == RelatedTreeItem::RecordRoot )
        {
            return d->m_rootBean->countRelatedElements(AlephERP::Record, AlephERP::PointToAll);
        }
        else if ( d->m_type == RelatedTreeItem::EmailRoot )
        {
            return d->m_rootBean->countRelatedElements(AlephERP::Email, AlephERP::PointToAll);
        }
        else if ( d->m_type == RelatedTreeItem::DocumentRoot )
        {
            return d->m_rootBean->countRelatedElements(AlephERP::Document, AlephERP::PointToAll);
        }
    }
    return TreeItem::childCount();
}

BaseBeanPointer RelatedTreeItem::bean() const
{
    return d->m_rootBean;
}

QPointer<DBRelation> RelatedTreeItem::dbRelation() const
{
    return d->m_dbRelation;
}

RelatedTreeItem::ItemType RelatedTreeItem::type() const
{
    return d->m_type;
}

void RelatedTreeItem::setType(RelatedTreeItem::ItemType type)
{
    d->m_type = type;
}

RelatedElement *RelatedTreeItem::relatedElement()
{
    return d->m_element;
}

void RelatedTreeItem::setAttachmentRow(int row)
{
    d->m_attachmentRow = row;
}

int RelatedTreeItem::attachmentRow()
{
    return d->m_attachmentRow;
}

QPixmap RelatedTreeItem::image() const
{
    if ( d->m_pixmap.isNull() )
    {
        if ( d->m_type == RelatedTreeItem::RecordRoot )
        {
            d->m_pixmap = QPixmap(":/generales/images/record.png");
        }
        else if ( d->m_type == RelatedTreeItem::DBRelationRoot )
        {
            d->m_pixmap = QPixmap(":/generales/images/folder.png");
        }
        else if ( d->m_type == RelatedTreeItem::EmailRoot )
        {
            d->m_pixmap = QPixmap(":/generales/images/email.png");
        }
        else if ( d->m_type == RelatedTreeItem::DocumentRoot )
        {
            d->m_pixmap = QPixmap(":/generales/images/document.png");
        }
        else if ( d->m_type == RelatedTreeItem::Record || d->m_type == RelatedTreeItem::Email || d->m_type == RelatedTreeItem::Document )
        {
            d->m_pixmap = d->m_element->pixmap();
        }
        else if ( d->m_type == RelatedTreeItem::EmailAttachment )
        {
#ifdef ALEPHERP_SMTP_SUPPORT
            if ( d->m_attachmentRow >= 0 && d->m_attachmentRow < d->m_element->email().attachmentCount() )
            {
                ::EmailAttachment attachment;
                attachment = d->m_element->email().attachments().at(d->m_attachmentRow);
                d->m_pixmap = CommonsFunctions::pixmapFromMimeType(attachment.type());
            }
#endif
        }
        else if ( d->m_type == RelatedTreeItem::DBRelationChildRecord && !d->m_rootBean.isNull() )
        {
            d->m_pixmap = d->m_rootBean->metadata()->pixmap().isNull() ? QPixmap(":/generales/images/record.png") : d->m_rootBean->metadata()->pixmap();
        }
        if ( !d->m_pixmap.isNull() )
        {
            d->m_pixmap = d->m_pixmap.scaled(QSize(16,16));
        }
    }
    return d->m_pixmap;
}

QVariant RelatedTreeItem::data(int column, int role) const
{
    QFont font;

    switch ( role )
    {
    case Qt::DisplayRole:
        if ( column == 0 )
        {
            if ( d->m_type == RelatedTreeItem::RecordRoot )
            {
                return QObject::tr("Registros relacionados (%1)").arg(alephERPSettings->locale()->toString(childCount()));
            }
            else if ( d->m_type == RelatedTreeItem::EmailRoot )
            {
                return QObject::tr("Correos electrónicos enviados (%1)").arg(alephERPSettings->locale()->toString(childCount()));
            }
            else if ( d->m_type == RelatedTreeItem::DocumentRoot )
            {
                return QObject::tr("Documentos relacionados (%1)").arg(alephERPSettings->locale()->toString(childCount()));
            }
            else if ( d->m_type == RelatedTreeItem::DBRelationRoot && !d->m_dbRelation.isNull() )
            {
                BaseBeanMetadata *relBean = BeansFactory::metadataBean(d->m_dbRelation->metadata()->tableName());
                if ( relBean != NULL )
                {
                    return QObject::tr("Registros descendientes o ancestros: %1 (%2)").arg(relBean->alias()).arg(alephERPSettings->locale()->toString(childCount()));
                }
                else
                {
                    return QString("");
                }
            }
            else if ( !d->m_element.isNull() && (d->m_type == RelatedTreeItem::Record || d->m_type == RelatedTreeItem::Email || d->m_type == RelatedTreeItem::Document ) )
            {
                // Si es un documento, nos aseguramos que se carga
                if ( d->m_type == RelatedTreeItem::Document )
                {
#ifdef ALEPHERP_DOC_MANAGEMENT
                    AERPDocMngmntDocument *doc = d->m_element->document();
                    if ( doc == NULL )
                    {
                        return QObject::tr("Error obteniendo el documento");
                    }
#endif
                }
                return d->m_element->displayText();
            }
            else if ( !d->m_element.isNull() && d->m_type == RelatedTreeItem::EmailAttachment )
            {
#ifdef ALEPHERP_SMTP_SUPPORT
                if ( d->m_attachmentRow < 0 || d->m_attachmentRow >= d->m_element->email().attachmentCount() )
                {
                    return "";
                }
                ::EmailAttachment attachment = d->m_element->email().attachments().at(d->m_attachmentRow);
                double sizeOnMb = attachment.sizeOnBytes() / (1024*1024);
                QString name = QString("%1 (%2)").arg(attachment.emailFileName()).arg(CommonsFunctions::sizeHuman(sizeOnMb));
                return name;
#else
                return QString();
#endif
            }
            else if ( d->m_type == RelatedTreeItem::DBRelationChildRecord )
            {
                if ( !d->m_rootBean.isNull() )
                {
                    return d->m_rootBean->toString();
                }
                else
                {
                    return QString("");
                }
            }
        }
        break;

    case Qt::TextAlignmentRole:
        return int(Qt::AlignLeft | Qt::AlignVCenter);
        break;

    case AlephERP::MouseCursorRole:
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
        if ( column == 0 )
        {
            return Qt::PointingHandCursor;
        }
        else
        {
            return Qt::ArrowCursor;
        }
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
        if ( column == 0 )
        {
            return static_cast<int>(Qt::PointingHandCursor);
        }
        else
        {
            return static_cast<int>(Qt::ArrowCursor);
        }
#endif
        break;

    case Qt::FontRole:
        if ( d->m_type == RelatedTreeItem::Record )
        {
            if ( !d->m_element.isNull() && (d->m_element->relatedBean().isNull() || !d->m_element->found()) )
            {
                font.setStrikeOut(true);
            }
        }
        else if ( d->m_type == RelatedTreeItem::Email )
        {
            if ( !d->m_element.isNull() && !d->m_element->found() )
            {
                font.setStrikeOut(true);
            }
        }
        else if ( d->m_type == RelatedTreeItem::Document )
        {
#ifdef ALEPHERP_DOC_MANAGEMENT
            if ( !d->m_element.isNull() && (d->m_element->document() == NULL || !d->m_element->found()) )
            {
                font.setStrikeOut(true);
            }
#endif
        }
        return font;
        break;

    case Qt::BackgroundRole:
        break;

    case Qt::ForegroundRole:
        break;

    case Qt::DecorationRole:
        return image();
        break;

    case Qt::ToolTipRole:
        if ( d->m_type == RelatedTreeItem::RecordRoot || d->m_type == RelatedTreeItem::EmailRoot || d->m_type == RelatedTreeItem::DocumentRoot )
        {
            return QVariant();
        }
        else if ( !d->m_element.isNull() )
        {
            if ( d->m_type == RelatedTreeItem::Document )
            {
#ifdef ALEPHERP_DOC_MANAGEMENT
                AERPDocMngmntDocument *doc = d->m_element->document();
                if ( doc == NULL )
                {
                    return QObject::tr("Error obteniendo el documento.");
                }
                if ( !d->m_element->found() )
                {
                    return QObject::tr("Elemento borrado.");
                }
#endif
            }
            else if ( d->m_type == RelatedTreeItem::Record )
            {
                if ( d->m_element->relatedBean().isNull() || !d->m_element->found() )
                {
                    return QObject::tr("Elemento borrado.");
                }
            }
            else if ( d->m_type == RelatedTreeItem::Email )
            {
                if ( !d->m_element.isNull() && !d->m_element->found() )
                {
                    return QObject::tr("Elemento borrado.");
                }
            }
            return d->m_element->tooltipText();
        }
        break;

    default:
        break;
    }
    return QVariant();
}
