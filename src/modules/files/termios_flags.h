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
#ifdef NLDLY
  TERMIOS_FLAG(c_oflag,NLDLY,"NLDLY")
#endif
#ifdef CRDLY
  TERMIOS_FLAG(c_oflag,CRDLY,"CRDLY")
#endif
#ifdef TABDLY
  TERMIOS_FLAG(c_oflag,TABDLY,"TABDLY")
#endif
#ifdef BSDLY
  TERMIOS_FLAG(c_oflag,BSDLY,"BSDLY")
#endif
#ifdef VTDLY
  TERMIOS_FLAG(c_oflag,VTDLY,"VTDLY")
#endif
#ifdef FFDLY
  TERMIOS_FLAG(c_oflag,FFDLY,"FFDLY")
#endif
#ifdef OXTABS
  TERMIOS_FLAG(c_cflag,OXTABS,"OXTABS")
#endif
#ifdef ONOEOT
  TERMIOS_FLAG(c_cflag,ONOEOT,"ONOEOT")
#endif

#ifdef CS5
  TERMIOS_FLAG(c_cflag,CS5,"CS5")
#endif
#ifdef CS6
  TERMIOS_FLAG(c_cflag,CS6,"CS6")
#endif
#ifdef CS7
  TERMIOS_FLAG(c_cflag,CS7,"CS7")
#endif
#ifdef CS8
  TERMIOS_FLAG(c_cflag,CS8,"CS8")
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
