#pike __REAL_VERSION__

//! Same as the @[ISO] calendar,
//! but with austrian as the default language.
//!
//! This calendar exist only for backwards compatible 
//! purposes. 

inherit Calendar.ISO;

private protected mixed __initstuff=lambda()
{
   default_rules=default_rules->set_language("austrian");
}();
