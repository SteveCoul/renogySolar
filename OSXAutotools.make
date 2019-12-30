WORKDIR=$(PWD)/autotools_osx
AUTOMAKE_VERSION=1.12
AUTOCONF_VERSION=2.68

$(WORKDIR)/runtime/bin/automake: $(WORKDIR)/automake-$(AUTOMAKE_VERSION)/Makefile
	cd $(dir $^) && PATH=$(PATH):$(WORKDIR)/runtime/bin make install

$(WORKDIR)/automake-$(AUTOMAKE_VERSION)/Makefile: $(WORKDIR)/automake-$(AUTOMAKE_VERSION)/configure autoconf
	cd $(dir $@) && PATH=$(PATH):$(WORKDIR)/runtime/bin ./configure --prefix=$(WORKDIR)/runtime

$(WORKDIR)/automake-$(AUTOMAKE_VERSION)/configure: $(WORKDIR)/automake-$(AUTOMAKE_VERSION).tar.gz
	cd $(WORKDIR) && tar zxvf $(notdir $^)
	touch $@

$(WORKDIR)/automake-$(AUTOMAKE_VERSION).tar.gz:
	mkdir -p $(dir $@)
	cd $(dir $@) && curl -O https://ftp.gnu.org/gnu/automake/automake-$(AUTOMAKE_VERSION).tar.gz


autoconf: $(WORKDIR)/runtime/bin/autoconf

$(WORKDIR)/runtime/bin/autoconf: $(WORKDIR)/autoconf-$(AUTOCONF_VERSION)/Makefile
	cd $(dir $^) && make install

$(WORKDIR)/autoconf-$(AUTOCONF_VERSION)/Makefile: $(WORKDIR)/autoconf-$(AUTOCONF_VERSION)/configure.ac
	cd $(dir $@) && ./configure --prefix=$(WORKDIR)/runtime

$(WORKDIR)/autoconf-$(AUTOCONF_VERSION)/configure.ac: $(WORKDIR)/autoconf-$(AUTOCONF_VERSION).tar.gz
	cd $(WORKDIR) && tar zxvf $(notdir $^)
	touch $@

$(WORKDIR)/autoconf-$(AUTOCONF_VERSION).tar.gz:
	mkdir -p $(dir $@)
	cd $(dir $@) && curl -O https://ftp.gnu.org/gnu/autoconf/autoconf-$(AUTOCONF_VERSION).tar.gz
