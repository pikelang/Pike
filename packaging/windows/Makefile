
BUILD=../../build/winpkg

GIMP_HOME = http://www.gimp.org/~tml/gimp/win32
GLIB_ARCH = glib-2.0.4-20020703.zip
GLIB_URL  = $(GIMP_HOME)/$(GLIB_ARCH)
ICONV_ARCH = libiconv-1.7.zip
ICONV_URL  = $(GIMP_HOME)/$(ICONV_ARCH)
INTL_ARCH = libintl-0.10.40-20020310.zip
INTL_URL  = $(GIMP_HOME)/$(INTL_ARCH)
GTK_ARCH  = gtk+-1.3.0-20020801.zip
GTK_URL   = $(GIMP_HOME)/$(GTK_ARCH)
Z_ARCH    = zlib-1.1.3.zip
Z_URL     = $(GIMP_HOME)/$(Z_ARCH)
PNG_ARCH  = libpng-1.2.0.zip
PNG_URL   = $(GIMP_HOME)/$(PNG_ARCH)
JPEG_ARCH = libjpeg-6b.zip
JPEG_URL  = $(GIMP_HOME)/$(JPEG_ARCH)
TIFF_ARCH = libtiff-3.5.7.zip
TIFF_URL  = $(GIMP_HOME)/$(TIFF_ARCH)
FREETYPE_ARCH = freetype-2.0.5-20020112.zip
FREETYPE_URL  = $(GIMP_HOME)/$(FREETYPE_ARCH)

dlls: glib gtk zlib libpng libjpeg libtiff freetype

glib: libiconv libintl $(BUILD)
	(cd $(BUILD) && wget $(GLIB_URL))
	mkdir $(BUILD)/tmp
	(cd $(BUILD)/tmp && unzip ../$(GLIB_ARCH) && mv lib/libglib-2.0-0.dll ..)
	(cd $(BUILD)/tmp && mv lib/libgmodule-2.0-0.dll ..)
	(cd $(BUILD)/tmp && mv lib/libgobject-2.0-0.dll ..)
	(cd $(BUILD)/tmp && mv lib/libgthread-2.0-0.dll ..)
	rm -r $(BUILD)/tmp
	rm $(BUILD)/$(GLIB_ARCH)

libiconv: $(BUILD)
	(cd $(BUILD) && wget $(ICONV_URL))
	(make extract ARCHIVE=$(ICONV_ARCH) FILE=lib/iconv.dll)

libintl: $(BUILD)
	(cd $(BUILD) && wget $(INTL_URL))
	(make extract ARCHIVE=$(INTL_ARCH) FILE=lib/libintl-1.dll)

gtk: $(BUILD)
	(cd $(BUILD) && wget $(GTK_URL))
	mkdir $(BUILD)/tmp
	(cd $(BUILD)/tmp && unzip ../$(GTK_ARCH) && mv lib/libgtk-0.dll ..)
	(cd $(BUILD)/tmp && mv lib/libgdk-0.dll ..)
	rm -r $(BUILD)/tmp
	rm $(BUILD)/$(GTK_ARCH)

zlib: $(BUILD)
	(cd $(BUILD) && wget $(Z_URL))
	(make extract ARCHIVE=$(Z_ARCH) FILE=lib/libz.dll)

libpng: $(BUILD)
	(cd $(BUILD) && wget $(PNG_URL))
	(make extract ARCHIVE=$(PNG_ARCH) FILE=lib/libpng-3.dll)

libjpeg: $(BUILD)
	(cd $(BUILD) && wget $(JPEG_URL))
	(make extract ARCHIVE=$(JPEG_ARCH) FILE=lib/libjpeg6b.dll)

libtiff: $(BUILD)
	(cd $(BUILD) && wget $(TIFF_URL))
	(make extract ARCHIVE=$(TIFF_ARCH) FILE=lib/libtiff.dll)

freetype: $(BUILD)
	(cd $(BUILD) && wget $(FREETYPE_URL))
	(make extract ARCHIVE=$(FREETYPE_ARCH) FILE=lib/libfreetype-6.dll)

extract:
	mkdir $(BUILD)/tmp
	(cd $(BUILD)/tmp && unzip ../$(ARCHIVE) && mv $(FILE) ..)
	rm -r $(BUILD)/tmp
	rm $(BUILD)/$(ARCHIVE)


$(BUILD): ../../build
	@test -d $(BUILD) || mkdir $(BUILD)

../../build:
	@test -d ../../build || mkdir ../../build