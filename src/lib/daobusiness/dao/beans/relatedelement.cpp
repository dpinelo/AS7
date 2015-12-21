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
#include <QtXml>
#include "relatedelement.h"
#include "globales.h"
#include "qlogger.h"
#include "dao/beans/basebean.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/basedao.h"
#include "dao/aerptransactioncontext.h"
#include "models/envvars.h"
#include "business/aerploggeduser.h"
#ifdef ALEPHERP_SMTP_SUPPORT
#include "dao/emaildao.h"
#endif
#ifdef ALEPHERP_DOC_MANAGEMENT
#include "dao/beans/aerpdocmngmntdocument.h"
#include "business/docmngmnt/aerpdocumentdaowrapper.h"
#endif

#include "configuracion.h"

class RelatedElementPrivate
{
public:
    RelatedElement *q_ptr;
    int m_id;
    bool m_modified;
    AlephERP::RelatedElementTypes m_type;
    BaseBeanPointer m_relatedBean;
    BaseBeanPointer m_rootBean;
    QStringList m_categories;
    QString m_xml;
    QString m_relatedTableName;
    qlonglong m_relatedDbOid;
    QDateTime m_timeStamp;
    QString m_user;
    QPixmap m_pixmap;
    bool m_relatedIsLoaded;
    QString m_additionalInfo;
#ifdef ALEPHERP_SMTP_SUPPORT
    EmailObject m_email;
#endif
#ifdef ALEPHERP_DOC_MANAGEMENT
    QPointer<AERPDocMngmntDocument> m_document;
#endif
    AlephERP::RelatedElementCardinality m_cardinality;
    RelatedElement::RelatedElementStates m_state;
    bool m_found;

    RelatedElementPrivate(RelatedElement *qq) : q_ptr(qq)
    {
        m_state = RelatedElement::INSERT;
        m_modified = false;
        m_relatedDbOid = 0;
        m_id = 0;
        m_found = false;
        m_relatedIsLoaded = false;
    }

    QString emailDisplayText() const;
    QString beanDisplayText() const;
    QString documentDisplayText() const;
    QString emailTooltipText() const;
    QString beanTooltipText() const;
    QString documentTooltipText() const;
    bool loadDocument();
};

void RelatedElement::setRelatedTableName(const QString &value)
{
    d->m_relatedTableName = value;
}

void RelatedElement::setRelatedDbOid(qlonglong value)
{
    d->m_relatedDbOid = value;
}

RelatedElement::RelatedElement(QObject *parent) :
    QObject(parent), d(new RelatedElementPrivate(this))
{
    d->m_modified = false;
    setObjectName(QString("%1").arg(alephERPSettings->uniqueId()));
}

RelatedElement::~RelatedElement()
{
    delete d;
    d = NULL;
}

int RelatedElement::id() const
{
    return d->m_id;
}

void RelatedElement::setId(int id)
{
    d->m_id = id;
    setModified(true);
}

qlonglong RelatedElement::relatedDbOid() const
{
    if ( d->m_type == AlephERP::Record )
    {
        if ( !d->m_relatedBean.isNull() )
        {
            return d->m_relatedBean->dbOid();
        }
    }
    else if ( d->m_type == AlephERP::Email )
    {
#ifdef ALEPHERP_SMTP_SUPPORT
        return d->m_email.dbOid();
#else
        return -1;
#endif
    }
    else if ( d->m_type == AlephERP::Document )
    {
#ifdef ALEPHERP_DOC_MANAGEMENT
        return d->m_document.isNull() ? d->m_relatedDbOid : d->m_document->dbOid();
#else
        return -1;
#endif
    }
    return d->m_relatedDbOid;
}

qlonglong RelatedElement::rootDbOid()
{
    if ( d->m_rootBean.isNull() )
    {
        return -1;
    }
    return d->m_rootBean->dbOid();
}

AlephERP::RelatedElementTypes RelatedElement::type() const
{
    return d->m_type;
}

