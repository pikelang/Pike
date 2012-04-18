// $Id$

#pike __REAL_VERSION__

protected constant small_ext2type = ([
  "html" : "text/html",
  "txt"  : "text/plain",
  "css"  : "text/css",
  "gif"  : "image/gif",
  "jpg"  : "image/jpeg",
  "jpeg" : "image/jpeg",
  "png"  : "image/png",
]);

protected mapping ext2type = ([

// This list is divided into 2 sections: the first contains all entries
// present from the IANA media types list on 2002-02-05, with the exception
// of application types.
//
// the second portion of the list consists of registered and common 
// types as of 2012-04-17. The bulk of this list comes from the public
// domain mime types list published by the Apache Software Foundation.

// -------------------- Audio ---------------------

// IANA registered

  "726"     : "audio/32kadpcm",
// audio/3gpp
// audio/3gpp2
// audio/ac3
// audio/AMR
// audio/AMR-WB
// audio/AMR-WB+
// audio/ATRAC-ADVANCED-LOSSLESS
// audio/ATRAC-X
// audio/ATRAC3
  "au"      : "audio/basic",
  "snd"     : "audio/basic",
// audio/BV16
// audio/BV32
// audio/clearmode
// audio/CN
// audio/DAT12
  "dls"     : "audio/dls",
// audio/dsr-es201108
// audio/dsr-es202050
// audio/dsr-es202211
// audio/dsr-es202212
// audio/DVI4
// audio/eac3
// audio/EVRC
// audio/EVRC0
// audio/EVRC1
// audio/EVRCB
// audio/EVRCB0
// audio/EVRCB1
// audio/example
// audio/G719
// audio/G722
// audio/G7221
// audio/G723
// audio/G726-16
// audio/G726-24
// audio/G726-32
// audio/G726-40
// audio/G728
// audio/G729
// audio/G7291
// audio/G729D
// audio/G729E
// audio/G.722.1
// audio/GSM
// audio/GSM-EFR
// audio/iLBC
// audio/L8
  "l16"     : "audio/L16",
// audio/L20
// audio/L24
// audio/LPC
// audio/mobile-xmf
// audio/MP4A-LATM
// audio/mpa-robust
  "abs"     : "audio/mpeg",
  "mp2"     : "audio/mpeg",
  "mp3"     : "audio/mpeg",
// "mpa"      : "audio/mpeg",
  "mpega"   : "audio/mpeg",
  "oga"     : "audio/ogg",
  "ogg"     : "audio/ogg",
  "spx"     : "audio/ogg",
// audio/parityfec
// audio/PCMA
// audio/PCMU
  "sid"     : "audio/prs.sid",
  "psid"    : "audio/prs.sid",
// audio/telephone-event
// audio/tone
// audio/VDVI
// audio/vnd.3gpp.iufp
// audio/vnd.cisco.nse
// audio/vnd.cns.anp1
// audio/vnd.cns.inf1
  "eol"     : "audio/vnd.digital-winds",
  "plj"     : "audio/vnd.everad.plj",
  "lvp"     : "audio/vnd.lucent.voice",
  "vbk"     : "audio/vnd.nortel.vbk",
  "ecelp4800" : "audio/vnd.nuera.ecelp4800",
  "ecelp7470" : "audio/vnd.nuera.ecelp7470",
  "ecelp9600" : "audio/vnd.nuera.ecelp9600",
// audio/vnd.octel.sbc
  "qcp"     : "audio/vnd.qcelp",
// audio/vnd.rhetorex.32kadpcm
// audio/vnd.vmx.cvsd

// Other

  "aif"     : "audio/x-aiff",
  "aifc"    : "audio/x-aiff",
  "aiff"    : "audio/x-aiff",
  "cht"     : "audio/x-dspeech",
  "dus"     : "audio/x-dspeech",
  "es"      : "audio/echospeech",
  "gsm"     : "audio/gsm",
  "la"      : "audio/x-nspaudio",
  "lma"     : "audio/x-nspaudio",
  "m3u"     : "audio/x-mpegurl",
  "mid"     : "audio/midi",
  "midi"    : "audio/midi",
  "mod"     : "audio/x-mod",
  "mp3url"  : "audio/x-mpegurl",
  "pls"     : "audio/x-mpegurl",
  "ra"      : "audio/x-pn-realaudio",
  "ram"     : "audio/x-pn-realaudio",
  "rmi"     : "audio/midi",
  "rmm"     : "audio/x-pn-realaudio",
  "rmf"     : "audio/rmf",
  "rpm"     : "audio/x-pn-realaudio-plugin",
  "sd2"     : "audio/x-sd2",
  "tsi"     : "audio/tsplayer",
  "vqf"     : "audio/x-twinvq",
  "vqe"     : "audio/x-twinvq-plugin",
  "vql"     : "audio/x-twinvq",
  "vox"     : "audio/voxware",
  "wav"     : "audio/x-wav",
  "wtx"     : "audio/x-wtx",


// -------------------- Image ---------------------

// IANA registered

  "cgm"     : "image/cgm",
// image/g3fax
  // (shortcut)  "gif"     : "image/gif",
  "ief"     : "image/ief",
  "jfif"    : "image/jpeg",
  "jpe"     : "image/jpeg",
  // (shortcut)  "jpeg"    : "image/jpeg",
  // (shortcut)  "jpg"     : "image/jpeg",
  "pjp"     : "image/jpeg",
  "pjpg"    : "image/jpeg",
// image/naplps
  // (shortcut)  "png"     : "image/png",
  "btif"    : "image/prs.btif",
  "btf"     : "image/prs.btif",
  "pti"     : "image/prs.pti",
  "tif"     : "image/tiff",
  "tiff"    : "image/tiff",
// image/vnd.cns.inf2
  "djvu"    : "image/vnd.djvu",
  "djv"     : "image/vnd.djvu",
  "dwg"     : "image/vnd.dwg",
  "dxf"     : "image/vnd.dxf",
  "fbs"     : "image/vnd.fastbidsheet",
  "fpx"     : "image/vnd.fpx",
  "fst"     : "image/vnd.fst",
  "mmr"     : "image/vnd.fujixerox.edmics-mmr",
  "rlc"     : "image/vnd.fujixerox.edmics-rlc",
// image/vnd.mix
// image/vnd.net-fpx
  "svf"     : "image/vnd.svf",
  "wbmp"    : "image/vnd.wap.wbmp",
  "xif"     : "image/vnd.xiff",


  "svg"     : "image/svg+xml",
  "svgz"    : "image/svg+xml",

// Other

  "bmp"     : "image/x-MS-bmp",
  "cmx"     : "image/x-cmx",
  "clp"     : "image/x-clp",
  "cod"     : "image/cis-cod",
  "cpi"     : "image/cpi",
  "cut"     : "image/x-halo-cut",
  "dsf"     : "image/x-mgx-dsf",
  "emf"     : "image/x-emf",
  "etf"     : "image/x-etf",
  "fh"      : "image/x-freehand",
  "fh4"     : "image/x-freehand",
  "fh5"     : "image/x-freehand",
  "fh7"     : "image/x-freehand",
  "fhc"     : "image/x-freehand",
  "fif"     : "image/fif",
  "iff"     : "image/ilbm",
  "ilbm"    : "image/ilbm",
  "jps"     : "image/x-jps",
  "mac"     : "image/x-macpaint",
  "mcf"     : "image/vasa",
  "pbm"     : "image/x-portable-bitmap",
  "pct"     : "image/pict",
  "pcd"     : "image/x-photo-cd",
  "pgm"     : "image/x-portable-graymap",
  "pic"     : "image/pict",
  "pict"    : "image/pict",
  "pnm"     : "image/x-portable-anymap",
  "pntg"    : "image/x-macpaint",
  "ppm"     : "image/x-portable-pixmap",
  "qif"     : "image/x-quicktime",
  "qti"     : "image/x-quicktime",
  "qtif"    : "image/x-quicktime",
  "ras"     : "image/x-cmu-raster",
  "rf"      : "image/vnd.rn-realflash",
  "rgb"     : "image/x-rgb",
  "rip"     : "image/rip",
  "rp"      : "image/vnd.rn-realpix",
  "rs"      : "image/x-sun-raster",
  "svh"     : "image/svh",
  "tga"     : "image/x-targa",
  "wi"      : "image/wavelet",
  "xbm"     : "image/x-xbitmap",
  "xpm"     : "image/x-xpixmap",
  "xwd"     : "image/x-xwindowdump",


// -------------------- Video ---------------------

// IANA registered

// video/DV
// video/MP4V-ES
  "m1v"     : "video/mpeg",
  "mpa"     : "video/mpeg",
  "mpe"     : "video/mpeg",
  "mpeg"    : "video/mpeg",
  "mpegv"   : "video/mpeg",
  "mpg"     : "video/mpeg",
  "mpv"     : "video/mpeg",
  "vbs"     : "video/mpeg",
// video/nv
// video/prityfec
// video/pointer
  "moov"    : "video/quicktime",
  "mov"     : "video/quicktime",
  "qt"      : "video/quicktime",
  "fvt"     : "video/vnd.fvt",
// video/vnd.motorola.video
// video/vnd.motorola.videop
  "mxu"     : "video/vnd.mpegurl",
  "m4u"     : "video/vnd.mpegurl",
  "nim"     : "video/vnd.nokia.interleaved-multimedia",
  "viv"     : "video/vnd.vivo",
  "vivo"    : "video/vnd.vivo",

// Other

  "afl"     : "video/animaflex",
  "anim3"   : "video/x-anim",
  "anim5"   : "video/x-anim",
  "anim7"   : "video/x-anim",
  "anim8"   : "video/x-anim",
  "asf"     : "video/x-ms-asf",
  "asx"     : "video/x-ms-asf",
  "avi"     : "video/x-msvideo",
  "dif"     : "video/x-dv",
  "dv"      : "video/x-dv",
  "fli"     : "video/x-fli",
  "ivf"     : "video/x-ivf",
  "lsf"     : "video/x-ms-asf",
  "lsx"     : "video/x-ms-asx",
  "mng"     : "video/mng",
  "movie"   : "video/x-sgi-movie",
  "mp2v"    : "video/x-mpeg2",
  "mpv2"    : "video/x-mpeg2",
  "rv"      : "video/vnd.rn-realvideo",
  "vgm"     : "video/x-videogram",
  "vgp"     : "video/x-videogram-plugin",
  "vgx"     : "video/x-videogram",
  "vos"     : "video/vosaic",
  "xdr"     : "video/x-videogram",


// -------------------- Text ---------------------

// IANA registered

  "ics"     : "text/calendar",
  "ifb"     : "text/calendar",
  // (shortcut)  "css"     : "text/css",
// text/directory
// text/enriched
  "htm"     : "text/html",
  // (shorcut) "html"    : "text/html",
  "htp"     : "text/html",
  "rxml"    : "text/html",
  "shtml"   : "text/html",
  "spider"  : "text/html",
  "spml"    : "text/html",
// text/parityfec
  "bat"     : "text/plain",
  "java"    : "text/plain",
  "text"    : "text/plain",
  // (shortcut)  "txt"     : "text/plain",
  "tag"     : "text/prs.lines.tag",
  "dsc"     : "text/prs.lines.tag",
// text/rfc822-headers
  "rtx"     : "text/richtext",
  "rtf"     : "text/rtf",
  "sgm"     : "text/sgml",
  "sgml"    : "text/sgml",
// text/t140
  "tsv"     : "text/tab-separated-values",
  "uris"    : "text/uri-list",
  "uri"     : "text/uri-list",
  "abc"     : "text/vnd.abc",
  "curl"    : "text/vnd.curl",
  "dms"     : "text/DMClientScript",
  "fly"     : "text/vnd.fly",
  "flx"     : "text/vnd.fmi.flexstor",
// "3dml"     : "text/vnd.in3d.3dml",
// "3dm"      : "text/vnd.in3d.3dml",
  "spot"    : "text/vnd.in3d.spot",
  "spo"     : "text/vnd.in3d.spot",
// text/vnd.IPTC.NewsML
// text/vnd.IPTC.NITF
// text/vnd.latex-z
// text/vnd.motorola.reflex
  "mpf"     : "text/vnd.ms-mediapackage",
  "ccc"     : "text/vnd.net2phone.commcenter.command",
  "jad"     : "text/vnd.sun.jme2.app-descriptor",
  "si"      : "text/wap.si",
  "sl"      : "text/wap.sl",
  "wml"     : "text/vnd.wap.wml",
  "wmls"    : "text/vnd.wap.wmlscript",
  "xml"     : "text/xml",
// text/xml-external-parsed-entity

// Other

  "c"       : "text/x-c-code",
  "cc"      : "text/x-c++-code",
  "cpp"     : "text/x-c++-code",
  "etx"     : "text/x-setext",
  "h"       : "text/x-include-file",
  "hdml"    : "text/x-hdml",
  "htt"     : "text/webviewhtml",
  "pgr"     : "text/parsnegar-document",
  "pike"    : "text/x-pike-code",
  "pmod"    : "text/x-pike-code",
  "rt"      : "text/vnd.rn-realtext",
// "src"     : "text/speech",
// "talk"    : "text/speech",
  "vcf"     : "text/x-vCard",
  "vcs"     : "text/x-vCalendar",


// -------------------- Application ---------------------

// IANA registered

  "arj"     : "application/octet-stream",
  "bin"     : "application/octet-stream",
  "com"     : "application/octet-stream",
  "dll"     : "application/octet-stream",
  "drv"     : "application/octet-stream",
  "exe"     : "application/octet-stream",
  "so"      : "application/octet-stream",
  "ai"      : "application/postscript",
  "eps"     : "application/postscript",
  "ps"      : "application/postscript",
  "oda"     : "application/oda",
// application/atomicmail
// application/andrew-inset
// application/slate
// application/wita
// application/dec-dx
// application/dca-rft
// application/activemessage
// "rtf"      : "application/rtf",
// application/applefile
  "hqx"     : "application/mac-binhex40",
// application/news-message-id
// application/news-transmission
  "wpd"     : "application/wordperfect5.1",
  "wp6"     : "application/wordperfect5.1",
  "pdf"     : "application/pdf",
  "zip"     : "application/zip",
// application/macwriteii
  "doc"     : "application/msword",
  "dot"     : "application/msword",
// application/remote-printing
// application/mathematica
// application/cybercash
// application/commonground
// application/iges
// application/riscos
// application/eshop
// application/x400-bp
// application/sgml
// application/cals-1840
// application/pgp-encrypted
// application/pgp-signature
// application/pgp-keys
// application/vnd.framemaker
// application/vnd.mif
  "xla"     : "application/vnd.ms-excel",
  "xlb"     : "application/vnd.ms-excel",
  "xlc"     : "application/vnd.ms-excel",
  "xld"     : "application/vnd.ms-excel",
  "xll"     : "application/vnd.ms-excel",
  "xlm"     : "application/vnd.ms-excel",
  "xls"     : "application/vnd.ms-excel",
  "xlt"     : "application/vnd.ms-excel",
  "xlw"     : "application/vnd.ms-excel",
  "ppa"     : "application/vnd.ms-powerpoint",
  "pot"     : "application/vnd.ms-powerpoint",
  "ppt"     : "application/vnd.ms-powerpoint",
  "pps"     : "application/vnd.ms-powerpoint",
  "pwz"     : "application/vnd.ms-powerpoint",
// application/vnd.ms-project
// application/vnd.ms-works
// application/vnd.ms-tnef
// application/vnd.svd
// application/vnd.music-niff
// application/vnd.ms-artgalry
// application/vnd.truedoc
// application/vnd.koan
// application/vnd.street-stream
  "fdf"     : "application/vnd.fdf",
// application/set-payment-initiation
// application/set-payment
// application/set-registration-initiation
// application/set-registration
// application/vnd.seemail
// application/vnd.businessobjects
// application/vnd.meridian-slingshot
// application/vnd.xara
// application/sgml-open-catalog
// application/vnd.rapid
// application/vnd.japannet-registration-wakeup
// application/vnd.japannet-verification-wakeup
// application/vnd.japannet-payment-wakeup
// application/vnd.japannet-directory-service
// application/vnd.intertrust.digibox
// application/vnd.intertrust.nncp
// application/prs.alvestrand.titrax-sheet
// application/vnd.noblenet-web
// application/vnd.noblenet-sealer
// application/vnd.noblenet-directory
// application/prs.nprend
// application/vnd.webturbo
// application/hyperstudio
// application/vnd.shana.informed.formtemplate
// application/vnd.shana.informed.formdata
// application/vnd.shana.informed.package
// application/vnd.shana.informed.interchange
// application/vnd.$commerce_battelle
// application/vnd.osa.netdeploy
// application/vnd.ibm.MiniPay
// application/vnd.japannet-jpnstore-wakeup
// application/vnd.japannet-setstore-wakeup
// application/vnd.japannet-verification
// application/vnd.japannet-registration
// application/vnd.hp-HPGL
// application/vnd.hp-PCL
// application/vnd.hp-PCLXL
// application/vnd.musician
// application/vnd.FloGraphIt
// application/vnd.intercon.formnet
// application/vemmi
// application/vnd.ms-asf
// application/vnd.ecdis-update
  "pdb"     : "application/vnd.powerbuilder6",
// application/vnd.powerbuilder6-s
  "lwp"     : "application/vnd.lotus-wordpro",
  "sam"     : "application/vnd.lotus-wordpro",
  "apr"     : "application/vnd.lotus-approach",
  "vew"     : "application/vnd.lotus-approach",
  "123"     : "application/vnd.lotus-1-2-3",
  "wk1"     : "application/vnd.lotus-1-2-3",
  "wk3"     : "application/vnd.lotus-1-2-3",
  "wk4"     : "application/vnd.lotus-1-2-3",
  "or2"     : "application/vnd.lotus-organizer",
  "or3"     : "application/vnd.lotus-organizer",
  "org"     : "application/vnd.lotus-organizer",
  "scm"     : "application/vnd.lotus-screencam",
  "pre"     : "application/vnd.lotus-freelance",
  "prz"     : "application/vnd.lotus-freelance",
// application/vnd.fujitsu.oasys
// application/vnd.fujitsu.oasys2
// application/vnd.swiftview-ics
// application/vnd.dna
// application/prs.cww
// application/vnd.wt.stf
// application/vnd.dxr
// application/vnd.mitsubishi.misty-guard.trustweb
// application/vnd.ibm.modcap
// application/vnd.acucobol
// application/vnd.fujitsu.oasys3
// application/marc
// application/vnd.fujitsu.oasysprs
// application/vnd.fujitsu.oasysgp
// application/vnd.visio
// application/vnd.netfpx
// application/vnd.audiograph
// application/vnd.epson.salt
// application/vnd.3M.Post-it-Notes
// application/vnd.novadigm.EDX
// application/vnd.novadigm.EXT
// application/vnd.novadigm.EDM
// application/vnd.claymore
// application/vnd.comsocaller
  "p7c"     : "application/x-pkcs7-mime",
  "p7m"     : "application/x-pkcs7-mime",
  "p7s"     : "application/x-pkcs7-signature",
// application/pkcs10
// application/vnd.yellowriver-custom-menu
// application/vnd.ecowin.chart
// application/vnd.ecowin.series
// application/vnd.ecowin.filerequest
// application/vnd.ecowin.fileupdate
// application/vnd.ecowin.seriesrequest
// application/vnd.ecowin.seriesupdate
// application/EDIFACT
// application/EDI-X12
// application/EDI-Consent
// application/vnd.wrq-hp3000-labelled
// application/vnd.minisoft-hp3000-save
// application/vnd.ffsns
// application/vnd.hp-hps
// application/vnd.fujixerox.docuworks
// "xml"     : "application/xml",
// application/vnd.anser-web-funds-transfer-initiation
// application/vnd.anser-web-certificate-issue-initiation
// application/vnd.is-xpr
// application/vnd.intu.qbo
// application/vnd.publishare-delta-tree
// application/vnd.cybank
// application/batch-SMTP
// application/vnd.uplanet.alert
// application/vnd.uplanet.cacheop
// application/vnd.uplanet.list
// application/vnd.uplanet.listcmd
// application/vnd.uplanet.channel
// application/vnd.uplanet.bearer-choice
// application/vnd.uplanet.signal
// application/vnd.uplanet.alert-wbxml
// application/vnd.uplanet.cacheop-wbmxl
// application/vnd.uplanet.list-wbxml
// application/vnd.uplanet.listcmd-wbxml
// application/vnd.uplanet.channel-wbxml
// application/vnd.uplanet.bearer-choice-wbxml
// application/vnd.epson.quickanime
// application/vnd.commonspace
// application/vnd.fut-misnet
// application/vnd.xfdl
// application/vnd.intu.qfx
// application/vnd.epson.ssf
// application/vnd.epson.msf
// application/vnd.powerbuilder7
// application/vnd.powerbuilder7-s
// application/vnd.lotus-notes
// application/pkixcmp
  "wmlc"    : "application/vnd.wap.wmlc",
  "wmlsc"   : "application/vnd.wap.wmlscriptc",
// application/vnd.motorola.flexsuite
  "wbxml"   : "application/vnd.wap.wbxml",
// application/vnd.motorola.flexsuite.wem
// application/vnd.motorola.flexsuite.kmr
// application/vnd.motorola.flexsuite.adsi
// application/vnd.motorola.flexsuite.fis
// application/vnd.motorola.flexsuite.gotap
// application/vnd.motorola.flexsuite.ttc
// application/vnd.ufdl
// application/vnd.accpac.simply.imp
// application/vnd.accpac.simply.aso
// application/vnd.vcx
// application/ipp
// application/ocsp-request
// application/ocsp-response
// application/vnd.previewsystems.box
// application/vnd.mediastation.cdkey
// application/vnd.pg.format
// application/vnd.pg.osasli
// application/vnd.hp-hpid
// application/pkix-cert
// application/pkix-crl
// application/vnd.Mobius.TXF
// application/vnd.Mobius.PLC
// application/vnd.Mobius.DIS
// application/vnd.Mobius.DAF
// application/vnd.Mobius.MSL
// application/vnd.cups-raster
// application/vnd.cups-postscript
// application/vnd.cups-raw
// application/index
// application/index.cmd
// application/index.response
// application/index.obj
// application/index.vnd
// application/vnd.triscape.mxs
// application/vnd.powerbuilder75
// application/vnd.powerbuilder75-s
// application/vnd.dpgraph
// application/http
  "sdp"     : "application/sdp",
// vnd.eudora.data
// vnd.fujixerox.docuworks.binder
// vnd.vectorworks
// vnd.grafeq
// vnd.bmi
// vnd.ericsson.quickcall
// vnd.hzn-3d-crossword
// vnd.wap.slc
// vnd.wap.sic
// vnd.groove-injector
// vnd.fujixerox.ddd
// vnd.groove-account
// vnd.groove-identity-message
// vnd.groove-tool-message
// vnd.groove-tool-template
// vnd.groove-vcard
// vnd.ctc-posml
// vnd.canon-lips
// vnd.canon-cpdl
// vnd.trueapp
// vnd.s3sms
// iotp
// vnd.mcd
// vnd.httphone
// vnd.informix-visionary
// vnd.msign
// vnd.ms-lrm
// vnd.contact.cmsg
// vnd.epson.esf

// Other

  "aab"     : "application/x-authorware-bin",
  "aam"     : "application/x-authorware-map",
  "aas"     : "application/x-authorware-seg",
  "adr"     : "application/x-msaddr",
  "alt"     : "application/x-up-alert",
  "aos"     : "application/x-nokia-9000-communicator-add-on-software",
  "asd"     : "application/astound",
  "asn"     : "application/astound",
  "asp"     : "application/x-asap",
  "asz"     : "application/astound",
  "axs"     : "application/olescript",
  "bcpio"   : "application/x-bcpio",
  "bld"     : "application/bld",
  "bld2"    : "application/bld2",
  "cairn"   : "application/x-cairn",
  "ca_cert" : "application/x-x509-ca-cert",
  "cert"    : "application/x-x509-ca-cert",
  "cer"     : "application/x-x509-ca-cert",
  "cdf"     : "application/x-netcdf",
// "cdf"      : "application/x-cdf", // This is what IE thinks
  "cdr"     : "application/x-coreldraw",
  "cdrw"    : "application/x-coreldraw",
  "chat"    : "application/x-chat",
  "che"     : "application/x-up-cacheop",
  "ckl"     : "application/x-fortezza-ckl",
  "class"   : "application/x-java-vm",
  "cnc"     : "application/x-cnc",
  "coda"    : "application/x-coda",
  "con"     : "application/x-connector",
  "cpio"    : "application/x-cpio",
  "crl"     : "application/x-pkcs7-crl",
  "crt"     : "application/x-x509-ca-cert",
  "csh"     : "application/x-csh",
  "csm"     : "application/x-cu-seeme",
  "cu"      : "application/x-cu-seeme",
  "dcr"     : "application/x-director",
  "der"     : "application/x-x509-ca-cert",
  "dir"     : "application/x-director",
// "dms"      : "application/x-diskmasher",
  "dnt"     : "application/donut",
  "donut"   : "application/donut",
  "dst"     : "application/tajima",
  "dvi"     : "application/x-dvi",
  "dxr"     : "application/x-director",
  "enc"     : "application/pre-encrypted",
  "erf"     : "application/x-hsp-erf",
  "evy"     : "application/x-envoy",
// "fif"      : "application/fractals",
  "fml"     : "application/fml",
  "frl"     : "application/freeloader",
  "fs"      : "application/X-FSRecipe",
  "gtar"    : "application/x-gtar",
  "hdf"     : "application/x-hdf",
  "hpf"     : "application/x-icq",
  "ica"     : "application/x-ica",
  "ins"     : "application/x-internet-signup",
// "ins"      : "application/x-NET-install",
  "ips"     : "application/x-ipscript",
  "ipx"     : "application/x-ipix",
  "isp"     : "application/x-internet-signup",
  "iw"      : "application/iconauthor",
  "iwm"     : "application/iconauthor",
  "jar"     : "application/x-java-archive",
  "js"      : "application/x-javascript",
  "jsc"     : "application/x-javascript-config",
  "latex"   : "application/x-latex",
  "lha"     : "application/lha",
  "ltx"     : "application/x-latex",
  "lzh"     : "application/lha",
  "mad"     : "application/vnd.ms-access",
  "maf"     : "application/vnd.ms-access",
  "man"     : "application/x-troff-man",
  "mam"     : "application/vnd.ms-access",
  "map"     : "application/x-imagemap",
  "maq"     : "application/vnd.ms-access",
  "mar"     : "application/vnd.ms-access",
  "mat"     : "application/vnd.ms-access",
  "mbd"     : "application/mbdlet",
  "mda"     : "application/vnd.ms-access",
  "mdb"     : "application/vnd.ms-access",
  "mde"     : "application/vnd.ms-access",
  "mdn"     : "application/vnd.ms-access",
  "mdt"     : "application/vnd.ms-access",
  "mdw"     : "application/vnd.ms-access",
  "mdz"     : "application/vnd.ms-access",
  "me"      : "application/x-troff-me",
  "mfp"     : "application/mirage",
  "mif"     : "application/x-mif",
  "mm"      : "application/x-meme",
  "mocha"   : "application/x-javascript",
  "mpire"   : "application/x-mpire",
  "mpl"     : "application/x-mpire",
  "ms"      : "application/x-troff-ms",
  "msw"     : "application/binary",
  "mw"      : "application/math",
  "n2p"     : "application/n2p",
  "nc"      : "application/x-netcdf",
  "npx"     : "application/x-netfpx",
  "nsc"     : "application/x-nschat",
  "nspc"    : "application/x-ns-proxy-autoconfig",
  "ofml"    : "application/ofml",
  "pac"     : "application/x-ns-proxy-autoconfig",
  "page"    : "application/x-coda",
  "pfr"     : "application/font-tdpfr",
  "pics"    : "application/pics-service",
  "pl"      : "application/x-perl",
  "po"      : "application/x-form",
  "pqf"     : "application/x-cprplayer",
  "pqi"     : "application/cprplayer",
  "psd"     : "application/x-photoshop",
  "psr"     : "application/datawindow",
  "ptlk"    : "application/listenup",
  "qrt"     : "application/quest",
  "quake"   : "application/x-qplug-plugin",
  "rm"      : "application/vnd.rn-realmedia",
  "rnx"     : "application/vnd.rn-realplayer",
  "roff"    : "application/x-troff",
// "rp"       : "application/x-pn-realmedia",
  "rrf"     : "application/x-InstallFromTheWeb",
  "rtc"     : "application/rtc",
  "sca"     : "application/x-supercard",
  "sc2"     : "application/vnd.ms-schedule",
  "scd"     : "application/vnd.ms-schedule",
  "sch"     : "application/vnd.ms-schedule",
// "scm"      : "application/x-icq",
  "sdf"     : "application/e-score",
  "sgf"     : "application/x-go-sgf",
  "sh"      : "application/x-sh",
  "shar"    : "application/x-shar",
  "shw"     : "application/presentations",
  "sit"     : "application/x-stuffit",
  "skd"     : "application/x-koan",
  "skm"     : "application/x-koan",
  "skp"     : "application/x-koan",
  "skt"     : "application/x-koan",
  "smi"     : "application/smil",
  "smil"    : "application/smil",
  "sml"     : "application/smil",
  "smp"     : "application/studiom",
  "spl"     : "application/futuresplash",
  "spr"     : "application/x-sprite",
  "sprite"  : "application/x-sprite",
  "src"     : "application/x-wais-source",
  "ssm"     : "application/streamingmedia",
  "stk"     : "application/hstu",
  "sv4cpic" : "application/x-sv4cpio",
  "sv4crc"  : "application/x-sv4crc",
  "swa"     : "application/x-director",
  "swf"     : "application/x-shockwave-flash",
  "t"       : "application/x-troff",
  "tar"     : "application/unix-tar",
  "talk"    : "application/talker",
  "tbk"     : "application/toolbook",
  "tcl"     : "application/x-tcl",
  "tex"     : "application/x-tex",
  "texi"    : "application/x-texinfo",
  "texinfo" : "application/x-texinfo",
  "tmv"     : "application/x-Parable-Thing",
  "tr"      : "application/x-troff",
  "tsp"     : "application/dsptype",
  "txi"     : "application/x-texinfo",
  "uin"     : "application/x-icq",
  "user"    : "application/x-x509-user-cert",
  "usr"     : "application/x-x509-user-cert",
  "ustar"   : "application/x-ustar",
  "vbd"     : "application/activexdocument",
  "vcd"     : "application/x-cdlink",
  "vmd"     : "application/vocaltec-media-desc",
  "vmf"     : "application/vocaltec-media-file",
  "wid"     : "application/x-DemoShield",
  "wis"     : "application/x-InstallShield",
  "wlt"     : "application/x-mswallet",
  "wpc"     : "application/wpc",
  "wri"     : "application/x-write",
  "xcf"     : "application/x-gimp",
  "zpa"     : "application/pcphoto",

// Dreamcast specific
  "lcd"     : "application/x-dreamcast-lcdimg",
  "vmi"     : "application/x-dreamcast-vms-info",
  "vms"     : "application/x-dreamcast-vms",


// -------------------- Model ---------------------

// IANA registered

// model/iges
// model/mesh
  "dwf"     : "model/vnd.dwf",
  "3dml"    : "model/vnd.flatland.3dml",
  "3dm"     : "model/vnd.flatland.3dml",
  "gdl"     : "model/vnd.gdl",
// "gsm"      : "model/vnd.gdl",
  "win"     : "model/vnd.gdl",
  "dor"     : "model/vnd.gdl",
  "lmp"     : "model/vnd.gdl",
  "rsm"     : "model/vnd.gdl",
  "msm"     : "model/vnd.gdl",
  "ism"     : "model/vnd.gdl",
// model/vnd.gs-gdl
  "gtw"     : "model/vnd.gtw",
  "mts"     : "model/vnd.mts",
  "x_b"     : "model/vnd.parasolid.transmit.binary",
  "xmt_bin" : "model/vnd.parasolid.transmit.binary",
  "x_t"     : "model/vnd.parasolid.transmit.text",
  "xmt_txt" : "model/vnd.parasolid.transmit.text",
  "vtu"     : "model/vnd.vtu",
  "vrml"    : "model/vrml",
  "wrl"     : "model/vrml",
  "wrml"    : "model/vrml",


// -------------------- Other ---------------------

// LivePicture PhotoVista RealSpace
  "ivr"     : "i-world/i-vrml",

  "3dmf"    : "x-world/x-3dmf",
  "qd3"     : "x-world/x-3dmf",
  "qd3d"    : "x-world/x-3dmf",
  "svr"     : "x-world/x-svr",
  "vrt"     : "x-world/x-vrt",
  "wrz"     : "x-world/x-vrml",

// "talk"     : "plugin/talker",
  "wan"     : "plugin/wanimate",
  "waf"     : "plugin/wanimate",

  "vts"     : "workbook/formulaone",
  "vtts"    : "workbook/formulaone",

  "u98"     : "urdu/urdu98",

  "ice"     : "x-conference/x-cooltalk",


//
// ---------- PART II -------------
//

  "ez"          : "application/andrew-inset",
  "aw"          : "application/applixware",
  "atom"        : "application/atom+xml",
  "atomcat"     : "application/atomcat+xml",
  "atomsvc"     : "application/atomsvc+xml",
  "ccxml"       : "application/ccxml+xml",
  "cdmia"       : "application/cdmi-capability",
  "cdmic"       : "application/cdmi-container",
  "cdmid"       : "application/cdmi-domain",
  "cdmio"       : "application/cdmi-object",
  "cdmiq"       : "application/cdmi-queue",
  "davmount"    : "application/davmount+xml",
  "dbk"         : "application/docbook+xml",
  "dssc"        : "application/dssc+der",
  "xdssc"       : "application/dssc+xml",
  "ecma"        : "application/ecmascript",
  "emma"        : "application/emma+xml",
  "epub"        : "application/epub+zip",
  "exi"         : "application/exi",
  "gml"         : "application/gml+xml",
  "gpx"         : "application/gpx+xml",
  "gxf"         : "application/gxf",
  "ink"         : "application/inkml+xml",
  "inkml"       : "application/inkml+xml",
  "ipfix"       : "application/ipfix",
  "ser"         : "application/java-serialized-object",
  "json"        : "application/json",
  "jsonml"      : "application/jsonml+json",
  "lostxml"     : "application/lost+xml",
  "cpt"         : "application/mac-compactpro",
  "mads"        : "application/mads+xml",
  "mrc"         : "application/marc",
  "mrcx"        : "application/marcxml+xml",
  "ma"          : "application/mathematica",
  "nb"          : "application/mathematica",
  "mb"          : "application/mathematica",
  "mathml"      : "application/mathml+xml",
  "mbox"        : "application/mbox",
  "mscml"       : "application/mediaservercontrol+xml",
  "metalink"    : "application/metalink+xml",
  "meta4"       : "application/metalink4+xml",
  "mets"        : "application/mets+xml",
  "mods"        : "application/mods+xml",
  "m21"         : "application/mp21",
  "mp21"        : "application/mp21",
  "mp4s"        : "application/mp4",
  "mxf"         : "application/mxf",
  "lrf"         : "application/octet-stream",
  "dist"        : "application/octet-stream",
  "distz"       : "application/octet-stream",
  "pkg"         : "application/octet-stream",
  "bpk"         : "application/octet-stream",
  "dump"        : "application/octet-stream",
  "elc"         : "application/octet-stream",
  "deploy"      : "application/octet-stream",
  "opf"         : "application/oebps-package+xml",
  "ogx"         : "application/ogg",
  "omdoc"       : "application/omdoc+xml",
  "onetoc"      : "application/onenote",
  "onetoc2"     : "application/onenote",
  "onetmp"      : "application/onenote",
  "onepkg"      : "application/onenote",
  "oxps"        : "application/oxps",
  "xer"         : "application/patch-ops-error+xml",
  "pgp"         : "application/pgp-encrypted",
  "asc"         : "application/pgp-signature",
  "sig"         : "application/pgp-signature",
  "prf"         : "application/pics-rules",
  "p10"         : "application/pkcs10",
  "p8"          : "application/pkcs8",
  "ac"          : "application/pkix-attr-cert",
  "pkipath"     : "application/pkix-pkipath",
  "pki"         : "application/pkixcmp",
  "cww"         : "application/prs.cww",
  "pskcxml"     : "application/pskc+xml",
  "rdf"         : "application/rdf+xml",
  "rif"         : "application/reginfo+xml",
  "rnc"         : "application/relax-ng-compact-syntax",
  "rl"          : "application/resource-lists+xml",
  "rld"         : "application/resource-lists-diff+xml",
  "gbr"         : "application/rpki-ghostbusters",
  "mft"         : "application/rpki-manifest",
  "roa"         : "application/rpki-roa",
  "rsd"         : "application/rsd+xml",
  "rss"         : "application/rss+xml",
  "sbml"        : "application/sbml+xml",
  "scq"         : "application/scvp-cv-request",
  "scs"         : "application/scvp-cv-response",
  "spq"         : "application/scvp-vp-request",
  "spp"         : "application/scvp-vp-response",
  "setpay"      : "application/set-payment-initiation",
  "setreg"      : "application/set-registration-initiation",
  "shf"         : "application/shf+xml",
  "rq"          : "application/sparql-query",
  "srx"         : "application/sparql-results+xml",
  "gram"        : "application/srgs",
  "grxml"       : "application/srgs+xml",
  "sru"         : "application/sru+xml",
  "ssdl"        : "application/ssdl+xml",
  "ssml"        : "application/ssml+xml",
  "tei"         : "application/tei+xml",
  "teicorpus"   : "application/tei+xml",
  "tfi"         : "application/thraud+xml",
  "tsd"         : "application/timestamped-data",
  "plb"         : "application/vnd.3gpp.pic-bw-large",
  "psb"         : "application/vnd.3gpp.pic-bw-small",
  "pvb"         : "application/vnd.3gpp.pic-bw-var",
  "tcap"        : "application/vnd.3gpp2.tcap",
  "pwn"         : "application/vnd.3m.post-it-notes",
  "aso"         : "application/vnd.accpac.simply.aso",
  "imp"         : "application/vnd.accpac.simply.imp",
  "acu"         : "application/vnd.acucobol",
  "atc"         : "application/vnd.acucorp",
  "acutc"       : "application/vnd.acucorp",
  "air"         : "application/vnd.adobe.air-application-installer-package+zip",
  "fcdt"        : "application/vnd.adobe.formscentral.fcdt",
  "fxp"         : "application/vnd.adobe.fxp",
  "fxpl"        : "application/vnd.adobe.fxp",
  "xdp"         : "application/vnd.adobe.xdp+xml",
  "xfdf"        : "application/vnd.adobe.xfdf",
  "ahead"       : "application/vnd.ahead.space",
  "azf"         : "application/vnd.airzip.filesecure.azf",
  "azs"         : "application/vnd.airzip.filesecure.azs",
  "azw"         : "application/vnd.amazon.ebook",
  "acc"         : "application/vnd.americandynamics.acc",
  "ami"         : "application/vnd.amiga.ami",
  "apk"         : "application/vnd.android.package-archive",
  "cii"         : "application/vnd.anser-web-certificate-issue-initiation",
  "fti"         : "application/vnd.anser-web-funds-transfer-initiation",
  "atx"         : "application/vnd.antix.game-component",
  "mpkg"        : "application/vnd.apple.installer+xml",
  "m3u8"        : "application/vnd.apple.mpegurl",
  "swi"         : "application/vnd.aristanetworks.swi",
  "iota"        : "application/vnd.astraea-software.iota",
  "aep"         : "application/vnd.audiograph",
  "mpm"         : "application/vnd.blueice.multipass",
  "bmi"         : "application/vnd.bmi",
  "rep"         : "application/vnd.businessobjects",
  "cdxml"       : "application/vnd.chemdraw+xml",
  "mmd"         : "application/vnd.chipnuts.karaoke-mmd",
  "cdy"         : "application/vnd.cinderella",
  "cla"         : "application/vnd.claymore",
  "rp9"         : "application/vnd.cloanto.rp9",
  "c4g"         : "application/vnd.clonk.c4group",
  "c4d"         : "application/vnd.clonk.c4group",
  "c4f"         : "application/vnd.clonk.c4group",
  "c4p"         : "application/vnd.clonk.c4group",
  "c4u"         : "application/vnd.clonk.c4group",
  "c11amc"      : "application/vnd.cluetrust.cartomobile-config",
  "c11amz"      : "application/vnd.cluetrust.cartomobile-config-pkg",
  "csp"         : "application/vnd.commonspace",
  "cdbcmsg"     : "application/vnd.contact.cmsg",
  "cmc"         : "application/vnd.cosmocaller",
  "clkx"        : "application/vnd.crick.clicker",
  "clkk"        : "application/vnd.crick.clicker.keyboard",
  "clkp"        : "application/vnd.crick.clicker.palette",
  "clkt"        : "application/vnd.crick.clicker.template",
  "clkw"        : "application/vnd.crick.clicker.wordbank",
  "wbs"         : "application/vnd.criticaltools.wbs+xml",
  "pml"         : "application/vnd.ctc-posml",
  "ppd"         : "application/vnd.cups-ppd",
  "car"         : "application/vnd.curl.car",
  "pcurl"       : "application/vnd.curl.pcurl",
  "dart"        : "application/vnd.dart",
  "rdz"         : "application/vnd.data-vision.rdz",
  "uvf"         : "application/vnd.dece.data",
  "uvvf"        : "application/vnd.dece.data",
  "uvd"         : "application/vnd.dece.data",
  "uvvd"        : "application/vnd.dece.data",
  "uvt"         : "application/vnd.dece.ttml+xml",
  "uvvt"        : "application/vnd.dece.ttml+xml",
  "uvx"         : "application/vnd.dece.unspecified",
  "uvvx"        : "application/vnd.dece.unspecified",
  "uvz"         : "application/vnd.dece.zip",
  "uvvz"        : "application/vnd.dece.zip",
  "fe_launch"   : "application/vnd.denovo.fcselayout-link",
  "dna"         : "application/vnd.dna",
  "mlp"         : "application/vnd.dolby.mlp",
  "dpg"         : "application/vnd.dpgraph",
  "dfac"        : "application/vnd.dreamfactory",
  "kpxx"        : "application/vnd.ds-keypoint",
  "ait"         : "application/vnd.dvb.ait",
  "svc"         : "application/vnd.dvb.service",
  "geo"         : "application/vnd.dynageo",
  "mag"         : "application/vnd.ecowin.chart",
  "nml"         : "application/vnd.enliven",
  "esf"         : "application/vnd.epson.esf",
  "msf"         : "application/vnd.epson.msf",
  "qam"         : "application/vnd.epson.quickanime",
  "slt"         : "application/vnd.epson.salt",
  "ssf"         : "application/vnd.epson.ssf",
  "es3"         : "application/vnd.eszigno3+xml",
  "et3"         : "application/vnd.eszigno3+xml",
  "ez2"         : "application/vnd.ezpix-album",
  "ez3"         : "application/vnd.ezpix-package",
  "mseed"       : "application/vnd.fdsn.mseed",
  "seed"        : "application/vnd.fdsn.seed",
  "dataless"    : "application/vnd.fdsn.seed",
  "gph"         : "application/vnd.flographit",
  "ftc"         : "application/vnd.fluxtime.clip",
  "fm"          : "application/vnd.framemaker",
  "frame"       : "application/vnd.framemaker",
  "maker"       : "application/vnd.framemaker",
  "book"        : "application/vnd.framemaker",
  "fnc"         : "application/vnd.frogans.fnc",
  "ltf"         : "application/vnd.frogans.ltf",
  "fsc"         : "application/vnd.fsc.weblaunch",
  "oas"         : "application/vnd.fujitsu.oasys",
  "oa2"         : "application/vnd.fujitsu.oasys2",
  "oa3"         : "application/vnd.fujitsu.oasys3",
  "fg5"         : "application/vnd.fujitsu.oasysgp",
  "bh2"         : "application/vnd.fujitsu.oasysprs",
  "ddd"         : "application/vnd.fujixerox.ddd",
  "xdw"         : "application/vnd.fujixerox.docuworks",
  "xbd"         : "application/vnd.fujixerox.docuworks.binder",
  "fzs"         : "application/vnd.fuzzysheet",
  "txd"         : "application/vnd.genomatix.tuxedo",
  "ggb"         : "application/vnd.geogebra.file",
  "ggt"         : "application/vnd.geogebra.tool",
  "gex"         : "application/vnd.geometry-explorer",
  "gre"         : "application/vnd.geometry-explorer",
  "gxt"         : "application/vnd.geonext",
  "g2w"         : "application/vnd.geoplan",
  "g3w"         : "application/vnd.geospace",
  "gmx"         : "application/vnd.gmx",
  "kml"         : "application/vnd.google-earth.kml+xml",
  "kmz"         : "application/vnd.google-earth.kmz",
  "gqf"         : "application/vnd.grafeq",
  "gqs"         : "application/vnd.grafeq",
  "gac"         : "application/vnd.groove-account",
  "ghf"         : "application/vnd.groove-help",
  "gim"         : "application/vnd.groove-identity-message",
  "grv"         : "application/vnd.groove-injector",
  "gtm"         : "application/vnd.groove-tool-message",
  "tpl"         : "application/vnd.groove-tool-template",
  "vcg"         : "application/vnd.groove-vcard",
  "hal"         : "application/vnd.hal+xml",
  "zmm"         : "application/vnd.handheld-entertainment+xml",
  "hbci"        : "application/vnd.hbci",
  "les"         : "application/vnd.hhe.lesson-player",
  "hpgl"        : "application/vnd.hp-hpgl",
  "hpid"        : "application/vnd.hp-hpid",
  "hps"         : "application/vnd.hp-hps",
  "jlt"         : "application/vnd.hp-jlyt",
  "pcl"         : "application/vnd.hp-pcl",
  "pclxl"       : "application/vnd.hp-pclxl",
  "sfd-hdstx"   : "application/vnd.hydrostatix.sof-data",
  "mpy"         : "application/vnd.ibm.minipay",
  "afp"         : "application/vnd.ibm.modcap",
  "listafp"     : "application/vnd.ibm.modcap",
  "list3820"    : "application/vnd.ibm.modcap",
  "irm"         : "application/vnd.ibm.rights-management",
  "sc"          : "application/vnd.ibm.secure-container",
  "icc"         : "application/vnd.iccprofile",
  "icm"         : "application/vnd.iccprofile",
  "igl"         : "application/vnd.igloader",
  "ivp"         : "application/vnd.immervision-ivp",
  "ivu"         : "application/vnd.immervision-ivu",
  "igm"         : "application/vnd.insors.igm",
  "xpw"         : "application/vnd.intercon.formnet",
  "xpx"         : "application/vnd.intercon.formnet",
  "i2g"         : "application/vnd.intergeo",
  "qbo"         : "application/vnd.intu.qbo",
  "qfx"         : "application/vnd.intu.qfx",
  "rcprofile"   : "application/vnd.ipunplugged.rcprofile",
  "irp"         : "application/vnd.irepository.package+xml",
  "xpr"         : "application/vnd.is-xpr",
  "fcs"         : "application/vnd.isac.fcs",
  "jam"         : "application/vnd.jam",
  "rms"         : "application/vnd.jcp.javame.midlet-rms",
  "jisp"        : "application/vnd.jisp",
  "joda"        : "application/vnd.joost.joda-archive",
  "ktz"         : "application/vnd.kahootz",
  "ktr"         : "application/vnd.kahootz",
  "karbon"      : "application/vnd.kde.karbon",
  "chrt"        : "application/vnd.kde.kchart",
  "kfo"         : "application/vnd.kde.kformula",
  "flw"         : "application/vnd.kde.kivio",
  "kon"         : "application/vnd.kde.kontour",
  "kpr"         : "application/vnd.kde.kpresenter",
  "kpt"         : "application/vnd.kde.kpresenter",
  "ksp"         : "application/vnd.kde.kspread",
  "kwd"         : "application/vnd.kde.kword",
  "kwt"         : "application/vnd.kde.kword",
  "htke"        : "application/vnd.kenameaapp",
  "kia"         : "application/vnd.kidspiration",
  "kne"         : "application/vnd.kinar",
  "knp"         : "application/vnd.kinar",
  "sse"         : "application/vnd.kodak-descriptor",
  "lasxml"      : "application/vnd.las.las+xml",
  "lbd"         : "application/vnd.llamagraphics.life-balance.desktop",
  "lbe"         : "application/vnd.llamagraphics.life-balance.exchange+xml",
  "nsf"         : "application/vnd.lotus-notes",
  "portpkg"     : "application/vnd.macports.portpkg",
  "mcd"         : "application/vnd.mcd",
  "mc1"         : "application/vnd.medcalcdata",
  "cdkey"       : "application/vnd.mediastation.cdkey",
  "mwf"         : "application/vnd.mfer",
  "mfm"         : "application/vnd.mfmp",
  "flo"         : "application/vnd.micrografx.flo",
  "igx"         : "application/vnd.micrografx.igx",
  "daf"         : "application/vnd.mobius.daf",
  "dis"         : "application/vnd.mobius.dis",
  "mbk"         : "application/vnd.mobius.mbk",
  "mqy"         : "application/vnd.mobius.mqy",
  "msl"         : "application/vnd.mobius.msl",
  "plc"         : "application/vnd.mobius.plc",
  "txf"         : "application/vnd.mobius.txf",
  "mpn"         : "application/vnd.mophun.application",
  "mpc"         : "application/vnd.mophun.certificate",
  "xul"         : "application/vnd.mozilla.xul+xml",
  "cil"         : "application/vnd.ms-artgalry",
  "cab"         : "application/vnd.ms-cab-compressed",
  "xlam"        : "application/vnd.ms-excel.addin.macroenabled.12",
  "xlsb"        : "application/vnd.ms-excel.sheet.binary.macroenabled.12",
  "xlsm"        : "application/vnd.ms-excel.sheet.macroenabled.12",
  "xltm"        : "application/vnd.ms-excel.template.macroenabled.12",
  "eot"         : "application/vnd.ms-fontobject",
  "chm"         : "application/vnd.ms-htmlhelp",
  "ims"         : "application/vnd.ms-ims",
  "lrm"         : "application/vnd.ms-lrm",
  "thmx"        : "application/vnd.ms-officetheme",
  "cat"         : "application/vnd.ms-pki.seccat",
  "stl"         : "application/vnd.ms-pki.stl",
  "ppam"        : "application/vnd.ms-powerpoint.addin.macroenabled.12",
  "pptm"        : "application/vnd.ms-powerpoint.presentation.macroenabled.12",
  "sldm"        : "application/vnd.ms-powerpoint.slide.macroenabled.12",
  "ppsm"        : "application/vnd.ms-powerpoint.slideshow.macroenabled.12",
  "potm"        : "application/vnd.ms-powerpoint.template.macroenabled.12",
  "mpp"         : "application/vnd.ms-project",
  "mpt"         : "application/vnd.ms-project",
  "docm"        : "application/vnd.ms-word.document.macroenabled.12",
  "dotm"        : "application/vnd.ms-word.template.macroenabled.12",
  "wps"         : "application/vnd.ms-works",
  "wks"         : "application/vnd.ms-works",
  "wcm"         : "application/vnd.ms-works",
  "wdb"         : "application/vnd.ms-works",
  "wpl"         : "application/vnd.ms-wpl",
  "xps"         : "application/vnd.ms-xpsdocument",
  "mseq"        : "application/vnd.mseq",
  "mus"         : "application/vnd.musician",
  "msty"        : "application/vnd.muvee.style",
  "taglet"      : "application/vnd.mynfc",
  "nlu"         : "application/vnd.neurolanguage.nlu",
  "ntf"         : "application/vnd.nitf",
  "nitf"        : "application/vnd.nitf",
  "nnd"         : "application/vnd.noblenet-directory",
  "nns"         : "application/vnd.noblenet-sealer",
  "nnw"         : "application/vnd.noblenet-web",
  "ngdat"       : "application/vnd.nokia.n-gage.data",
  "n-gage"      : "application/vnd.nokia.n-gage.symbian.install",
  "rpst"        : "application/vnd.nokia.radio-preset",
  "rpss"        : "application/vnd.nokia.radio-presets",
  "edm"         : "application/vnd.novadigm.edm",
  "edx"         : "application/vnd.novadigm.edx",
  "ext"         : "application/vnd.novadigm.ext",
  "odc"         : "application/vnd.oasis.opendocument.chart",
  "otc"         : "application/vnd.oasis.opendocument.chart-template",
  "odb"         : "application/vnd.oasis.opendocument.database",
  "odf"         : "application/vnd.oasis.opendocument.formula",
  "odft"        : "application/vnd.oasis.opendocument.formula-template",
  "odg"         : "application/vnd.oasis.opendocument.graphics",
  "otg"         : "application/vnd.oasis.opendocument.graphics-template",
  "odi"         : "application/vnd.oasis.opendocument.image",
  "oti"         : "application/vnd.oasis.opendocument.image-template",
  "odp"         : "application/vnd.oasis.opendocument.presentation",
  "otp"         : "application/vnd.oasis.opendocument.presentation-template",
  "ods"         : "application/vnd.oasis.opendocument.spreadsheet",
  "ots"         : "application/vnd.oasis.opendocument.spreadsheet-template",
  "odt"         : "application/vnd.oasis.opendocument.text",
  "odm"         : "application/vnd.oasis.opendocument.text-master",
  "ott"         : "application/vnd.oasis.opendocument.text-template",
  "oth"         : "application/vnd.oasis.opendocument.text-web",
  "xo"          : "application/vnd.olpc-sugar",
  "dd2"         : "application/vnd.oma.dd2+xml",
  "oxt"         : "application/vnd.openofficeorg.extension",
  "pptx"        : "application/vnd.openxmlformats-officedocument.presentationml.presentation",
  "sldx"        : "application/vnd.openxmlformats-officedocument.presentationml.slide",
  "ppsx"        : "application/vnd.openxmlformats-officedocument.presentationml.slideshow",
  "potx"        : "application/vnd.openxmlformats-officedocument.presentationml.template",
  "xlsx"        : "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
  "xltx"        : "application/vnd.openxmlformats-officedocument.spreadsheetml.template",
  "docx"        : "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
  "dotx"        : "application/vnd.openxmlformats-officedocument.wordprocessingml.template",
  "mgp"         : "application/vnd.osgeo.mapguide.package",
  "dp"          : "application/vnd.osgi.dp",
  "esa"         : "application/vnd.osgi.subsystem",
  "pqa"         : "application/vnd.palm",
  "oprc"        : "application/vnd.palm",
  "paw"         : "application/vnd.pawaafile",
  "str"         : "application/vnd.pg.format",
  "ei6"         : "application/vnd.pg.osasli",
  "efif"        : "application/vnd.picsel",
  "wg"          : "application/vnd.pmi.widget",
  "plf"         : "application/vnd.pocketlearn",
  "pbd"         : "application/vnd.powerbuilder6",
  "box"         : "application/vnd.previewsystems.box",
  "mgz"         : "application/vnd.proteus.magazine",
  "qps"         : "application/vnd.publishare-delta-tree",
  "ptid"        : "application/vnd.pvi.ptid1",
  "qxd"         : "application/vnd.quark.quarkxpress",
  "qxt"         : "application/vnd.quark.quarkxpress",
  "qwd"         : "application/vnd.quark.quarkxpress",
  "qwt"         : "application/vnd.quark.quarkxpress",
  "qxl"         : "application/vnd.quark.quarkxpress",
  "qxb"         : "application/vnd.quark.quarkxpress",
  "bed"         : "application/vnd.realvnc.bed",
  "mxl"         : "application/vnd.recordare.musicxml",
  "musicxml"    : "application/vnd.recordare.musicxml+xml",
  "cryptonote"  : "application/vnd.rig.cryptonote",
  "rmvb"        : "application/vnd.rn-realmedia-vbr",
  "link66"      : "application/vnd.route66.link66+xml",
  "st"          : "application/vnd.sailingtracker.track",
  "see"         : "application/vnd.seemail",
  "sema"        : "application/vnd.sema",
  "semd"        : "application/vnd.semd",
  "semf"        : "application/vnd.semf",
  "ifm"         : "application/vnd.shana.informed.formdata",
  "itp"         : "application/vnd.shana.informed.formtemplate",
  "iif"         : "application/vnd.shana.informed.interchange",
  "ipk"         : "application/vnd.shana.informed.package",
  "twd"         : "application/vnd.simtech-mindmapper",
  "twds"        : "application/vnd.simtech-mindmapper",
  "mmf"         : "application/vnd.smaf",
  "teacher"     : "application/vnd.smart.teacher",
  "sdkm"        : "application/vnd.solent.sdkm+xml",
  "sdkd"        : "application/vnd.solent.sdkm+xml",
  "dxp"         : "application/vnd.spotfire.dxp",
  "sfs"         : "application/vnd.spotfire.sfs",
  "sdc"         : "application/vnd.stardivision.calc",
  "sda"         : "application/vnd.stardivision.draw",
  "sdd"         : "application/vnd.stardivision.impress",
  "smf"         : "application/vnd.stardivision.math",
  "sdw"         : "application/vnd.stardivision.writer",
  "vor"         : "application/vnd.stardivision.writer",
  "sgl"         : "application/vnd.stardivision.writer-global",
  "smzip"       : "application/vnd.stepmania.package",
  "sm"          : "application/vnd.stepmania.stepchart",
  "sxc"         : "application/vnd.sun.xml.calc",
  "stc"         : "application/vnd.sun.xml.calc.template",
  "sxd"         : "application/vnd.sun.xml.draw",
  "std"         : "application/vnd.sun.xml.draw.template",
  "sxi"         : "application/vnd.sun.xml.impress",
  "sti"         : "application/vnd.sun.xml.impress.template",
  "sxm"         : "application/vnd.sun.xml.math",
  "sxw"         : "application/vnd.sun.xml.writer",
  "sxg"         : "application/vnd.sun.xml.writer.global",
  "stw"         : "application/vnd.sun.xml.writer.template",
  "sus"         : "application/vnd.sus-calendar",
  "susp"        : "application/vnd.sus-calendar",
  "svd"         : "application/vnd.svd",
  "sis"         : "application/vnd.symbian.install",
  "sisx"        : "application/vnd.symbian.install",
  "xsm"         : "application/vnd.syncml+xml",
  "bdm"         : "application/vnd.syncml.dm+wbxml",
  "xdm"         : "application/vnd.syncml.dm+xml",
  "tao"         : "application/vnd.tao.intent-module-archive",
  "pcap"        : "application/vnd.tcpdump.pcap",
  "cap"         : "application/vnd.tcpdump.pcap",
  "dmp"         : "application/vnd.tcpdump.pcap",
  "tmo"         : "application/vnd.tmobile-livetv",
  "tpt"         : "application/vnd.trid.tpt",
  "mxs"         : "application/vnd.triscape.mxs",
  "tra"         : "application/vnd.trueapp",
  "ufd"         : "application/vnd.ufdl",
  "ufdl"        : "application/vnd.ufdl",
  "utz"         : "application/vnd.uiq.theme",
  "umj"         : "application/vnd.umajin",
  "unityweb"    : "application/vnd.unity",
  "uoml"        : "application/vnd.uoml+xml",
  "vcx"         : "application/vnd.vcx",
  "vsd"         : "application/vnd.visio",
  "vst"         : "application/vnd.visio",
  "vss"         : "application/vnd.visio",
  "vsw"         : "application/vnd.visio",
  "vis"         : "application/vnd.visionary",
  "vsf"         : "application/vnd.vsf",
  "wtb"         : "application/vnd.webturbo",
  "nbp"         : "application/vnd.wolfram.player",
  "wqd"         : "application/vnd.wqd",
  "stf"         : "application/vnd.wt.stf",
  "xar"         : "application/vnd.xara",
  "xfdl"        : "application/vnd.xfdl",
  "hvd"         : "application/vnd.yamaha.hv-dic",
  "hvs"         : "application/vnd.yamaha.hv-script",
  "hvp"         : "application/vnd.yamaha.hv-voice",
  "osf"         : "application/vnd.yamaha.openscoreformat",
  "osfpvg"      : "application/vnd.yamaha.openscoreformat.osfpvg+xml",
  "saf"         : "application/vnd.yamaha.smaf-audio",
  "spf"         : "application/vnd.yamaha.smaf-phrase",
  "cmp"         : "application/vnd.yellowriver-custom-menu",
  "zir"         : "application/vnd.zul",
  "zirz"        : "application/vnd.zul",
  "zaz"         : "application/vnd.zzazz.deck+xml",
  "vxml"        : "application/voicexml+xml",
  "wgt"         : "application/widget",
  "hlp"         : "application/winhlp",
  "wsdl"        : "application/wsdl+xml",
  "wspolicy"    : "application/wspolicy+xml",
  "7z"          : "application/x-7z-compressed",
  "abw"         : "application/x-abiword",
  "ace"         : "application/x-ace-compressed",
  "dmg"         : "application/x-apple-diskimage",
  "x32"         : "application/x-authorware-bin",
  "u32"         : "application/x-authorware-bin",
  "torrent"     : "application/x-bittorrent",
  "blb"         : "application/x-blorb",
  "blorb"       : "application/x-blorb",
  "bz"          : "application/x-bzip",
  "bz2"         : "application/x-bzip2",
  "boz"         : "application/x-bzip2",
  "cbr"         : "application/x-cbr",
  "cba"         : "application/x-cbr",
  "cbt"         : "application/x-cbr",
  "cbz"         : "application/x-cbr",
  "cb7"         : "application/x-cbr",
  "cfs"         : "application/x-cfs-compressed",
  "pgn"         : "application/x-chess-pgn",
  "deb"         : "application/x-debian-package",
  "udeb"        : "application/x-debian-package",
  "dgc"         : "application/x-dgc-compressed",
  "cst"         : "application/x-director",
  "cct"         : "application/x-director",
  "cxt"         : "application/x-director",
  "w3d"         : "application/x-director",
  "fgd"         : "application/x-director",
  "wad"         : "application/x-doom",
  "ncx"         : "application/x-dtbncx+xml",
  "dtb"         : "application/x-dtbook+xml",
  "res"         : "application/x-dtbresource+xml",
  "eva"         : "application/x-eva",
  "bdf"         : "application/x-font-bdf",
  "gsf"         : "application/x-font-ghostscript",
  "psf"         : "application/x-font-linux-psf",
  "otf"         : "application/x-font-otf",
  "pcf"         : "application/x-font-pcf",
  "snf"         : "application/x-font-snf",
  "ttf"         : "application/x-font-ttf",
  "ttc"         : "application/x-font-ttf",
  "pfa"         : "application/x-font-type1",
  "pfb"         : "application/x-font-type1",
  "pfm"         : "application/x-font-type1",
  "afm"         : "application/x-font-type1",
  "woff"        : "application/x-font-woff",
  "arc"         : "application/x-freearc",
  "gca"         : "application/x-gca-compressed",
  "ulx"         : "application/x-glulx",
  "gnumeric"    : "application/x-gnumeric",
  "gramps"      : "application/x-gramps-xml",
  "install"     : "application/x-install-instructions",
  "iso"         : "application/x-iso9660-image",
  "jnlp"        : "application/x-java-jnlp-file",
  "mie"         : "application/x-mie",
  "prc"         : "application/x-mobipocket-ebook",
  "mobi"        : "application/x-mobipocket-ebook",
  "application" : "application/x-ms-application",
  "lnk"         : "application/x-ms-shortcut",
  "wmd"         : "application/x-ms-wmd",
  "wmz"         : "application/x-ms-wmz",
  "xbap"        : "application/x-ms-xbap",
  "obd"         : "application/x-msbinder",
  "crd"         : "application/x-mscardfile",
  "msi"         : "application/x-msdownload",
  "mvb"         : "application/x-msmediaview",
  "m13"         : "application/x-msmediaview",
  "m14"         : "application/x-msmediaview",
  "wmf"         : "application/x-msmetafile",
  "wmz"         : "application/x-msmetafile",
  "emz"         : "application/x-msmetafile",
  "mny"         : "application/x-msmoney",
  "pub"         : "application/x-mspublisher",
  "trm"         : "application/x-msterminal",
  "nzb"         : "application/x-nzb",
  "p12"         : "application/x-pkcs12",
  "pfx"         : "application/x-pkcs12",
  "p7b"         : "application/x-pkcs7-certificates",
  "spc"         : "application/x-pkcs7-certificates",
  "p7r"         : "application/x-pkcs7-certreqresp",
  "rar"         : "application/x-rar-compressed",
  "ris"         : "application/x-research-info-systems",
  "xap"         : "application/x-silverlight-app",
  "sql"         : "application/x-sql",
  "sitx"        : "application/x-stuffitx",
  "srt"         : "application/x-subrip",
  "sv4cpio"     : "application/x-sv4cpio",
  "t3"          : "application/x-t3vm-image",
  "gam"         : "application/x-tads",
  "tfm"         : "application/x-tex-tfm",
  "obj"         : "application/x-tgif",
  "fig"         : "application/x-xfig",
  "xlf"         : "application/x-xliff+xml",
  "xpi"         : "application/x-xpinstall",
  "xz"          : "application/x-xz",
  "z1"          : "application/x-zmachine",
  "z2"          : "application/x-zmachine",
  "z3"          : "application/x-zmachine",
  "z4"          : "application/x-zmachine",
  "z5"          : "application/x-zmachine",
  "z6"          : "application/x-zmachine",
  "z7"          : "application/x-zmachine",
  "z8"          : "application/x-zmachine",
  "xaml"        : "application/xaml+xml",
  "xdf"         : "application/xcap-diff+xml",
  "xenc"        : "application/xenc+xml",
  "xhtml"       : "application/xhtml+xml",
  "xht"         : "application/xhtml+xml",
  "xsl"         : "application/xml",
  "dtd"         : "application/xml-dtd",
  "xop"         : "application/xop+xml",
  "xpl"         : "application/xproc+xml",
  "xslt"        : "application/xslt+xml",
  "xspf"        : "application/xspf+xml",
  "mxml"        : "application/xv+xml",
  "xhvml"       : "application/xv+xml",
  "xvml"        : "application/xv+xml",
  "xvm"         : "application/xv+xml",
  "yang"        : "application/yang",
  "yin"         : "application/yin+xml",
  "adp"         : "audio/adpcm",
  "kar"         : "audio/midi",
  "mp4a"        : "audio/mp4",
  "mpga"        : "audio/mpeg",
  "mp2a"        : "audio/mpeg",
  "m2a"         : "audio/mpeg",
  "m3a"         : "audio/mpeg",
  "s3m"         : "audio/s3m",
  "sil"         : "audio/silk",
  "uva"         : "audio/vnd.dece.audio",
  "uvva"        : "audio/vnd.dece.audio",
  "dra"         : "audio/vnd.dra",
  "dts"         : "audio/vnd.dts",
  "dtshd"       : "audio/vnd.dts.hd",
  "pya"         : "audio/vnd.ms-playready.media.pya",
  "weba"        : "audio/webm",
  "aac"         : "audio/x-aac",
  "caf"         : "audio/x-caf",
  "flac"        : "audio/x-flac",
  "mka"         : "audio/x-matroska",
  "wax"         : "audio/x-ms-wax",
  "wma"         : "audio/x-ms-wma",
  "rmp"         : "audio/x-pn-realaudio-plugin",
  "xm"          : "audio/xm",
  "cdx"         : "chemical/x-cdx",
  "cif"         : "chemical/x-cif",
  "cmdf"        : "chemical/x-cmdf",
  "cml"         : "chemical/x-cml",
  "csml"        : "chemical/x-csml",
  "xyz"         : "chemical/x-xyz",
  "g3"          : "image/g3fax",
  "gif"         : "image/gif",
  "jpeg"        : "image/jpeg",
  "jpg"         : "image/jpeg",
  "ktx"         : "image/ktx",
  "png"         : "image/png",
  "sgi"         : "image/sgi",
  "uvi"         : "image/vnd.dece.graphic",
  "uvvi"        : "image/vnd.dece.graphic",
  "uvg"         : "image/vnd.dece.graphic",
  "uvvg"        : "image/vnd.dece.graphic",
  "sub"         : "image/vnd.dvb.subtitle",
  "mdi"         : "image/vnd.ms-modi",
  "wdp"         : "image/vnd.ms-photo",
  "webp"        : "image/webp",
  "3ds"         : "image/x-3ds",
  "ico"         : "image/x-icon",
  "pcx"         : "image/x-pcx",
  "eml"         : "message/rfc822",
  "mime"        : "message/rfc822",
  "igs"         : "model/iges",
  "iges"        : "model/iges",
  "msh"         : "model/mesh",
  "mesh"        : "model/mesh",
  "silo"        : "model/mesh",
  "dae"         : "model/vnd.collada+xml",
  "x3db"        : "model/x3d+binary",
  "x3dbz"       : "model/x3d+binary",
  "x3dv"        : "model/x3d+vrml",
  "x3dvz"       : "model/x3d+vrml",
  "x3d"         : "model/x3d+xml",
  "x3dz"        : "model/x3d+xml",
  "appcache"    : "text/cache-manifest",
  "css"         : "text/css",
  "csv"         : "text/csv",
  "html"        : "text/html",
  "n3"          : "text/n3",
  "txt"         : "text/plain",
  "conf"        : "text/plain",
  "def"         : "text/plain",
  "list"        : "text/plain",
  "log"         : "text/plain",
  "in"          : "text/plain",
  "ttl"         : "text/turtle",
  "urls"        : "text/uri-list",
  "vcard"       : "text/vcard",
  "dcurl"       : "text/vnd.curl.dcurl",
  "mcurl"       : "text/vnd.curl.mcurl",
  "scurl"       : "text/vnd.curl.scurl",
  "sub"         : "text/vnd.dvb.subtitle",
  "gv"          : "text/vnd.graphviz",
  "s"           : "text/x-asm",
  "asm"         : "text/x-asm",
  "cxx"         : "text/x-c",
  "hh"          : "text/x-c",
  "dic"         : "text/x-c",
  "f"           : "text/x-fortran",
  "for"         : "text/x-fortran",
  "f77"         : "text/x-fortran",
  "f90"         : "text/x-fortran",
  "nfo"         : "text/x-nfo",
  "opml"        : "text/x-opml",
  "p"           : "text/x-pascal",
  "pas"         : "text/x-pascal",
  "sfv"         : "text/x-sfv",
  "uu"          : "text/x-uuencode",
  "3gp"         : "video/3gpp",
  "3g2"         : "video/3gpp2",
  "h261"        : "video/h261",
  "h263"        : "video/h263",
  "h264"        : "video/h264",
  "jpgv"        : "video/jpeg",
  "jpm"         : "video/jpm",
  "jpgm"        : "video/jpm",
  "mj2"         : "video/mj2",
  "mjp2"        : "video/mj2",
  "mp4"         : "video/mp4",
  "mp4v"        : "video/mp4",
  "mpg4"        : "video/mp4",
  "m2v"         : "video/mpeg",
  "ogv"         : "video/ogg",
  "uvh"         : "video/vnd.dece.hd",
  "uvvh"        : "video/vnd.dece.hd",
  "uvm"         : "video/vnd.dece.mobile",
  "uvvm"        : "video/vnd.dece.mobile",
  "uvp"         : "video/vnd.dece.pd",
  "uvvp"        : "video/vnd.dece.pd",
  "uvs"         : "video/vnd.dece.sd",
  "uvvs"        : "video/vnd.dece.sd",
  "uvv"         : "video/vnd.dece.video",
  "uvvv"        : "video/vnd.dece.video",
  "dvb"         : "video/vnd.dvb.file",
  "pyv"         : "video/vnd.ms-playready.media.pyv",
  "uvu"         : "video/vnd.uvvu.mp4",
  "uvvu"        : "video/vnd.uvvu.mp4",
  "webm"        : "video/webm",
  "f4v"         : "video/x-f4v",
  "flv"         : "video/x-flv",
  "m4v"         : "video/x-m4v",
  "mkv"         : "video/x-matroska",
  "mk3d"        : "video/x-matroska",
  "mks"         : "video/x-matroska",
  "vob"         : "video/x-ms-vob",
  "wm"          : "video/x-ms-wm",
  "wmv"         : "video/x-ms-wmv",
  "wmx"         : "video/x-ms-wmx",
  "wvx"         : "video/x-ms-wvx",
  "smv"         : "video/x-smv",

]);

//! @decl string ext_to_media_type(string extension)
//! @belongs MIME
//! Returns the MIME media type for the provided filename @[extension].
//! Currently 469 file extensions are known to this method. Zero will
//! be returned on unknown file extensions.

string `()(string ext) {
  return small_ext2type[ext] || ext2type[ext];
}

protected mixed cast(string to)
{
  if(to=="mapping") return small_ext2type + ext2type;
}
