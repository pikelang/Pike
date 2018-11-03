RAGEL = ragel -G2 -C

%.c: rl/%.rl rl/json5_defaults.rl
	$(RAGEL) -o $@ $<

source: json5_parser.c json5_string.c json5_mapping.c json5_array.c json5_number.c json5_string_utf8.c 
