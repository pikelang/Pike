# SED script to generate errno_remap.h from errnos.list
#
# Grubba 2025-10-03

# Add a warning at the begining of the output.
1i\
/* Generated from errnos.list by mkerrno_remap.sed. Do NOT edit. */
1i\

1i\
/* Make WSA* errnos that do not exist as a plain errno available
1i\
 * without the WSA prefix.
1i\
 *
1i\
 * NB: This file is loaded by fdlib.h and should NOT be loaded
1i\
 *     directly by anything else.
1i\
 */
1i\

1i\
#ifndef FDLIB_H
1i\
#error Do NOT #include errno_remap.h explicitly.
1i\
#endif
1i\


# Skip empty lines.
/^$/d

# Skip comments and go to the next line.
/^#/d

# Skip if it does not look like a UNIX errno.
# Ie: Require errno to start with 'E' and to contain no '_'.
/^[^E]/d
/_/d

# Hold the input line.
h

# Emit code for the WSA errno.
g
s/\([^ 	]*\).*/#if defined(WSA\1) \&\& !defined(\1)/
p
g
s/\([^ 	]*\).*/#define \1 WSA\1/
p
g
s/\([^ 	]*\).*/#endif \/* WSA\1 \&\& !\1 *\//
p
