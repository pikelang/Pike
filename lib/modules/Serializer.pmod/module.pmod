#pike __REAL_VERSION__

//! Serialization interface.
//!
//! This module contains APIs to simplify
//! serialization and deserialization of
//! objects.
//!
//! @seealso
//!   @[serialize()], @[deserialize()]

constant Serializable = Builtin.Serializable;
constant serialize = Builtin.serialize;
constant deserialize = Builtin.deserialize;


//! Simple base for an object that can be serialized by encode_value.
//! Also supports decoding.
//!
//! Uses @[Serializable] as it's base.
//!
//! Simply inherit this class in the classes you want to have encoded
//! and decoded.
//!
//! Note that it's not automatically recursive, objects assigned to
//! variables in this object have to be Encodeable on their own for
//! encode to work.
//!
//! When decoding only variables that existed at the time the object
//! was encoded are assigned, that is, if the class now has more
//! variables they new variables will be set to 0.
class Encodeable
{
    inherit Serializable;

    //! Callback for encoding the object. Returns a mapping from
    //! variable name to value.
    //!
    //! Called automatically by @[encode_value], not normally called
    //! manually.
    mapping(string:mixed) _encode()
    {
        mapping res = ([]);;
        _serialize( this, lambda( mixed val, string name, mixed type ) 
                          {
                              res[name] = val;
                          });
        return res;
    }

    // Override the one from Serializable.
    void _deserialize_variable(function deserializer,
                               function(mixed:void) setter,
                               string symbol,
                               mixed symbol_type)
    {
        deserializer(setter,symbol,symbol_type);
    }

    //! Callback for decoding the object. Sets variables in the object
    //! from the values in the mapping.
    //!
    //! Called automatically by @[decode_value], not normally called
    //! manually.
    void _decode(mapping(string:mixed) m)
    {
        _deserialize( this, lambda( function(mixed:void) set,
                                    string name, mixed type )
                            {
                                set( m[name] );
                            } );
    }
}
