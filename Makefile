PACKAGE=timelink-touchwin-paint
VERSION?=$(shell git describe | sed -n -r 's/v([0-9]+(\.[0-9]+){0,3})(\-[0-9]+)?(\-[0-9a-z]+)?/\1\3/p' | tr - +)

all: rpm 

rpm-prepare:
	@if [ -d $(PACKAGE)-$(VERSION) ]; then rm -rf $(PACKAGE)-$(VERSION); fi
	mkdir -p $(PACKAGE)-$(VERSION)
	pushd src/; git clean -df
	cp -r src/* $(PACKAGE)-$(VERSION)
	@if [ ! -d ~/rpmbuild/SOURCES/ ]; then mkdir -p ~/rpmbuild/SOURCES/; fi
	@if [ ! -d ~/rpmbuild/SPECS/ ]; then mkdir -p ~/rpmbuild/SPECS/; fi
	tar jcf ~/rpmbuild/SOURCES/$(PACKAGE)-$(VERSION).tar.bz2 $(PACKAGE)-$(VERSION)
	rm -rf $(PACKAGE)-$(VERSION)
	@sed -i -e 's/^Name:.*/Name: $(PACKAGE)/g' $(PACKAGE).spec
	@sed -i -e 's/^Version:.*/Version: $(VERSION)/g' $(PACKAGE).spec
	cp $(PACKAGE).spec ~/rpmbuild/SPECS

rpm: rpm-prepare
	oldpwd=`pwd`; \
	cd ~/rpmbuild/SPECS; \
	rpmbuild -ba $(PACKAGE).spec \
	| tee /dev/tty | grep "^Wrote" | cut -d' ' -f 2 | xargs -i cp {} $$oldpwd

.PHONY: rpm-prepare
