#pike __REAL_VERSION__

// $Id: Error.pmod,v 1.2 2004/04/20 13:56:52 nilsson Exp $

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