void RelatedElement::setType(AlephERP::RelatedElementTypes value)
{
    d->m_type = value;
    // Damos valores por defecto de la imagen asociada
    if ( d->m_type == AlephERP::Record )
    {
        d->m_pixmap = QPixmap(":/generales/images/record.png");
    }
    else if ( d->m_type == AlephERP::Email )
    {
        d->m_pixmap = QPixmap(":/generales/images/email.png");
    }
    else if ( d->m_type == AlephERP::Document )
    {
        d->m_pixmap = QPixmap(":/generales/images/document.png");
    }
    setModified(true);
}

QString RelatedElement::relatedTableName() const
{
    if ( d->m_relatedBean.isNull() )
    {
        return d->m_relatedTableName;
    }
    return d->m_relatedBean->metadata()->tableName();
}

QString RelatedElement::rootTableName() const
{
    if ( d->m_rootBean.isNull() )
    {
        return "";
    }
    return d->m_rootBean->metadata()->tableName();
}

BaseBeanPointer RelatedElement::relatedBean()
{
    if ( d->m_relatedTableName.isEmpty() )
    {
        return d->m_relatedBean;
    }
    if ( d->m_relatedBean.isNull() && !d->m_rootBean.isNull())
    {
        d->m_relatedBean = BeansFactory::instance()->newBaseBean(d->m_relatedTableName, false);
        d->m_relatedBean->setParent(this);
        AERPTransactionContext::instance()->addToContext(d->m_rootBean->actualContext(), d->m_relatedBean);
        d->m_found = BaseDAO::selectByOid(d->m_relatedDbOid, d->m_relatedBean.data());
        if ( d->m_found )
        {
            d->m_relatedIsLoaded = true;
        }
        else
        {
            QLogger::QLog_Info(AlephERP::stLogOther, QString::fromUtf8("No existe el registro seleccionado de tabla: [%1] y OID: [%2]").arg(d->m_relatedTableName).arg(d->m_relatedDbOid));
        }
        connect(d->m_relatedBean.data(), SIGNAL(beanModified(BaseBean*,bool)), this, SIGNAL(relatedBeanModified(BaseBean*)));
        connect(d->m_relatedBean.data(), SIGNAL(destroyed()), this, SLOT(beanUnloaded()));
    }
    return d->m_relatedBean;
}

QStringList RelatedElement::categories() const
{
    return d->m_categories;
}

void RelatedElement::setCategories(const QStringList &value)
{
    d->m_categories = value;
    setModified(true);
}

void RelatedElement::addCategory(const QString &category)
{
    d->m_categories.append(category);
}

void RelatedElement::removeCategory(const QString &category)
{
    d->m_categories.removeAll(category);
}

bool RelatedElement::hasCategories(const QStringList &values)
{
    foreach (const QString &value, values)
    {
        if ( !d->m_categories.contains(value) )
        {
            return false;
        }
    }
    return true;
}

AlephERP::RelatedElementCardinality RelatedElement::cardinality() const
{
    return d->m_cardinality;
}

void RelatedElement::setCardinality(AlephERP::RelatedElementCardinality cardinality)
{
    d->m_cardinality = cardinality;
}

bool RelatedElement::found() const
{
    return d->m_found;
}

void RelatedElement::setFound(bool value)
{
    d->m_found = value;
}

bool RelatedElement::relatedIsLoaded() const
{
    return d->m_relatedIsLoaded;
}

QString RelatedElement::additionalInfo() const
{
    return d->m_additionalInfo;
}

void RelatedElement::setAdditionalInfo(const QString &v)
{
    if ( v != d->m_additionalInfo )
    {
        d->m_additionalInfo = v;
        emit hasBeenModified(this);
    }
}

#ifdef ALEPHERP_SMTP_SUPPORT
EmailObject RelatedElement::email() const
{
    return d->m_email;
}

void RelatedElement::setEmail(const EmailObject &email)
{
    d->m_email = email;
    d->m_found = true;
    d->m_relatedIsLoaded = true;
    setModified(true);
}

#endif

