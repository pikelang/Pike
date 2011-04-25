/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifdef TERMIOS_FLAG

#ifdef IGNBRK
  TERMIOS_FLAG(c_iflag,IGNBRK,"IGNBRK")
#endif
#ifdef BRKINT
  TERMIOS_FLAG(c_iflag,BRKINT,"BRKINT")
#endif
#ifdef IGNPAR
  TERMIOS_FLAG(c_iflag,IGNPAR,"IGNPAR")
#endif
#ifdef PARMRK
  TERMIOS_FLAG(c_iflag,PARMRK,"PARMRK")
#endif
#ifdef INPCK
  TERMIOS_FLAG(c_iflag,INPCK,"INPCK")
#endif
#ifdef ISTRIP
  TERMIOS_FLAG(c_iflag,ISTRIP,"ISTRIP")
#endif
#ifdef INLCR
  TERMIOS_FLAG(c_iflag,INLCR,"INLCR")
#endif
#ifdef IGNCR
  TERMIOS_FLAG(c_iflag,IGNCR,"IGNCR")
#endif
#ifdef ICRNL
  TERMIOS_FLAG(c_iflag,ICRNL,"ICRNL")
#endif
#ifdef IUCLC
  TERMIOS_FLAG(c_iflag,IUCLC,"IUCLC")
#endif
#ifdef IXON
  TERMIOS_FLAG(c_iflag,IXON,"IXON")
#endif
#ifdef IXANY
  TERMIOS_FLAG(c_iflag,IXANY,"IXANY")
#endif
#ifdef IXOFF
  TERMIOS_FLAG(c_iflag,IXOFF,"IXOFF")
#endif
#ifdef IMAXBEL
  TERMIOS_FLAG(c_iflag,IMAXBEL,"IMAXBEL")
#endif

#ifdef OPOST
  TERMIOS_FLAG(c_oflag,OPOST,"OPOST")
#endif
#ifdef OLCUC
  TERMIOS_FLAG(c_oflag,OLCUC,"OLCUC")
#endif
#ifdef ONLCR
  TERMIOS_FLAG(c_oflag,ONLCR,"ONLCR")
#endif
#ifdef OCRNL
  TERMIOS_FLAG(c_oflag,OCRNL,"OCRNL")
#endif
#ifdef ONOCR
  TERMIOS_FLAG(c_oflag,ONOCR,"ONOCR")
#endif
#ifdef ONLRET
  TERMIOS_FLAG(c_oflag,ONLRET,"ONLRET")
#endif
#ifdef OFILL
  TERMIOS_FLAG(c_oflag,OFILL,"OFILL")
#endif
#ifdef OFDEL
  TERMIOS_FLAG(c_oflag,OFDEL,"OFDEL")
#endif
#ifdef OXTABS
  TERMIOS_FLAG(c_cflag,OXTABS,"OXTABS")
#endif
#ifdef ONOEOT
  TERMIOS_FLAG(c_cflag,ONOEOT,"ONOEOT")
#endif

#ifdef CSTOPB
  TERMIOS_FLAG(c_cflag,CSTOPB,"CSTOPB")
#endif
#ifdef CREAD
  TERMIOS_FLAG(c_cflag,CREAD,"CREAD")
#endif
#ifdef PARENB
  TERMIOS_FLAG(c_cflag,PARENB,"PARENB")
#endif
#ifdef PARODD
  TERMIOS_FLAG(c_cflag,PARODD,"PARODD")
#endif
#ifdef HUPCL
  TERMIOS_FLAG(c_cflag,HUPCL,"HUPCL")
#endif
#ifdef CLOCAL
  TERMIOS_FLAG(c_cflag,CLOCAL,"CLOCAL")
#endif

#ifdef CRTSCTS
  TERMIOS_FLAG(c_cflag,CRTSCTS,"CRTSCTS")
#endif

#ifdef ISIG
  TERMIOS_FLAG(c_lflag,ISIG,"ISIG")
#endif
#ifdef ICANON
  TERMIOS_FLAG(c_lflag,ICANON,"ICANON")
#endif
#ifdef XCASE
  TERMIOS_FLAG(c_lflag,XCASE,"XCASE")
#endif
#ifdef ECHO
  TERMIOS_FLAG(c_lflag,ECHO,"ECHO")
#endif
#ifdef ECHOE
  TERMIOS_FLAG(c_lflag,ECHOE,"ECHOE")
#endif
#ifdef ECHOK
  TERMIOS_FLAG(c_lflag,ECHOK,"ECHOK")
