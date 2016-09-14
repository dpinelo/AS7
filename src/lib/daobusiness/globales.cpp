//---------------------------------------------------------------------------
#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtNetwork>

#include <alepherpdaobusiness.h>
#include <iostream>
#include <cstdlib>

#ifdef ALEPHERP_SMTP_SUPPORT
#include "business/smtp/aerpsmtpinterface.h"
#endif

#ifdef ALEPHERP_BUILT_IN_SMTP_SUPPORT
#include "business/smtp/builtinsmtpobject.h"
#endif

#ifdef ALEPHERP_NO_LIBMAGIC
#include "business/mime/freedesktopmime.h"
#endif
#ifdef ALEPHERP_LIBMAGIC
#include <magic.h>
#endif

#ifdef ALEPHERP_DOC_MANAGEMENT
#include "business/docmngmnt/aerpdocumentdaopluginiface.h"
#include "business/docmngmnt/aerpscanneriface.h"
#include "business/docmngmnt/aerpextractplainconteniface.h"
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#ifndef Q_OS_DARWIN
#ifndef Q_OS_ANDROID
#ifdef Q_OS_UNIX
#include <X11/XKBlib.h>
#include <QX11Info>
#undef KeyPress
#undef KeyRelease
#undef FocusIn
#undef FocusOut
// #undef those Xlib #defines that conflict with QEvent::Type enum
#endif
#endif
#endif

#ifdef ALEPHERP_QTDESIGNERBUILTIN
#include "qdesigner_workbench.h"
#include "qdesigner_formwindow.h"
#include "qdesigner_toolwindow.h"
#include <QtDesigner/QDesignerComponents>
#include "qdesigner_actions.h"
#include "3rdparty/qtdesigner-4/src/designer/mainwindow.h"
#endif

bool m_alepherpProcessEvents = false;

CommonsFunctions::CommonsFunctions(QObject *parent) : QObject(parent)
{

}

void CommonsFunctions::processEvents()
{
    if ( !m_alepherpProcessEvents )
    {
        m_alepherpProcessEvents = true;
        qApp->processEvents();
        m_alepherpProcessEvents = false;
    }
    else
    {
        qDebug() << "CommonsFunctions::processEvents: Anidación de llamadas a processEvents()";
    }
}

/**
 * @brief CommonsFunctions::round
 * Redondea una número a los dígitos indicados. Hace un redondeo a par
 * @param value
 * @param Digits
 * @return
 */
double CommonsFunctions::round(double value, int decimalPoints)
{
    /*
En los siguientes ejemplos, se desea redondear cada número a las centésimas (el último dígito requerido es el segundo dígito después de la coma decimal):
a) 4,123 ⇒ Regla 1: Si el dígito a la derecha del último requerido es menor que 5, el último dígito requerido se deja intacto. Respuesta: 4,12
b) 8,627 ⇒ Regla 2: Si el dígito a la derecha del último requerido es mayor que 5, el último dígito requerido se aumenta una unidad. Respuesta: 8,63
c) 9,425110 ⇒ Regla 3: Si el dígito a la derecha del último requerido es un 5 no seguido de ceros, el último dígito requerido se aumenta una unidad. Respuesta: 9,43
d) 7,385 o 7,385000 ⇒ Regla 4: Si el dígito a la derecha del último requerido es un 5 seguido de ceros, el último dígito requerido se deja intacto si es par. Respuesta: 7,38
e) 6,275 o 6,275000 ⇒ Regla 5: Si el dígito a la derecha del último requerido es un 5 o seguido de ceros, el último dígito requerido se aumenta una unidad si es impar. Respuesta: 6,28
    */

    double numLeft = value;
    double numRight = value;
    double numTopLeft = value;
    int digitLeft, digitRight, digitTopLeft;

    numLeft *= pow(10, decimalPoints + 1);
    numTopLeft *= pow(10, decimalPoints + 2);
    digitLeft = fmod(numLeft, 10);
    digitTopLeft = fmod(numTopLeft, 10);

    if (digitLeft == 5 && digitTopLeft == 0)
    {
        numRight *= pow(10, decimalPoints);
        digitRight = fmod(numRight, 10);

        if (digitRight % 2 == 0) // if even
        {
            return floor(value * pow(10, decimalPoints)) / pow(10, decimalPoints);
        }
        else // otherwise it's odd
        {
            return ceil(value * pow(10, decimalPoints)) / pow(10, decimalPoints);
        }
    }
    else // standard round-to-nearest
    {
        return std::round(value * pow(10, decimalPoints)) / pow(10, decimalPoints);
    }
}

/**
 * @brief CommonsFunctins::setOverrideCursor
 * Maneja los tipos de punteros del ratón, teniendo en cuenta el hilo de ejecución
 * @param cursor
 */
void CommonsFunctions::setOverrideCursor(const QCursor &cursor)
{
    if ( QThread::currentThread() == qApp->thread() )
    {
        qApp->setOverrideCursor(cursor);
    }
}

void CommonsFunctions::restoreOverrideCursor()
{
    if ( QThread::currentThread() == qApp->thread() )
    {
        if ( qApp->overrideCursor() != 0 )
        {
            qApp->restoreOverrideCursor();
        }
    }
}

int CommonsFunctions::countDirsAndFiles(const QString &absolutePath)
{
    int count = 0;
    QDirIterator it(absolutePath, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QString entry = it.next();
        if ( entry != "." && entry != ".." )
        {
            count++;
        }
    }
    return count;
}

