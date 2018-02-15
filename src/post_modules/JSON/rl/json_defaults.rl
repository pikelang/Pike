%%{ 
    machine JSOND;

    number_start = [\-+.] | digit;
    array_start = '[';
    mapping_start = '{';
    string_start = '"';
    json_true = 'true';
    json_false = 'false';
    json_null = 'null';
    value_start = number_start | array_start | mapping_start | string_start | 'n' | 't' | 'f';
    myspace = [ \n\r\t];
    unicode = ((0 .. 0xd777) | (0xe000 .. 0x10ffff)) - (0x00 .. 0x1f);
    
}%%
