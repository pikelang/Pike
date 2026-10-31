#!/bin/sh

#
# Run etags with regexps suitable for Pike files.
#

HI=`printf '\200-\377'`

exec etags -l none \
  -r '/[ \t]*\(\<\(public\|inline\|final\|static\|protected\|local\|optional\|private\|variant\)\>[ \t]\{1,\}\)*\(\<class\>\)[ \t]\{1,\}\<\([a-zA-Z'"$HI"'_][a-zA-Z'"$HI"'_0-9]*\)\>/\4/' \
  -r '/[ \t]*\(\<\(mixed\|float\|int\|program\|string\|function\|function(.*)\|array\|array(.*)\|mapping\|mapping(.*)\|multiset\|multiset(.*)\|object\|object(.*)\|void\|constant\|class\)\>)*\|\<\([A-Z'"$HI"'][a-zA-Z'"$HI"'_0-9]*\)\>\)[ \t]\{1,\}\(\<\([_a-zA-Z'"$HI"'][_a-zA-Z'"$HI"'0-9]*\)\>\|``?\(!=\|->=?\|<[<=]\|==\|>[=>]\|\[\]=?\|()\|[%-!^&+*<>|~\/]\)\)[ \t]*(/\4/' \
  -r '/#[ \t]*define[ \t]+\([_a-zA-Z]+\)(?/\1/' "$@"
