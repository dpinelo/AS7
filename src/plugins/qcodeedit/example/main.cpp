
#include "window.h"

#include <cstdio>

#include <QApplication>

void MessageHandler(QtMsgType t, const char *msg)
{
    static const char *type[] =
    {
        "",
        "QCE::Warning  : ",
        "QCE::Critical : ",
        "QCE::Fatal    : "
    };

    //fprintf(stderr, "%s%s\n", type[t], msg);
    fprintf(stdout, "%s%s\n", type[t], msg);

    if ( t & 0x02 )
    {
        //fprintf(stderr, "aborting...\n");
        fprintf(stdout, "aborting...\n");

        *((int*)0) = 0;

#if defined(Q_OS_UNIX) && defined(_EDYUK_DEBUG_)
        abort(); // trap; generates core dump
#else
        exit(-1); // goodbye cruel world
#endif
    }
}

int main (int argc, char **argv)
{
    QApplication app(argc, argv);

    QApplication::setApplicationName("QCodeEdit::Demo");
    QApplication::setOrganizationName("Luc BRUANT aka fullmetalcoder");
    QApplication::setOrganizationDomain("qcodeedit.edyuk.org");

    qInstallMsgHandler(MessageHandler);

    Window window;

    if ( argc > 1 )
    {
        window.load(argv[argc - 1]);
    }

    window.show();

    int ret = app.exec();

    return ret;
}
