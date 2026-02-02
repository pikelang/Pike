# SED script to generate add-errnos.pmod from errnos.list
#
# Grubba 2026-03-01

# Add a warning at the begining of the output.
1i\
/* Generated from errnos.list by mkerrnos.sed. Do NOT edit. */\
\
//! @namespace predef::\
\
//! @module System\
\


# Alias ELAST and LASTERRNO.
$a\
\
//! @endmodule\
\
//! @endnamespace

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

# Emit doc for the plain errno.
g
s/\([^ 	]*\).*/\/\/\! \@decl constant int \1/
p
g
s/\([^ 	]*\)[ 	]*\([^#	]*\).*/\/\/\! \2/
p
g
/^B_/s/.*/\/\/\! \@note/p
g
/^B_/s/.*/\/\/\!   This errno is only available on BeOS\/Haiku platforms./p
g
/^E.*_.*/s/.*/\/\/\! \@note/p
g
/^E.*_.*/s/.*/\/\/\!   This errno is only available on GNU\/Hurd platforms./p
g
/#/s/.*/\/\/\! \@note/p
g
/#/s/.*#\(.*\)/\/\/\!   This errno is only available on \1 platforms (if at all)./p

# Add an empty line to separate doc for the next errno.
s/.*//p


# Go to the next line if it does not look like a UNIX errno.
# Ie: Require errno to start with 'E' and to contain no '_'.
g
/^[^E]/d
/_/d

# Also: No WSA errnos for OS-specific errors.
/AIX/d
/Cygwin/d
/FreeBSD/d
/HPUX/d
/Hurd/d
/Irix/d
/Minix/d
/MirBSD/d
/OSF\/1/d
/Solaris/d

# Emit code for the WSA errno.
g
s/\([^ 	]*\).*/\/\/\! \@decl constant int WSA\1/
p
g
s/\([^ 	]*\)[ 	]*\([^#	]*\).*/\/\/\! \2/
p
i\
//! @note\
//!   This errno is only available on NT/WIN32 platforms (if at all).

# Add an empty line to separate doc for the next errno.
s/.*//p
