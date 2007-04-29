CLEANFILES=*~

INCLUDES =

#_comp=${COMPILE}
#COMPILE=@${_comp}

EXTRA_DIST =  common/commonconf.h common/integers.h \
  common/check_failure.h gtk2confpages.c \
  gtk2confpages.h sequview.xpm

bin_PROGRAMS = sequview

sequview_SOURCES =  archive.c archive.h \
  sequconfig.c sequconfig.h \
  configvars.c gtk2config.h gtk2config.c  \
  configvars.h main.c  \
  tmpfile.c tmpfile.h filelist.c filelist.h gui.h \
  gtk2int.c gtk2int.h gtk2error.c gtk2error.h\
  util.c util.h imagelib.h im2int.c im2int.h imagelist.h imagelist.c \
  canvas.c canvas.h \
  common/conf.h common/conf.c common/iolet.c common/llist.c \
  common/useful.h common/useful.c common/file.h common/file.c  \
  common/iolet.h common/llist.h common/gen_cli.c common/gen_cli.h \
  common/getopt_clone.c common/getopt_clone.h common/defines.h

sequview_LDFLAGS =
sequview_LDADD = 
sequview_CFLAGS = -Icommon

all: sequview

