//!	Same as the ISO calendar,
//!	but with swedish as the default language.
//!
//!	This calendar exists only for backwards compatibility
//!	purposes.
//!

#pike __REAL_VERSION__

import ".";
inherit ISO:ISO;

private static mixed __initstuff=lambda()
{
   default_rules=default_rules->set_language("SE_sv");
}();
