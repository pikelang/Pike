/*
 * $Id: Ports.pmod,v 1.3 1999/11/04 13:57:34 grubba Exp $
 *
 * IP port assignments
 *
 * Henrik Grubbström 1998-06-23
 */

// Contains all UDP ports assigned for private use as of RFC 1700
constant private_udp = ([
  "mail":              24,	//    any private mail system
  "printer":           35,	//    any private printer server
  "terminal":          57,	//    any private terminal access
  "file":              59,	//    any private file service
  "dial":              75,	//    any private dial out service
  "rje":               77,	//    any private RJE service
  "terminal-link":     87,	//    any private terminal link
]);

// Contains all TCP ports assigned for private use as of RFC 1700
constant private_tcp = ([
  "mail":              24,	//    any private mail system
  "printer":           35,	//    any private printer server
  "terminal":          57,	//    any private terminal access
  "file":              59,	//    any private file service
  "dial":              75,	//    any private dial out service
  "rje":               77,	//    any private RJE service
  "terminal-link":     87,	//    any private terminal link
]);

// Contains all non-private UDP port assignments as of RFC 1700
constant udp = ([
  "tcpmux":             1,	//    TCP Port Service Multiplexer
  "compressnet-mgmt":   2,	//    Management Utility
  "compressnet":        3,	//    Compression Process
  "rje":                5,	//    Remote Job Entry
  "echo":               7,	//    Echo
  "discard":            9,	//    Discard
  "systat":            11,	//    Active Users
  "daytime":           13,	//    Daytime
  "qotd":              17,	//    Quote of the Day
  "msp":               18,	//    Message Send Protocol
  "chargen":           19,	//    Character Generator
  "ftp-data":          20,	//    File Transfer [Default Data]
  "ftp":               21,	//    File Transfer [Control]
  "telnet":            23,	//    Telnet
  "smtp":              25,	//    Simple Mail Transfer
  "nsw-fe":            27,	//    NSW User System FE
  "msg-icp":           29,	//    MSG ICP
  "msg-auth":          31,	//    MSG Authentication
  "dsp":               33,	//    Display Support Protocol
  "time":              37,	//    Time
  "rap":               38,	//    Route Access Protocol
  "rlp":               39,	//    Resource Location Protocol
  "graphics":          41,	//    Graphics
  "nameserver":        42,	//    Host Name Server
  "nicname":           43,	//    Who Is
  "mpm-flags":         44,	//    MPM FLAGS Protocol
  "mpm":               45,	//    Message Processing Module [recv]
  "mpm-snd":           46,	//    MPM [default send]
  "ni-ftp":            47,	//    NI FTP
  "auditd":            48,	//    Digital Audit Daemon
  "login":             49,	//    Login Host Protocol
  "re-mail-ck":        50,	//    Remote Mail Checking Protocol
  "la-maint":          51,	//    IMP Logical Address Maintenance
  "xns-time":          52,	//    XNS Time Protocol
  "domain":            53,	//    Domain Name Server
  "xns-ch":            54,	//    XNS Clearinghouse
  "isi-gl":            55,	//    ISI Graphics Language
  "xns-auth":          56,	//    XNS Authentication
  "xns-mail":          58,	//    XNS Mail
  "ni-mail":           61,	//    NI MAIL
  "acas":              62,	//    ACA Services
  "covia":             64,	//    Communications Integrator (CI)
  "tacacs-ds":         65,	//    TACACS-Database Service
  "sql*net":           66,	//    Oracle SQL*NET
  "bootps":            67,	//    Bootstrap Protocol Server
  "bootpc":            68,	//    Bootstrap Protocol Client
  "tftp":              69,	//    Trivial File Transfer
  "gopher":            70,	//    Gopher
  "netrjs-1":          71,	//    Remote Job Service
  "netrjs-2":          72,	//    Remote Job Service
  "netrjs-3":          73,	//    Remote Job Service
  "netrjs-4":          74,	//    Remote Job Service
  "deos":              76,	//    Distributed External Object Store
  "vettcp":            78,	//    vettcp
  "finger":            79,	//    Finger
  "www-http":          80,	//    World Wide Web HTTP
  "hosts2-ns":         81,	//    HOSTS2 Name Server
  "xfer":              82,	//    XFER Utility
  "mit-ml-dev":        83,	//    MIT ML Device
  "ctf":               84,	//    Common Trace Facility
  "mit-ml-dev":        85,	//    MIT ML Device
  "mfcobol":           86,	//    Micro Focus Cobol
  "kerberos":          88,	//    Kerberos
  "su-mit-tg":         89,	//    SU/MIT Telnet Gateway
  "dnsix":             90,	//    DNSIX Securit Attribute Token Map
  "mit-dov":           91,	//    MIT Dover Spooler
  "npp":               92,	//    Network Printing Protocol
  "dcp":               93,	//    Device Control Protocol
  "objcall":           94,	//    Tivoli Object Dispatcher
  "supdup":            95,	//    SUPDUP
  "dixie":             96,	//    DIXIE Protocol Specification
  "swift-rvf":         97,	//    Swift Remote Vitural File Protocol
  "tacnews":           98,	//    TAC News
  "metagram":          99,	//    Metagram Relay
  "hostname":         101,	//    NIC Host Name Server
  "iso-tsap":         102,	//    ISO-TSAP
  "gppitnp":          103,	//    Genesis Point-to-Point Trans Net
  "acr-nema":         104,	//    ACR-NEMA Digital Imag. & Comm. 300
  "csnet-ns":         105,	//    Mailbox Name Nameserver
  "3com-tsmux":       106,	//    3COM-TSMUX
  "rtelnet":          107,	//    Remote Telnet Service
  "snagas":           108,	//    SNA Gateway Access Server
  "pop2":             109,	//    Post Office Protocol - Version 2
  "pop3":             110,	//    Post Office Protocol - Version 3
  "sunrpc":           111,	//    SUN Remote Procedure Call
  "mcidas":           112,	//    McIDAS Data Transmission Protocol
  "auth":             113,	//    Authentication Service
  "audionews":        114,	//    Audio News Multicast
  "sftp":             115,	//    Simple File Transfer Protocol
  "ansanotify":       116,	//    ANSA REX Notify
  "uucp-path":        117,	//    UUCP Path Service
  "sqlserv":          118,	//    SQL Services
  "nntp":             119,	//    Network News Transfer Protocol
  "cfdptkt":          120,	//    CFDPTKT
  "erpc":             121,	//    Encore Expedited Remote Pro.Call
  "smakynet":         122,	//    SMAKYNET
  "ntp":              123,	//    Network Time Protocol
  "ansatrader":       124,	//    ANSA REX Trader
  "locus-map":        125,	//    Locus PC-Interface Net Map Ser
  "unitary":          126,	//    Unisys Unitary Login
  "locus-con":        127,	//    Locus PC-Interface Conn Server
  "gss-xlicen":       128,	//    GSS X License Verification
  "pwdgen":           129,	//    Password Generator Protocol
  "cisco-fna":        130,	//    cisco FNATIVE
  "cisco-tna":        131,	//    cisco TNATIVE
  "cisco-sys":        132,	//    cisco SYSMAINT
  "statsrv":          133,	//    Statistics Service
  "ingres-net":       134,	//    INGRES-NET Service
  "loc-srv":          135,	//    Location Service
  "profile":          136,	//    PROFILE Naming System
  "netbios-ns":       137,	//    NETBIOS Name Service
  "netbios-dgm":      138,	//    NETBIOS Datagram Service
  "netbios-ssn":      139,	//    NETBIOS Session Service
  "emfis-data":       140,	//    EMFIS Data Service
  "emfis-cntl":       141,	//    EMFIS Control Service
  "bl-idm":           142,	//    Britton-Lee IDM
  "imap2":            143,	//    Interim Mail Access Protocol v2
  "news":             144,	//    NewS
  "uaac":             145,	//    UAAC Protocol
  "iso-tp0":          146,	//    ISO-IP0
  "iso-ip":           147,	//    ISO-IP
  "cronus":           148,	//    CRONUS-SUPPORT
  "aed-512":          149,	//    AED 512 Emulation Service
  "sql-net":          150,	//    SQL-NET
  "hems":             151,	//    HEMS
  "bftp":             152,	//    Background File Transfer Program
  "sgmp":             153,	//    SGMP
  "netsc-prod":       154,	//    NETSC
  "netsc-dev":        155,	//    NETSC
  "sqlsrv":           156,	//    SQL Service
  "knet-cmp":         157,	//    KNET/VM Command/Message Protocol
  "pcmail-srv":       158,	//    PCMail Server
  "nss-routing":      159,	//    NSS-Routing
  "sgmp-traps":       160,	//    SGMP-TRAPS
  "snmp":             161,	//    SNMP
  "snmptrap":         162,	//    SNMPTRAP
  "cmip-man":         163,	//    CMIP/TCP Manager
  "smip-agent":       164,	//    CMIP/TCP Agent
  "xns-courier":      165,	//    Xerox
  "s-net":            166,	//    Sirius Systems
  "namp":             167,	//    NAMP
  "rsvd":             168,	//    RSVD
  "send":             169,	//    SEND
  "print-srv":        170,	//    Network PostScript
  "multiplex":        171,	//    Network Innovations Multiplex
  "cl/1":             172,	//    Network Innovations CL/1
  "xyplex-mux":       173,	//    Xyplex
  "mailq":            174,	//    MAILQ
  "vmnet":            175,	//    VMNET
  "genrad-mux":       176,	//    GENRAD-MUX
  "xdmcp":            177,	//    X Display Manager Control Protocol
  "NextStep":         178,	//    NextStep Window Server
  "bgp":              179,	//    Border Gateway Protocol
  "ris":              180,	//    Intergraph
  "unify":            181,	//    Unify
  "audit":            182,	//    Unisys Audit SITP
  "ocbinder":         183,	//    OCBinder
  "ocserver":         184,	//    OCServer
  "remote-kis":       185,	//    Remote-KIS
  "kis":              186,	//    KIS Protocol
  "aci":              187,	//    Application Communication Interface
  "mumps":            188,	//    Plus Five's MUMPS
  "qft":              189,	//    Queued File Transport
  "cacp":             190,	//    Gateway Access Control Protocol
  "prospero":         191,	//    Prospero Directory Service
  "osu-nms":          192,	//    OSU Network Monitoring System
  "srmp":             193,	//    Spider Remote Monitoring Protocol
  "irc":              194,	//    Internet Relay Chat Protocol
  "dn6-nlm-aud":      195,	//    DNSIX Network Level Module Audit
  "dn6-smm-red":      196,	//    DNSIX Session Mgt Module Audit Redir
  "dls":              197,	//    Directory Location Service
  "dls-mon":          198,	//    Directory Location Service Monitor
  "smux":             199,	//    SMUX
  "src":              200,	//    IBM System Resource Controller
  "at-rtmp":          201,	//    AppleTalk Routing Maintenance
  "at-nbp":           202,	//    AppleTalk Name Binding
  "at-3":             203,	//    AppleTalk Unused
  "at-echo":          204,	//    AppleTalk Echo
  "at-5":             205,	//    AppleTalk Unused
  "at-zis":           206,	//    AppleTalk Zone Information
  "at-7":             207,	//    AppleTalk Unused
  "at-8":             208,	//    AppleTalk Unused
  "tam":              209,	//    Trivial Authenticated Mail Protocol
  "z39.50":           210,	//    ANSI Z39.50
  "914c/g":           211,	//    Texas Instruments 914C/G Terminal
  "anet":             212,	//    ATEXSSTR
  "ipx":              213,	//    IPX
  "vmpwscs":          214,	//    VM PWSCS
  "softpc":           215,	//    Insignia Solutions
  "atls":             216,	//    Access Technology License Server
  "dbase":            217,	//    dBASE Unix
  "mpp":              218,	//    Netix Message Posting Protocol
  "uarps":            219,	//    Unisys ARPs
  "imap3":            220,	//    Interactive Mail Access Protocol v3
  "fln-spx":          221,	//    Berkeley rlogind with SPX auth
  "rsh-spx":          222,	//    Berkeley rshd with SPX auth
  "cdc":              223,	//    Certificate Distribution Center
  "sur-meas":         243,	//    Survey Measurement
  "link":             245,	//    LINK
  "dsp3270":          246,	//    Display Systems Protocol
  "pdap":             344,	//    Prospero Data Access Protocol
  "pawserv":          345,	//    Perf Analysis Workbench
  "zserv":            346,	//    Zebra server
  "fatserv":          347,	//    Fatmen Server
  "csi-sgwp":         348,	//    Cabletron Management Protocol
  "clearcase":        371,	//    Clearcase
  "ulistserv":        372,	//    Unix Listserv
  "legent-1":         373,	//    Legent Corporation
  "legent-2":         374,	//    Legent Corporation
  "hassle":           375,	//    Hassle
  "nip":              376,	//    Amiga Envoy Network Inquiry Proto
  "tnETOS":           377,	//    NEC Corporation
  "dsETOS":           378,	//    NEC Corporation
  "is99c":            379,	//    TIA/EIA/IS-99 modem client
  "is99s":            380,	//    TIA/EIA/IS-99 modem server
  "hp-collector":     381,	//    hp performance data collector
  "hp-managed-node":  382,	//    hp performance data managed node
  "hp-alarm-mgr":     383,	//    hp performance data alarm manager
  "arns":             384,	//    A Remote Network Server System
  "asa":              386,	//    ASA Message Router Object Def.
  "aurp":             387,	//    Appletalk Update-Based Routing Pro.
  "unidata-ldm":      388,	//    Unidata LDM Version 4
  "ldap":             389,	//    Lightweight Directory Access Protocol
  "uis":              390,	//    UIS
  "synotics-relay":   391,	//    SynOptics SNMP Relay Port
  "synotics-broker":  392,	//    SynOptics Port Broker Port
  "dis":              393,	//    Data Interpretation System
  "embl-ndt":         394,	//    EMBL Nucleic Data Transfer
  "netcp":            395,	//    NETscout Control Protocol
  "netware-ip":       396,	//    Novell Netware over IP
  "mptn":             397,	//    Multi Protocol Trans. Net.
  "kryptolan":        398,	//    Kryptolan
  "work-sol":         400,	//    Workstation Solutions
  "ups":              401,	//    Uninterruptible Power Supply
  "genie":            402,	//    Genie Protocol
  "decap":            403,	//    decap
  "nced":             404,	//    nced
  "ncld":             405,	//    ncld
  "imsp":             406,	//    Interactive Mail Support Protocol
  "timbuktu":         407,	//    Timbuktu
  "prm-sm":           408,	//    Prospero Resource Manager Sys. Man.
  "prm-nm":           409,	//    Prospero Resource Manager Node Man.
  "decladebug":       410,	//    DECLadebug Remote Debug Protocol
  "rmt":              411,	//    Remote MT Protocol
  "synoptics-trap":   412,	//    Trap Convention Port
  "smsp":             413,	//    SMSP
  "infoseek":         414,	//    InfoSeek
  "bnet":             415,	//    BNet
  "silverplatter":    416,	//    Silverplatter
  "onmux":            417,	//    Onmux
  "hyper-g":          418,	//    Hyper-G
  "ariel1":           419,	//    Ariel
  "smpte":            420,	//    SMPTE
  "ariel2":           421,	//    Ariel
  "ariel3":           422,	//    Ariel
  "opc-job-start":    423,	//    IBM Operations Planning and Control Start
  "opc-job-track":    424,	//    IBM Operations Planning and Control Track
  "icad-el":          425,	//    ICAD
  "smartsdp":         426,	//    smartsdp
  "svrloc":           427,	//    Server Location
  "ocs_cmu":          428,	//    OCS_CMU
  "ocs_amu":          429,	//    OCS_AMU
  "utmpsd":           430,	//    UTMPSD
  "utmpcd":           431,	//    UTMPCD
  "iasd":             432,	//    IASD
  "nnsp":             433,	//    NNSP
  "mobileip-agent":   434,	//    MobileIP-Agent
  "mobilip-mn":       435,	//    MobilIP-MN
  "dna-cml":          436,	//    DNA-CML
  "comscm":           437,	//    comscm
  "dsfgw":            438,	//    dsfgw
  "dasp":             439,	//    dasp      tommy@inlab.m.eunet.de
  "sgcp":             440,	//    sgcp
  "decvms-sysmgt":    441,	//    decvms-sysmgt
  "cvc_hostd":        442,	//    cvc_hostd
  "https":            443,	//    https  MCom
  "snpp":             444,	//    Simple Network Paging Protocol
  "microsoft-ds":     445,	//    Microsoft-DS
  "ddm-rdb":          446,	//    DDM-RDB
  "ddm-dfm":          447,	//    DDM-RFM
  "ddm-byte":         448,	//    DDM-BYTE
  "as-servermap":     449,	//    AS Server Mapper
  "tserver":          450,	//    TServer
  "biff":             512,	//    used by mail system to notify users
  "who":              513,	//    maintains data bases showing who's
  "syslog":           514,	//
  "printer":          515,	//    spooler
  "talk":             517,	//    like tenex link, but across
  "ntalk":            518,	//
  "utime":            519,	//    unixtime
  "router":           520,	//    local routing process (on site);
  "timed":            525,	//    timeserver
  "tempo":            526,	//    newdate
  "courier":          530,	//    rpc
  "conference":       531,	//    chat
  "netnews":          532,	//    readnews
  "netwall":          533,	//    for emergency broadcasts
  "apertus-ldp":      539,	//    Apertus Technologies Load Determination
  "uucp":             540,	//    uucpd
  "uucp-rlogin":      541,	//    uucp-rlogin  sl@wimsey.com
  "klogin":           543,	//
  "kshell":           544,	//    krcmd
  "new-rwho":         550,	//    new-who
  "dsf":              555,	//
  "remotefs":         556,	//    rfs server
  "rmonitor":         560,	//    rmonitord
  "monitor":          561,	//
  "chshell":          562,	//    chcmd
  "9pfs":             564,	//    plan 9 file service
  "whoami":           565,	//    whoami
  "meter":            570,	//    demon
  "meter":            571,	//    udemon
  "ipcserver":        600,	//    Sun IPC server
  "nqs":              607,	//    nqs
  "urm":              606,	//    Cray Unified Resource Manager
  "sift-uft":         608,	//    Sender-Initiated/Unsolicited File Transfer
  "npmp-trap":        609,	//    npmp-trap
  "npmp-local":       610,	//    npmp-local
  "npmp-gui":         611,	//    npmp-gui
  "ginad":            634,	//    ginad
  "mdqs":             666,	//
  "elcsd":            704,	//    errlog copy/server daemon
  "entrustmanager":   709,	//    EntrustManager
  "netviewdm1":       729,	//    IBM NetView DM/6000 Server/Client
  "netviewdm2":       730,	//    IBM NetView DM/6000 send/tcp
  "netviewdm3":       731,	//    IBM NetView DM/6000 receive/tcp
  "netgw":            741,	//    netGW
  "netrcs":           742,	//    Network based Rev. Cont. Sys.
  "flexlm":           744,	//    Flexible License Manager
  "fujitsu-dev":      747,	//    Fujitsu Device Control
  "ris-cm":           748,	//    Russell Info Sci Calendar Manager
  "kerberos-adm":     749,	//    kerberos administration
  "loadav":           750,	//
  "pump":             751,	//
  "qrh":              752,	//
  "rrh":              753,	//
  "tell":             754,	//     send
  "nlogin":           758,	//
  "con":              759,	//
  "ns":               760,	//
  "rxe":              761,	//
  "quotad":           762,	//
  "cycleserv":        763,	//
  "omserv":           764,	//
  "webster":          765,	//
  "phonebook":        767,	//    phone
  "vid":              769,	//
  "cadlock":          770,	//
  "rtip":             771,	//
  "cycleserv2":       772,	//
  "notify":           773,	//
  "acmaint_dbd":      774,	//
  "acmaint_transd":   775,	//
  "wpages":           776,	//
  "wpgs":             780,	//
  "concert":          786,	//       Concert
  "mdbs_daemon":      800,	//
  "device":           801,	//
  "xtreelic":         996,	//        Central Point Software
  "maitrd":           997,	//
  "puparp":           998,	//
  "applix":           999,	//        Applix ac
  "puprouter":        999,	//
  "ock":             1000,	//
  "blackjack":       1025,	//   network blackjack
  "iad1":            1030,	//   BBN IAD
  "iad2":            1031,	//   BBN IAD
  "iad3":            1032,	//   BBN IAD
  "instl_boots":     1067,	//   Installation Bootstrap Proto. Serv.
  "instl_bootc":     1068,	//   Installation Bootstrap Proto. Cli.
  "socks":           1080,	//   Socks
  "ansoft-lm-1":     1083,	//   Anasoft License Manager
  "ansoft-lm-2":     1084,	//   Anasoft License Manager
  "nfa":             1155,	//   Network File Access
  "nerv":            1222,	//   SNI R&D network
  "hermes":          1248,	//
  "alta-ana-lm":     1346,	//   Alta Analytics License Manager
  "bbn-mmc":         1347,	//   multi media conferencing
  "bbn-mmx":         1348,	//   multi media conferencing
  "sbook":           1349,	//   Registration Network Protocol
  "editbench":       1350,	//   Registration Network Protocol
  "equationbuilder": 1351,	//   Digital Tool Works (MIT)
  "lotusnote":       1352,	//   Lotus Note
  "relief":          1353,	//   Relief Consulting
  "rightbrain":      1354,	//   RightBrain Software
  "intuitive edge":  1355,	//   Intuitive Edge
  "cuillamartin":    1356,	//   CuillaMartin Company
  "pegboard":        1357,	//   Electronic PegBoard
  "connlcli":        1358,	//   CONNLCLI
  "ftsrv":           1359,	//   FTSRV
  "mimer":           1360,	//   MIMER
  "linx":            1361,	//   LinX
  "timeflies":       1362,	//   TimeFlies
  "ndm-requester":   1363,	//   Network DataMover Requester
  "ndm-server":      1364,	//   Network DataMover Server
  "adapt-sna":       1365,	//   Network Software Associates
  "netware-csp":     1366,	//   Novell NetWare Comm Service Platform
  "dcs":             1367,	//   DCS
  "screencast":      1368,	//   ScreenCast
  "gv-us":           1369,	//   GlobalView to Unix Shell
  "us-gv":           1370,	//   Unix Shell to GlobalView
  "fc-cli":          1371,	//   Fujitsu Config Protocol
  "fc-ser":          1372,	//   Fujitsu Config Protocol
  "chromagrafx":     1373,	//   Chromagrafx
  "molly":           1374,	//   EPI Software Systems
  "bytex":           1375,	//   Bytex
  "ibm-pps":         1376,	//   IBM Person to Person Software
  "cichlid":         1377,	//   Cichlid License Manager
  "elan":            1378,	//   Elan License Manager
  "dbreporter":      1379,	//   Integrity Solutions
  "telesis-licman":  1380,	//   Telesis Network License Manager
  "apple-licman":    1381,	//   Apple Network License Manager
  "udt_os":          1382,	//
  "gwha":            1383,	//   GW Hannaway Network License Manager
  "os-licman":       1384,	//   Objective Solutions License Manager
  "atex_elmd":       1385,	//   Atex Publishing License Manager
  "checksum":        1386,	//   CheckSum License Manager
  "cadsi-lm":        1387,	//   Computer Aided Design Software Inc LM
  "objective-dbc":   1388,	//   Objective Solutions DataBase Cache
  "iclpv-dm":        1389,	//   Document Manager
  "iclpv-sc":        1390,	//   Storage Controller
  "iclpv-sas":       1391,	//   Storage Access Server
  "iclpv-pm":        1392,	//   Print Manager
  "iclpv-nls":       1393,	//   Network Log Server
  "iclpv-nlc":       1394,	//   Network Log Client
  "iclpv-wsm":       1395,	//   PC Workstation Manager software
  "dvl-activemail":  1396,	//   DVL Active Mail
  "audio-activmail": 1397,	//   Audio Active Mail
  "video-activmail": 1398,	//   Video Active Mail
  "cadkey-licman":   1399,	//   Cadkey License Manager
  "cadkey-tablet":   1400,	//   Cadkey Tablet Daemon
  "goldleaf-licman": 1401,	//   Goldleaf License Manager
  "prm-sm-np":       1402,	//   Prospero Resource Manager
  "prm-nm-np":       1403,	//   Prospero Resource Manager
  "igi-lm":          1404,	//   Infinite Graphics License Manager
  "ibm-res":         1405,	//   IBM Remote Execution Starter
  "netlabs-lm":      1406,	//   NetLabs License Manager
  "dbsa-lm":         1407,	//   DBSA License Manager
  "sophia-lm":       1408,	//   Sophia License Manager
  "here-lm":         1409,	//   Here License Manager
  "hiq":             1410,	//   HiQ License Manager
  "af":              1411,	//   AudioFile
  "innosys":         1412,	//   InnoSys
  "innosys-acl":     1413,	//   Innosys-ACL
  "ibm-mqseries":    1414,	//   IBM MQSeries
  "dbstar":          1415,	//   DBStar
  "novell-lu6.2":    1416,	//   Novell LU6.2
  "timbuktu-srv2":   1418,	//   Timbuktu Service 2 Port
  "timbuktu-srv3":   1419,	//   Timbuktu Service 3 Port
  "timbuktu-srv4":   1420,	//   Timbuktu Service 4 Port
  "gandalf-lm":      1421,	//   Gandalf License Manager
  "autodesk-lm":     1422,	//   Autodesk License Manager
  "essbase":         1423,	//   Essbase Arbor Software
  "hybrid":          1424,	//   Hybrid Encryption Protocol
  "zion-lm":         1425,	//   Zion Software License Manager
  "sas-1":           1426,	//   Satellite-data Acquisition System 1
  "mloadd":          1427,	//   mloadd monitoring tool
  "informatik-lm":   1428,	//   Informatik License Manager
  "nms":             1429,	//   Hypercom NMS
  "tpdu":            1430,	//   Hypercom TPDU
  "rgtp":            1431,	//   Reverse Gosip Transport
  "blueberry-lm":    1432,	//   Blueberry Software License Manager
  "ms-sql-s":        1433,	//   Microsoft-SQL-Server
  "ms-sql-m":        1434,	//   Microsoft-SQL-Monitor
  "ibm-cics":        1435,	//   IBM CISC
  "sas-2":           1436,	//   Satellite-data Acquisition System 2
  "tabula":          1437,	//   Tabula
  "eicon-server":    1438,	//   Eicon Security Agent/Server
  "eicon-x25":       1439,	//   Eicon X25/SNA Gateway
  "eicon-slp":       1440,	//   Eicon Service Location Protocol
  "cadis-1":         1441,	//   Cadis License Management
  "cadis-2":         1442,	//   Cadis License Management
  "ies-lm":          1443,	//   Integrated Engineering Software
  "marcam-lm":       1444,	//   Marcam  License Management
  "proxima-lm":      1445,	//   Proxima License Manager
  "ora-lm":          1446,	//   Optical Research Associates License Manager
  "apri-lm":         1447,	//   Applied Parallel Research LM
  "oc-lm":           1448,	//   OpenConnect License Manager
  "peport":          1449,	//   PEport
  "dwf":             1450,	//   Tandem Distributed Workbench Facility
  "infoman":         1451,	//   IBM Information Management
  "gtegsc-lm":       1452,	//   GTE Government Systems License Man
  "genie-lm":        1453,	//   Genie License Manager
  "esl-lm":          1455,	//   ESL License Manager
  "dca":             1456,	//   DCA
  "valisys-lm":      1457,	//    Valisys License Manager
  "nrcabq-lm":       1458,	//    Nichols Research Corp.
  "proshare1":       1459,	//    Proshare Notebook Application
  "proshare2":       1460,	//    Proshare Notebook Application
  "ibm_wrless_lan":  1461,	//    IBM Wireless LAN
  "world-lm":        1462,	//    World License Manager
  "nucleus":         1463,	//    Nucleus
  "msl_lmd":         1464,	//    MSL License Manager
  "pipes":           1465,	//    Pipes Platform  mfarlin@peerlogic.com
  "oceansoft-lm":    1466,	//    Ocean Software License Manager
  "csdmbase":        1467,	//    CSDMBASE
  "csdm":            1468,	//    CSDM
  "aal-lm":          1469,	//    Active Analysis Limited License Manager
  "uaiact":          1470,	//    Universal Analytics
  "csdmbase":        1471,	//    csdmbase
  "csdm":            1472,	//    csdm
  "openmath":        1473,	//    OpenMath
  "telefinder":      1474,	//    Telefinder
  "taligent-lm":     1475,	//    Taligent License Manager
  "clvm-cfg":        1476,	//    clvm-cfg
  "ms-sna-server":   1477,	//    ms-sna-server
  "ms-sna-base":     1478,	//    ms-sna-base
  "dberegister":     1479,	//    dberegister
  "pacerforum":      1480,	//    PacerForum
  "airs":            1481,	//    AIRS
  "miteksys-lm":     1482,	//    Miteksys License Manager
  "afs":             1483,	//    AFS License Manager
  "confluent":       1484,	//    Confluent License Manager
  "lansource":       1485,	//    LANSource
  "nms_topo_serv":   1486,	//    nms_topo_serv
  "localinfosrvr":   1487,	//    LocalInfoSrvr
  "docstor":         1488,	//    DocStor
  "dmdocbroker":     1489,	//    dmdocbroker
  "insitu-conf":     1490,	//    insitu-conf
  "anynetgateway":   1491,	//    anynetgateway
  "stone-design-1":  1492,	//    stone-design-1
  "netmap_lm":       1493,	//    netmap_lm
  "ica":             1494,	//    ica
  "cvc":             1495,	//    cvc
  "liberty-lm":      1496,	//    liberty-lm
  "rfx-lm":          1497,	//    rfx-lm
  "watcom-sql":      1498,	//    Watcom-SQL
  "fhc":             1499,	//    Federico Heinz Consultora
  "vlsi-lm":         1500,	//    VLSI License Manager
  "sas-3":           1501,	//    Satellite-data Acquisition System 3
  "shivadiscovery":  1502,	//    Shiva
  "imtc-mcs":        1503,	//    Databeam
  "evb-elm":         1504,	//    EVB Software Engineering License Manager
  "funkproxy":       1505,	//    Funk Software, Inc.
  "ingreslock":      1524,	//    ingres
  "orasrv":          1525,	//    oracle
  "prospero-np":     1525,	//    Prospero Directory Service non-priv
  "pdap-np":         1526,	//    Prospero Data Access Prot non-priv
  "tlisrv":          1527,	//    oracle
  "coauthor":        1529,	//    oracle
  "issd":            1600,	//
  "nkd":             1650,	//
  "proshareaudio":   1651,	//   proshare conf audio
  "prosharevideo":   1652,	//   proshare conf video
  "prosharedata":    1653,	//   proshare conf data
  "prosharerequest": 1654,	//   proshare conf request
  "prosharenotify":  1655,	//   proshare conf notify
  "netview-aix-1":   1661,	//   netview-aix-1
  "netview-aix-2":   1662,	//   netview-aix-2
  "netview-aix-3":   1663,	//   netview-aix-3
  "netview-aix-4":   1664,	//   netview-aix-4
  "netview-aix-5":   1665,	//   netview-aix-5
  "netview-aix-6":   1666,	//   netview-aix-6
  "licensedaemon":   1986,	//   cisco license management
  "tr-rsrb-p1":      1987,	//   cisco RSRB Priority 1 port
  "tr-rsrb-p2":      1988,	//   cisco RSRB Priority 2 port
  "tr-rsrb-p3":      1989,	//   cisco RSRB Priority 3 port
  "mshnet":          1989,	//   MHSnet system
  "stun-p1":         1990,	//   cisco STUN Priority 1 port
  "stun-p2":         1991,	//   cisco STUN Priority 2 port
  "stun-p3":         1992,	//   cisco STUN Priority 3 port
  "ipsendmsg":       1992,	//   IPsendmsg
  "snmp-tcp-port":   1993,	//   cisco SNMP TCP port
  "stun-port":       1994,	//   cisco serial tunnel port
  "perf-port":       1995,	//   cisco perf port
  "tr-rsrb-port":    1996,	//   cisco Remote SRB port
  "gdp-port":        1997,	//   cisco Gateway Discovery Protocol
  "x25-svc-port":    1998,	//   cisco X.25 service (XOT)
  "tcp-id-port":     1999,	//   cisco identification port
  "callbook":        2000,	//
  "wizard":          2001,	//    curry
  "globe":           2002,	//
  "emce":            2004,	//    CCWS mm conf
  "oracle":          2005,	//
  "raid-cc":         2006,	//    raid
  "raid-am":         2007,	//
  "terminaldb":      2008,	//
  "whosockami":      2009,	//
  "pipe_server":     2010,	//
  "servserv":        2011,	//
  "raid-ac":         2012,	//
  "raid-cd":         2013,	//
  "raid-sf":         2014,	//
  "raid-cs":         2015,	//
  "bootserver":      2016,	//
  "bootclient":      2017,	//
  "rellpack":        2018,	//
  "about":           2019,	//
  "xinupageserver":  2020,	//
  "xinuexpansion1":  2021,	//
  "xinuexpansion2":  2022,	//
  "xinuexpansion3":  2023,	//
  "xinuexpansion4":  2024,	//
  "xribs":           2025,	//
  "scrabble":        2026,	//
  "shadowserver":    2027,	//
  "submitserver":    2028,	//
  "device2":         2030,	//
  "blackboard":      2032,	//
  "glogger":         2033,	//
  "scoremgr":        2034,	//
  "imsldoc":         2035,	//
  "objectmanager":   2038,	//
  "lam":             2040,	//
  "interbase":       2041,	//
  "isis":            2042,	//
  "isis-bcast":      2043,	//
  "rimsl":           2044,	//
  "cdfunc":          2045,	//
  "sdfunc":          2046,	//
  "dls":             2047,	//
  "dls-monitor":     2048,	//
  "shilp":           2049,	//
  "dlsrpn":          2065,	//   Data Link Switch Read Port Number
  "dlswpn":          2067,	//   Data Link Switch Write Port Number
  "ats":             2201,	//   Advanced Training System Program
  "rtsserv":         2500,	//   Resource Tracking system server
  "rtsclient":       2501,	//   Resource Tracking system client
  "www-dev":         2784,	//   world wide web - development
  "NSWS":            3049,	//
  "ccmail":          3264,	//   cc:mail/lotus
  "dec-notes":       3333,	//   DEC Notes
  "mapper-nodemgr":  3984,	//     MAPPER network node manager
  "mapper-mapethd":  3985,	//     MAPPER TCP/IP server
  "mapper-ws_ethd":  3986,	//     MAPPER workstation server
  "bmap":            3421,	//   Bull Apprise portmapper
  "udt_os":          3900,	//   Unidata UDT OS
  "nuts_dem":        4132,	//   NUTS Daemon
  "nuts_bootp":      4133,	//   NUTS Bootp Server
  "unicall":         4343,	//   UNICALL
  "krb524":          4444,	//   KRB524
  "rfa":             4672,	//   remote file access server
  "commplex-main":   5000,	//
  "commplex-link":   5001,	//
  "rfe":             5002,	//   radio free ethernet
  "telelpathstart":  5010,	//   TelepathStart
  "telelpathattack": 5011,	//   TelepathAttack
  "mmcc":            5050,	//   multimedia conference control tool
  "rmonitor_secure": 5145,	//
  "aol":             5190,	//   America-Online
  "padl2sim":        5236,	//
  "hacl-hb":         5300,	//        # HA cluster heartbeat
  "hacl-gs":         5301,	//        # HA cluster general services
  "hacl-cfg":        5302,	//        # HA cluster configuration
  "hacl-probe":      5303,	//        # HA cluster probing
  "hacl-local":      5304,	//
  "hacl-test":       5305,	//
  "x11":             6000,	//   X Window System
  "sub-process":     6111,	//   HP SoftBench Sub-Process Control
  "meta-corp":       6141,	//   Meta Corporation License Manager
  "aspentec-lm":     6142,	//   Aspen Technology License Manager
  "watershed-lm":    6143,	//   Watershed License Manager
  "statsci1-lm":     6144,	//   StatSci License Manager - 1
  "statsci2-lm":     6145,	//   StatSci License Manager - 2
  "lonewolf-lm":     6146,	//   Lone Wolf Systems License Manager
  "montage-lm":      6147,	//   Montage License Manager
  "xdsxdm":          6558,	//
  "afs3-fileserver": 7000,	//   file server itself
  "afs3-callback":   7001,	//   callbacks to cache managers
  "afs3-prserver":   7002,	//   users & groups database
  "afs3-vlserver":   7003,	//   volume location database
  "afs3-kaserver":   7004,	//   AFS/Kerberos authentication service
  "afs3-volser":     7005,	//   volume managment server
  "afs3-errors":     7006,	//   error interpretation service
  "afs3-bos":        7007,	//   basic overseer process
  "afs3-update":     7008,	//   server-to-server updater
  "afs3-rmtsys":     7009,	//   remote cache manager service
  "ups-onlinet":     7010,	//   onlinet uninterruptable power supplies
  "font-service":    7100,	//   X Font Service
  "fodms":           7200,	//   FODMS FLIP
  "man":             9535,	//
  "isode-dua":      17007,	//
]);

