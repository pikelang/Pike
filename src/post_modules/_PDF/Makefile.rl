RAGEL = ragel -G2 -C

rl/%.c: rl/%.rl
	$(RAGEL) -o $@ $<
rl/%.dot: rl/%.rl
	$(RAGEL) -p -V -o $@ $<
rl/%.png: rl/%.dot
	dot -Tpng -o $@ $<

source: rl/pdf.c
