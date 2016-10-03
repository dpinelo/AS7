
#define PACKAGE foo
#include "backtrace.h"
#include <alepherpdaobusiness.h>
#ifdef Q_OS_WIN
#include <tchar.h>
#endif

QFile output;
QString cmd;
QString stackFile;

#ifdef _MSC_VER
#define SIGABRT 22
#include <io.h>
#endif
#if defined(Q_OS_UNIX) || defined(Q_OS_LINUX)
#include <signal.h>
#endif

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
/*
    CONTEXT context;
    memset(&context, 0, sizeof(CONTEXT));
    context.ContextFlags = CONTEXT_FULL;
    RtlCaptureContext(&context);
 */
        QString dumpPath = QDir::toNativeSeparators(alephERPSettings->dataPath());
        qDebug() << "Se va a crear el dump en " << dumpPath;
        createMiniDump(info, dumpPath);
        qDebug() << " Creado";
        windowsStackTrace(output, info->ContextRecord);
        output.close();
    }

    /* Mostramos al usuario la traza del error */
    QDir dir;
    dir.cd(QApplication::applicationDirPath());
    exit(1 + system(cmd.toStdString().c_str()));
    return 0;
}

#ifdef __MINGW32__
bool initBfdCtx(struct bfd_ctx *bc, const char *procname)
{
    bc->handle = NULL;
    bc->symbol = NULL;

    bfd *b = bfd_openr(procname, 0);
    if (!b)
    {
        bfd_error_type errorType = bfd_get_error();
        QString err = QString("Failed to open bfd from [%1]. Reason: [%2]\n").arg(procname).arg(bfd_errmsg(errorType));
        output.write(err.toUtf8());
        return false;
    }

    int r1 = bfd_check_format(b, bfd_object);
    int r2 = bfd_check_format_matches(b, bfd_object, NULL);
    int r3 = bfd_get_file_flags(b) & HAS_SYMS;

    if (!(r1 && r2 && r3))
    {
        bfd_error_type errorType = bfd_get_error();
        bfd_close(b);
        QString err = QString("Failed to init bfd from [%1]. Reason: [%2]\n").arg(procname).arg(bfd_errmsg(errorType));
        output.write(err.toUtf8());
        return false;
    }

    void *symbol_table;

    unsigned dummy = 0;
    if (bfd_read_minisymbols(b, FALSE, &symbol_table, &dummy) == 0)
    {
        if (bfd_read_minisymbols(b, TRUE, &symbol_table, &dummy) < 0)
        {
            bfd_error_type errorType = bfd_get_error();
            free(symbol_table);
            bfd_close(b);
            QString err = QString("Failed to read symbols from [%1]. Reason: [%2].\n").arg(procname).arg(bfd_errmsg(errorType));
            output.write(err.toLatin1());
            return false;
        }
    }

    bc->handle = b;
    bc->symbol = static_cast<asymbol **>(symbol_table);

    return true;
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
    if (!initBfdCtx(&bc, procname))
    {
        return NULL;
    }
    set->next = (bfd_set *)calloc(1, sizeof(*set));
    set->bc = (bfd_ctx *) malloc(sizeof(struct bfd_ctx));
    memcpy(set->bc, &bc, sizeof(bc));
    set->name = strdup(procname);

    return set->bc;
}

void lookup_section(bfd *abfd, asection *sec, void *opaque_data)
{
    struct find_info *data = static_cast<struct find_info *>(opaque_data);

    if (data->func)
        return;

    if (!(bfd_get_section_flags(abfd, sec) & SEC_ALLOC))
        return;

    bfd_vma vma = bfd_get_section_vma(abfd, sec);
    if (data->counter < vma || vma + bfd_get_section_size(sec) <= data->counter)
        return;

    bfd_find_nearest_line(abfd, sec, data->symbol, data->counter - vma, &(data->file), &(data->func), &(data->line));
}

void find(struct bfd_ctx *b, DWORD offset, const char **file, const char **func, unsigned *line)
{
    struct find_info data;
    data.func = NULL;
    data.symbol = b->symbol;
    data.counter = offset;
    data.file = NULL;
    data.func = NULL;
    data.line = 0;

    bfd_map_over_sections(b->handle, &lookup_section, &data);
    if (file) {
        *file = data.file;
    }
    if (func) {
        *func = data.func;
    }
    if (line) {
        *line = data.line;
    }
}
#endif

/**
 * @brief windowsStackTrace
 * @param output
 * Obtiene un stack trace en el momento en el que se produce el crash
 */