bool CommonsFunctions::removeDir(const QString &dirName)
{
    bool result = true;
    QDir dir(dirName);

    if (dir.exists(dirName)) {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (info.isDir()) {
                result = removeDir(info.absoluteFilePath());
            }
            else {
                result = QFile::remove(info.absoluteFilePath());
            }

            if (!result) {
                return result;
            }
        }
        result = dir.rmdir(dirName);
    }
    return result;
}

QString CommonsFunctions::removeAccents(const QString &s)
{
    QString diacriticLetters = QString::fromUtf8("ŠŒŽšœžŸ¥µÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝßàáâãäåæçèéêëìíîïðñòóôõöøùúûüýÿ");
    QStringList noDiacriticLetters;
    noDiacriticLetters << "S"<<"OE"<<"Z"<<"s"<<"oe"<<"z"<<"Y"<<"Y"<<"u"<<"A"<<"A"<<"A"<<"A"<<"A"<<"A"<<"AE"<<"C"<<"E"<<"E"<<"E"<<"E"<<"I"<<"I"<<"I"<<"I"<<"D"<<"N"<<"O"<<"O"<<"O"<<"O"<<"O"<<"O"<<"U"<<"U"<<"U"<<"U"<<"Y"<<"s"<<"a"<<"a"<<"a"<<"a"<<"a"<<"a"<<"ae"<<"c"<<"e"<<"e"<<"e"<<"e"<<"i"<<"i"<<"i"<<"i"<<"o"<<"n"<<"o"<<"o"<<"o"<<"o"<<"o"<<"o"<<"u"<<"u"<<"u"<<"u"<<"y"<<"y";

    QString output = "";
    for (int i = 0; i < s.length(); ++i) {
        QChar c = s[i];
        int dIndex = diacriticLetters.indexOf(c);
        if (dIndex < 0) {
            output.append(c);
        } else {
            QString replacement = noDiacriticLetters[dIndex];
            output.append(replacement);
        }
    }

    return output;
}

/**
 * @brief CommonsFunctions::processToHtml
 * @param s
 * @return
 * Permite que una cadena con ciertos caracteres de retorno de línea, se trasladen a HTML
 */
QString CommonsFunctions::processToHtml(const QString &s)
{
    QString result = s;
    result.replace("\r\n", "<br/>");
    result.replace("\n\r", "<br/>");
    result.replace('\n', "<br/>");
    result.replace('\r', "<br/>");
    return result;
}

#ifdef ALEPHERP_DEVTOOLS
QString CommonsFunctions::prettyDiff(const QList<Diff> &diffs)
{
    QString html;
    QString text;
    foreach(const Diff &aDiff, diffs)
    {
        text = aDiff.text;
        text.replace("&", "&amp;").replace("<", "&lt;")
        .replace(">", "&gt;").replace("\n", "<br>")
        .replace("\t", "&nbsp;&nbsp;&nbsp;&nbsp;");
        switch (aDiff.operation)
        {
        case DiffCode::DIFF_INSERT:
            html += QString("<ins style=\"font-weight:bold;color:navy;background:aqua\">") + text
                    + QString("</ins>");
            break;
        case DiffCode::DIFF_DELETE:
            html += QString("<del style=\"font-weight:bold;color:red;background:lightsalmon\">") + text
                    + QString("</del>");
            break;
        case DiffCode::DIFF_EQUAL:
            html += QString("<span>") + text + QString("</span>");
            break;
        }
    }
    html.prepend("<html><body>");
    html.append("</body></html>");
    return html;
}

QString CommonsFunctions::prettyDiffResult(const QList<Diff> &diffs)
{
    QString html;
    QString text;
    foreach(const Diff &aDiff, diffs)
    {
        text = aDiff.text;
        text.replace("&", "&amp;").replace("<", "&lt;")
        .replace(">", "&gt;").replace("\n", "<br>")
        .replace("\t", "&nbsp;&nbsp;&nbsp;&nbsp;");
        switch (aDiff.operation)
        {
        case DiffCode::DIFF_INSERT:
            html += QString("<ins style=\"font-weight:bold;color:navy;background:aqua\">") + text
                    + QString("</ins>");
            break;
        case DiffCode::DIFF_DELETE:
            html += QString("<ins style=\"font-weight:bold;color:navy;background:lightsalmon;text-decoration:line-through;\">") + text
                    + QString("</ins>");
            break;
        case DiffCode::DIFF_EQUAL:
            html += QString("<span>") + text + QString("</span>");
            break;
        }
    }
    html.prepend("<html><body>");
    html.append("</body></html>");
    return html;
}

#ifdef ALEPHERP_QTDESIGNERBUILTIN
QDesignerWorkbench *CommonsFunctions::openQtDesigner(const QString &file)
{
    QDesignerWorkbench *m_workbench = NULL;
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    m_workbench = new QDesignerWorkbench();
    QDesignerComponents::initializeResources();
    CommonsFunctions::restoreOverrideCursor();
    if ( m_workbench->mainWindowBase() != NULL )
    {
        for(int i = 0 ; i < m_workbench->formWindowCount() ; i++)
        {
            QMdiSubWindow *rc = qobject_cast<QMdiSubWindow *>(m_workbench->formWindow(i)->parentWidget());
            m_workbench->formWindow(i)->close();
            if ( rc != NULL )
            {
                rc->close();
            }
            else
            {
                m_workbench->formWindow(i)->close();
            }
        }
        m_workbench->mainWindowBase()->show();
    }
    m_workbench->readInForm(file);
    return m_workbench;
}
#endif
#endif