#ifdef ALEPHERP_DOC_MANAGEMENT
AERPDocMngmntDocument *RelatedElement::document()
{
    if ( d->m_document.isNull() )
    {
        d->m_relatedIsLoaded =  d->loadDocument();
        d->m_found = d->m_relatedIsLoaded;
    }
    return d->m_document.data();
}

void RelatedElement::setDocument(AERPDocMngmntDocument *doc)
{
    d->m_document = QPointer<AERPDocMngmntDocument>(doc);
    // Obtenemos el pixmap del documento
    d->m_pixmap = CommonsFunctions::pixmapFromMimeType(doc->mimeType());
    d->m_relatedDbOid = doc->dbOid();
    d->m_found = true;
    d->m_relatedIsLoaded = true;
}

#endif

void RelatedElement::setRootBean(BaseBean *root)
{
    d->m_rootBean = BaseBeanPointer(root);
    setModified(true);
}

void RelatedElement::setRelatedBean(BaseBeanPointer related, bool takeOwnerShip)
{
    d->m_relatedBean = related;
    d->m_found = true;
    d->m_relatedIsLoaded = true;
    if ( !d->m_rootBean.isNull() )
    {
        AERPTransactionContext::instance()->addToContext(d->m_rootBean->actualContext(), d->m_relatedBean);
    }
    if ( takeOwnerShip )
    {
        related->setParent(this);
    }
    d->m_relatedTableName = related->metadata()->tableName();
    d->m_relatedDbOid = related->dbOid();
    // Leemos el pixmap del registro relacionado
    d->m_pixmap = related->metadata()->pixmap();
    if ( d->m_pixmap.isNull() )
    {
        d->m_pixmap = QPixmap(":/generales/images/record.png");
    }
    connect(d->m_relatedBean.data(), SIGNAL(beanModified(BaseBean*,bool)), this, SIGNAL(relatedBeanModified(BaseBean*)));
    connect(d->m_relatedBean.data(), SIGNAL(destroyed()), this, SLOT(beanUnloaded()));
    setModified(true);
}

BaseBeanPointer RelatedElement::newRelatedBean(const QString &tableName)
{
    BaseBeanPointer bean = BeansFactory::instance()->newBaseBean(tableName);
    d->m_type = AlephERP::Record;
    setRelatedBean(bean, true);
    d->m_found = true;
    return d->m_relatedBean;
}

void RelatedElement::setState(RelatedElement::RelatedElementStates state)
{
    d->m_state = state;
    setModified(true);
}

RelatedElement::RelatedElementStates RelatedElement::state()
{
    return d->m_state;
}

QString RelatedElement::toXml() const
{
    QString xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    xml += QString("<element>");
    xml += QString("<timestamp><![CDATA[%1]]></timestamp>").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    xml += QString("<user>%1</user>").arg(AERPLoggedUser::instance()->userName());
    xml += QString("<type>%1</type>").arg(stringType());
    xml += QString("<additionalInfo><![CDATA[%1]]></additionalInfo>").arg(additionalInfo());
    foreach (const QString &category, d->m_categories)
    {
        xml += QString("<category>%1</category>").arg(category);
    }

    if ( d->m_type == AlephERP::Email )
    {
#ifdef ALEPHERP_SMTP_SUPPORT
        xml += QString("<email><id>%1</id><oid>%2</oid></email>").arg(d->m_email.id()).arg(d->m_email.dbOid());
#endif
    }
    else if ( d->m_type == AlephERP::Record && !d->m_relatedBean.isNull() )
    {
        xml += "<record>";
        xml += QString("<tablename>%1</tablename>").arg(d->m_relatedBean->metadata()->tableName());
        xml += QString("<pkey>%1</pkey>").arg(d->m_relatedBean->pkSerializedValue());
        xml += QString("<oid>%1</oid>").arg(d->m_relatedBean->dbOid());
        xml += "</record>";
    }
    else if ( d->m_type == AlephERP::Document )
    {
#ifdef ALEPHERP_DOC_MANAGEMENT
        if ( d->m_document.isNull() )
        {
            d->m_found = d->loadDocument();
            d->m_relatedIsLoaded = d->m_found;
        }
        if ( !d->m_document.isNull() )
        {
            xml += "<document>";
            xml += QString("<id>%1</id>").arg(d->m_document->id());
            xml += QString("<oid>%1</oid>").arg(d->m_document->dbOid());
            xml += QString("<path>%1</path>").arg(d->m_document->path());
            xml += QString("<name>%1</name>").arg(d->m_document->name());
            xml += "</document>";
        }
#endif
    }
    xml += QString("</element>");
    d->m_xml = xml;
    return d->m_xml;
}

