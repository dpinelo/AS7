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
#include "aerpspellnumber.h"
#include "globales.h"

AERPSpellNumber::AERPSpellNumber()
{
}

QStringList englishListSmallNumbers()
{
    QStringList numbers;
    numbers << QObject::trUtf8("zero");
    numbers << QObject::trUtf8("one");
    numbers << QObject::trUtf8("two");
    numbers << QObject::trUtf8("three");
    numbers << QObject::trUtf8("four");
    numbers << QObject::trUtf8("five");
    numbers << QObject::trUtf8("six");
    numbers << QObject::trUtf8("seven");
    numbers << QObject::trUtf8("eight");
    numbers << QObject::trUtf8("nine");
    numbers << QObject::trUtf8("ten");
    numbers << QObject::trUtf8("eleven");
    numbers << QObject::trUtf8("twelve");
    numbers << QObject::trUtf8("thirten");
    numbers << QObject::trUtf8("fourten");
    numbers << QObject::trUtf8("fiften");
    numbers << QObject::trUtf8("sixten");
    numbers << QObject::trUtf8("seventen");
    numbers << QObject::trUtf8("eighten");
    numbers << QObject::trUtf8("nineten");
    return numbers;
}

QStringList englishListThousandPowers()
{
    QStringList numbers;
    numbers << QObject::trUtf8(" billion");
    numbers << QObject::trUtf8(" million");
    numbers << QObject::trUtf8(" thousand");
    numbers << QObject::trUtf8("");
    return numbers;
}

QStringList englishListDecades()
{
    QStringList numbers;
    numbers << "";
    numbers << QObject::trUtf8("twenty");
    numbers << QObject::trUtf8("thirty");
    numbers << QObject::trUtf8("forty");
    numbers << QObject::trUtf8("fifty");
    numbers << QObject::trUtf8("sixty");
    numbers << QObject::trUtf8("seventy");
    numbers << QObject::trUtf8("eighty");
    numbers << QObject::trUtf8("ninety");
    return numbers;
}

QString englishSpellHundreds(unsigned n)
{
    QStringList smallNumbers = englishListSmallNumbers();
    QString res;
    if (n > 99)
    {
        res = smallNumbers[n/100];
        res += QObject::trUtf8(" hundred");
        n %= 100;
        if (n)
        {
            res += QObject::trUtf8(" y ");
        }
    }
    if (n >= 20)
    {
        QStringList decades = englishListDecades();
        res += decades[n/10];
        n %= 10;
        if (n)
        {
            res += "-";
        }
    }
    if (n < 20 && n > 0)
    {
        res += smallNumbers[n];
    }
    return res;
}

QString englishSpellNumber(Spellable n)
{
    QStringList smallNumbers = englishListSmallNumbers();
    if (n < 20)
    {
        return smallNumbers[n];
    }
    QString res;
    QStringList scaleName = englishListThousandPowers();
    int scaleIndex = 0;
    Spellable scaleFactor = 1000000000;	// 1 billion
    while (scaleFactor > 0)
    {
        if (n >= scaleFactor)
        {
            Spellable h = n / scaleFactor;
            res += englishSpellHundreds(h) + scaleName[scaleIndex];
            n %= scaleFactor;
            if (n)
            {
                res += ", ";
            }
        }
        scaleFactor /= 1000;
        ++scaleIndex;

    }
    return res;
}

QString spanishConvertDigit(int digit)
{
    QString result;
    switch (digit)
    {
    case 1:
        result = "uno";
        break;
    case 2:
        result = "dos";
        break;
    case 3:
        result = "tres";
        break;
    case 4:
        result = "cuatro";
        break;
    case 5:
        result = "cinco";
        break;
    case 6:
        result = "seis";
        break;
    case 7:
        result = "siete";
        break;
    case 8:
        result = "ocho";
        break;
    case 9:
        result = "nueve";
        break;
    }
    return result;
}


