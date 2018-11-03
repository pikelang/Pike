%%{ 
    machine JSOND;

    number_start = [\-+.] | digit;
    array_start = '[';
    mapping_start = '{';
    string_start = '"';
    json_true = 'true';
    json_false = 'false';
    json_null = 'null';
    single_line_comment_start = '//';
    multi_line_comment_start = '/*';
    value_start = number_start | array_start | mapping_start | string_start | 'n' | 't' | 'f';
    identifier = [a-zA-Z_][a-zA-Z_0-9]*;
    whitespace = [ \n\r\t];
    myspace = whitespace|single_line_comment_start|multi_line_comment_start;
    unicode = (0 .. 0x10ffff) - (0x00 .. 0x1f) - (0xd800 .. 0xdfff);
    
}%%
