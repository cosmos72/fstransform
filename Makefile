DESTDIR:=/usr/local/bin
TARGET:=Release

ifneq ($(DEBUG),)
  TARGET:=Debug
endif

all:
	$(MAKE) -C fsremap/$(TARGET) $@ && $(MAKE) -C fsmove/$(TARGET) $@

install: all
	install fstransform/fstransform.sh fsremap/$(TARGET)/fsremap fsmove/$(TARGET)/fsmove $(DESTDIR)
	
%:
	$(MAKE) -C fsremap/$(TARGET) $@ && $(MAKE) -C fsmove/$(TARGET) $@
