all:
	$(MAKE) -C fsremap/Debug $@
	$(MAKE) -C fsmove/Debug $@

%:
	$(MAKE) -C fsremap/Debug $@
	$(MAKE) -C fsmove/Debug $@
