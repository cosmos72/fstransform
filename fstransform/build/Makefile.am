sbin_SCRIPTS = fstransform

fstransform:  ../fstransform-sh
	sed 's/%PACKAGE_VERSION%/$(PACKAGE_VERSION)/g' <$< >fstransform.tmp
	if test -x /bin/dash ; then \
	  sed 's_#!/bin/bash_#!/bin/dash_g' <fstransform.tmp >$@ ; \
	  rm -f fstransform.tmp ; \
	else \
	  mv fstransform.tmp $@ ; \
	fi
	chmod +x $@

../fstransform-sh:

clean: clean-am
	rm -f fstransform fstransform.tmp
