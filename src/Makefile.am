bin_PROGRAMS = cgoban

SUBDIRS = gmp client

INCLUDES = -I $(top_srcdir)/wmslib/src

cgoban_LDADD = \
	gmp/libgmp.a\
	client/libclient.a\
	../wmslib/src/abut/libabut.a\
	../wmslib/src/but/libbut.a\
	../wmslib/src/wms/libwms.a\
	../wmslib/src/libwmsutils.a

cgoban_SOURCES = \
	arena.c \
	cgbuts.c \
	cgoban.c \
	cm2pm.c \
	control.c \
	crwin.c \
	drawStone.c \
	editBoard.c \
	editInfo.c \
	editTool.c \
	gameSetup.c \
	goBoard.c \
	goGame.c \
	goHash.c \
	goPic.c \
	goScore.c \
	goTime.c \
	goban.c \
	help.c \
	local.c \
	main.c \
	msg.c \
	plasma.c \
	setup.c \
	sgf.c \
	sgfIn.c \
	sgfMap.c \
	sgfOut.c \
	sgfPlay.c

noinst_HEADERS = \
	arena.h \
	cgbuts.h \
	cgoban.h \
	cm2pm.h \
	control.h \
	crwin.h \
	drawStone.h \
	editBoard.h \
	editInfo.h \
	editTool.h \
	gameSetup.h \
	goBoard.h \
	goGame.h \
	goHash.h \
	goPic.h \
	goScore.h \
	goTime.h \
	goban.h \
	help.h \
	local.h \
	main.h \
	msg.h \
	plasma.h \
	setup.h \
	sgf.h \
	sgfIn.h \
	sgfMap.h \
	sgfOut.h \
	sgfPlay.h
