lib_LTLIBRARIES = libgitg-1.0.la

INCLUDES =								\
	-I$(top_srcdir)							\
	-I$(srcdir)							\
	$(GITG_CFLAGS)							\
	$(WARN_CFLAGS)							\
	$(DISABLE_DEPRECATED_CFLAGS)					\
	-DDATADIR=\""$(datadir)"\"					\
	-DLIBDIR=\""$(libdir)"\"					\
	-DGITG_LOCALEDIR=\""$(datadir)/locale"\"

libgitg_1_0_la_LDFLAGS = \
	-export-dynamic -no-undefined -export-symbols-regex "^[^_].*"

libgitg_1_0_la_LIBADD = $(GITG_LIBS)

BUILT_SOURCES =			\
	gitg-enum-types.h	\
	gitg-enum-types.c

INST_H_FILES =			\
	$(BUILT_H_FILES)	\
	gitg-changed-file.h	\
	gitg-commit.h		\
	gitg-lane.h		\
	gitg-ref.h		\
	gitg-repository.h	\
	gitg-revision.h		\
	gitg-runner.h		\
	gitg-types.h

NOINST_H_FILES =		\
	gitg-color.h		\
	gitg-config.h		\
	gitg-debug.h		\
	gitg-i18n.h		\
	gitg-lanes.h		\
	gitg-utils.h

C_FILES =			\
	$(BUILT_SOURCES)	\
	gitg-changed-file.c	\
	gitg-color.c		\
	gitg-commit.c		\
	gitg-config.c		\
	gitg-debug.c		\
	gitg-i18n.c		\
	gitg-lane.c		\
	gitg-lanes.c		\
	gitg-ref.c		\
	gitg-repository.c	\
	gitg-revision.c		\
	gitg-runner.c		\
	gitg-utils.c

ENUM_H_FILES =			\
	gitg-changed-file.h

libgitg_1_0_la_SOURCES = 	\
	$(INST_H_FILES)		\
	$(NOINST_H_FILES)	\
	$(C_FILES)

headerdir = $(prefix)/include/libgitg-1.0/libgitg
header_DATA = $(INST_H_FILES)

EXTRA_DIST = 				\
	gitg-enum-types.h.template	\
	gitg-enum-types.c.template

CLEANFILES = $(BUILT_SOURCES)

dist-hook:
	cd $(distdir); rm -f $(BUILT_SOURCES)

gitg-enum-types.h: gitg-enum-types.h.template $(ENUM_H_FILES) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template gitg-enum-types.h.template $(ENUM_H_FILES)) > $@

gitg-enum-types.c: gitg-enum-types.c.template $(ENUM_H_FILES) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template gitg-enum-types.c.template $(ENUM_H_FILES)) > $@

