
#if constant(_Regexp_PCRE.UTF8_SUPPORTED)
#define PCRE_GOT_WIDESTRINGS
#define GOOD _Regexp_PCRE.StudiedWidestring
#define QUICK _Regexp_PCRE.Widestring
#else
#define GOOD _Regexp_PCRE.Studied
#define QUICK _Regexp_PCRE.Plain
#endif