// Contains all non-private TCP port assignments as of RFC 1700
// Extended with some non-official.
constant tcp = ([
  "tcpmux":           	1,	//    TCP Port Service Multiplexer
  "compressnet-mgmt": 	2,	//    Management Utility
  "compressnet":      	3,	//    Compression Process
  "rje":              	5,	//    Remote Job Entry
  "echo":             	7,	//    Echo
  "discard":          	9,	//    Discard
  "systat":            11,	//    Active Users
  "daytime":           13,	//    Daytime
  "netstat":           15,	//    Unassigned [was netstat]
  "qotd":              17,	//    Quote of the Day
  "msp":               18,	//    Message Send Protocol
  "chargen":           19,	//    Character Generator
  "ftp-data":          20,	//    File Transfer [Default Data]
  "ftp":               21,	//    File Transfer [Control]
  "ssh":               22,	//    Secure Shell
  "telnet":            23,	//    Telnet
  "smtp":              25,	//    Simple Mail Transfer
  "nsw-fe":            27,	//    NSW User System FE
  "msg-icp":           29,	//    MSG ICP
  "msg-auth":          31,	//    MSG Authentication
  "dsp":               33,	//    Display Support Protocol
  "time":              37,	//    Time
  "rap":               38,	//    Route Access Protocol
  "rlp":               39,	//    Resource Location Protocol
  "graphics":          41,	//    Graphics
  "nameserver":        42,	//    Host Name Server
  "nicname":           43,	//    Who Is
  "mpm-flags":         44,	//    MPM FLAGS Protocol
  "mpm":               45,	//    Message Processing Module [recv]
  "mpm-snd":           46,	//    MPM [default send]
  "ni-ftp":            47,	//    NI FTP
  "auditd":            48,	//    Digital Audit Daemon
  "login":             49,	//    Login Host Protocol
  "re-mail-ck":        50,	//    Remote Mail Checking Protocol
  "la-maint":          51,	//    IMP Logical Address Maintenance
  "xns-time":          52,	//    XNS Time Protocol
  "domain":            53,	//    Domain Name Server
  "xns-ch":            54,	//    XNS Clearinghouse
  "isi-gl":            55,	//    ISI Graphics Language
  "xns-auth":          56,	//    XNS Authentication
  "xns-mail":          58,	//    XNS Mail
  "ni-mail":           61,	//    NI MAIL
  "acas":              62,	//    ACA Services
  "covia":             64,	//    Communications Integrator (CI)
  "tacacs-ds":         65,	//    TACACS-Database Service
  "sql*net":           66,	//    Oracle SQL*NET
  "bootps":            67,	//    Bootstrap Protocol Server
  "bootpc":            68,	//    Bootstrap Protocol Client
  "tftp":              69,	//    Trivial File Transfer
  "gopher":            70,	//    Gopher
  "netrjs-1":          71,	//    Remote Job Service
  "netrjs-2":          72,	//    Remote Job Service
  "netrjs-3":          73,	//    Remote Job Service
  "netrjs-4":          74,	//    Remote Job Service
  "deos":              76,	//    Distributed External Object Store
  "vettcp":            78,	//    vettcp
  "finger":            79,	//    Finger
  "www-http":          80,	//    World Wide Web HTTP
  "hosts2-ns":         81,	//    HOSTS2 Name Server
  "xfer":              82,	//    XFER Utility
  "mit-ml-dev":        83,	//    MIT ML Device
  "ctf":               84,	//    Common Trace Facility
  "mit-ml-dev":        85,	//    MIT ML Device
  "mfcobol":           86,	//    Micro Focus Cobol
  "kerberos":          88,	//    Kerberos
  "su-mit-tg":         89,	//    SU/MIT Telnet Gateway
  "dnsix":             90,	//    DNSIX Securit Attribute Token Map
  "mit-dov":           91,	//    MIT Dover Spooler
  "npp":               92,	//    Network Printing Protocol
  "dcp":               93,	//    Device Control Protocol
  "objcall":           94,	//    Tivoli Object Dispatcher
  "supdup":            95,	//    SUPDUP
  "dixie":             96,	//    DIXIE Protocol Specification
  "swift-rvf":         97,	//    Swift Remote Vitural File Protocol
  "tacnews":           98,	//    TAC News
  "metagram":          99,	//    Metagram Relay
  "newacct":          100,	//    [unauthorized use]
  "hostname":         101,	//    NIC Host Name Server
  "iso-tsap":         102,	//    ISO-TSAP
  "gppitnp":          103,	//    Genesis Point-to-Point Trans Net
  "acr-nema":         104,	//    ACR-NEMA Digital Imag. & Comm. 300
  "csnet-ns":         105,	//    Mailbox Name Nameserver
  "3com-tsmux":       106,	//    3COM-TSMUX
  "rtelnet":          107,	//    Remote Telnet Service
  "snagas":           108,	//    SNA Gateway Access Server
  "pop2":             109,	//    Post Office Protocol - Version 2
  "pop3":             110,	//    Post Office Protocol - Version 3
  "sunrpc":           111,	//    SUN Remote Procedure Call
  "mcidas":           112,	//    McIDAS Data Transmission Protocol
  "auth":             113,	//    Authentication Service
  "audionews":        114,	//    Audio News Multicast
  "sftp":             115,	//    Simple File Transfer Protocol
  "ansanotify":       116,	//    ANSA REX Notify
  "uucp-path":        117,	//    UUCP Path Service
  "sqlserv":          118,	//    SQL Services
  "nntp":             119,	//    Network News Transfer Protocol
  "cfdptkt":          120,	//    CFDPTKT
  "erpc":             121,	//    Encore Expedited Remote Pro.Call
  "smakynet":         122,	//    SMAKYNET
  "ntp":              123,	//    Network Time Protocol
  "ansatrader":       124,	//    ANSA REX Trader
  "locus-map":        125,	//    Locus PC-Interface Net Map Ser
  "unitary":          126,	//    Unisys Unitary Login
  "locus-con":        127,	//    Locus PC-Interface Conn Server
  "gss-xlicen":       128,	//    GSS X License Verification
  "pwdgen":           129,	//    Password Generator Protocol
  "cisco-fna":        130,	//    cisco FNATIVE
  "cisco-tna":        131,	//    cisco TNATIVE
  "cisco-sys":        132,	//    cisco SYSMAINT
  "statsrv":          133,	//    Statistics Service
  "ingres-net":       134,	//    INGRES-NET Service
  "loc-srv":          135,	//    Location Service
  "profile":          136,	//    PROFILE Naming System
  "netbios-ns":       137,	//    NETBIOS Name Service
  "netbios-dgm":      138,	//    NETBIOS Datagram Service
  "netbios-ssn":      139,	//    NETBIOS Session Service
  "emfis-data":       140,	//    EMFIS Data Service
  "emfis-cntl":       141,	//    EMFIS Control Service
  "bl-idm":           142,	//    Britton-Lee IDM
  "imap2":            143,	//    Interim Mail Access Protocol v2
  "news":             144,	//    NewS
  "uaac":             145,	//    UAAC Protocol
  "iso-tp0":          146,	//    ISO-IP0
  "iso-ip":           147,	//    ISO-IP
  "cronus":           148,	//    CRONUS-SUPPORT
  "aed-512":          149,	//    AED 512 Emulation Service
  "sql-net":          150,	//    SQL-NET
  "hems":             151,	//    HEMS
  "bftp":             152,	//    Background File Transfer Program
  "sgmp":             153,	//    SGMP
  "netsc-prod":       154,	//    NETSC
  "netsc-dev":        155,	//    NETSC
  "sqlsrv":           156,	//    SQL Service
  "knet-cmp":         157,	//    KNET/VM Command/Message Protocol
  "pcmail-srv":       158,	//    PCMail Server
  "nss-routing":      159,	//    NSS-Routing
  "sgmp-traps":       160,	//    SGMP-TRAPS
  "snmp":             161,	//    SNMP
  "snmptrap":         162,	//    SNMPTRAP
  "cmip-man":         163,	//    CMIP/TCP Manager
  "cmip-agent":       164,	//    CMIP/TCP Agent
  "xns-courier":      165,	//    Xerox
  "s-net":            166,	//    Sirius Systems
  "namp":             167,	//    NAMP
  "rsvd":             168,	//    RSVD
  "send":             169,	//    SEND
  "print-srv":        170,	//    Network PostScript
  "multiplex":        171,	//    Network Innovations Multiplex
  "cl/1":             172,	//    Network Innovations CL/1
  "xyplex-mux":       173,	//    Xyplex
  "mailq":            174,	//    MAILQ
  "vmnet":            175,	//    VMNET
  "genrad-mux":       176,	//    GENRAD-MUX
  "xdmcp":            177,	//    X Display Manager Control Protocol
  "nextstep":         178,	//    NextStep Window Server
  "bgp":              179,	//    Border Gateway Protocol
  "ris":              180,	//    Intergraph
  "unify":            181,	//    Unify
  "audit":            182,	//    Unisys Audit SITP
  "ocbinder":         183,	//    OCBinder
  "ocserver":         184,	//    OCServer
  "remote-kis":       185,	//    Remote-KIS
  "kis":              186,	//    KIS Protocol
  "aci":              187,	//    Application Communication Interface
  "mumps":            188,	//    Plus Five's MUMPS
  "qft":              189,	//    Queued File Transport
  "gacp":             190,	//    Gateway Access Control Protocol
  "prospero":         191,	//    Prospero Directory Service
  "osu-nms":          192,	//    OSU Network Monitoring System
  "srmp":             193,	//    Spider Remote Monitoring Protocol
  "irc":              194,	//    Internet Relay Chat Protocol
  "dn6-nlm-aud":      195,	//    DNSIX Network Level Module Audit
  "dn6-smm-red":      196,	//    DNSIX Session Mgt Module Audit Redir
  "dls":              197,	//    Directory Location Service
  "dls-mon":          198,	//    Directory Location Service Monitor
  "smux":             199,	//    SMUX
  "src":              200,	//    IBM System Resource Controller
  "at-rtmp":          201,	//    AppleTalk Routing Maintenance
  "at-nbp":           202,	//    AppleTalk Name Binding
  "at-3":             203,	//    AppleTalk Unused
  "at-echo":          204,	//    AppleTalk Echo
  "at-5":             205,	//    AppleTalk Unused
  "at-zis":           206,	//    AppleTalk Zone Information
  "at-7":             207,	//    AppleTalk Unused
  "at-8":             208,	//    AppleTalk Unused
  "tam":              209,	//    Trivial Authenticated Mail Protocol
  "z39.50":           210,	//    ANSI Z39.50
  "914c/g":           211,	//    Texas Instruments 914C/G Terminal
  "anet":             212,	//    ATEXSSTR
  "ipx":              213,	//    IPX
  "vmpwscs":          214,	//    VM PWSCS
  "softpc":           215,	//    Insignia Solutions
  "atls":             216,	//    Access Technology License Server
  "dbase":            217,	//    dBASE Unix
  "mpp":              218,	//    Netix Message Posting Protocol
  "uarps":            219,	//    Unisys ARPs
  "imap3":            220,	//    Interactive Mail Access Protocol v3
  "fln-spx":          221,	//    Berkeley rlogind with SPX auth
  "rsh-spx":          222,	//    Berkeley rshd with SPX auth
  "cdc":              223,	//    Certificate Distribution Center
  "sur-meas":         243,	//    Survey Measurement
  "link":             245,	//    LINK
  "dsp3270":          246,	//    Display Systems Protocol
  "pdap":             344,	//    Prospero Data Access Protocol
  "pawserv":          345,	//    Perf Analysis Workbench
  "zserv":            346,	//    Zebra server
  "fatserv":          347,	//    Fatmen Server
  "csi-sgwp":         348,	//    Cabletron Management Protocol
  "clearcase":        371,	//    Clearcase
  "ulistserv":        372,	//    Unix Listserv
  "legent-1":         373,	//    Legent Corporation
  "legent-2":         374,	//    Legent Corporation
  "hassle":           375,	//    Hassle
  "nip":              376,	//    Amiga Envoy Network Inquiry Proto
  "tnETOS":           377,	//    NEC Corporation
  "dsETOS":           378,	//    NEC Corporation
  "is99c":            379,	//    TIA/EIA/IS-99 modem client
  "is99s":            380,	//    TIA/EIA/IS-99 modem server
  "hp-collector":     381,	//    hp performance data collector
  "hp-managed-node":  382,	//    hp performance data managed node
  "hp-alarm-mgr":     383,	//    hp performance data alarm manager
  "arns":             384,	//    A Remote Network Server System
  "ibm-app":          385,	//    IBM Application
  "ibm-app":          385,	//    IBM Application
  "asa":              386,	//    ASA Message Router Object Def.
  "aurp":             387,	//    Appletalk Update-Based Routing Pro.
  "unidata-ldm":      388,	//    Unidata LDM Version 4
  "ldap":             389,	//    Lightweight Directory Access Protocol
  "uis":              390,	//    UIS
  "synotics-relay":   391,	//    SynOptics SNMP Relay Port
  "synotics-broker":  392,	//    SynOptics Port Broker Port
  "dis":              393,	//    Data Interpretation System
  "embl-ndt":         394,	//    EMBL Nucleic Data Transfer
  "netcp":            395,	//    NETscout Control Protocol
  "netware-ip":       396,	//    Novell Netware over IP
  "mptn":             397,	//    Multi Protocol Trans. Net.
  "kryptolan":        398,	//    Kryptolan
  "work-sol":         400,	//    Workstation Solutions
  "ups":              401,	//    Uninterruptible Power Supply
  "genie":            402,	//    Genie Protocol
  "decap":            403,	//    decap
  "nced":             404,	//    nced
  "ncld":             405,	//    ncld
  "imsp":             406,	//    Interactive Mail Support Protocol
  "timbuktu":         407,	//    Timbuktu
  "prm-sm":           408,	//    Prospero Resource Manager Sys. Man.
  "prm-nm":           409,	//    Prospero Resource Manager Node Man.
  "decladebug":       410,	//    DECLadebug Remote Debug Protocol
  "rmt":              411,	//    Remote MT Protocol
  "synoptics-trap":   412,	//    Trap Convention Port
  "smsp":             413,	//    SMSP
  "infoseek":         414,	//    InfoSeek
  "bnet":             415,	//    BNet
  "silverplatter":    416,	//    Silverplatter
  "onmux":            417,	//    Onmux
  "hyper-g":          418,	//    Hyper-G
  "ariel1":           419,	//    Ariel
  "smpte":            420,	//    SMPTE
  "ariel2":           421,	//    Ariel
  "ariel3":           422,	//    Ariel
  "opc-job-start":    423,	//    IBM Operations Planning and Control Start
  "opc-job-track":    424,	//    IBM Operations Planning and Control Track
  "icad-el":          425,	//    ICAD
  "smartsdp":         426,	//    smartsdp
  "svrloc":           427,	//    Server Location
  "ocs_cmu":          428,	//    OCS_CMU
  "ocs_amu":          429,	//    OCS_AMU
  "utmpsd":           430,	//    UTMPSD
  "utmpcd":           431,	//    UTMPCD
  "iasd":             432,	//    IASD
  "nnsp":             433,	//    NNSP
  "mobileip-agent":   434,	//    MobileIP-Agent
  "mobilip-mn":       435,	//    MobilIP-MN
  "dna-cml":          436,	//    DNA-CML
  "comscm":           437,	//    comscm
  "dsfgw":            438,	//    dsfgw
  "dasp":             439,	//    dasp      Thomas Obermair
  "sgcp":             440,	//    sgcp
  "decvms-sysmgt":    441,	//    decvms-sysmgt
  "cvc_hostd":        442,	//    cvc_hostd
  "https":            443,	//    https  MCom
  "snpp":             444,	//    Simple Network Paging Protocol
  "microsoft-ds":     445,	//    Microsoft-DS
  "ddm-rdb":          446,	//    DDM-RDB
  "ddm-dfm":          447,	//    DDM-RFM
  "ddm-byte":         448,	//    DDM-BYTE
  "as-servermap":     449,	//    AS Server Mapper
  "tserver":          450,	//    TServer
  "exec":             512,	//    remote process execution;
  "login":            513,	//    remote login a la telnet;
  "cmd":              514,	//    like exec, but automatic
  "printer":          515,	//    spooler
  "talk":             517,	//    like tenex link, but across
  "ntalk":            518,	//
  "utime":            519,	//    unixtime
  "efs":              520,	//    extended file name server
  "timed":            525,	//    timeserver
  "tempo":            526,	//    newdate
  "courier":          530,	//    rpc
  "conference":       531,	//    chat
  "netnews":          532,	//    readnews
  "netwall":          533,	//    for emergency broadcasts
  "apertus-ldp":      539,	//    Apertus Technologies Load Determination
  "uucp":             540,	//    uucpd
  "uucp-rlogin":      541,	//    uucp-rlogin  Stuart Lynne
  "klogin":           543,	//
  "kshell":           544,	//    krcmd
  "new-rwho":         550,	//    new-who
  "dsf":              555,	//
  "remotefs":         556,	//    rfs server
  "rmonitor":         560,	//    rmonitord
  "monitor":          561,	//
  "chshell":          562,	//    chcmd
  "snews":	      563,	//    NNTP over SSL
  "9pfs":             564,	//    plan 9 file service
  "whoami":           565,	//    whoami
  "meter":            570,	//    demon
  "meter":            571,	//    udemon
  "ipcserver":        600,	//    Sun IPC server
  "nqs":              607,	//    nqs
  "urm":              606,	//    Cray Unified Resource Manager
  "sift-uft":         608,	//    Sender-Initiated/Unsolicited File Transfer
  "npmp-trap":        609,	//    npmp-trap
  "npmp-local":       610,	//    npmp-local
  "npmp-gui":         611,	//    npmp-gui
  "ginad":            634,	//    ginad
  "mdqs":             666,	//
  "doom":             666,	//    doom Id Software
  "doom":             666,	//    doom Id Software
  "elcsd":            704,	//    errlog copy/server daemon
  "entrustmanager":   709,	//    EntrustManager
  "netviewdm1":       729,	//    IBM NetView DM/6000 Server/Client
  "netviewdm2":       730,	//    IBM NetView DM/6000 send/tcp
  "netviewdm3":       731,	//    IBM NetView DM/6000 receive/tcp
  "netgw":            741,	//    netGW
  "netrcs":           742,	//    Network based Rev. Cont. Sys.
  "flexlm":           744,	//    Flexible License Manager
  "fujitsu-dev":      747,	//    Fujitsu Device Control
  "ris-cm":           748,	//    Russell Info Sci Calendar Manager
  "kerberos-adm":     749,	//    kerberos administration
  "rfile":            750,	//
  "pump":             751,	//
  "qrh":              752,	//
  "rrh":              753,	//
  "tell":             754,	//     send
  "nlogin":           758,	//
  "con":              759,	//
  "ns":               760,	//
  "rxe":              761,	//
  "quotad":           762,	//
  "cycleserv":        763,	//
  "omserv":           764,	//
  "webster":          765,	//
  "phonebook":        767,	//    phone
  "vid":              769,	//
  "cadlock":          770,	//
  "rtip":             771,	//
  "cycleserv2":       772,	//
  "submit":           773,	//
  "rpasswd":          774,	//
  "entomb":           775,	//
  "wpages":           776,	//
  "wpgs":             780,	//
  "concert":          786,	//       Concert
  "mdbs_daemon":      800,	//
  "device":           801,	//
  "xtreelic":         996,	//        Central Point Software
  "maitrd":           997,	//
  "busboy":           998,	//
  "garcon":           999,	//
  "puprouter":        999,	//
  "cadlock":         1000,	//
  "blackjack":       1025,	//   network blackjack
  "iad1":            1030,	//   BBN IAD
  "iad2":            1031,	//   BBN IAD
  "iad3":            1032,	//   BBN IAD
  "instl_boots":     1067,	//   Installation Bootstrap Proto. Serv.
  "instl_bootc":     1068,	//   Installation Bootstrap Proto. Cli.
  "socks":           1080,	//   Socks
  "ansoft-lm-1":     1083,	//   Anasoft License Manager
  "ansoft-lm-2":     1084,	//   Anasoft License Manager
  "nfa":             1155,	//   Network File Access
  "nerv":            1222,	//   SNI R&D network
  "hermes":          1248,	//
  "alta-ana-lm":     1346,	//   Alta Analytics License Manager
  "bbn-mmc":         1347,	//   multi media conferencing
  "bbn-mmx":         1348,	//   multi media conferencing
  "sbook":           1349,	//   Registration Network Protocol
  "editbench":       1350,	//   Registration Network Protocol
  "equationbuilder": 1351,	//   Digital Tool Works (MIT)
  "lotusnote":       1352,	//   Lotus Note
  "relief":          1353,	//   Relief Consulting
  "rightbrain":      1354,	//   RightBrain Software
  "intuitive edge":  1355,	//   Intuitive Edge
  "cuillamartin":    1356,	//   CuillaMartin Company
  "pegboard":        1357,	//   Electronic PegBoard
  "connlcli":        1358,	//   CONNLCLI
  "ftsrv":           1359,	//   FTSRV
  "mimer":           1360,	//   MIMER
  "linx":            1361,	//   LinX
  "timeflies":       1362,	//   TimeFlies
  "ndm-requester":   1363,	//   Network DataMover Requester
  "ndm-server":      1364,	//   Network DataMover Server
  "adapt-sna":       1365,	//   Network Software Associates
  "netware-csp":     1366,	//   Novell NetWare Comm Service Platform
  "dcs":             1367,	//   DCS
  "screencast":      1368,	//   ScreenCast
  "gv-us":           1369,	//   GlobalView to Unix Shell
  "us-gv":           1370,	//   Unix Shell to GlobalView
  "fc-cli":          1371,	//   Fujitsu Config Protocol
  "fc-ser":          1372,	//   Fujitsu Config Protocol
  "chromagrafx":     1373,	//   Chromagrafx
  "molly":           1374,	//   EPI Software Systems
  "bytex":           1375,	//   Bytex
  "ibm-pps":         1376,	//   IBM Person to Person Software
  "cichlid":         1377,	//   Cichlid License Manager
  "elan":            1378,	//   Elan License Manager
  "dbreporter":      1379,	//   Integrity Solutions
  "telesis-licman":  1380,	//   Telesis Network License Manager
  "apple-licman":    1381,	//   Apple Network License Manager
  "udt_os":          1382,	//
  "gwha":            1383,	//   GW Hannaway Network License Manager
  "os-licman":       1384,	//   Objective Solutions License Manager
  "atex_elmd":       1385,	//   Atex Publishing License Manager
  "checksum":        1386,	//   CheckSum License Manager
  "cadsi-lm":        1387,	//   Computer Aided Design Software Inc LM
  "objective-dbc":   1388,	//   Objective Solutions DataBase Cache
  "iclpv-dm":        1389,	//   Document Manager
  "iclpv-sc":        1390,	//   Storage Controller
  "iclpv-sas":       1391,	//   Storage Access Server
  "iclpv-pm":        1392,	//   Print Manager
  "iclpv-nls":       1393,	//   Network Log Server
  "iclpv-nlc":       1394,	//   Network Log Client
  "iclpv-wsm":       1395,	//   PC Workstation Manager software
  "dvl-activemail":  1396,	//   DVL Active Mail
  "audio-activmail": 1397,	//   Audio Active Mail
  "video-activmail": 1398,	//   Video Active Mail
  "cadkey-licman":   1399,	//   Cadkey License Manager
  "cadkey-tablet":   1400,	//   Cadkey Tablet Daemon
  "goldleaf-licman": 1401,	//   Goldleaf License Manager
  "prm-sm-np":       1402,	//   Prospero Resource Manager
  "prm-nm-np":       1403,	//   Prospero Resource Manager
  "igi-lm":          1404,	//   Infinite Graphics License Manager
  "ibm-res":         1405,	//   IBM Remote Execution Starter
  "netlabs-lm":      1406,	//   NetLabs License Manager
  "dbsa-lm":         1407,	//   DBSA License Manager
  "sophia-lm":       1408,	//   Sophia License Manager
  "here-lm":         1409,	//   Here License Manager
  "hiq":             1410,	//   HiQ License Manager
  "af":              1411,	//   AudioFile
  "innosys":         1412,	//   InnoSys
  "innosys-acl":     1413,	//   Innosys-ACL
  "ibm-mqseries":    1414,	//   IBM MQSeries
  "dbstar":          1415,	//   DBStar
  "novell-lu6.2":    1416,	//   Novell LU6.2
  "timbuktu-srv1":   1417,	//   Timbuktu Service 1 Port
  "timbuktu-srv1":   1417,	//   Timbuktu Service 1 Port
  "timbuktu-srv2":   1418,	//   Timbuktu Service 2 Port
  "timbuktu-srv3":   1419,	//   Timbuktu Service 3 Port
  "timbuktu-srv4":   1420,	//   Timbuktu Service 4 Port
  "gandalf-lm":      1421,	//   Gandalf License Manager
  "autodesk-lm":     1422,	//   Autodesk License Manager
  "essbase":         1423,	//   Essbase Arbor Software
  "hybrid":          1424,	//   Hybrid Encryption Protocol
  "zion-lm":         1425,	//   Zion Software License Manager
  "sas-1":           1426,	//   Satellite-data Acquisition System 1
  "mloadd":          1427,	//   mloadd monitoring tool
  "informatik-lm":   1428,	//   Informatik License Manager
  "nms":             1429,	//   Hypercom NMS
  "tpdu":            1430,	//   Hypercom TPDU
  "rgtp":            1431,	//   Reverse Gosip Transport
  "blueberry-lm":    1432,	//   Blueberry Software License Manager
  "ms-sql-s":        1433,	//   Microsoft-SQL-Server
  "ms-sql-m":        1434,	//   Microsoft-SQL-Monitor
  "ibm-cics":        1435,	//   IBM CISC
  "sas-2":           1436,	//   Satellite-data Acquisition System 2
  "tabula":          1437,	//   Tabula
  "eicon-server":    1438,	//   Eicon Security Agent/Server
  "eicon-x25":       1439,	//   Eicon X25/SNA Gateway
  "eicon-slp":       1440,	//   Eicon Service Location Protocol
  "cadis-1":         1441,	//   Cadis License Management
  "cadis-2":         1442,	//   Cadis License Management
  "ies-lm":          1443,	//   Integrated Engineering Software
  "marcam-lm":       1444,	//   Marcam  License Management
  "proxima-lm":      1445,	//   Proxima License Manager
  "ora-lm":          1446,	//   Optical Research Associates License Manager
  "apri-lm":         1447,	//   Applied Parallel Research LM
  "oc-lm":           1448,	//   OpenConnect License Manager
  "peport":          1449,	//   PEport
  "dwf":             1450,	//   Tandem Distributed Workbench Facility
  "infoman":         1451,	//   IBM Information Management
  "gtegsc-lm":       1452,	//   GTE Government Systems License Man
  "genie-lm":        1453,	//   Genie License Manager
  "interhdl_elmd":   1454,	//   interHDL License Manager
  "interhdl_elmd":   1454,	//   interHDL License Manager
  "esl-lm":          1455,	//   ESL License Manager
  "dca":             1456,	//   DCA
  "valisys-lm":      1457,	//    Valisys License Manager
  "nrcabq-lm":       1458,	//    Nichols Research Corp.
  "proshare1":       1459,	//    Proshare Notebook Application
  "proshare2":       1460,	//    Proshare Notebook Application
  "ibm_wrless_lan":  1461,	//    IBM Wireless LAN
  "world-lm":        1462,	//    World License Manager
  "nucleus":         1463,	//    Nucleus
  "msl_lmd":         1464,	//    MSL License Manager
  "pipes":           1465,	//    Pipes Platform
  "oceansoft-lm":    1466,	//    Ocean Software License Manager
  "csdmbase":        1467,	//    CSDMBASE
  "csdm":            1468,	//    CSDM
  "aal-lm":          1469,	//    Active Analysis Limited License Manager
  "uaiact":          1470,	//    Universal Analytics
  "csdmbase":        1471,	//    csdmbase
  "csdm":            1472,	//    csdm
  "openmath":        1473,	//    OpenMath
  "telefinder":      1474,	//    Telefinder
  "taligent-lm":     1475,	//    Taligent License Manager
  "clvm-cfg":        1476,	//    clvm-cfg
  "ms-sna-server":   1477,	//    ms-sna-server
  "ms-sna-base":     1478,	//    ms-sna-base
  "dberegister":     1479,	//    dberegister
  "pacerforum":      1480,	//    PacerForum
  "airs":            1481,	//    AIRS
  "miteksys-lm":     1482,	//    Miteksys License Manager
  "afs":             1483,	//    AFS License Manager
  "confluent":       1484,	//    Confluent License Manager
  "lansource":       1485,	//    LANSource
  "nms_topo_serv":   1486,	//    nms_topo_serv
  "localinfosrvr":   1487,	//    LocalInfoSrvr
  "docstor":         1488,	//    DocStor
  "dmdocbroker":     1489,	//    dmdocbroker
  "insitu-conf":     1490,	//    insitu-conf
  "anynetgateway":   1491,	//    anynetgateway
  "stone-design-1":  1492,	//    stone-design-1
  "netmap_lm":       1493,	//    netmap_lm
  "ica":             1494,	//    ica
  "cvc":             1495,	//    cvc
  "liberty-lm":      1496,	//    liberty-lm
  "rfx-lm":          1497,	//    rfx-lm
  "watcom-sql":      1498,	//    Watcom-SQL
  "fhc":             1499,	//    Federico Heinz Consultora
  "vlsi-lm":         1500,	//    VLSI License Manager
  "sas-3":           1501,	//    Satellite-data Acquisition System 3
  "shivadiscovery":  1502,	//    Shiva
  "imtc-mcs":        1503,	//    Databeam
  "evb-elm":         1504,	//    EVB Software Engineering License Manager
  "funkproxy":       1505,	//    Funk Software, Inc.
  "ingreslock":      1524,	//    ingres
  "orasrv":          1525,	//    oracle
  "prospero-np":     1525,	//    Prospero Directory Service non-priv
  "pdap-np":         1526,	//    Prospero Data Access Prot non-priv
  "tlisrv":          1527,	//    oracle
  "coauthor":        1529,	//    oracle
  "issd":            1600,	//
  "nkd":             1650,	//
  "proshareaudio":   1651,	//   proshare conf audio
  "prosharevideo":   1652,	//   proshare conf video
  "prosharedata":    1653,	//   proshare conf data
  "prosharerequest": 1654,	//   proshare conf request
  "prosharenotify":  1655,	//   proshare conf notify
  "netview-aix-1":   1661,	//   netview-aix-1
  "netview-aix-2":   1662,	//   netview-aix-2
  "netview-aix-3":   1663,	//   netview-aix-3
  "netview-aix-4":   1664,	//   netview-aix-4
  "netview-aix-5":   1665,	//   netview-aix-5
  "netview-aix-6":   1666,	//   netview-aix-6
  "licensedaemon":   1986,	//   cisco license management
  "tr-rsrb-p1":      1987,	//   cisco RSRB Priority 1 port
  "tr-rsrb-p2":      1988,	//   cisco RSRB Priority 2 port
  "tr-rsrb-p3":      1989,	//   cisco RSRB Priority 3 port
  "mshnet":          1989,	//   MHSnet system
  "stun-p1":         1990,	//   cisco STUN Priority 1 port
  "stun-p2":         1991,	//   cisco STUN Priority 2 port
  "stun-p3":         1992,	//   cisco STUN Priority 3 port
  "ipsendmsg":       1992,	//   IPsendmsg
  "snmp-tcp-port":   1993,	//   cisco SNMP TCP port
  "stun-port":       1994,	//   cisco serial tunnel port
  "perf-port":       1995,	//   cisco perf port
  "tr-rsrb-port":    1996,	//   cisco Remote SRB port
  "gdp-port":        1997,	//   cisco Gateway Discovery Protocol
  "x25-svc-port":    1998,	//   cisco X.25 service (XOT)
  "tcp-id-port":     1999,	//   cisco identification port
  "callbook":        2000,	//
  "dc":              2001,	//
  "globe":           2002,	//
  "mailbox":         2004,	//
  "berknet":         2005,	//
  "invokator":       2006,	//
  "dectalk":         2007,	//
  "conf":            2008,	//
  "news":            2009,	//
  "search":          2010,	//
  "raid-cc":         2011,	//    raid
  "ttyinfo":         2012,	//
  "raid-am":         2013,	//
  "troff":           2014,	//
  "cypress":         2015,	//
  "bootserver":      2016,	//
  "cypress-stat":    2017,	//
  "terminaldb":      2018,	//
  "whosockami":      2019,	//
  "xinupageserver":  2020,	//
  "servexec":        2021,	//
  "down":            2022,	//
  "xinuexpansion3":  2023,	//
  "xinuexpansion4":  2024,	//
  "ellpack":         2025,	//
  "scrabble":        2026,	//
  "shadowserver":    2027,	//
  "submitserver":    2028,	//
  "device2":         2030,	//
  "blackboard":      2032,	//
  "glogger":         2033,	//
  "scoremgr":        2034,	//
  "imsldoc":         2035,	//
  "objectmanager":   2038,	//
  "lam":             2040,	//
  "interbase":       2041,	//
  "isis":            2042,	//
  "isis-bcast":      2043,	//
  "rimsl":           2044,	//
  "cdfunc":          2045,	//
  "sdfunc":          2046,	//
  "dls":             2047,	//
  "dls-monitor":     2048,	//
  "shilp":           2049,	//
  "dlsrpn":          2065,	//   Data Link Switch Read Port Number
  "dlswpn":          2067,	//   Data Link Switch Write Port Number
  "ats":             2201,	//   Advanced Training System Program
  "rtsserv":         2500,	//   Resource Tracking system server
  "rtsclient":       2501,	//   Resource Tracking system client
  "hp-3000-telnet":  2564,	//   HP 3000 NS/VT block mode telnet
  "www-dev":         2784,	//   world wide web - development
  "NSWS":            3049,	//
  "ccmail":          3264,	//   cc:mail/lotus
  "dec-notes":       3333,	//   DEC Notes
  "mapper-nodemgr":  3984,	//     MAPPER network node manager
  "mapper-mapethd":  3985,	//     MAPPER TCP/IP server
  "mapper-ws_ethd":  3986,	//     MAPPER workstation server
  "bmap":            3421,	//   Bull Apprise portmapper
  "udt_os":          3900,	//   Unidata UDT OS
  "nuts_dem":        4132,	//   NUTS Daemon
  "nuts_bootp":      4133,	//   NUTS Bootp Server
  "unicall":         4343,	//   UNICALL
  "krb524":          4444,	//   KRB524
  "rfa":             4672,	//   remote file access server
  "commplex-main":   5000,	//
  "commplex-link":   5001,	//
  "rfe":             5002,	//   radio free ethernet
  "telelpathstart":  5010,	//   TelepathStart
  "telelpathattack": 5011,	//   TelepathAttack
  "mmcc":            5050,	//   multimedia conference control tool
  "rmonitor_secure": 5145,	//
  "aol":             5190,	//   America-Online
  "padl2sim":        5236,	//
  "hacl-hb":         5300,	//        # HA cluster heartbeat
  "hacl-gs":         5301,	//        # HA cluster general services
  "hacl-cfg":        5302,	//        # HA cluster configuration
  "hacl-probe":      5303,	//        # HA cluster probing
  "hacl-local":      5304,	//
  "hacl-test":       5305,	//
  "x11":             6000,	//   X Window System
  "sub-process":     6111,	//   HP SoftBench Sub-Process Control
  "meta-corp":       6141,	//   Meta Corporation License Manager
  "aspentec-lm":     6142,	//   Aspen Technology License Manager
  "watershed-lm":    6143,	//   Watershed License Manager
  "statsci1-lm":     6144,	//   StatSci License Manager - 1
  "statsci2-lm":     6145,	//   StatSci License Manager - 2
  "lonewolf-lm":     6146,	//   Lone Wolf Systems License Manager
  "montage-lm":      6147,	//   Montage License Manager
  "xdsxdm":          6558,	//
  "afs3-fileserver": 7000,	//   file server itself
  "afs3-callback":   7001,	//   callbacks to cache managers
  "afs3-prserver":   7002,	//   users & groups database
  "afs3-vlserver":   7003,	//   volume location database
  "afs3-kaserver":   7004,	//   AFS/Kerberos authentication service
  "afs3-volser":     7005,	//   volume managment server
  "afs3-errors":     7006,	//   error interpretation service
  "afs3-bos":        7007,	//   basic overseer process
  "afs3-update":     7008,	//   server-to-server updater
  "afs3-rmtsys":     7009,	//   remote cache manager service
  "ups-onlinet":     7010,	//   onlinet uninterruptable power supplies
  "font-service":    7100,	//   X Font Service
  "fodms":           7200,	//   FODMS FLIP
  "man":             9535,	//
  "isode-dua":      17007,	//
]);

