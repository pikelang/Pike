#pike 9.0
#require constant(9.0::Fuse.run)

inherit Fuse;

//! Prior to Pike 9.0 this function called @[predef::exit()]
//! with the error code.
//!
//! @seealso
//!   @[9.0::Fuse.run()]
void run( Operations handler, array(string) args )
{
  exit(::run(handler, args));
}
