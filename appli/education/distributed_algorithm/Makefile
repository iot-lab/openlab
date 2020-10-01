FIRMWARE = distributed_algorithm.elf

ROOT = ../../..
BUILD_M3 = $(ROOT)/build.m3


.PHONY: all
all: $(FIRMWARE)

$(FIRMWARE): $(BUILD_M3)/bin/$(FIRMWARE)
	cp -v $^ $@

.PHONY: $(BUILD_M3)/bin/$(FIRMWARE)

$(BUILD_M3)/bin/$(FIRMWARE): | $(BUILD_M3)
	make -C $(BUILD_M3) $(basename $(FIRMWARE))

$(BUILD_M3):
	mkdir -p $@
	cd -P $@; cmake .. -DPLATFORM=iotlab-m3
