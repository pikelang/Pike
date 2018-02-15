#pike __REAL_VERSION__

#if constant (Gettext.gettext)
inherit Gettext;
#else
constant this_program_does_not_exist = 1;
#endif
