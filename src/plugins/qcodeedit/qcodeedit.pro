message( "---------------------------------" )
message( "CONSTRUYENDO QCODEEDIT..." )
message( "---------------------------------" )

TEMPLATE = subdirs

DEFINES += _QCODE_EDIT_BUILD_

CONFIG += plugin shared

contains(QT_VERSION, ^4\\.[0-8]\\..*) {
    CONFIG += designer
}
!contains(QT_VERSION, ^4\\.[0-8]\\..*) {
    QT += designer
}

SUBDIRS += lib


contains(QT_VERSION, ^4\\.[0-8]\\..*) {
    QT += gui
}
!contains(QT_VERSION, ^4\\.[0-8]\\..*) {
    QT += widgets printsupport
}

CONFIG(debug, debug|release) {
	# placeholder
	QCODE_EDIT_EXTRA_DEFINES += _DEBUG_BUILD_
} else {
	# placeholder
	QCODE_EDIT_EXTRA_DEFINES += _RELEASE_BUILD_
}

include(install.pri)
