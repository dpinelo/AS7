message( "---------------------------------" )
message( "CONSTRUYENDO OPENRPTPLUGIN..." )
message( "---------------------------------" )

TEMPLATE = subdirs

SUBDIRS = openrpt/common \
          openrpt/OpenRPT/Dmtx_Library \
          openrpt/OpenRPT/renderer \
          openrpt/OpenRPT/wrtembed \
          openrpt/OpenRPT/writer \
          scriptplugin \
          alepherpplugin

CONFIG += ordered