/**
 * @brief CommonsFunctions::timeToFormat
 * Devuelve adecuadamente formateada, en una cadena human readable, el tiempo mostrado en segundos
 * @param seconds
 * @return
 */
QString CommonsFunctions::timeToFormat(int seconds)
{
    int days = (int) seconds / (24 * 60 * 60);
    int hours = (int) ((seconds - (days * 24 * 60 * 60))) / (60 * 60);
    int minutes = (int) ((seconds  - (days * 24 * 60 * 60) - (hours * 60 * 60)) / 60);
    int secs = seconds - (days * 24 * 60 * 60) - (hours * 60 * 60) - (minutes * 60);
    return trUtf8("%1 días, %2h:%3m:%4s").
           arg(alephERPSettings->locale()->toString(days)).
           arg(hours, 2, 10, QChar('0')).arg(minutes, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
}

/**
 * @brief CommonsFunctions::contrastedColor
 * Devuelve un color suficientemente constradado a los pasados...
 * @param bg
 * @param fg
 * @return
 */
QColor CommonsFunctions::contrastedColor(const QColor &bg, const QColor &fg)
{
    const int HUE_BASE = (bg.hue() == -1) ? 90 : bg.hue();
    int h, s, v;

    h = HUE_BASE % 360;
    s = 240;
    v = int(qMax(bg.value(), fg.value()) * 0.85);

    // take care of corner cases
    const int M = 35;
    if (   (h < bg.hue() + M &&h > bg.hue() - M)
            || (h < fg.hue() + M &&h > fg.hue() - M))
    {
        h = (bg.hue() + fg.hue()) % 360;
        s = ((bg.saturation() + fg.saturation()) / 2) % 256;
        v = ((bg.value() + fg.value()) / 2) % 256;
    }

    return QColor::fromHsv(h, s, v);
}

/**
 * @brief CommonsFunctions::capsOn
 * Devuelve si la tecla CapsLock está activada
 * @return
 */
bool CommonsFunctions::capsOn()
{
#ifndef Q_OS_ANDROID
#ifdef Q_OS_WIN // MS Windows version
    return GetKeyState(VK_CAPITAL) == 1;
#endif
#ifdef Q_OS_DARWIN
    return false;
#endif
#ifndef Q_OS_DARWIN
#ifdef Q_OS_UNIX // X11 version
    unsigned int n = 0;
    Display *d = QX11Info::display();
    XkbGetIndicatorState(d, XkbUseCoreKbd, &n);
    return (n & 0x01) == 1;
#endif
#endif
#else
    return false;
#endif
}

QString CommonsFunctions::mimeType(const QString &absolutePath)
{
#ifdef ALEPHERP_NO_LIBMAGIC
    QFreeDesktopMime mime;
    return mime.fromFile(absolutePath);
#endif
#ifdef ALEPHERP_LIBMAGIC
    QString result;
    QByteArray baPath = absolutePath.toLatin1();
    char *actual_file = baPath.data();
    const char *magic_full;
    magic_t magic_cookie;
    /*MAGIC_MIME tells magic to return a mime of the file, but you can specify different things*/
    magic_cookie = magic_open(MAGIC_MIME);
    if (magic_cookie == NULL)
    {
        qDebug() << "Unable to initialize magic library";
        return "";
    }
    qDebug() << "Loading default magic database";
    if (magic_load(magic_cookie, NULL) != 0)
    {
        qDebug() << "cannot load magic database - " << magic_error(magic_cookie);
        magic_close(magic_cookie);
        return "";

    }
    magic_full = magic_file(magic_cookie, actual_file);
    result = QString(magic_full);
    magic_close(magic_cookie);
    return result;
#endif

}

/**
 * @brief CommonsFunctions::iconFromMimeType
 * Devuelve el icono configurado en la aplicación para el mime type pasado
 * @param mimeType
 * @return
 */
QIcon CommonsFunctions::iconFromMimeType(const QString &mimeType)
{
    QPixmap pixmap = CommonsFunctions::pixmapFromMimeType(mimeType);
    QIcon icon(pixmap);
    return icon;
}

QPixmap CommonsFunctions::pixmapFromMimeType(const QString &mimeType)
{
    QPixmap icon;
    if ( mimeType.toLower().contains(QStringLiteral("bmp")) )
    {
        icon = QPixmap(QStringLiteral(":/mime/mimeicons/bmp.png"));
    }
    else if ( mimeType.toLower().contains(QStringLiteral("excel")) )
    {
        icon = QPixmap(QStringLiteral(":/mime/mimeicons/excel.png"));
    }
    else if ( mimeType.toLower().contains(QStringLiteral("font-ttf")) )
    {
        icon = QPixmap(QStringLiteral(":/mime/mimeicons/font_truemimeType.png"));
    }
    else if ( mimeType.toLower().contains(QStringLiteral("font")) )
    {
        icon = QPixmap(QStringLiteral(":/mime/mimeicons/font.png"));
    }
    else if ( mimeType.toLower().contains(QStringLiteral("gif")) )
    {
        icon = QPixmap(QStringLiteral(":/mime/mimeicons/gif.png"));
    }
    else if ( mimeType.toLower().contains(QStringLiteral("html")) )
    {
        icon = QPixmap(QStringLiteral(":/mime/mimeicons/html.png"));
    }
    else if ( mimeType.toLower().contains(QStringLiteral("java")) )
    {
        icon = QPixmap(QStringLiteral(":/mime/mimeicons/java.png"));
    }
    else if ( mimeType.toLower().contains(QStringLiteral("jpg")) )
    {
        icon = QPixmap(QStringLiteral(":/mime/mimeicons/jpg.png"));
    }
    else if ( mimeType.toLower().contains(QStringLiteral("pdf")) )
    {
        icon = QPixmap(QStringLiteral(":/mime/mimeicons/pdf.png"));
    }
    else if ( mimeType.toLower().contains(QStringLiteral("vnd.openxmlformats-officedocument.presentationml")) )
    {
        icon = QPixmap(QStringLiteral(":/mime/mimeicons/powerpoint.png"));
    }
    else if ( mimeType.toLower().contains(QStringLiteral("msword")) )
    {
        icon = QPixmap(QStringLiteral(":/mime/mimeicons/word.png"));
    }
    else if ( mimeType.toLower().contains(QStringLiteral("zip")) )
    {
        icon = QPixmap(QStringLiteral(":/mime/mimeicons/zip.png"));
    }
    else
    {
        icon = QPixmap(QStringLiteral(":/mime/mimeicons/empty.png"));
    }
    return icon;
}

QString CommonsFunctions::prefixFromMimeType(const QString &type)
{
#ifdef ALEPHERP_NO_LIBMAGIC
    QFreeDesktopMime mime;
    return mime.acronym(type);
#endif
}

/**
 * @brief CommonsFunctions::recursiveDirEntryList
 * Devuelve un string list con todos los elementos que cuelgan de un directorio
 * @param path
 * @return
 */
QStringList CommonsFunctions::recursiveDirEntryList(const QString &path)
{
    QStringList result;
    QFileInfo item(path);
    if ( item.exists() && item.isDir() )
    {
        QDir dir(path);
        result = dir.entryList();
        foreach (const QString &subItem, result)
        {
            result.append(recursiveDirEntryList(subItem));
        }
    }
    return result;
}


/**
	Esta función se encarga de validar el CIF. Para ello, seguimos lo siguiente:
	El CIF consta de 9 dígitos alfanumémericos con la siguiente estructura:
	T  P  P  N  N  N  N  N  C
	Siendo:
	T: Letra de tipo de Organización, una de las siguientes: A,B,C,D,E,F,G,H,K,L,M,N,P,Q,S.
	P: Código provincial.
	N: Númeración secuenial dentro de la provincia.
	C: Dígito de control, un número o letra: A-1,B-2,C-3,D-4,E-5,F-6,G-7,H-8,I-9,J-0.

	El último es el dígito de control que puede ser un número ó una letra.
	Según la información que me habeis hecho llegar la forma en que se asigna número
	o una letra es la siguiente:
<ul>
<li>Será una LETRA si la clave de entidad es K, P, Q ó S</li>
<li>Será un NUMERO si la entidad es A, B, E ó H.</li>
<li>Para otras claves de entidad: el dígito podrá ser tanto número como letra.</li>
</ul>
Las operaciones para calcular este dígito de control se realizan sobre los siete dígitos centrales y son las siguientes:
<ul>
<li>Sumar los dígitos de la posiciones pares. Suma = A </li>
<li>Para cada uno de los dígitos de la posiciones impares, multiplicarlo por 2 y
sumar los dígitos del resultado.</li>
<li>Ej.: ( 8 * 2 = 16 --> 1 + 6 = 7 ). Acumular el resultado. Suma = B</li>
<li>Calcular la suma A + B = C </li>
<li>Tomar sólo el dígito de las unidades de C y restárselo a 10. Esta resta nos da D.</li>
<li>A partir de D ya se obtiene el dígito de control. Si ha de ser numérico es
directamente D y si se trata de una letra se corresponde con la relación:
 A = 1, B = 2, C = 3, D = 4, E = 5, F = 6, G = 7, H = 8, I = 9, J = 10</li>
*/
bool CommonsFunctions::cifValid(const QString &cif)
{
    int A = 0;
    int B = 0;
    int C;
    QString temp;
    int D;
    QString comprobacionLetras = "ABCDEFGHIJ";

    // El CIF tiene 9 digitos. Lo que no tenga 9 digitos, no es válido.
    if ( cif.length() != 9 )
    {
        return false;
    }
    // Si es un CIF, el primer dígito debe ser una letra, y después 8 números
    QRegExp regexp ("[A-Za-z][0-9]{8}");
    if ( !regexp.exactMatch (cif) )
    {
        return false;
    }

    // Sumamos los dígitos de las posiciones pares. Suma = A.
    // Ejemplo: A58818501. Utilizamos los dígitos centrales 58818501
    // Sumamos los dígitos pares: A = 8 + 1 + 5 = 14
    for ( int i = 2 ; i < 8 ; i = i + 2 )
    {
        A = A + cif.at(i).digitValue();
    }
    // Para cada uno de los dígitos de la posiciones impares, multiplicarlo por 2 y sumar
    // los dígitos del resultado.
    // Ej.: Posiciones impares:
    //	5 * 2 = 10 -> 1 + 0 = 1
    //	8 * 2 = 16 -> 1 + 6 = 7
    //	8 * 2 = 16 -> 1 + 6 = 7
    //	0 * 2 = 0 -> = 0
    //	Sumamos los resultados: B = 1 + 7 + 7 + 0 = 15
    for ( int i = 1 ; i < 8 ; i = i + 2 )
    {
        int posImpar;
        posImpar = cif.at(i).digitValue() * 2;
        if ( posImpar > 9 )
        {
            temp = QString("%1").arg(posImpar);
            B = B + temp.at(0).digitValue() + temp.at(1).digitValue();
        }
        else
        {
            B = B + posImpar;
        }
    }
    // Calcular la suma A + B = C
    // Ej: Suma parcial: C = A + B = 14 + 15 = 29
    C = A + B;
    // Tomar sólo el dígito de las unidades de C y restárselo a 10. Esta resta nos da D.
    // Ej: El dígito de las unidades de C es 9.
    // Se lo restamos a 10 y nos da: D = 10 - 9 = 1
    if ( C > 10 )
    {
        temp = QString("%1").arg(C);
        D = 10 - temp.at(1).digitValue();
    }
    else
    {
        D = 10 - C;
    }
    if ( D == 10 )
    {
        D = 0;
    }

    // A partir de D ya se obtiene el dígito de control. Si ha de ser numérico es
    // directamente D y si se trata de una letra se corresponde con la relación:
    // Será una LETRA si la clave de entidad es K, P, Q ó S<
    // Será un NUMERO si la entidad es A, B, E ó H.
    if ( cif.at(0) == 'K' || cif.at(0) == 'P' || cif.at(0) == 'Q' || cif.at(0) == 'S' )
    {
        if ( cif.at(8) == comprobacionLetras.at(D-1) )
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else if ( cif.at(0) == 'A' || cif.at(0) == 'B' || cif.at(0) == 'E' || cif.at(0) == 'H' )
    {
        if ( D == cif.at(8).digitValue() )
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return true;
    }
}

/**
	Comprueba la validez de un NIF. El nif debe venir en el formato
	NNNNNNNNL
	donde N son numeros y L una letra. Es decir, 8 números y una letra.
*/
bool CommonsFunctions::nifValid(const QString &nifOriginal)
{
    // Eliminamos los "-" que el usuario haya podido meter
    QString nif = nifOriginal;
    nif = nif.remove(QChar('-'));

    const char letra[] = "TRWAGMYFPDXBNJZSQVHLCKE";
    const int kTAM = 9; // numero de cifras para el DNI
    int dni;
    bool ok;

    if ( nif.length() != kTAM )
    {
        return false;
    }
    // Si es un NIF, tenemos 8 dígitos, y después una letra.
    QRegExp regexp ("[0-9]{8}[A-Za-z]");
    if ( !regexp.exactMatch (nif) )
    {
        return false;
    }
    dni = nif.left(8).toInt(&ok);
    if ( !ok )
    {
        return false;
    }
    dni %= 23;
    if ( letra[dni] == nif.at(8) )
    {
        return true;
    }
    else
    {
        return false;
    }
}

int toInt(const char c)
{
    return c-'0';
}

/**
 * @brief CommonsFunctions::creditCardValidLuhnTest
 * Comprueba la validez de una tarjeta de crédito, según el test de Luhn
 * http://rosettacode.org/wiki/Luhn_test_of_credit_card_numbers#C.2B.2B
 * @param creditCard
 * @return
 */
bool CommonsFunctions::creditCardValidLuhnTest(const QString &creditCard)
{
    QString temp(creditCard);
    QByteArray ba = temp.remove(QChar(' ')).toLatin1();
    const char *id = ba.constData();
    bool is_odd_dgt = true;
    int s = 0;
    const char *cp;

    for(cp=id; *cp; cp++);
    while(cp > id)
    {
        --cp;
        int k = toInt(*cp);
        if (is_odd_dgt)
        {
            s += k;
        }
        else
        {
            s += (k!=9)? (2*k)%9 : 9;
        }
        is_odd_dgt = !is_odd_dgt;
    }
    return 0 == s%10;
}

/**
 * @brief CommonsFunctions::dateFormat
 * Devuelve la fecha que utilizará el sistema. Se determinará de varias formas: Si existe una variable de entorno establecida
 * o si no la hay, a partir de la de sistema
 * @return
 */
QString CommonsFunctions::dateFormat()
{
    if ( EnvVars::instance()->contains(AlephERP::stDateFormat) )
    {
        return EnvVars::instance()->var(AlephERP::stDateFormat).toString();
    }
    return alephERPSettings->locale()->dateFormat(QLocale::ShortFormat);
}

QString CommonsFunctions::dateTimeFormat()
{
    if ( EnvVars::instance()->contains(AlephERP::stDateTimeFormat) )
    {
        return EnvVars::instance()->var(AlephERP::stDateTimeFormat).toString();
    }
    return alephERPSettings->locale()->dateTimeFormat(QLocale::ShortFormat);
}

bool CommonsFunctions::openPDF(QUrl urlFile)
{
    // Si hay definido un visor externo de PDF utilizamos ese
    if ( !alephERPSettings->externalPDFViewer().isEmpty() )
    {
        QString linea;

        linea = QString("%1 %2").
                arg( QDir::fromNativeSeparators ( alephERPSettings->externalPDFViewer() ) ).
                arg( urlFile.toLocalFile() );
        QProcess::execute ( linea );
    }
    else
    {
        if ( !alephERPSettings->internalPDFViewer() )
        {
            QDesktopServices::openUrl(urlFile);
        }
    }
    return true;
    /*	QString filename;

    	Poppler::Document* document = Poppler::Document::load(urlFile.path());
    	if (!document || document->isLocked()) {
    		delete document;
    		return false;
    	}

    	// Access page of the PDF file
    	Poppler::Page* pdfPage = document->page(0);  // Document starts at page 0
    	if (pdfPage == 0) {
    		delete document;
    		return false;
    	}

    	// Generate a QImage of the rendered page
    	QImage image = pdfPage->renderToImage();
    	if (image.isNull()) {
    		delete document;
    		return false;
    	}

    // ... use image ...
    	VisorPDFForm *visor = new VisorPDFForm;
    	visor->show();
    	visor->setImage(image);

    // after the usage, the page must be deleted
    	delete pdfPage;

    	delete document;
    	return true;*/
}

void CommonsFunctions::centerWindow (QDialog *dlg)
{
    QDesktopWidget *desktop = QApplication::desktop();
    QRect rectDesktop = desktop->availableGeometry();
    QRect rectWindow = dlg->frameGeometry();
    QPoint posFinal((rectDesktop.width() - rectWindow.width()) / 2, (rectDesktop.height() - rectWindow.height()) / 2);
    dlg->move(posFinal);
}

/**
  Devuelve el primer elemento QDialog, que está entre los ancestros de wid
  */
QDialog * CommonsFunctions::parentDialog(QWidget *wid)
{
    QDialog *dlg = NULL;
    QObject *tmp = wid->parent();
    while ( dlg == NULL && tmp != NULL )
    {
        dlg = qobject_cast<QDialog *> (tmp);
        tmp = tmp->parent();
    }
    return dlg;
}

AERPBaseDialog *CommonsFunctions::aerpParentDialog(QObject *widget)
{
    AERPBaseDialog *dlg = NULL;
    if ( widget == NULL )
    {
        return NULL;
    }
    QObject *tmp = widget->parent();
    while ( dlg == NULL && tmp != NULL )
    {
        dlg = qobject_cast<AERPBaseDialog *> (tmp);
        tmp = tmp->parent();
    }
    return dlg;
}

QWidget *CommonsFunctions::firstFocusWidget(QWidget *widget)
{
    QWidget *w = widget->nextInFocusChain();
    do
    {
        if ( w != NULL && w->focusPolicy() != Qt::NoFocus )
        {
            QLabel *lbl = qobject_cast<QLabel *>(w);
            if ( lbl )
            {
                if ( lbl->buddy() != NULL && lbl->buddy()->property(AlephERP::stAerpControl).toBool() && lbl->buddy()->property(AlephERP::stDataEditable).toBool() )
                {
                    return w;
                }
            }
            else if ( !w->property(AlephERP::stAerpControl).toBool() || w->property(AlephERP::stDataEditable).toBool() )
            {
                return w;
            }
        }
        w = w->nextInFocusChain();
    } while (w);
    return NULL;
}

QString CommonsFunctions::sizeHuman(double sizeOnBytes)
{
    QStringList list;
    list << "Kb" << "Mb" << "Gb" << "Tb";

    QStringListIterator i(list);
    QString unit("bytes");

    while(sizeOnBytes >= 1024.0 && i.hasNext())
    {
        unit = i.next();
        sizeOnBytes /= 1024.0;
    }
    return (alephERPSettings->locale()->toString(sizeOnBytes, 'f', 2) + ' ' + unit);
}

/**
 * Elimina todo el contenido de un directorio
 **/
void CommonsFunctions::removeContentsFromDir(const QString &dirName, const QStringList &exceptions)
{
    QDir dir(dirName);

    if (dir.exists(dirName))
    {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst))
        {
            if (info.suffix() != "sqlite" && info.suffix() != "fdb" && !exceptions.contains(info.fileName()))
            {
                if (info.isDir())
                {
                    CommonsFunctions::removeContentsFromDir(info.absoluteFilePath(), exceptions);
                }
                else
                {
                    QFile::remove(info.absoluteFilePath());
                }
            }
        }
    }
}

