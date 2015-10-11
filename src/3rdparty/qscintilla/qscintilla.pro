# The project file for the QScintilla library.
#
# Copyright (c) 2012 Riverbank Computing Limited <info@riverbankcomputing.com>
# 
# This file is part of QScintilla.
# 
# This file may be used under the terms of the GNU General Public
# License versions 2.0 or 3.0 as published by the Free Software
# Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
# included in the packaging of this file.  Alternatively you may (at
# your option) use any later version of the GNU General Public
# License if such license has been publicly approved by Riverbank
# Computing Limited (or its successors, if any) and the KDE Free Qt
# Foundation. In addition, as a special exception, Riverbank gives you
# certain additional rights. These rights are described in the Riverbank
# GPL Exception version 1.1, which can be found in the file
# GPL_EXCEPTION.txt in this package.
# 
# If you are unsure which license is appropriate for your use, please
# contact the sales department at sales@riverbankcomputing.com.
# 
# This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
# WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

message( "---------------------------------" )
message( "CONSTRUYENDO QSCINTILLA..." )
message( "---------------------------------" )


# This must be kept in sync with Python/configure.py, Python/configure-old.py,
# example-Qt4Qt5/application.pro and designer-Qt4Qt5/designer.pro.
!win32:VERSION = 11.0.0

include( ../../../config.pri )

TEMPLATE = lib
TARGET = qscintilla2
CONFIG += qt warn_off dll thread
INCLUDEPATH = . include lexlib src Qt4Qt5
DEFINES = QSCINTILLA_MAKE_DLL SCINTILLA_QT SCI_LEXER

QT += widgets \
      printsupport

greaterThan(QT_MINOR_VERSION, 1) {
    macx:QT += macextras
}

win32-msvc* {
    CONFIG(release, debug|release) {
        MOC_DIR = $$ALEPHERPPATH/build/tmp/$$QT_VERSION/$$BUILDTYPE/$$APPNAME/release/moc
    }
    CONFIG(debug, debug|release) {
        MOC_DIR = $$ALEPHERPPATH/build/tmp/$$QT_VERSION/$$BUILDTYPE/$$APPNAME/debug/moc
    }
}

# Comment this in if you want the internal Scintilla classes to be placed in a
# Scintilla namespace rather than pollute the global namespace.
#DEFINES += SCI_NAMESPACE

# Handle both Qt v4 and v3.
unix {
    target.path =  $$BUILDPATH/lib
    header.path =  $$BUILDPATH/include
    header.files = Qsci
    trans.path =   $$BUILDPATH/bin
    trans.files =  qscintilla_*.qm
}
win32 {
    target.path =  $$BUILDPATH
    header.path =  $$BUILDPATH/include
    header.files = Qsci
    trans.path =   $$BUILDPATH
    trans.files =  qscintilla_*.qm
}

#INSTALLS += header trans target

