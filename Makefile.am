AUTOMAKE_OPTIONS = 1.4

SUBDIRS = man6 wmslib src

EXTRA_DIST = seigen-minoru.sgf bootstrap cgoban1.spec cgoban_icon.png

bin_SCRIPTS = grab_cgoban

# use noinst_SCRIPTS to create a symbolic link to the cgoban
# executable
noinst_SCRIPTS = cgoban$(EXEEXT)

MOSTLYCLEANFILES = cgoban$(EXEEXT)

cgoban$(EXEEXT): $(top_builddir)/src/cgoban$(EXEEXT)
	ln -s $(top_builddir)/src/cgoban$(EXEEXT) cgoban$(EXEEXT)