void RelatedElement::setXml(const QString &xml)
{
    d->m_xml = xml;
    // Parseamos el XML para obtener los datos.
    QString errorString;
    int errorLine, errorColumn;

    if ( d->m_xml.isEmpty() )
    {
        return;
    }

    QDomDocument domDocument ;
    if ( domDocument.setContent(d->m_xml, true, &errorString, &errorLine, &errorColumn) )
    {
        QDomElement root = domDocument.documentElement();
        QDomNode n = root.firstChildElement("user");
        if ( !n.isNull() )
        {
            d->m_user = QObject::trUtf8(n.toElement().text().toUtf8());
        }
        n = root.firstChildElement("timestamp");
        if ( !n.isNull() )
        {
            d->m_timeStamp = QDateTime::fromString(QObject::trUtf8(n.toElement().text().toUtf8()));
        }
        n = root.firstChildElement("additionalInfo");
        if ( !n.isNull() )
        {
            d->m_additionalInfo = n.toElement().text().toUtf8();
        }
        QDomNodeList categoryList = root.elementsByTagName("category");
        for (int i = 0 ; i < categoryList.size() ; i++ )
        {
            d->m_categories.append(categoryList.at(i).toElement().text());
        }
        n = root.firstChildElement("type");
        if ( !n.isNull() )
        {
            setStringType(n.toElement().text().toUtf8());
            if ( type() == AlephERP::Email )
            {
#ifdef ALEPHERP_SMTP_SUPPORT
                QDomElement emailElement = root.firstChildElement("email");
                if ( !emailElement.isNull() )
                {
                    QDomNode nEmail = emailElement.firstChildElement("id");
                    if ( !nEmail.isNull() )
                    {
                        int id = nEmail.toElement().text().toUtf8().toInt();
                        d->m_relatedIsLoaded = EmailDAO::loadEmail(id, &(d->m_email));
                        d->m_found = d->m_relatedIsLoaded;
                    }
                }
                d->m_pixmap = QPixmap(":/generales/images/email.png");
#endif
            }
            else if ( type() == AlephERP::Document )
            {
#ifdef ALEPHERP_DOC_MANAGEMENT
                QDomElement documentElement = root.firstChildElement("document");
                if ( !documentElement.isNull() )
                {
                    QDomNode nDoc = documentElement.firstChildElement("id");
                    if ( !nDoc.isNull() )
                    {
                        int idDocument;
                        bool ok;
                        QString error;
                        idDocument = nDoc.toElement().text().toInt(&ok);
                        if ( ok )
                        {
                            d->m_document = AERPDocumentDAOWrapper::instance()->document(idDocument, this);
                            if ( !d->m_document.isNull() )
                            {
                                d->m_pixmap = CommonsFunctions::pixmapFromMimeType(d->m_document->mimeType());
                                d->m_relatedDbOid = d->m_document->dbOid();
                                d->m_found = true;
                                d->m_relatedIsLoaded = true;
                            }
                            else
                            {
                                QDomElement documentElement = root.firstChildElement("document");
                                if ( !documentElement.isNull() )
                                {
                                    d->m_document = new AERPDocMngmntDocument(this);
                                    QDomNode nName = documentElement.firstChildElement("name");
                                    if ( !nName.isNull() )
                                    {
                                        d->m_document->setName(nName.toElement().text());
                                    }
                                    QDomNode nPath = documentElement.firstChildElement("path");
                                    if ( !nPath.isNull() )
                                    {
                                        d->m_document->setPath(nPath.toElement().text());
                                    }
                                }
                            }
                        }
                        else
                        {
                            QLogger::QLog_Error(AlephERP::stLogOther, QString("RelatedElement::setXml: ERROR: AERPDocumentDAOPluginIface Plugin nulo. [%1] o ID no valido: [%2]").arg(error).arg(nDoc.toElement().text()));
                        }
                    }
                }
#endif
            }
        }
    }
    else
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("RelatedElement::setXml: ERROR: Linea: [%1] Columna: [%2] Error: [%3]").arg(errorLine).arg(errorColumn).arg(errorString));
    }
    setModified(true);
}

