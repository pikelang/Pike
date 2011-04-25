// $Id$

#pike __REAL_VERSION__

static constant small_ext2type = ([
  "html" : "text/html",
  "txt"  : "text/plain",
  "css"  : "text/css",
  "gif"  : "image/gif",
  "jpg"  : "image/jpeg",
  "jpeg" : "image/jpeg",
  "png"  : "image/png",
]);

static mapping ext2type = ([

// Last synchronized with IANA media types list: 2002-02-05 (except
// applications)

// -------------------- Audio ---------------------

// IANA registered

  "726"     : "audio/32kadpcm",
  "au"      : "audio/basic",
  "snd"     : "audio/basic",
// audio/DAT12
// audio/G.722.1
  "l16"     : "audio/L16",
// audio/L20
// audio/L24
// audio/MP4A-LATM
// audio/mpa-robust
  "abs"     : "audio/mpeg",
  "mp2"     : "audio/mpeg",
  "mp3"     : "audio/mpeg",
// "mpa"      : "audio/mpeg",
  "mpega"   : "audio/mpeg",
// audio/parityfec
  "sid"     : "audio/prs.sid",
  "psid"    : "audio/prs.sid",
// audio/telephone-event
// audio/tone
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
  "gif"     : "image/gif",
  "ief"     : "image/ief",
  "jfif"    : "image/jpeg",
  "jpe"     : "image/jpeg",
  "jpeg"    : "image/jpeg",
  "jpg"     : "image/jpeg",
  "pjp"     : "image/jpeg",
  "pjpg"    : "image/jpeg",
// image/naplps
  "png"     : "image/png",
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


// Pending registration

  "svg"     : "image/svg",

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
  "css"     : "text/css",
// text/directory
// text/enriched
  "htm"     : "text/html",
  "html"    : "text/html",
  "htp"     : "text/html",
  "rxml"    : "text/html",
  "shtml"   : "text/html",
  "spider"  : "text/html",
  "spml"    : "text/html",
// text/parityfec
  "bat"     : "text/plain",
  "java"    : "text/plain",
  "text"    : "text/plain",
  "txt"     : "text/plain",
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

]);

//! @decl string ext_to_media_type(string extension)
//! @belongs MIME
//! Returns the MIME media type for the provided filename @[extension].
//! Zero will be returned on unknown file extensions.

string `()(string ext) {
  return small_ext2type[ext] || ext2type[ext];
}
