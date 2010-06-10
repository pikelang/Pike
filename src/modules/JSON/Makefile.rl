RAGEL = ragel -G2 -C

%.c: rl/%.rl rl/json_defaults.rl
	$(RAGEL) -o $@ $<
json_parser.c: rl/json.rl rl/json_defaults.rl
	$(RAGEL) -o $@ $<

source: json_parser.c json_string.c json_mapping.c json_array.c json_number.c json_utf8.c json_string_utf8.c json_mapping_utf8.c json_array_utf8.c json_number_utf8.c 
