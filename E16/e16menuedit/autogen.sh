#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
CONFIGURE=configure.ac
XMLTO_LANG="C"

DIE=0

(test -f $srcdir/$CONFIGURE) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level package directory"
    exit 1
}

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`autoconf' installed."
  echo "Download the appropriate package for your distribution,"
  echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
  DIE=1
}

(grep "^AC_PROG_INTLTOOL" $srcdir/$CONFIGURE >/dev/null) && {
  (intltoolize --version) < /dev/null > /dev/null 2>&1 || {
    echo 
    echo "**Error**: You must have \`intltool' installed."
    echo "You can get it from:"
    echo "  ftp://ftp.gnome.org/pub/GNOME/"
    DIE=1
  }
}

#(grep "^AM_PROG_XML_I18N_TOOLS" $srcdir/$CONFIGURE >/dev/null) && {
#  (xml-i18n-toolize --version) < /dev/null > /dev/null 2>&1 || {
#    echo
#    echo "**Error**: You must have \`xml-i18n-toolize' installed."
#    echo "You can get it from:"
#    echo "  ftp://ftp.gnome.org/pub/GNOME/"
#    DIE=1
#  }
#}

(grep "^AM_GLIB_GNU_GETTEXT" $srcdir/$CONFIGURE >/dev/null) && {
  (grep "sed.*POTFILES" $srcdir/$CONFIGURE) > /dev/null || \
  (glib-gettextize --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "**Error**: You must have \`glib' installed."
    echo "You can get it from: ftp://ftp.gtk.org/pub/gtk"
    DIE=1
  }
}

(automake --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`automake' installed."
  echo "You can get it from: ftp://ftp.gnu.org/pub/gnu/"
  DIE=1
  NO_AUTOMAKE=yes
}

(which xmlto) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`xmlto' installed."
  echo "You can get it from: http://cyberelk.net/tim/xmlto/"
  DIE=1
  NO_XMLTO=yes
}

# if no automake, don't bother testing for aclocal
test -n "$NO_AUTOMAKE" || (aclocal --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: Missing \`aclocal'.  The version of \`automake'"
  echo "installed doesn't appear recent enough."
  echo "You can get automake from ftp://ftp.gnu.org/pub/gnu/"
  DIE=1
}

if test "$DIE" -eq 1; then
  exit 1
fi

if test -z "$*"; then
  echo "**Warning**: I am going to run \`configure' with no arguments."
  echo "If you wish to pass any to it, please specify them on the"
  echo \`$0\'" command line."
  echo
fi

case $CC in
xlc )
  am_opt=--include-deps;;
esac

for coin in `find $srcdir -path $srcdir/CVS -prune -o -name $CONFIGURE -print`
do 
  dr=`dirname $coin`
  if test -f $dr/NO-AUTO-GEN; then
    echo skipping $dr -- flagged as no auto-gen
  else
    echo processing $dr
    ( cd $dr

      aclocalinclude="$ACLOCAL_FLAGS"

      if grep "^AM_GLIB_GNU_GETTEXT" $CONFIGURE >/dev/null; then
	echo "Creating $dr/aclocal.m4 ..."
	test -r $dr/aclocal.m4 || touch $dr/aclocal.m4
	echo "Running glib-gettextize...  Ignore non-fatal messages."
	echo "no" | glib-gettextize --force --copy
	echo "Making $dr/aclocal.m4 writable ..."
	test -r $dr/aclocal.m4 && chmod u+w $dr/aclocal.m4
      fi
      if grep "^AC_PROG_INTLTOOL" $CONFIGURE >/dev/null; then
        echo "Running intltoolize..."
	intltoolize --copy --force --automake
      fi
 #     if grep "^AM_PROG_XML_I18N_TOOLS" $CONFIGURE >/dev/null; then
 #       echo "Running xml-i18n-toolize..."
#	xml-i18n-toolize --copy --force --automake
#      fi

      test -n "$NO_XMLTO" || {
        echo "creating help for lang $XMLTO_LANG"
        for xlang in $XMLTO_LANG
        do
          xmlto -o help/$xlang html-nochunks help/$xlang/e16menuedit2.xml
        done
      }
      
      echo "Running aclocal $aclocalinclude ..."
      aclocal $aclocalinclude
      if grep "^AM_CONFIG_HEADER" $CONFIGURE >/dev/null; then
	echo "Running autoheader..."
	autoheader
      fi
      echo "Running automake --gnu $am_opt ..."
      automake --add-missing --gnu $am_opt
      echo "Running autoconf ..."
      autoconf
    )
  fi
done

conf_flags="--enable-maintainer-mode"

if test x$NOCONFIGURE = x; then
  echo Running $srcdir/configure $conf_flags "$@" ...
  $srcdir/configure $conf_flags "$@" \
  && echo Now type \`make\' to compile. || exit 1
else
  echo Skipping configure process.
fi
