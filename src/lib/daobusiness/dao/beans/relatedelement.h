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

#ifndef RELATEDELEMENT_H
#define RELATEDELEMENT_H

#include <QtCore>
#include <QtScript>
#include <aerpcommon.h>
#include "dao/beans/basebean.h"
#ifdef ALEPHERP_SMTP_SUPPORT
#include "dao/emaildao.h"
#endif

class RelatedElementPrivate;
class BeansFactory;
#ifdef ALEPHERP_DOC_MANAGEMENT
class AERPDocMngmntDocument;
#endif

/**
 * @brief The RelatedElement class
 * Esta clase representará la relación con otros elementos, que será modelada en base de datos
 * no mediante la típica relación de integridad referencial, sino a través de una definición
 * XML
 */
class RelatedElement : public QObject, public QScriptable
{
    Q_OBJECT
    Q_ENUMS(RelatedElementStates)

    /** ID de base de datos del elemento relacionado */
    Q_PROPERTY(int id READ id WRITE setId)
    /** Tipo de elemento relacionado... otro registro de otra tabla, un correo electrónico... */
    Q_PROPERTY(AlephERP::RelatedElementTypes type READ type WRITE setType)
    /** Identifica a la tabla origen o maestra del registro al que pertenece la relación */
    Q_PROPERTY(QString rootTableName READ rootTableName)
    /** OID del registro padre de esta relación */
    Q_PROPERTY(qlonglong rootDbOid READ rootDbOid)
    /** Identifica a la tabla maestra u origen del registro relacionado */
    Q_PROPERTY(QString relatedTableName READ relatedTableName)
    /** OID del registro relacionado */
    Q_PROPERTY(qlonglong relatedDbOid READ relatedDbOid)
    /** Posible categoría en la que clasificar los elementos relacionados. Por ejemplo, "Impuestos" */
    Q_PROPERTY(QStringList categories READ categories WRITE setCategories)
    /** Estado de inserción en base de datos */
    Q_PROPERTY(RelatedElementStates state READ state)
    Q_PROPERTY(QString xml READ toXml)
    /** Devuelve la referencia al bean que está relacionado con este */
    Q_PROPERTY(BaseBeanPointer relatedBean READ relatedBean)
    /** Indica si el objeto relacionado está cargado o no */
    Q_PROPERTY(bool relatedIsLoaded READ relatedIsLoaded)
    Q_PROPERTY(QString user READ user)
    Q_PROPERTY(QString displayText READ displayText)
    Q_PROPERTY(QString tooltipText READ tooltipText)
#ifdef ALEPHERP_SMTP_SUPPORT
    /** Email relacionado */
    Q_PROPERTY(EmailObject email READ email WRITE setEmail)
#endif
#ifdef ALEPHERP_DOC_MANAGEMENT
    Q_PROPERTY(AERPDocMngmntDocument* document READ document WRITE setDocument)
#endif
    /** Pixmap que representa a la relación */
    Q_PROPERTY(QPixmap pixmap READ pixmap WRITE setPixmap)
    /** Información adicional que podría introducirse, a modo de texto */
    Q_PROPERTY(QString additionalInfo READ additionalInfo WRITE setAdditionalInfo)

    /** Marca de modificación */
    Q_PROPERTY(bool modified READ modified WRITE setModified)
    /** Tipo de relación: Podemos estar apuntando a un hijo, o un padre */
    Q_PROPERTY(AlephERP::RelatedElementCardinality cardinality READ cardinality)
    /** Indica si el elemento relacionado se ha encontrado en los orígenes de datos */
    Q_PROPERTY(bool found READ found WRITE setFound)

    friend class BaseBean;
    friend class BaseDAO;
    friend class RelatedDAO;

private:
    RelatedElementPrivate *d;

    void setRelatedTableName(const QString &value);
    void setRelatedPkey(const QString &value);
    void setRelatedDbOid(qlonglong value);

public:
    explicit RelatedElement(QObject *parent = 0);
    virtual ~RelatedElement();

    // Los elementos marcados como LINK indican que se crean virtualmente en tiempo de ejecución, pero no están
    // en base de datos.
    enum RelatedElementStates { INSERT = 1, UPDATE = 2, TO_BE_DELETED = 4, DELETED = 8, LINK = 16 };

    int id() const;
    void setId(int id);
    qlonglong relatedDbOid() const;
    qlonglong rootDbOid();
    AlephERP::RelatedElementTypes type() const;
    void setType(AlephERP::RelatedElementTypes value);
    QStringList categories() const;
    void setCategories(const QStringList &value);
    bool hasCategories(const QStringList &values);
    AlephERP::RelatedElementCardinality cardinality() const;
    void setCardinality(AlephERP::RelatedElementCardinality cardinality);
    bool found() const;
    void setFound(bool value);
    bool relatedIsLoaded() const;
    QString additionalInfo() const;
    void setAdditionalInfo(const QString &v);

    BaseBeanPointer rootBean();
    BaseBeanPointer relatedBean();
    void setRelatedBean(BaseBeanPointer related, bool takeOwnerShip = false);
    QString relatedTableName() const;
    QString rootTableName() const;
    RelatedElementStates state();
    QString toXml() const;
    QString stringType() const;
    QString user() const;
    QDateTime timeStamp() const;
    bool modified();
    QPixmap pixmap() const;
    void setPixmap(const QPixmap &pixmap);

    QString displayText() const;
    QString tooltipText() const;

#ifdef ALEPHERP_SMTP_SUPPORT
    EmailObject email() const;
    void setEmail(const EmailObject &email);
#endif
#ifdef ALEPHERP_DOC_MANAGEMENT
    AERPDocMngmntDocument *document();
    void setDocument(AERPDocMngmntDocument *doc);
#endif

    Q_INVOKABLE BaseBeanPointer newRelatedBean(const QString &tableName);

    static QScriptValue toScriptValue(QScriptEngine *engine, RelatedElement * const &in);
    static void fromScriptValue(const QScriptValue &object, RelatedElement * &out);
    static QScriptValue toScriptValuePointer(QScriptEngine *engine, RelatedElementPointer const &in);
    static void fromScriptValuePointer(const QScriptValue &object, RelatedElementPointer &out);
    static QString typeToString(AlephERP::RelatedElementTypes type);
    static AlephERP::RelatedElementTypes stringToType(const QString &value);

protected slots:
    void setModified(bool value);
    void beanUnloaded();

protected:
    void setRootBean(BaseBean *root);
    void setState(RelatedElementStates state);
    void setXml(const QString &xml);
    void setStringType(const QString &type);

signals:
    /** Esta señal se modifica cuando la definición del elemento relacionado ha sido modificada (pero quizás no sus datos
     * interiores) */
    void hasBeenModified(RelatedElement *);
    void relatedBeanModified(BaseBean *);

public slots:
    void touch();
    bool hasCategory(const QString &category);
    void addCategory(const QString &category);
    void removeCategory(const QString &category);
};

typedef QPointer<RelatedElement> RelatedElementPointer;
typedef QList<RelatedElementPointer> RelatedElementPointerList;

Q_DECLARE_METATYPE(RelatedElement*)
Q_DECLARE_METATYPE(RelatedElementPointer)
Q_DECLARE_METATYPE(RelatedElement::RelatedElementStates)
Q_DECLARE_METATYPE(QList<RelatedElement *>)
Q_DECLARE_METATYPE(RelatedElementPointerList)
Q_SCRIPT_DECLARE_QMETAOBJECT(RelatedElement, QObject*)

#endif // RELATEDELEMENT_H
