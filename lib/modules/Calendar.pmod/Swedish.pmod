#pike __REAL_VERSION__

//! Same as the @[ISO] calendar, but with Swedish as the default language.
//!
//! This calendar exist only for backwards compatible purposes. 

inherit .ISO:ISO;

private static mixed __initstuff=lambda()
{
   default_rules=default_rules->set_language("SE_sv");
}();
