#!/bin/sh

echo "/* File generated on `date`
by getwhitespace <UnicodeData.txt */
"

echo '#define SPACECASE16	SPACECASE8 \'
sed -n -e '
s/^\([0-9A-Fa-f]*\);[^;]*;[^;]*;[^;]*;\([^;]*\);/\1 \2 /
/;INFORMATION SEPARATOR /d
/^[^;]* [SB] / p
/^[^;]* [WS]S / p
/^[^;]* CS <[^>]*> 0020;/ p
' | sed -e '
y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/
/^00/ d
s/^\([^ ]*\) .*$/case 0x\1:\\/
'
echo ''
