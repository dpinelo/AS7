message( "---------------------------------" )
message( "CONSTRUYENDO MUPARSER..." )
message( "---------------------------------" )

include( ../../../config.pri)

TARGET = muparser
CONFIG += staticlib

!android {
    TEMPLATE = lib
}

INCLUDEPATH += include/

# Instalamos los archivos include... Es importante que esto esté antes de DESTDIR.
includesDest.path = $$BUILDPATH/include
includesDest.files = $$HEADERS
INSTALLS += includesDest

HEADERS = include/muParser.h \
          include/muParserBase.h \
          include/muParserBytecode.h \
          include/muParserCallback.h \
          include/muParserDef.h \
          include/muParserDLL.h \
          include/muParserError.h \
          include/muParserFixes.h \
          include/muParserInt.h \
          include/muParserTemplateMagic.h \
          include/muParserTest.h \
          include/muParserToken.h
	 
SOURCES = src/muParser.cpp \
          src/muParserBase.cpp \
          src/muParserBytecode.cpp \
          src/muParserCallback.cpp \
          src/muParserDLL.cpp \
          src/muParserError.cpp \
          src/muParserInt.cpp \
          src/muParserTest.cpp \
          src/muParserTokenReader.cpp
	  