#ifdef ALEPHERP_SMTP_SUPPORT
/**
 * @brief CommonsFunctions::loadSMTPPlugin
 * Carga el plugin para el envío de correos
 * @param name
 * @return
 */
AERPSMTPIface *CommonsFunctions::loadSMTPPlugin(const QString &pluginName, QString &error)
{
#ifdef ALEPHERP_BUILT_IN_SMTP_SUPPORT
    Q_UNUSED(pluginName)
    Q_UNUSED(error)
    static AERPBuiltInSMTPIface iface;
    return &iface;
#else
    AERPSMTPIface *iface = NULL;
    static QPluginLoader *pluginLoader;

    if ( pluginLoader == NULL )
    {
        QString reportPluginDir = QString("%1/plugins/smtp").arg(QApplication::applicationDirPath());
#if defined(Q_OS_WIN)
        QString pathPluginFile = QString("%1/%2.dll").arg(reportPluginDir).arg(pluginName);
#else
        QString pathPluginFile = QString("%1/lib%2.so").arg(reportPluginDir).arg(pluginName);
#endif
        QFile pluginFile(pathPluginFile);
        if (!pluginFile.exists())
        {
            error = QObject::trUtf8("No existe el plugin indicado: %1").arg(pluginName);
            return iface;
        }
        pluginLoader = new QPluginLoader(pathPluginFile, qApp);
    }
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    if ( !pluginLoader->isLoaded() )
    {
        if ( !pluginLoader->load() )
        {
            CommonsFunctions::restoreOverrideCursor();
            error = QObject::trUtf8("Ha ocurrido un error cargando el plugin: %1. \nEl error es: %2").
                    arg(pluginName).arg(pluginLoader->errorString());
        }
        else
        {
            iface = qobject_cast<AERPSMTPIface *>(pluginLoader->instance());
            CommonsFunctions::restoreOverrideCursor();
            if ( !iface )
            {
                error = QObject::trUtf8("No se cargó el plugin: %1. \nRazón desconocida.").arg(pluginName);
            }
        }
    }
    else
    {
        iface = qobject_cast<AERPSMTPIface *>(pluginLoader->instance());
        CommonsFunctions::restoreOverrideCursor();
        if ( !iface )
        {
            error = QObject::trUtf8("No se cargó el plugin: %1. \nRazón desconocida.").arg(pluginName);
        }
    }
    return iface;
#endif
}
#endif

