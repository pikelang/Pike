#ifndef CB_CRITBIT_H
#define CB_CRITBIT_H
#define CONCAT2_(a, b)		a ## b
#define CONCAT2(a, b)		CONCAT2_(a, b)
#define CONCAT3(a, b, c)	CONCAT2(a, CONCAT2(b, c))
#define CONCAT4(a, b, c, d)	CONCAT2(a, CONCAT3(b, c, d))
#define CONCAT5(a, b, c, d, e)	CONCAT2(a, CONCAT4(b, c, d, e))

#ifndef CB_NAMESPACE
# warn NAMESPACE NOT USED
#define CB_NAME(name)	CONCAT2(cb_, name)
#define CB_TYPE(name)	CONCAT3(cb_, name, _t)
#else
#define CB_NAME(name)	CONCAT4(cb_, CB_NAMESPACE, _, name)
#define CB_TYPE(name)	CONCAT5(cb_, CB_NAMESPACE, _, name, _t)
#endif

typedef struct cb_size {
    size_t bits;
    ptrdiff_t chars;
} cb_size;

#endif