#endif
#ifdef ECHONL
  TERMIOS_FLAG(c_lflag,ECHONL,"ECHONL")
#endif
#ifdef ECHOCTL
  TERMIOS_FLAG(c_lflag,ECHOCTL,"ECHOCTL")
#endif
#ifdef ECHOPRT
  TERMIOS_FLAG(c_lflag,ECHOPRT,"ECHOPRT")
#endif
#ifdef ECHOKE
  TERMIOS_FLAG(c_lflag,ECHOKE,"ECHOKE")
#endif
#ifdef FLUSHO
  TERMIOS_FLAG(c_lflag,FLUSHO,"FLUSHO")
#endif
#ifdef NOFLSH
  TERMIOS_FLAG(c_lflag,NOFLSH,"NOFLSH")
#endif
#ifdef TOSTOP
  TERMIOS_FLAG(c_lflag,TOSTOP,"TOSTOP")
#endif
#ifdef PENDIN
  TERMIOS_FLAG(c_lflag,PENDIN,"PENDIN")
#endif

#endif

#ifdef TERMIOS_SPEED

#ifdef B0
TERMIOS_SPEED(B0,0)
#endif
#ifdef B50
TERMIOS_SPEED(B50,50)
#endif
#ifdef B75
TERMIOS_SPEED(B75,75)
#endif
#ifdef B110
TERMIOS_SPEED(B110,110)
#endif
#ifdef B134
TERMIOS_SPEED(B134,134)
#endif
#ifdef B150
TERMIOS_SPEED(B150,150)
#endif
#ifdef B200
TERMIOS_SPEED(B200,200)
#endif
#ifdef B300
TERMIOS_SPEED(B300,300)
#endif
#ifdef B600
TERMIOS_SPEED(B600,600)
#endif
#ifdef B1200
TERMIOS_SPEED(B1200,1200)
#endif
#ifdef B1800
TERMIOS_SPEED(B1800,1800)
#endif
#ifdef B2400
TERMIOS_SPEED(B2400,2400)
#endif
#ifdef B4800
TERMIOS_SPEED(B4800,4800)
#endif
#ifdef B9600
TERMIOS_SPEED(B9600,9600)
#endif
#ifdef B19200
TERMIOS_SPEED(B19200,19200)
#endif
#ifdef B38400
TERMIOS_SPEED(B38400,38400)
#endif
#ifdef B57600
TERMIOS_SPEED(B57600,57600)
#endif
#ifdef B115200
TERMIOS_SPEED(B115200,115200)
#endif
#ifdef B230400
TERMIOS_SPEED(B230400,230400)
#endif

#endif

#ifdef TERMIOS_CHAR

#ifdef VINTR
   TERMIOS_CHAR(VINTR,"VINTR")
#endif
#ifdef VQUIT
   TERMIOS_CHAR(VQUIT,"VQUIT")
#endif
#ifdef VERASE
   TERMIOS_CHAR(VERASE,"VERASE")
#endif
#ifdef VKILL
   TERMIOS_CHAR(VKILL,"VKILL")
#endif
#ifdef VEOF
   TERMIOS_CHAR(VEOF,"VEOF")
#endif
#ifdef VTIME
   TERMIOS_CHAR(VTIME,"VTIME")
#endif
#ifdef VMIN
   TERMIOS_CHAR(VMIN,"VMIN")
#endif
#ifdef VSWTC
   TERMIOS_CHAR(VSWTC,"VSWTC")
#endif
#ifdef VSTART
   TERMIOS_CHAR(VSTART,"VSTART")
#endif
#ifdef VSTOP
   TERMIOS_CHAR(VSTOP,"VSTOP")
#endif
#ifdef VSUSP
   TERMIOS_CHAR(VSUSP,"VSUSP")
#endif
#ifdef VEOL
   TERMIOS_CHAR(VEOL,"VEOL")
#endif
#ifdef VREPRINT
   TERMIOS_CHAR(VREPRINT,"VREPRINT")
#endif
#ifdef VDISCARD
   TERMIOS_CHAR(VDISCARD,"VDISCARD")
#endif
#ifdef VWERASE
   TERMIOS_CHAR(VWERASE,"VWERASE")
#endif
#ifdef VLNEXT
   TERMIOS_CHAR(VLNEXT,"VLNEXT")
#endif
#ifdef VEOL2
   TERMIOS_CHAR(VEOL2,"VEOL2")
#endif

#endif
