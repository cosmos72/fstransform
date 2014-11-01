sbin_SCRIPTS = fstransform

fstransform:  ../fstransform.sh
	sed 's/%PACKAGE_VERSION%/$(PACKAGE_VERSION)/g' <../fstransform.sh >$@
	chmod +x $@

../fstransform.sh:

clean: clean-am
	rm -f fstransform.sh
