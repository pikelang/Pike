#!/bin/sh

sed 's@[-/a-zA-Z0-9.,_]*/\([-a-zA-Z0-9.,_]*\)@\1@g' > $1/dependencies