HEADERS = \
    Qt4Qt5/Qsci/qsciglobal.h \
    Qt4Qt5/Qsci/qsciscintilla.h \
    Qt4Qt5/Qsci/qsciscintillabase.h \
    Qt4Qt5/Qsci/qsciabstractapis.h \
    Qt4Qt5/Qsci/qsciapis.h \
    Qt4Qt5/Qsci/qscicommand.h \
    Qt4Qt5/Qsci/qscicommandset.h \
    Qt4Qt5/Qsci/qscidocument.h \
    Qt4Qt5/Qsci/qscilexer.h \
    Qt4Qt5/Qsci/qscilexerbash.h \
    Qt4Qt5/Qsci/qscilexerbatch.h \
    Qt4Qt5/Qsci/qscilexercmake.h \
    Qt4Qt5/Qsci/qscilexercpp.h \
    Qt4Qt5/Qsci/qscilexercsharp.h \
    Qt4Qt5/Qsci/qscilexercss.h \
    Qt4Qt5/Qsci/qscilexercustom.h \
    Qt4Qt5/Qsci/qscilexerd.h \
    Qt4Qt5/Qsci/qscilexerdiff.h \
    Qt4Qt5/Qsci/qscilexerfortran.h \
    Qt4Qt5/Qsci/qscilexerfortran77.h \
    Qt4Qt5/Qsci/qscilexerhtml.h \
    Qt4Qt5/Qsci/qscilexeridl.h \
    Qt4Qt5/Qsci/qscilexerjava.h \
    Qt4Qt5/Qsci/qscilexerjavascript.h \
    Qt4Qt5/Qsci/qscilexerlua.h \
    Qt4Qt5/Qsci/qscilexermakefile.h \
    Qt4Qt5/Qsci/qscilexermatlab.h \
    Qt4Qt5/Qsci/qscilexeroctave.h \
    Qt4Qt5/Qsci/qscilexerpascal.h \
    Qt4Qt5/Qsci/qscilexerperl.h \
    Qt4Qt5/Qsci/qscilexerpostscript.h \
    Qt4Qt5/Qsci/qscilexerpov.h \
    Qt4Qt5/Qsci/qscilexerproperties.h \
    Qt4Qt5/Qsci/qscilexerpython.h \
    Qt4Qt5/Qsci/qscilexerruby.h \
    Qt4Qt5/Qsci/qscilexerspice.h \
    Qt4Qt5/Qsci/qscilexersql.h \
    Qt4Qt5/Qsci/qscilexertcl.h \
    Qt4Qt5/Qsci/qscilexertex.h \
    Qt4Qt5/Qsci/qscilexerverilog.h \
    Qt4Qt5/Qsci/qscilexervhdl.h \
    Qt4Qt5/Qsci/qscilexerxml.h \
    Qt4Qt5/Qsci/qscilexeryaml.h \
    Qt4Qt5/Qsci/qscimacro.h \
    Qt4Qt5/Qsci/qscistyle.h \
    Qt4Qt5/Qsci/qscistyledtext.h \
    Qt4Qt5/ListBoxQt.h \
    Qt4Qt5/SciClasses.h \
    Qt4Qt5/SciNamespace.h \
    Qt4Qt5/ScintillaQt.h \
    include/ILexer.h \
    include/Platform.h \
    include/SciLexer.h \
    include/Scintilla.h \
    include/ScintillaWidget.h \
    lexlib/Accessor.h \
    lexlib/CharacterCategory.h \
    lexlib/CharacterSet.h \
    lexlib/LexAccessor.h \
    lexlib/LexerBase.h \
    lexlib/LexerModule.h \
    lexlib/LexerNoExceptions.h \
    lexlib/LexerSimple.h \
    lexlib/OptionSet.h \
    lexlib/PropSetSimple.h \
    lexlib/StyleContext.h \
    lexlib/SubStyles.h \
    lexlib/WordList.h \
    src/AutoComplete.h \
    src/CallTip.h \
    src/CaseConvert.h \
    src/CaseFolder.h \
    src/Catalogue.h \
    src/CellBuffer.h \
    src/CharClassify.h \
    src/ContractionState.h \
    src/Decoration.h \
    src/Document.h \
    src/Editor.h \
    src/EditView.h \
    src/ExternalLexer.h \
    src/FontQuality.h \
    src/Indicator.h \
    src/KeyMap.h \
    src/LineMarker.h \
    src/Partitioning.h \
    src/PerLine.h \
    src/PositionCache.h \
    src/RESearch.h \
    src/RunStyles.h \
    src/ScintillaBase.h \
    src/Selection.h \
    src/SplitVector.h \
    src/Style.h \
    src/UnicodeFromUTF8.h \
    src/UniConversion.h \
    src/ViewStyle.h \
    src/XPM.h \
    src/EditModel.h \
    src/MarginView.h \
    lexlib/StringCopy.h \
    Qt4Qt5/Qsci/qscilexeravs.h \
    Qt4Qt5/Qsci/qscilexercoffeescript.h \
    Qt4Qt5/Qsci/qscilexerpo.h

