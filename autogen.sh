#!/bin/sh
wget 'http://savannah.gnu.org/cgi-bin/viewcvs/*checkout*/config/config/config.sub'
wget 'http://savannah.gnu.org/cgi-bin/viewcvs/*checkout*/config/config/config.guess'

cp -f $(type -p gettextize) .
sed -i -e 's:read dummy < /dev/tty::' gettextize

./gettextize -f --no-changelog --po-dir=po && mv po/Makevars.template po/Makevars && aclocal -I m4 && libtoolize --copy --force --install --automake && autoconf && autoheader && automake --add-missing --copy