#ifdef ALEPHERP_DOC_MANAGEMENT
AERPDocumentDAOPluginIface *CommonsFunctions::documentDAOPluginInstance(QString &error)
{
    AERPDocumentDAOPluginIface *iface = NULL;
    static QPluginLoader *pluginLoader;

    QString pluginName = EnvVars::instance()->var(AlephERP::stDocMngmnt).toString();
    if ( pluginName.isEmpty() )
    {
        qDebug() << "CommonsFunctions::loadPlugin: El sistema de gestión documental no está preparado para este usuario.";
        error = QObject::trUtf8("CommonsFunctions::loadPlugin: El sistema de gestión documental no está preparado para este usuario.");
        return NULL;
    }

    if ( pluginLoader == NULL )
    {
        QString docPluginDir = QString("%1/plugins/doc").arg(QApplication::applicationDirPath());
#if defined(Q_OS_WIN)
        QString pathPluginFile = QString("%1/%2.dll").arg(docPluginDir).arg(pluginName);
#else
        QString pathPluginFile = QString("%1/lib%2.so").arg(docPluginDir).arg(pluginName);
#endif
        QFile pluginFile(pathPluginFile);
        if (!pluginFile.exists())
        {
            error = QObject::trUtf8("No existe el plugin indicado: %1").arg(pluginName);
            return iface;
        }
        else
        {
            // Este comando puede dar "not real memory leaks" en valgrind. La razón, se puede encontrar aquí:
            // https://bugreports.qt-project.org/browse/QTBUG-25279
            pluginLoader = new QPluginLoader(pathPluginFile, qApp);
        }
    }
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    if ( !pluginLoader->isLoaded() )
    {
        if ( !pluginLoader->load() )
        {
            CommonsFunctions::restoreOverrideCursor();
            error = QObject::trUtf8("Ha ocurrido un error cargando el plugin: %1. \nEl error es: %2").
                    arg(pluginName).arg(pluginLoader->errorString());
        }
        else
        {
            iface = qobject_cast<AERPDocumentDAOPluginIface *>(pluginLoader->instance());
            CommonsFunctions::restoreOverrideCursor();
            if ( !iface )
            {
                error = QObject::trUtf8("No se cargó el plugin: %1. \nRazón desconocida.").arg(pluginName);
            }
        }
    }
    else
    {
        iface = qobject_cast<AERPDocumentDAOPluginIface *>(pluginLoader->instance());
        CommonsFunctions::restoreOverrideCursor();
        if ( !iface )
        {
            error = QObject::trUtf8("No se cargó el plugin: %1. \nRazón desconocida.").arg(pluginName);
        }
    }
    return iface;
}

