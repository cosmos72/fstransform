sbin_SCRIPTS = fstransform.sh

fstransform.sh:  ../fstransform.sh
	sed 's/%PACKAGE_VERSION%/$(PACKAGE_VERSION)/g' <$<  >$@
	chmod +x $@

../fstransform.sh:

clean: clean-am
	rm -f fstransform.sh