SOURCES = \
    Qt4Qt5/qsciscintilla.cpp \
    Qt4Qt5/qsciscintillabase.cpp \
    Qt4Qt5/qsciabstractapis.cpp \
    Qt4Qt5/qsciapis.cpp \
    Qt4Qt5/qscicommand.cpp \
    Qt4Qt5/qscicommandset.cpp \
    Qt4Qt5/qscidocument.cpp \
    Qt4Qt5/qscilexer.cpp \
    Qt4Qt5/qscilexerbash.cpp \
    Qt4Qt5/qscilexerbatch.cpp \
    Qt4Qt5/qscilexercmake.cpp \
    Qt4Qt5/qscilexercpp.cpp \
    Qt4Qt5/qscilexercsharp.cpp \
    Qt4Qt5/qscilexercss.cpp \
    Qt4Qt5/qscilexercustom.cpp \
    Qt4Qt5/qscilexerd.cpp \
    Qt4Qt5/qscilexerdiff.cpp \
    Qt4Qt5/qscilexerfortran.cpp \
    Qt4Qt5/qscilexerfortran77.cpp \
    Qt4Qt5/qscilexerhtml.cpp \
    Qt4Qt5/qscilexeridl.cpp \
    Qt4Qt5/qscilexerjava.cpp \
    Qt4Qt5/qscilexerjavascript.cpp \
    Qt4Qt5/qscilexerlua.cpp \
    Qt4Qt5/qscilexermakefile.cpp \
    Qt4Qt5/qscilexermatlab.cpp \
    Qt4Qt5/qscilexeroctave.cpp \
    Qt4Qt5/qscilexerpascal.cpp \
    Qt4Qt5/qscilexerperl.cpp \
    Qt4Qt5/qscilexerpostscript.cpp \
    Qt4Qt5/qscilexerpov.cpp \
    Qt4Qt5/qscilexerproperties.cpp \
    Qt4Qt5/qscilexerpython.cpp \
    Qt4Qt5/qscilexerruby.cpp \
    Qt4Qt5/qscilexerspice.cpp \
    Qt4Qt5/qscilexersql.cpp \
    Qt4Qt5/qscilexertcl.cpp \
    Qt4Qt5/qscilexertex.cpp \
    Qt4Qt5/qscilexerverilog.cpp \
    Qt4Qt5/qscilexervhdl.cpp \
    Qt4Qt5/qscilexerxml.cpp \
    Qt4Qt5/qscilexeryaml.cpp \
    Qt4Qt5/qscimacro.cpp \
    Qt4Qt5/qscistyle.cpp \
    Qt4Qt5/qscistyledtext.cpp \
    Qt4Qt5/MacPasteboardMime.cpp \
    Qt4Qt5/InputMethod.cpp \
    Qt4Qt5/SciClasses.cpp \
    Qt4Qt5/ListBoxQt.cpp \
    Qt4Qt5/PlatQt.cpp \
    Qt4Qt5/ScintillaQt.cpp \
    lexers/LexA68k.cpp \
    lexers/LexAbaqus.cpp \
    lexers/LexAda.cpp \
    lexers/LexAPDL.cpp \
    lexers/LexAsm.cpp \
    lexers/LexAsn1.cpp \
    lexers/LexASY.cpp \
    lexers/LexAU3.cpp \
    lexers/LexAVE.cpp \
    lexers/LexAVS.cpp \
    lexers/LexBaan.cpp \
    lexers/LexBash.cpp \
    lexers/LexBasic.cpp \
    lexers/LexBullant.cpp \
    lexers/LexCaml.cpp \
    lexers/LexCLW.cpp \
    lexers/LexCmake.cpp \
    lexers/LexCOBOL.cpp \
    lexers/LexCoffeeScript.cpp \
    lexers/LexConf.cpp \
    lexers/LexCPP.cpp \
    lexers/LexCrontab.cpp \
    lexers/LexCsound.cpp \
    lexers/LexCSS.cpp \
    lexers/LexD.cpp \
    lexers/LexECL.cpp \
    lexers/LexEiffel.cpp \
    lexers/LexErlang.cpp \
    lexers/LexEScript.cpp \
    lexers/LexFlagship.cpp \
    lexers/LexForth.cpp \
    lexers/LexFortran.cpp \
    lexers/LexGAP.cpp \
    lexers/LexGui4Cli.cpp \
    lexers/LexHaskell.cpp \
    lexers/LexHTML.cpp \
    lexers/LexInno.cpp \
    lexers/LexKix.cpp \
    lexers/LexKVIrc.cpp \
    lexers/LexLaTeX.cpp \
    lexers/LexLisp.cpp \
    lexers/LexLout.cpp \
    lexers/LexLua.cpp \
    lexers/LexMagik.cpp \
    lexers/LexMarkdown.cpp \
    lexers/LexMatlab.cpp \
    lexers/LexMetapost.cpp \
    lexers/LexMMIXAL.cpp \
    lexers/LexModula.cpp \
    lexers/LexMPT.cpp \
    lexers/LexMSSQL.cpp \
    lexers/LexMySQL.cpp \
    lexers/LexNimrod.cpp \
    lexers/LexNsis.cpp \
    lexers/LexOpal.cpp \
    lexers/LexOScript.cpp \
    lexers/LexOthers.cpp \
    lexers/LexPascal.cpp \
    lexers/LexPB.cpp \
    lexers/LexPerl.cpp \
    lexers/LexPLM.cpp \
    lexers/LexPO.cpp \
    lexers/LexPOV.cpp \
    lexers/LexPowerPro.cpp \
    lexers/LexPowerShell.cpp \
    lexers/LexProgress.cpp \
    lexers/LexPS.cpp \
    lexers/LexPython.cpp \
    lexers/LexR.cpp \
    lexers/LexRebol.cpp \
    lexers/LexRuby.cpp \
    lexers/LexRust.cpp \
    lexers/LexScriptol.cpp \
    lexers/LexSmalltalk.cpp \
    lexers/LexSML.cpp \
    lexers/LexSorcus.cpp \
    lexers/LexSpecman.cpp \
    lexers/LexSpice.cpp \
    lexers/LexSQL.cpp \
    lexers/LexSTTXT.cpp \
    lexers/LexTACL.cpp \
    lexers/LexTADS3.cpp \
    lexers/LexTAL.cpp \
    lexers/LexTCL.cpp \
    lexers/LexTCMD.cpp \
    lexers/LexTeX.cpp \
    lexers/LexTxt2tags.cpp \
    lexers/LexVB.cpp \
    lexers/LexVerilog.cpp \
    lexers/LexVHDL.cpp \
    lexers/LexVisualProlog.cpp \
    lexers/LexYAML.cpp \
    lexlib/Accessor.cpp \
    lexlib/CharacterCategory.cpp \
    lexlib/CharacterSet.cpp \
    lexlib/LexerBase.cpp \
    lexlib/LexerModule.cpp \
    lexlib/LexerNoExceptions.cpp \
    lexlib/LexerSimple.cpp \
    lexlib/PropSetSimple.cpp \
    lexlib/StyleContext.cpp \
    lexlib/WordList.cpp \
    src/AutoComplete.cpp \
    src/CallTip.cpp \
    src/CaseConvert.cpp \
    src/CaseFolder.cpp \
    src/Catalogue.cpp \
    src/CellBuffer.cpp \
    src/CharClassify.cpp \
    src/ContractionState.cpp \
    src/Decoration.cpp \
    src/Document.cpp \
    src/Editor.cpp \
    src/ExternalLexer.cpp \
    src/Indicator.cpp \
    src/KeyMap.cpp \
    src/LineMarker.cpp \
    src/PerLine.cpp \
    src/PositionCache.cpp \
    src/RESearch.cpp \
    src/RunStyles.cpp \
    src/ScintillaBase.cpp \
    src/Selection.cpp \
    src/Style.cpp \
    src/UniConversion.cpp \
    src/ViewStyle.cpp \
    src/XPM.cpp \
    src/EditModel.cpp \
    src/EditView.cpp \
    src/MarginView.cpp \
    lexers/LexBibTeX.cpp \
    lexers/LexDMAP.cpp \
    lexers/LexDMIS.cpp \
    lexers/LexHex.cpp \
    lexers/LexRegistry.cpp \
    Qt4Qt5/qscilexercoffeescript.cpp \
    Qt4Qt5/qscilexeravs.cpp \
    Qt4Qt5/qscilexerpo.cpp \
    Qt4Qt5/qsciprinter.cpp

TRANSLATIONS = \
    Qt4Qt5/qscintilla_cs.ts \
    Qt4Qt5/qscintilla_de.ts \
    Qt4Qt5/qscintilla_es.ts \
    Qt4Qt5/qscintilla_fr.ts \
    Qt4Qt5/qscintilla_pt_br.ts \
    Qt4Qt5/qscintilla_ru.ts

SUBDIRS += \
    Qt4Qt5/qscintilla.pro \
    Qt4Qt5/Qt4Qt5.pro

DISTFILES += \
    Qt4Qt5/qscintilla_cs.qm \
    Qt4Qt5/qscintilla_de.qm \
    Qt4Qt5/qscintilla_es.qm \
    Qt4Qt5/qscintilla_fr.qm \
    Qt4Qt5/qscintilla_pt_br.qm \
    Qt4Qt5/qscintilla_ru.qm \
    Qt4Qt5/qscintilla_cs.ts \
    Qt4Qt5/qscintilla_de.ts \
    Qt4Qt5/qscintilla_es.ts \
    Qt4Qt5/qscintilla_fr.ts \
    Qt4Qt5/qscintilla_pt_br.ts \
    Qt4Qt5/qscintilla_ru.ts