bool CommonsFunctions::initRepo(QString &error)
{
    AERPDocumentDAOPluginIface *iface = CommonsFunctions::documentDAOPluginInstance(error);
    if ( iface == NULL || !error.isEmpty() )
    {
        return false;
    }
    if ( !iface->canInitRepo() )
    {
        error = trUtf8("Este driver no permite iniciar el repositorio.");
        return false;
    }
    if ( !iface->initRepo() )
    {
        error = iface->lastMessage();
        return false;
    }
    return true;
}

bool CommonsFunctions::isRepoInit(QString &error)
{
    AERPDocumentDAOPluginIface *iface = CommonsFunctions::documentDAOPluginInstance(error);
    if ( iface == NULL )
    {
        return false;
    }
    return iface->isRepoInit();
}

bool CommonsFunctions::connectDocRepo(QString &error)
{
    AERPDocumentDAOPluginIface *iface = CommonsFunctions::documentDAOPluginInstance(error);
    if ( iface == NULL )
    {
        return false;
    }
    bool r = iface->connect();
    error = iface->lastMessage();
    return r;
}

bool CommonsFunctions::disconnectDocRepo(QString &error)
{
    AERPDocumentDAOPluginIface *iface = CommonsFunctions::documentDAOPluginInstance(error);
    if ( iface == NULL )
    {
        return false;
    }
    bool r = iface->disconnect();
    error = iface->lastMessage();
    return r;
}

