
#define PACKAGE foo
#include "backtrace.h"
#include <alepherpdaobusiness.h>

QFile output;
QString cmd;
QString stackFile;

void createLogFileAndInitCrashVars(int signal = SIGABRT)
{
    QString lin;
    stackFile = alephERPSettings->dataPath() + "/stacktrace.log";
    cmd = qApp->applicationDirPath() + QString("/alepherp-ch %1").arg(stackFile);
    cmd = QDir::toNativeSeparators(cmd);

    qDebug() << "stackFile:" << stackFile;
    qDebug() << "command: " << cmd;

    // Creates the stacktrace file
    output.setFileName(stackFile);
    output.open(QFile::WriteOnly);

    if ( output.isOpen() )
    {
        qDebug() << "Fichero abierto.";
        lin = QString("** alephERP crashed after receive signal: %1 **\n\nDate/Time: %2\n")
            .arg(signal)
            .arg(QDateTime::currentDateTime().toString(QString("yyyy-MM-dd hh:mm:ss")));

        lin += QString("Compilation Qt version: %1\nRunning Qt version: %2\n\n")
                 .arg(QT_VERSION_STR)
                 .arg(qVersion());

        output.write(lin.toStdString().c_str(), lin.size());
    }
}

/**
 * @brief startCrashHandler
 * Gestor de backtrace. Basado en la implementación que existe en pgModeler.
 * @param signal
 */
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
void startLinuxCrashHandler(int signal)
{
    createLogFileAndInitCrashVars(signal);

    void *stack[30];
    size_t stack_size;
    char **symbols = nullptr;
    stack_size = backtrace(stack, 30);
    symbols = backtrace_symbols(stack, stack_size);

    if( output.isOpen() )
    {
        for(size_t i=0; i < stack_size; i++)
        {
            QString lin = QString("[%1] ").arg(stack_size-1-i) + QString(symbols[i]) + QString("\n");
            output.write(lin.toStdString().c_str(), lin.size());
        }
        free(symbols);
        output.close();
    }

    /* Mostramos al usuario la traza del error */
    QDir dir;
    dir.cd(QApplication::applicationDirPath());
    exit(1 + system(cmd.toStdString().c_str()));
}
#endif

#ifdef Q_OS_WIN
LONG WINAPI windowsExceptionFilter(LPEXCEPTION_POINTERS info)
{
    createLogFileAndInitCrashVars();

    if( output.isOpen() )
    {
        windowsStackTrace(output);
        output.close();
    }

    /* Mostramos al usuario la traza del error */
    QDir dir;
    dir.cd(QApplication::applicationDirPath());
    exit(1 + system(cmd.toStdString().c_str()));
    return 0;
}

/*
static int initBfdCtx(struct bfd_ctx *bc, const char *procname)
{
    bc->handle = NULL;
    bc->symbol = NULL;

    bfd *b = bfd_openr(procname, 0);
    if (!b)
    {
        QString err = QString("Failed to open bfd from [%1].\n").arg(procname);
        output.write(err.toLatin1());
        return 1;
    }

    int r1 = bfd_check_format(b, bfd_object);
    int r2 = bfd_check_format_matches(b, bfd_object, NULL);
    int r3 = bfd_get_file_flags(b) & HAS_SYMS;

    if (!(r1 && r2 && r3))
    {
        bfd_close(b);
        QString err = QString("Failed to init bfd from [%1].\n").arg(procname);
        output.write(err.toLatin1());
        return 1;
    }

    void *symbol_table;

    unsigned dummy = 0;
    if (bfd_read_minisymbols(b, FALSE, &symbol_table, &dummy) == 0)
    {
        if (bfd_read_minisymbols(b, TRUE, &symbol_table, &dummy) < 0)
        {
            free(symbol_table);
            bfd_close(b);
            QString err = QString("Failed to read symbols from [%1].\n").arg(procname);
            output.write(err.toLatin1());
            return 1;
        }
    }

    bc->handle = b;
    bc->symbol = static_cast<asymbol **>(symbol_table);

    return 0;
}

struct bfd_ctx *getBc(struct bfd_set *set , const char *procname)
{
    while(set->name)
    {
        if (strcmp(set->name, procname) == 0)
        {
            return set->bc;
        }
        set = set->next;
    }
    struct bfd_ctx bc;
    if (initBfdCtx(&bc, procname))
    {
        return NULL;
    }
    set->next = (bfd_set *)calloc(1, sizeof(*set));
    set->bc = (bfd_ctx *) malloc(sizeof(struct bfd_ctx));
    memcpy(set->bc, &bc, sizeof(bc));
    set->name = strdup(procname);

    return set->bc;
}
*/

