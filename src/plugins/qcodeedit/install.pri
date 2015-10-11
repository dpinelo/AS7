#
#	QCodeEdit's installer definitions.
#
include (../../../config.pri)

### START of INSTALL section
!build_pass {
	
	target.files += libqcodeedit*
	target.path = $$BUILDPATH/plugins/designer
	
	SUB = lib \
		lib/document \
		lib/language \
		lib/qnfa \
		lib/widgets
	
	for(s,SUB) {
		eval ( includes.files *= $${s}/*.h )
	}
	
	includes.path = $${QMAKE_INCDIR_QT}/QCodeEdit
	
	features.files += qcodeedit.prf
	features.path = $$[QT_INSTALL_PREFIX]/mkspecs/features
	
        #INSTALLS += target includes features
}
### END of INSTALL section
