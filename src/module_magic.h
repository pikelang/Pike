#ifndef MODULE_MAGIC_H
#define MODULE_MAGIC_H

#ifdef DYNAMIC_MODULE
#ifdef __NT__

#define pike_module_init mYDummyFunctioN1(void); __declspec(dllexport) void pike_module_init
#define pike_module_exit mYDummyFunctioN2(void); __declspec(dllexport) void pike_module_exit

#endif /* __NT__ */
#endif /* DYNAMIC_MODULE */


#ifndef PMOD_EXPORT
#define PMOD_EXPORT
#endif

#endif /* MODULE_MAGIC_H */