/**
 * @brief windowsStackTrace
 * @param output
 * Obtiene un stack trace en el momento en el que se produce el crash
 */
void windowsStackTrace(QFile &output)
{
    char procname[MAX_PATH];
    char symbol_buffer[sizeof(IMAGEHLP_SYMBOL) + 255];
    char moduleNameRaw[MAX_PATH];

    CONTEXT context;
    memset(&context, 0, sizeof(CONTEXT));
    context.ContextFlags = CONTEXT_FULL;
    RtlCaptureContext(&context);

    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    if ( !SymInitialize(process, NULL, TRUE) )
    {
        output.write("Failed to init symbol context\n.");
        return;
    }

    GetModuleFileNameA(NULL, procname, sizeof procname);

    bfd_init();
    struct bfd_set *set = (struct bfd_set *) calloc(1, sizeof(struct bfd_set *));
    struct bfd_ctx *bc = NULL;

    DWORD image;
    STACKFRAME64 stackframe;
    ZeroMemory(&stackframe, sizeof(STACKFRAME64));

#ifdef _M_IX86
    image = IMAGE_FILE_MACHINE_I386;
    stackframe.AddrPC.Offset = context.Eip;
    stackframe.AddrPC.Mode = AddrModeFlat;
    stackframe.AddrFrame.Offset = context.Ebp;
    stackframe.AddrFrame.Mode = AddrModeFlat;
    stackframe.AddrStack.Offset = context.Esp;
    stackframe.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
    image = IMAGE_FILE_MACHINE_AMD64;
    stackframe.AddrPC.Offset = context.Rip;
    stackframe.AddrPC.Mode = AddrModeFlat;
    stackframe.AddrFrame.Offset = context.Rsp;
    stackframe.AddrFrame.Mode = AddrModeFlat;
    stackframe.AddrStack.Offset = context.Rsp;
    stackframe.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
    image = IMAGE_FILE_MACHINE_IA64;
    stackframe.AddrPC.Offset = context.StIIP;
    stackframe.AddrPC.Mode = AddrModeFlat;
    stackframe.AddrFrame.Offset = context.IntSp;
    stackframe.AddrFrame.Mode = AddrModeFlat;
    stackframe.AddrBStore.Offset = context.RsBSP;
    stackframe.AddrBStore.Mode = AddrModeFlat;
    stackframe.AddrStack.Offset = context.IntSp;
    stackframe.AddrStack.Mode = AddrModeFlat;
#endif

    for (size_t i = 0; i < 128; i++)
    {
        BOOL result = StackWalk64(
                    image,
                    process,
                    thread,
                    &stackframe,
                    &context,
                    NULL,
                    SymFunctionTableAccess64,
                    SymGetModuleBase64,
                    NULL);

        if (!result)
        {
            break;
        }

        char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        // Obtenemos nombre del fichero fuente, y la línea del error...
        DWORD moduleBase = SymGetModuleBase(process, stackframe.AddrPC.Offset);
        const char * moduleName = "[unknown module]";
        if (moduleBase &&
            GetModuleFileNameA((HINSTANCE)moduleBase, moduleNameRaw, MAX_PATH))
        {
            moduleName = moduleNameRaw;
            // bc = getBc(set, moduleName);
        }

        DWORD64 displacement = 0;
        if (SymFromAddr(process, stackframe.AddrPC.Offset, &displacement, symbol))
        {
            QByteArray ba(symbol->Name, symbol->NameLen);
            QString txt = QString("[%1]: [%2]: ").arg(i).arg(moduleName);
            ba = ba.prepend(txt.toUtf8());
            ba = ba.append("\n");
            output.write(ba);
        }
        else
        {
            QString line = QString("[%1]: ???\n").arg(i);
            output.write(line.toUtf8());
        }
    }
    SymCleanup(process);
}

#endif
