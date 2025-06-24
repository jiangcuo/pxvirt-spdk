include /usr/share/dpkg/pkg-info.mk


export PVERELEASE = $(shell echo $(DEB_VERSION_UPSTREAM) | cut -d. -f1-2)
export VERSION = $(DEB_VERSION_UPSTREAM_REVISION)

PACKAGE = pxvirt-spdk
SRCDIR := src
ARCH:=$(shell dpkg-architecture -qDEB_BUILD_ARCH)

BUILDDIR ?= $(PACKAGE)-$(DEB_VERSION_UPSTREAM)

DEB = $(PACKAGE)_$(DEB_VERSION_UPSTREAM_REVISION)_$(ARCH).deb \
python3-spdk_$(DEB_VERSION_UPSTREAM_REVISION)_$(ARCH).deb


DSC=$(PACKAGE)_$(DEB_VERSION_UPSTREAM_REVISION).dsc
GITVERSION:=$(shell git rev-parse --short=16 HEAD)

all: $(SUBDIRS)
	set -e && for i in $(SUBDIRS); do $(MAKE) -C $$i; done

.PHONY: submodule
submodule:
ifeq ($(shell test -f "$(SRCDIR)/configure" && echo 1 || echo 0), 0)
	git submodule update --init --recursive
endif

$(BUILDDIR): submodule
	rm -rf $@ $@.tmp
	mkdir $@.tmp
	rsync -a debian cpumask src/* $@.tmp
	echo "git clone https://gitea.lierfang.com/pxcloud/spdk\\ngit checkout $(GITVERSION)" >  $@.tmp/debian/SOURCE
	echo "REPOID_GENERATED=$(GITVERSION)" > $@.tmp/debian/rules.env
	mv $@.tmp $@


.PHONY: deb 
deb: $(DEB)
$(DEB): $(BUILDDIR)
	cd $(BUILDDIR); dpkg-buildpackage -b -us -uc

.PHONY: distclean
distclean: clean

.PHONY: clean
clean:
	set -e && for i in $(SUBDIRS); do $(MAKE) -C $$i $@; done
	rm -f $(PACKAGE)*.tar* country.dat *.deb *.dsc *.build *.buildinfo *.changes
	rm -rf dest $(PACKAGE)-[0-9]*/

.PHONY: dinstall
dinstall: $(DEB)
	dpkg -i $(DEB)