#!/bin/sh

cd $1

(
  cat Makefile.src
  echo "# Depencies begin here"
  sed 's@[-/a-zA-Z0-9.,_]*/\([-a-zA-Z0-9.,_]*\)@\1@g'
) > Makefile.in

