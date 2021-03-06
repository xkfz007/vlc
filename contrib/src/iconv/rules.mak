# libiconv
LIBICONV_VERSION=1.14
LIBICONV_URL=$(GNU)/libiconv/libiconv-$(LIBICONV_VERSION).tar.gz

PKGS += iconv
# iconv cannot be detect with pkg-config, but it is mandated by POSIX.
# Hard-code based on the operating system.
ifndef HAVE_WIN32
PKGS_FOUND += iconv
endif

$(TARBALLS)/libiconv-$(LIBICONV_VERSION).tar.gz:
	$(call download_pkg,$(LIBICONV_URL),iconv)

.sum-iconv: libiconv-$(LIBICONV_VERSION).tar.gz

iconv: libiconv-$(LIBICONV_VERSION).tar.gz .sum-iconv
	$(UNPACK)
	$(APPLY) $(SRC)/iconv/win32.patch
	$(APPLY) $(SRC)/iconv/bins.patch
	$(APPLY) $(SRC)/iconv/libiconv-c11.patch
ifdef HAVE_WIN64
	$(APPLY) $(SRC)/iconv/libiconv-win64.patch
endif
	$(UPDATE_AUTOCONFIG) && cd $(UNPACK_DIR) && mv config.guess config.sub build-aux
	$(UPDATE_AUTOCONFIG) && cd $(UNPACK_DIR) && mv config.guess config.sub libcharset/build-aux
	$(MOVE)

.iconv: iconv
	cd $< && $(HOSTVARS) ./configure CFLAGS="$(CFLAGS) -fgnu89-inline" $(HOSTCONF) --disable-nls
	cd $< && $(MAKE) install
	touch $@
