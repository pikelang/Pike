// Yes this is very naughty but I know what I'm doing. The reason for
// this is to make "#pike __REAL_VERSION__" in the included file
// become "#pike 7.6". The reason for that is to make the
// master()->resolv() calls in it use master_76 so that the resolver
// looks up our compat module instead of the real thing. /mast
#undef __REAL_VERSION__
#define __REAL_VERSION__ 7.6

#include "../../../modules/Calendar.pmod/Timezone.pmod"
