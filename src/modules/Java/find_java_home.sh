#!/bin/sh
java -verbose fnord.fnord.fnord 2>&1 | \
  sed -e 's:^\[[^]/]*\(/.*\)/[^/]*\.jar[] ].*$:\1:' -e t \
      -e 's:^\[[^]/]*\(/.*\)/[^/]*\.zip[] ].*$:\1:' -e t \
      -e d | \
  sed -e 's:^.* from /:/:' -e 's:/[Cc]lasses$::' -e 's:/lib$::' -e '1q'
