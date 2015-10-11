/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo   *
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
#include "aerpcompleterhighlightdelegate.h"

class AERPCompleterHighlightDelegatePrivate
{
public:
    QString m_currentSearch;
    QChar m_replaceChar;

    AERPCompleterHighlightDelegatePrivate()
    {
    }
    QStringList splitText(const QString &originalText);
};

AERPCompleterHighlightDelegate::AERPCompleterHighlightDelegate(QObject *parent) :
    AERPItemDelegate(parent),
    d(new AERPCompleterHighlightDelegatePrivate)
{
}

AERPCompleterHighlightDelegate::~AERPCompleterHighlightDelegate()
{
    delete d;
}

QString AERPCompleterHighlightDelegate::currentSearch() const
{
    return d->m_currentSearch;
}

QChar AERPCompleterHighlightDelegate::replaceWildCardCharacter() const
{
    return d->m_replaceChar;
}

void AERPCompleterHighlightDelegate::setCurrentSearch(const QString &searchedText)
{
    d->m_currentSearch = searchedText;
}

void AERPCompleterHighlightDelegate::setReplaceWildCardCharacter(const QChar &c)
{
    d->m_replaceChar = c;
}

void AERPCompleterHighlightDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItemV4 options = option;
    initStyleOption(&options, index);

    QStringList content = d->splitText(options.text);

    painter->save();

    // Call this to get the focus rect and selection background.
    options.text = "";
    options.viewItemPosition = QStyleOptionViewItemV4::Middle;
    options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter, options.widget);

    QFont actualFont = options.font;
    QPen actualPen = painter->pen();
    QFontMetrics fm = painter->fontMetrics();
    QChar lastChar;

    painter->translate(options.rect.left(), options.rect.top());

    // Inicio de la cadena no coincidente
    QRect firstRect = QRect(fm.width(' '), 0, fm.width(content.at(0)), options.rect.height());
    lastChar = content.at(0).size() > 0 ? content.at(0)[content.at(0).size()-1] : QChar();
    painter->drawText(firstRect, Qt::AlignLeft | Qt::AlignVCenter, content.at(0));

    // Parte coincidente
    actualFont.setBold(true);
    painter->setPen(Qt::darkBlue);
    painter->setFont(actualFont);
    fm = painter->fontMetrics();
    QRect secondRect = QRect(firstRect.x() + firstRect.width() + fm.rightBearing(lastChar), 0, fm.width(content.at(1)), options.rect.height());
    painter->drawText(secondRect, Qt::AlignLeft | Qt::AlignVCenter, content.at(1));
    lastChar = content.at(1).size() > 0 ? content.at(1)[content.at(1).size()-1] : QChar();

    // Parte final no coincidente.
    actualFont.setBold(false);
    painter->setPen(actualPen);
    painter->setFont(actualFont);
    fm = painter->fontMetrics();
    QRect thirdRect = QRect(firstRect.x() + firstRect.width() + secondRect.width() + fm.rightBearing(lastChar), 0, fm.width(content.at(2)), options.rect.height());
    painter->drawText(thirdRect, Qt::AlignLeft | Qt::AlignVCenter, content.at(2));

    painter->restore();
}

QSize AERPCompleterHighlightDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItemV4 options = option;
    initStyleOption(&options, index);

    QFont actualFont = options.font;
    actualFont.setBold(true);

    QFontMetrics fm(actualFont);
    QString content = options.text;
    content.append(" ");
    QSize sz = fm.boundingRect(options.text).size();
    return QSize(sz.width(), sz.height() + fm.lineSpacing());
}

/**
 * @brief AERPCompleterHighlightDelegatePrivate::splitText
 * Esta función hace algo bastante ... difícil. Pongamos por ejemplo, que el valor a presentar es
 * AC000DC000EF0001
 * y que el usuario ha introducido una cadena de búsqueda
 * AC.D.E
 * devolvería en parts lo siguiente
 * parts[0] = AC000DC000E y true (es la parte que coincide)
 * parts[1] = F0001 y false (lo que resta sin coincidir
 * @param originalText
 * @param parts
 * @param matches
 */
QStringList AERPCompleterHighlightDelegatePrivate::splitText(const QString &originalText)
{
    QStringList list;

    if ( m_replaceChar.isNull() )
    {
        int initCurrentSearch = originalText.indexOf(m_currentSearch, 0, Qt::CaseInsensitive);
        if ( initCurrentSearch == -1 )
        {
            list.append(originalText);
            list.append("");
            list.append("");
        }
        else
        {
            list << originalText.left(initCurrentSearch);
            list << m_currentSearch;
            list << originalText.right(originalText.size() - list.at(0).size() - m_currentSearch.size());
        }
    }
    else
    {
        if ( !m_currentSearch.contains('.') )
        {
            int initCurrentSearch = originalText.indexOf(m_currentSearch, 0, Qt::CaseInsensitive);
            if ( initCurrentSearch == -1 )
            {
                list.append(originalText);
                list.append("");
                list.append("");
            }
            else
            {
                list << originalText.left(initCurrentSearch);
                list << m_currentSearch;
                list << originalText.right(originalText.size() - list.at(0).size() - m_currentSearch.size());
            }
        }
        else
        {
            QString regExpFormula;
            QStringList parts = m_currentSearch.split('.');
            foreach (const QString &part, parts)
            {
                if ( !part.isEmpty() )
                {
                    if ( regExpFormula.isEmpty() )
                    {
                        regExpFormula.append(part).append(QString("[%1]+").arg(m_replaceChar));
                    }
                    else
                    {
                        regExpFormula.append(part).append(QString("[%1]*").arg(m_replaceChar));
                    }
                }
            }
            QRegExp regExp(regExpFormula);
            regExp.setCaseSensitivity(Qt::CaseInsensitive);
            int initMatch = regExp.indexIn(originalText);
            int endMatch = initMatch + regExp.matchedLength();
            if ( initMatch == -1 )
            {
                list << originalText;
                list.append("");
                list.append("");
            }
            else
            {
                list << originalText.mid(0, initMatch);
                list << originalText.mid(initMatch, endMatch);
                list << originalText.mid(endMatch, originalText.size());
            }
        }
    }
    return list;
}
