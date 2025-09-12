# SED script to generate add-errnos.h from errnos.list
#
# Grubba 1998-05-13
#
# Comments added 2025-08-13.

# Add a warning at the begining of the output.
1i/* Generated from errnos.list by mkerrnos.sed. Do NOT edit. */
1i

# Alias ELAST and LASTERRNO.
$a
$a#if !defined(ELAST) && defined(LASTERRNO)
$aADD_ERRNO(LASTERRNO, "ELAST", "")
$a#elif !defined(LASTERRNO) && defined(ELAST)
$aADD_ERRNO(ELAST, "LASTERRNO", "")
$a#endif

# Hold the input line.
h

# Print empty lines as is and goto the next line.
/^$/p
g
/^$/d

# Convert comments and go to the next line.
g
/^#/s/#\(.*\)/\/*\1 *\//p
g
/^#/d

# Emit code for the plain errno.
g
s/\([^ 	]*\).*/#ifdef \1/
p
g
s/\([^ 	]*\)[ 	]*\([^#	]*\).*/ADD_ERRNO(\1, "\1", "\2")/
p
g
s/\([^ 	]*\).*/#endif \/* \1 *\//
p

# Go to the next line if it does not look like a UNIX errno.
# Ie: Require errno to start with 'E' and to contain no '_'.
g
/^[^E]/d
/_/d

# Emit code for the WSA errno.
g
s/\([^ 	]*\).*/#ifdef WSA\1/
p
g
s/\([^ 	]*\).*/#ifndef \1/
p
g
s/\([^ 	]*\)[ 	]*\([^#	]*\).*/ADD_ERRNO(WSA\1, "\1", "\2")/
p
g
s/\([^ 	]*\).*/#endif \/* WSA\1 \&\& \!\1 *\//
p
g
s/\([^ 	]*\)[ 	]*\([^#	]*\).*/ADD_ERRNO(WSA\1, "WSA\1", "\2")/
p
g
s/\([^ 	]*\).*/#endif \/* WSA\1 *\//
p
