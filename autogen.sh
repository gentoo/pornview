#!/bin/sh
wget 'http://savannah.gnu.org/cgi-bin/viewcvs/*checkout*/config/config/config.sub'
wget 'http://savannah.gnu.org/cgi-bin/viewcvs/*checkout*/config/config/config.guess'
gettextize --po-dir=po && mv po/Makevars.template po/Makevars && aclocal -I m4 && libtoolize --copy --force --install --automake && autoconf && autoheader && automake --add-missing --copy
