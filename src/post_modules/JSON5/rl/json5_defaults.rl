%%{ 
    machine JSON5D;
    number_identifiers = 'Infinity' | 'NaN';
    number_start = [\-+.] | 'I' | 'N' | digit;
    array_start = '[';
    mapping_start = '{';
    string_start = '"' | '\'';
    json5_true = 'true';
    json5_false = 'false';
    json5_null = 'null';
    value_start = number_start | array_start | mapping_start | string_start | 'n' | 't' | 'f';
    identifier = [a-zA-Z_][a-zA-Z_0-9]*;

    action c_comment { fgoto c_comment;}

    single_line_comment = ('//' . [^\n]*  . '\n');

    c_comment = '/*' . any* :>> '*/';

    whitespace = [ \n\r\t];

    myspace = (whitespace |
                c_comment |
		single_line_comment)+
              ; 

    unicode = (0 .. 0x10ffff) - (0x00 .. 0x1f) - (0xd800 .. 0xdfff);
    
}%%