AERPScannerIface *CommonsFunctions::loadScannerPlugin(QString &error)
{
    QString pluginName = EnvVars::instance()->var(AlephERP::stScanPlugin).toString();
    AERPScannerIface *iface = NULL;
    static QPluginLoader *pluginLoader;

    if ( pluginName.isEmpty() )
    {
        qDebug() << "CommonsFunctions::loadScannerPlugin: No existe un plugin para escaneo.";
        error = QObject::trUtf8("CommonsFunctions::loadScannerPlugin: El sistema de escaneo no está preparado para este usuario.");
        QLogger::QLog_Error(AlephERP::stLogOther, QString("CommonsFunctions::loadScannerPlugin: [%1]").arg(error));
        return NULL;
    }
    if ( pluginLoader == NULL )
    {
        QString docPluginDir = QString("%1/plugins/doc").arg(QApplication::applicationDirPath());
#if defined(Q_OS_WIN)
        QString pathPluginFile = QString("%1/%2.dll").arg(docPluginDir).arg(pluginName);
#else
        QString pathPluginFile = QString("%1/lib%2.so").arg(docPluginDir).arg(pluginName);
#endif
        QFile pluginFile(pathPluginFile);
        if (!pluginFile.exists())
        {
            error = QObject::trUtf8("No existe el plugin indicado: %1").arg(pluginName);
            QLogger::QLog_Error(AlephERP::stLogOther, QString("CommonsFunctions::loadScannerPlugin: [%1]").arg(error));
            return iface;
        }
        pluginLoader = new QPluginLoader(pathPluginFile, qApp);
    }
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    if ( !pluginLoader->isLoaded() )
    {
        if ( !pluginLoader->load() )
        {
            CommonsFunctions::restoreOverrideCursor();
            error = QObject::trUtf8("Ha ocurrido un error cargando el plugin: %1. \nEl error es: %2").
                    arg(pluginName).arg(pluginLoader->errorString());
            QLogger::QLog_Error(AlephERP::stLogOther, QString("CommonsFunctions::loadScannerPlugin: [%1]").arg(error));
            return NULL;
        }
        else
        {
            iface = qobject_cast<AERPScannerIface *>(pluginLoader->instance());
            CommonsFunctions::restoreOverrideCursor();
            if ( !iface )
            {
                error = QObject::trUtf8("No se cargó el plugin: %1. \nRazón desconocida.").arg(pluginName);
                QLogger::QLog_Error(AlephERP::stLogOther, QString("CommonsFunctions::loadScannerPlugin: [%1]").arg(error));
                return NULL;
            }
        }
    }
    else
    {
        iface = qobject_cast<AERPScannerIface *>(pluginLoader->instance());
        CommonsFunctions::restoreOverrideCursor();
        if ( !iface )
        {
            error = QObject::trUtf8("No se cargó el plugin: %1. \nRazón desconocida.").arg(pluginName);
            QLogger::QLog_Error(AlephERP::stLogOther, QString("CommonsFunctions::loadScannerPlugin: [%1]").arg(error));
            return NULL;
        }
    }
    return iface;
}

