#pike 7.5

inherit Protocols.SMTP;

class protocol {
  inherit Protocol;
}

class client {
  inherit Client;
  mapping(int:string) reply_codes = replycodes;
}
