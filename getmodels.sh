#!/bin/sh
# $Id: getmodels.sh,v 1.2 2001/10/09 10:31:18 jaq Exp $
# this script downloads the model descriptions and converts them to c code
# that's inserted into the main code.  Jamie Wilkinson's fault^Wcode.

# use it like ./getmodels.sh > newmodels

mkdir modeldir
cd modeldir

wget  http://www.geocities.com/stigeide/snake/

cat index.html | grep 3dsnake | sed -e s/^\[^\'\]\*\'// -e s/\'.\*$// \
	| grep -v '^.html' \
	| sed s@^@http://www.geocities.com/stigeide/snake/@ | wget -i -

rm -f index.html

for i in *.html; do
	echo -n "    { \""
	echo -n $i | sed 's/.html$//'
	echo "\","
	echo -n "        { "
	cat $i | grep -v '<' | tr -d '' | sed -e 's/0/ZERO, /g' -e 's/1/RIGHT, /g' -e 's/2/PIN, /g' -e 's/3/LEFT, /g' | sed 's/, $/ }/'
	echo "    },"
done

cd ..
rm -rf modeldir