QString spanishConvertTens(int number)
{
    QString result;
    int digit = 0;

    if ( number < 10 )
    {
        result = spanishConvertDigit(number);
    }
    else if ( number >= 10 && number <= 30 )
    {
        switch ( number )
        {
        case 10:
            result = "diez";
            break;
        case 11:
            result = "once";
            break;
        case 12:
            result = "doce";
            break;
        case 13:
            result = "trece";
            break;
        case 14:
            result = "catorce";
            break;
        case 15:
            result = "quince";
            break;
        case 16:
            result = "dieciseis";
            break;
        case 17:
            result = "diecisiete";
            break;
        case 18:
            result = "dieciocho";
            break;
        case 19:
            result = "diecinueve";
            break;
        case 20:
            result = "veinte";
            break;
        case 21:
            result = "veintiuno";
            break;
        case 22:
            result = QString::fromUtf8("veintidós");
            break;
        case 23:
            result = QString::fromUtf8("veintitrés");
            break;
        case 24:
            result = "veinticuatro";
            break;
        case 25:
            result = "veinticinco";
            break;
        case 26:
            result = "veintiseis";
            break;
        case 27:
            result = "veintisiete";
            break;
        case 28:
            result = "veintiocho";
            break;
        case 29:
            result = "veintinueve";
            break;
        }
    }
    else
    {
        if ( number >= 30 && number < 40 )
        {
            result = "treinta ";
            digit = number - 30;
        }
        else if ( number >= 40 && number < 50 )
        {
            result = "cuarenta ";
            digit = number - 40;
        }
        else if ( number >= 50 && number < 60 )
        {
            result = "cincuenta ";
            digit = number - 50;
        }
        else if ( number >= 60 && number < 70 )
        {
            result = "sesenta ";
            digit = number - 60;
        }
        else if ( number >= 70 && number < 80 )
        {
            result = "setenta ";
            digit = number - 70;
        }
        else if ( number >= 80 && number < 90 )
        {
            result = "ochenta ";
            digit = number - 80;
        }
        else if ( number >= 90 && number < 99 )
        {
            result = "noventa ";
            digit = number - 90;
        }
        if ( digit > 0 )
        {
            result = result + "y " + spanishConvertDigit(digit);
        }
    }
    return result;
}

QString spanishConvertHundreds(int number)
{
    QString result;
    int tens = 0;

    if ( number == 0 )
    {
        return result;
    }

    if ( number == 100 )
    {
        return QString("cien");
    }
    else if ( number > 100 && number < 200 )
    {
        result = "ciento ";
        tens = number - 100;
    }
    else if ( number > 200 && number < 300 )
    {
        result = "doscientos ";
        tens = number - 200;
    }
    else if ( number > 300 && number < 400 )
    {
        result = "trescientos ";
        tens = number - 300;
    }
    else if ( number > 400 && number < 500 )
    {
        result = "cuatrocientos ";
        tens = number - 400;
    }
    else if ( number > 500 && number < 600 )
    {
        result = "quinientos ";
        tens = number - 500;
    }
    else if ( number > 600 && number < 700 )
    {
        result = "seiscientos ";
        tens = number - 600;
    }
    else if ( number > 700 && number < 800 )
    {
        result = "setecientos ";
        tens = number - 700;
    }
    else if ( number > 800 && number < 900 )
    {
        result = "ochocientos ";
        tens = number - 800;
    }
    else if ( number > 900 && number < 1000 )
    {
        result = "novecientos ";
        tens = number - 900;
    }
    else if ( number < 100 )
    {
        result = spanishConvertTens(number);
    }
    if ( tens > 0 )
    {
        result += spanishConvertTens(tens);
    }
    return result;
}

QString spanishSpellNumber(Spellable number)
{
    QString myNumber = QString("%1").arg(number);
    QString result;
    int count = 0;

    QStringList place;
    place << "";
    place << " mil ";
    place << " millones ";
    place << " billones ";
    place << " trillones ";

    while ( !myNumber.isEmpty() )
    {
        if ( count < place.size() )
        {
            QString temp = myNumber.right(3);
            int iTemp = temp.toInt();
            QString tempResult = spanishConvertHundreds(iTemp);
            if ( iTemp == 1 )
            {
                switch(count)
                {
                case 0:
                    tempResult = "uno ";
                    break;
                case 1:
                    tempResult = "mil ";
                    break;
                case 2:
                    tempResult = QString::fromUtf8("un millón ");
                    break;
                case 3:
                    tempResult = QString::fromUtf8("un billón ");
                    break;
                case 4:
                    tempResult = QString::fromUtf8("un trillón ");
                    break;
                }
                result = tempResult + result;
            }
            else
            {
                result = tempResult + place.at(count) + result;
            }
        }
        if ( myNumber.size() > 3 )
        {
            myNumber = myNumber.mid(0, myNumber.size()-3);
        }
        else
        {
            myNumber.clear();
        }
        count++;
    }
    return result;
}

QString AERPSpellNumber::spellNumber(Spellable n, const QString &language)
{
    return spellNumber(n, 0, language);
}

QString AERPSpellNumber::spellNumber(double n, int decimalPlaces, const QString &language)
{
    Spellable integerPart = (Spellable) n;
    Spellable decimalPart = (Spellable) ((CommonsFunctions::round(n, decimalPlaces) - integerPart) * pow(10, decimalPlaces));
    QString strIntegerPart, strDecimalPart;

    if ( language == "en" )
    {
        strIntegerPart = englishSpellNumber(integerPart);
        if ( decimalPart != 0 )
        {
            strDecimalPart = englishSpellNumber(decimalPart);
        }
    }
    else if ( language == "es" )
    {
        strIntegerPart = spanishSpellNumber(integerPart);
        if ( decimalPart != 0 )
        {
            strDecimalPart = spanishSpellNumber(decimalPart);
        }
    }
    QString result;
    if ( !strDecimalPart.isEmpty() )
    {
        result = QObject::trUtf8("%1 con %2").arg(strIntegerPart).arg(strDecimalPart);
    }
    else
    {
        result = strIntegerPart;
    }
    return result;
}
