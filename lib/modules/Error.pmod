#pike __REAL_VERSION__

// $Id: Error.pmod,v 1.1 2004/04/18 02:19:38 mast Exp $

//! @class Generic
//!
//! Class for exception objects for errors of unspecified type.
//!
//! @note
//! For historical reasons, erro

//! @endclass

constant Generic = __builtin.GenericError;

constant Index = __builtin.IndexError;

constant BadArgument = __builtin.BadArgumentError;

constant Math = __builtin.MathError;

constant Resource = __builtin.ResourceError;

constant Permission = __builtin.PermissionError;

constant Cpp = __builtin.CppError;

constant Compilation = __builtin.CompilationError;

constant MasterLoad = __builtin.MasterLoadError;

constant ModuleLoad = __builtin.ModuleLoadError;
