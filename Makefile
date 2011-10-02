all:
	$(MAKE) -C fsremap $@
	$(MAKE) -C fsmove $@

%:
	$(MAKE) -C fsremap $@
	$(MAKE) -C fsmove $@
