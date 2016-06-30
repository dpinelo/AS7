message( "---------------------------------" )
message( "CONSTRUYENDO MUPARSER-X..." )
message( "---------------------------------" )

include( ../../../config.pri)

TARGET = muparserx
CONFIG += staticlib

!android {
    TEMPLATE = lib
}

QMAKE_CXXFLAGS += -Wall -pedantic

SOURCES += \
    mpVariable.cpp \
    mpValueCache.cpp \
    mpValue.cpp \
    mpValReader.cpp \
    mpTokenReader.cpp \
    mpTest.cpp \
    mpScriptTokens.cpp \
    mpRPN.cpp \
    mpParserBase.cpp \
    mpParser.cpp \
    mpParserMessageProvider.cpp \
    mpPackageUnit.cpp \
    mpPackageStr.cpp \
    mpPackageNonCmplx.cpp \
    mpPackageMatrix.cpp \
    mpPackageCommon.cpp \
    mpPackageCmplx.cpp \
    mpOprtNonCmplx.cpp \
    mpOprtMatrix.cpp \
    mpOprtIndex.cpp \
    mpOprtCmplx.cpp \
    mpOprtPostfixCommon.cpp \
    mpOprtBinCommon.cpp \
    mpOprtBinAssign.cpp \
    mpIValue.cpp \
    mpIValReader.cpp \
    mpIToken.cpp \
    mpIPackage.cpp \
    mpIOprt.cpp \
    mpIfThenElse.cpp \
    mpICallback.cpp \
    mpFuncStr.cpp \
    mpFuncNonCmplx.cpp \
    mpFuncMatrix.cpp \
    mpFuncCommon.cpp \
    mpFuncCmplx.cpp \
    mpError.cpp

OTHER_FILES +=

HEADERS += \
    utGeneric.h \
    suSortPred.h \
    mpVariable.h \
    mpValueCache.h \
    mpValue.h \
    mpValReader.h \
    mpTypes.h \
    mpTokenReader.h \
    mpTest.h \
    mpStack.h \
    mpScriptTokens.h \
    mpRPN.h \
    mpParserBase.h \
    mpParser.h \
    mpParserMessageProvider.h \
    mpPackageUnit.h \
    mpPackageStr.h \
    mpPackageNonCmplx.h \
    mpPackageMatrix.h \
    mpPackageCommon.h \
    mpPackageCmplx.h \
    mpOprtNonCmplx.h \
    mpOprtMatrix.h \
    mpOprtIndex.h \
    mpOprtCmplx.h \
    mpOprtPostfixCommon.h \
    mpOprtBinCommon.h \
    mpOprtBinAssign.h \
    mpMatrixError.h \
    mpMatrix.h \
    mpIValue.h \
    mpIValReader.h \
    mpIToken.h \
    mpIPrecedence.h \
    mpIPackage.h \
    mpIOprt.h \
    mpIfThenElse.h \
    mpICallback.h \
    mpFwdDecl.h \
    mpFuncStr.h \
    mpFuncNonCmplx.h \
    mpFuncMatrix.h \
    mpFuncCommon.h \
    mpFuncCmplx.h \
    mpError.h \
    mpDefines.h

INCLUDEPATH += .