QString RelatedElement::stringType() const
{
    return RelatedElement::typeToString(d->m_type);
}

QString RelatedElement::user() const
{
    return d->m_user;
}

void RelatedElement::setStringType(const QString &type)
{
    d->m_type = RelatedElement::stringToType(type);
    setModified(true);
}

void RelatedElement::touch()
{
    setModified(true);
}

bool RelatedElement::hasCategory(const QString &category)
{
    QStringList cats(category);
    return hasCategories(cats);
}

QScriptValue RelatedElement::toScriptValue(QScriptEngine *engine, RelatedElement * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void RelatedElement::fromScriptValue(const QScriptValue &object, RelatedElement *&out)
{
    out = qobject_cast<RelatedElement *>(object.toQObject());
}

QScriptValue RelatedElement::toScriptValuePointer(QScriptEngine *engine, const RelatedElementPointer &in)
{
    return engine->newQObject(in.data(), QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void RelatedElement::fromScriptValuePointer(const QScriptValue &object, RelatedElementPointer &out)
{
    out = qobject_cast<RelatedElement *>(object.toQObject());
}

QString RelatedElement::typeToString(AlephERP::RelatedElementTypes type)
{
    if ( type == AlephERP::Email )
    {
        return "email";
    }
    else if ( type == AlephERP::Record )
    {
        return "record";
    }
    else if ( type == AlephERP::Document )
    {
        return "document";
    }
    return "";
}

AlephERP::RelatedElementTypes RelatedElement::stringToType(const QString &value)
{
    if ( value == "email" )
    {
        return AlephERP::Email;
    }
    else if ( value == "record" )
    {
        return AlephERP::Record;
    }
    else if ( value == "document" )
    {
        return AlephERP::Document;
    }
    return AlephERP::Record;
}

QDateTime RelatedElement::timeStamp() const
{
    return d->m_timeStamp;
}

bool RelatedElement::modified()
{
    return d->m_modified;
}

QPixmap RelatedElement::pixmap() const
{
    return d->m_pixmap;
}

void RelatedElement::setPixmap(const QPixmap &pixmap)
{
    d->m_pixmap = pixmap;
}

/**
 * @brief RelatedElement::displayText
 * Proporciona un texto mono que representa el contenido.
 * @return
 */
QString RelatedElement::displayText() const
{
    if ( d->m_type == AlephERP::Email )
    {
        return d->emailDisplayText();
    }
    else if ( d->m_type == AlephERP::Record )
    {
        return d->beanDisplayText();
    }
    else if ( d->m_type == AlephERP::Document )
    {
        return d->documentDisplayText();
    }
    return QString();
}

QString RelatedElement::tooltipText() const
{
    if ( d->m_type == AlephERP::Email )
    {
        return d->emailTooltipText();
    }
    else if ( d->m_type == AlephERP::Record )
    {
        return d->beanTooltipText();
    }
    else if ( d->m_type == AlephERP::Document )
    {
        return d->documentTooltipText();
    }
    return QString();
}

BaseBeanPointer RelatedElement::rootBean()
{
    return d->m_rootBean;
}

void RelatedElement::setModified(bool value)
{
    d->m_modified = value;
    if ( d->m_modified )
    {
        emit hasBeenModified(this);
    }
}

void RelatedElement::beanUnloaded()
{
    if ( d != NULL )
    {
        d->m_relatedIsLoaded = false;
    }
}

QString RelatedElementPrivate::emailDisplayText() const
{
#ifdef ALEPHERP_SMTP_SUPPORT
    QString content = QObject::trUtf8("Para: %1 con fecha: %2 y Asunto: %3").
                      arg(m_email.to().join(", ")).
                      arg(alephERPSettings->locale()->toString(m_email.dateTime(), "dd/MM/yyyy HH:mm:ss")).
                      arg(m_email.subject());
    return content;
#else
    return QString();
#endif
}

QString RelatedElementPrivate::beanDisplayText() const
{
    BaseBeanPointer b = q_ptr->relatedBean();
    if ( !b.isNull() )
    {
        return b->toString();
    }
    else
    {
        if ( !q_ptr->found() )
        {
            BaseBeanMetadata *m = BeansFactory::metadataBean(q_ptr->relatedTableName());
            if ( m != NULL )
            {
                return QString::fromUtf8("Enlace a registro: %1 perdido.").arg(m->alias());
            }
            else
            {
                return QString::fromUtf8("Enlace a registro de tabla desconocida perdido.");
            }
        }
    }
    return QString();
}

QString RelatedElementPrivate::documentDisplayText() const
{
    QString content;
#ifdef ALEPHERP_DOC_MANAGEMENT
    if ( !m_document.isNull() )
    {
        content = m_document->name();
    }
#endif
    return content;
}

QString RelatedElementPrivate::emailTooltipText() const
{
#ifdef ALEPHERP_SMTP_SUPPORT
    QString content = QObject::trUtf8("<i>Correo electrónico</i><br/>");
    content = QString("%1<b>Para</b>: %2<br/>").arg(content).arg(m_email.to().join(", "));
    content = QString("%1<b>Copia</b>: %2<br/>").arg(content).arg(m_email.copy().join(", "));
    content = QString("%1<b>Asunto</b>: %2<br/>").arg(content).arg(m_email.subject());
    content = QString("%1<b>Fecha</b>: %2").arg(content).arg(alephERPSettings->locale()->toString(m_email.dateTime(), "dd/MM/yyyy HH:mm:ss"));
#else
    QString content;
#endif
    return content;
}

QString RelatedElementPrivate::beanTooltipText() const
{
    return QString();
}

QString RelatedElementPrivate::documentTooltipText() const
{
    QString content = QObject::trUtf8("<i>Documento</i><br/>");
#ifdef ALEPHERP_DOC_MANAGEMENT
    if ( !m_document.isNull() )
    {
        content = QString("%1<b>Nombre</b>: %2<br/>").arg(content).arg(m_document->name());
        content = QString("%1<b>Descripción</b>: %2<br/>").arg(content).arg(m_document->description());
        content = QString("%1<b>Palabras claves</b>: %2<br/>").arg(content).arg(m_document->keywords().join(", "));
        content = QString("%1<b>Última versión</b>: %2<br/> modificada por <i>%3</i> por última vez el <i>%4</i>").
                  arg(content).arg(m_document->lastVersion()).arg(m_document->lastModificationUser()).
                  arg(alephERPSettings->locale()->toString(m_document->lastModificationTime()));
        AERPDocumentVersion *version = m_document->version(m_document->lastVersion());
        if ( version != NULL )
        {
            content = QString("%1<b>Tamaño</b>: %2<br/>").arg(content).arg(CommonsFunctions::sizeHuman(version->sizeOnBytes()));
        }
    }
#endif
    return content;
}

bool RelatedElementPrivate::loadDocument()
{
    bool result = false;
#ifdef ALEPHERP_DOC_MANAGEMENT
    int id;
    bool ok;
    id = m_relatedPkey.toInt(&ok);
    if ( ok )
    {
        m_document = AERPDocumentDAOWrapper::instance()->document(id, q_ptr);
    }
#endif
    return result;
}
