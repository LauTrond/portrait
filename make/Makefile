.PHONY : all clean
SUB_MODULE := camera edit imgtest

all clean :
	for m in $(SUB_MODULE); do $(MAKE) -C $$m $@; done;