AERPExtractPlainContentIface *CommonsFunctions::loadPlainContentPlugin(QString &error)
{
    QString pluginName = EnvVars::instance()->var(AlephERP::stExtractContentPlugin).toString();
    if ( pluginName.isEmpty() )
    {
        pluginName = "plaincontentplugin";
    }
    AERPExtractPlainContentIface *iface = NULL;
    static QPluginLoader *pluginLoader;

    if ( pluginName.isEmpty() )
    {
        qDebug() << "CommonsFunctions::loadPlainContentPlugin: No existe un plugin para extracción de contenido.";
        error = QObject::trUtf8("CommonsFunctions::loadPlainContentPlugin: El sistema de extracción de contenido no está preparado para este usuario.");
        return NULL;
    }
    if ( pluginLoader == NULL )
    {
        QString docPluginDir = QString("%1/plugins/doc").arg(QApplication::applicationDirPath());
#if defined(Q_OS_WIN)
        QString pathPluginFile = QString("%1/%2.dll").arg(docPluginDir).arg(pluginName);
#else
        QString pathPluginFile = QString("%1/lib%2.so").arg(docPluginDir).arg(pluginName);
#endif
        QFile pluginFile(pathPluginFile);
        if (!pluginFile.exists())
        {
            error = QObject::trUtf8("No existe el plugin indicado: %1").arg(pluginName);
            return iface;
        }
        pluginLoader = new QPluginLoader(pathPluginFile, qApp);
    }
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    if ( !pluginLoader->isLoaded() )
    {
        if ( !pluginLoader->load() )
        {
            CommonsFunctions::restoreOverrideCursor();
            error = QObject::trUtf8("Ha ocurrido un error cargando el plugin: %1. \nEl error es: %2").
                    arg(pluginName).arg(pluginLoader->errorString());
            return NULL;
        }
        else
        {
            iface = qobject_cast<AERPExtractPlainContentIface *>(pluginLoader->instance());
            CommonsFunctions::restoreOverrideCursor();
            if ( !iface )
            {
                error = QObject::trUtf8("No se cargó el plugin: %1. \nRazón desconocida.").arg(pluginName);
                return NULL;
            }
        }
    }
    else
    {
        iface = qobject_cast<AERPExtractPlainContentIface *>(pluginLoader->instance());
        CommonsFunctions::restoreOverrideCursor();
        if ( !iface )
        {
            error = QObject::trUtf8("No se cargó el plugin: %1. \nRazón desconocida.").arg(pluginName);
            return NULL;
        }
    }
    return iface;
}
#endif