void windowsStackTrace(QFile &output, PCONTEXT context)
{
    char procname[MAX_PATH];
    char moduleNameRaw[MAX_PATH];

    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    if ( !SymInitialize(process, NULL, TRUE) )
    {
        output.write("Failed to init symbol context->\n");
        return;
    }

    GetModuleFileNameA(NULL, procname, sizeof procname);

#ifdef __MINGW32__
    bfd_init();
    struct bfd_set *set = (struct bfd_set *) calloc(1, sizeof(struct bfd_set *));
    struct bfd_ctx *bc = NULL;
#endif

    DWORD image;
    STACKFRAME64 stackframe;
    ZeroMemory(&stackframe, sizeof(STACKFRAME64));

#ifdef _M_IX86
    image = IMAGE_FILE_MACHINE_I386;
    stackframe.AddrPC.Offset = context->Eip;
    stackframe.AddrPC.Mode = AddrModeFlat;
    stackframe.AddrFrame.Offset = context->Ebp;
    stackframe.AddrFrame.Mode = AddrModeFlat;
    stackframe.AddrStack.Offset = context->Esp;
    stackframe.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
    image = IMAGE_FILE_MACHINE_AMD64;
    stackframe.AddrPC.Offset = context->Rip;
    stackframe.AddrPC.Mode = AddrModeFlat;
    stackframe.AddrFrame.Offset = context->Rsp;
    stackframe.AddrFrame.Mode = AddrModeFlat;
    stackframe.AddrStack.Offset = context->Rsp;
    stackframe.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
    image = IMAGE_FILE_MACHINE_IA64;
    stackframe.AddrPC.Offset = context->StIIP;
    stackframe.AddrPC.Mode = AddrModeFlat;
    stackframe.AddrFrame.Offset = context->IntSp;
    stackframe.AddrFrame.Mode = AddrModeFlat;
    stackframe.AddrBStore.Offset = context->RsBSP;
    stackframe.AddrBStore.Mode = AddrModeFlat;
    stackframe.AddrStack.Offset = context->IntSp;
    stackframe.AddrStack.Mode = AddrModeFlat;
#endif

    for (size_t i = 0; i < 128; i++)
    {
        BOOL result = StackWalk64(
                    image,
                    process,
                    thread,
                    &stackframe,
                    context,
                    NULL,
                    SymFunctionTableAccess64,
                    SymGetModuleBase64,
                    NULL);

        if (!result)
        {
            break;
        }

        // Obtenemos nombre del fichero fuente, y la línea del error...
        DWORD moduleBase = SymGetModuleBase(process, stackframe.AddrPC.Offset);
        const char *moduleName = "unknown module";
        if (moduleBase &&
            GetModuleFileNameA((HINSTANCE)moduleBase, moduleNameRaw, MAX_PATH))
        {
            moduleName = moduleNameRaw;
#ifdef __MINGW32__
            bc = getBc(set, moduleName);
#endif
        }

        const char * file = NULL;
        const char * func = NULL;
        unsigned line = 0;

#ifdef __MINGW32__
        if (bc)
        {
            find(bc, stackframe.AddrPC.Offset, &file, &func, &line);
        }
#endif

        QString qFileName = "unknown file name";
        QString qFuncName = "unknown function name";
        QString qLine;

        if (file == NULL)
        {
            char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
            PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol->MaxNameLen = MAX_SYM_NAME;

            DWORD64 displacement = 0;
            if (SymFromAddr(process, stackframe.AddrPC.Offset, &displacement, symbol))
            {
                QByteArray ba (symbol->Name, symbol->NameLen);
                qFuncName = QString(ba);
            }
            else
            {
                qFuncName ="???";
            }
        }
        else
        {
            qFileName = QString("%1").arg(file);
            if (func == NULL)
            {
                qFuncName = QString("0x%1").arg(stackframe.AddrPC.Offset);
            }
            else
            {
                qFuncName = QString("%1").arg(func);
                qLine = QString("%1").arg(line);
            }
        }
        QString txt = QString("[%1]: [%2]: [%3]: %4 (%5)\n").
                arg(i).
                arg(moduleName).
                arg(qFileName).
                arg(qFuncName).
                arg(qLine);
        output.write(txt.toUtf8());
    }
    SymCleanup(process);
}


BOOL CALLBACK miniDumpCallback(
    PVOID                            pParam,
    const PMINIDUMP_CALLBACK_INPUT   pInput,
    PMINIDUMP_CALLBACK_OUTPUT        pOutput
);
bool isDataSectionNeeded( const WCHAR* pModuleName );

