.PHONY : all clean
SUB_MODULES := common camera datain edit imgtest

all clean :
	@for m in $(SUB_MODULES); do echo "make: $$m"; $(MAKE) -C $$m $@; done;
