#!/bin/sh
#
# this script downloads the model descriptions and converts them to the
# model format used by glsnake.  Jamie Wilkinson's fault^Wcode.

# use it like ./getmodels.sh > newmodels

mkdir modeldir
cd modeldir

wget  http://www.geocities.com/stigeide/snake/

cat index.html | grep 3dsnake | sed -e s/^\[^\'\]\*\'// -e s/\'.\*$// \
	| grep -v '^.html' \
	| sed s@^@http://www.geocities.com/stigeide/snake/@ | wget -i -

rm -f index.html

echo "# These models were taken from http://www.geocities.com/stigeide/snake/"
echo "# which is linked to from Eric And Thomas's Rubiks Snake page at"
echo "# http://home.t-online.de/home/thlet.wolter/"
echo "#"
echo "# This file is generated automatically by tools/getmodels.sh"
echo ""

for i in *.html; do
	echo -n $i | sed 's/.html$//'
	echo -n ": "
	cat $i | grep -v '<' | tr -d ''
done

cd ..
rm -rf modeldir

