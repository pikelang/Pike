constant replace_from=("\240 \n\r\t!\"#$%&'()*+,-./0123456789:;<=>?@[\]^_`{|}~¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿×÷")/"";
constant replace_to=({" "})*(sizeof(replace_from));


array(string) tokenize(string in)
{
  // FIXME: Make this buffered, may run out of pike stack?
  return (in/" ") - ({ "" });
}


string normalize(string in)
{
  in=lower_case(in);
  return replace(in, replace_from, replace_to);
}
