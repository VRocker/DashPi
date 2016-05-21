SUBDIRS = 

export BUILDROOTDIR = $(CURDIR)/buildroot
export SKELDIR = 	$(CURDIR)/skel
export OUTPUTDIR =	$(SKELDIR)/dashpi
export ROOTDIR = 	$(BUILDROOTDIR)/output/target
export IMAGEDIR =	$(BUILDROOTDIR)/output/images

default: all

all:
	rm -rf $(OUTPUTDIR)/bin/*
	mkdir -p $(OUTPUTDIR)/bin
	mkdir -p $(OUTPUTDIR)/lib
	mkdir -p $(SKELDIR)/recordings
	for subdir in $(SUBDIRS); do \
		(cd $$subdir && $(MAKE) clean && $(MAKE)) \
	done;
	(cd $(BUILDROOTDIR);make)
	cp buildroot/output/images/zImage .

setup:
	./checkconfig.sh
	(cd $(BUILDROOTDIR);make)

clean:
	for subdir in $(SUBDIRS); do \
		(cd $$subdir && $(MAKE) clean) \
	done;
	rm -rf $(OUTPUTDIR)/bin
	rm -rf $(OUTPUTDIR)/lib
	rm -f zImage
