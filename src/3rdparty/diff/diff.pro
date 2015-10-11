message( "---------------------------------" )
message( "CONSTRUYENDO DIFF..." )
message( "---------------------------------" )

include (../../../config.pri)

TEMPLATE = lib
CONFIG += staticlib
TARGET = diff

SOURCES = diff_match_patch.cpp
HEADERS = diff_match_patch.h

FORMS = 
RESOURCES = 

# Instalamos los archivos include
includesTarget.path = $$BUILDPATH/include/diff
includesTarget.files = $$HEADERS

#INSTALLS += includesTarget