void createMiniDump(LPEXCEPTION_POINTERS info, const QString &path)
{
    // Open the file
    QString miniDumpPath = path + "\\crashdump.dmp";
    /*
    wchar_t wArray[4000];
    miniDumpPath.toWCharArray(wArray);
    qDebug() << "Se abrirá Abierto fichero para el dump " << wArray;
    HANDLE hFile = CreateFile(_T(wArray), GENERIC_READ | GENERIC_WRITE,
        0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
        */
    QFile miniDumpFile(miniDumpPath);
    if ( !miniDumpFile.open(QIODevice::ReadWrite))
    {
        qDebug() << "No se pudo crear el fichero minidump";
        return;
    }
    HANDLE hFile = (HANDLE) _get_osfhandle(miniDumpFile.handle());

    if( ( hFile != NULL ) && ( hFile != INVALID_HANDLE_VALUE ) )
    {
        qDebug() << "Abierto fichero para el dump " << miniDumpPath;
        // Create the minidump

        MINIDUMP_EXCEPTION_INFORMATION mdei;

        mdei.ThreadId           = GetCurrentThreadId();
        mdei.ExceptionPointers  = info;
        mdei.ClientPointers     = FALSE;

        MINIDUMP_CALLBACK_INFORMATION mci;

        mci.CallbackRoutine     = (MINIDUMP_CALLBACK_ROUTINE)miniDumpCallback;
        mci.CallbackParam       = 0;

        MINIDUMP_TYPE mdt       = (MINIDUMP_TYPE)(MiniDumpWithPrivateReadWriteMemory |
                                                  MiniDumpWithDataSegs |
                                                  MiniDumpWithHandleData |
                                                  MiniDumpWithFullMemoryInfo |
                                                  MiniDumpWithThreadInfo |
                                                  MiniDumpWithUnloadedModules );

        BOOL rv = MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(),
            hFile, mdt, (info != 0) ? &mdei : 0, 0, &mci );

        if( !rv )
            _tprintf( _T("MiniDumpWriteDump failed. Error: %u \n"), GetLastError() );
        else
            _tprintf( _T("Minidump created.\n") );

        // Close the file

        CloseHandle( hFile );
    }
    else
    {
        _tprintf( _T("CreateFile failed. Error: %u \n"), GetLastError() );
    }
}

BOOL CALLBACK miniDumpCallback(
    PVOID                            pParam,
    const PMINIDUMP_CALLBACK_INPUT   pInput,
    PMINIDUMP_CALLBACK_OUTPUT        pOutput
)
{
    BOOL bRet = FALSE;

    // Check parameters

    if( pInput == 0 )
        return FALSE;

    if( pOutput == 0 )
        return FALSE;

    // Process the callbacks
    switch( pInput->CallbackType )
    {
        case IncludeModuleCallback:
        {
            // Include the module into the dump
            bRet = TRUE;
        }
        break;

        case IncludeThreadCallback:
        {
            // Include the thread into the dump
            bRet = TRUE;
        }
        break;

        case ModuleCallback:
        {
            // Are data sections available for this module ?

            if( pOutput->ModuleWriteFlags & ModuleWriteDataSeg )
            {
                // Yes, they are, but do we need them?

                if( !isDataSectionNeeded( pInput->Module.FullPath ) )
                {
                    wprintf( L"Excluding module data sections: %s \n", pInput->Module.FullPath );

                    pOutput->ModuleWriteFlags &= (~ModuleWriteDataSeg);
                }
            }

            bRet = TRUE;
        }
        break;

        case ThreadCallback:
        {
            // Include all thread information into the minidump
            bRet = TRUE;
        }
        break;

        case ThreadExCallback:
        {
            // Include this information
            bRet = TRUE;
        }
        break;

        case MemoryCallback:
        {
            // We do not include any information here -> return FALSE
            bRet = FALSE;
        }
        break;

        case CancelCallback:
            break;
    }

    return bRet;
}


///////////////////////////////////////////////////////////////////////////////
// This function determines whether we need data sections of the given module
//

bool isDataSectionNeeded( const WCHAR* pModuleName )
{
    // Check parameters

    if( pModuleName == 0 )
    {
        Q_ASSERT( _T("Parameter is null.") );
        return false;
    }

    // Extract the module name

    WCHAR szFileName[_MAX_FNAME] = L"";

    _wsplitpath( pModuleName, NULL, NULL, szFileName, NULL );


    // Compare the name with the list of known names and decide

    // Note: For this to work, the executable name must be "mididump.exe"
    if( wcsicmp( szFileName, L"alepherp" ) == 0 )
    {
        return true;
    }
    else if( wcsicmp( szFileName, L"daobusiness" ) == 0 )
    {
        return true;
    }
    else if( wcsicmp( szFileName, L"qscintilla2" ) == 0 )
    {
        return true;
    }
    else if( wcsicmp( szFileName, L"smtpclient" ) == 0 )
    {
        return true;
    }
    else if( wcsicmp( szFileName, L"qwwrichtextedit" ) == 0 )
    {
        return true;
    }
    else if( wcsicmp( szFileName, L"ntdll" ) == 0 )
    {
        return true;
    }

    // Complete
    return false;
}

#endif
