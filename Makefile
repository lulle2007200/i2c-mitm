PROJECT_NAME := i2c-mitm
I2C_MITM_TID := 0100000000001366

TARGETS := sysmodule

all: $(TARGETS)

sysmodule:
	$(MAKE) -C $@

clean:
	$(MAKE) -C sysmodule clean
	rm -rf dist

dist: all
	rm -rf dist

	mkdir -p dist/atmosphere/contents/$(I2C_MITM_TID)
	cp sysmodule/out/nintendo_nx_arm64_armv8a/release/sysmodule.nsp dist/atmosphere/contents/$(I2C_MITM_TID)/exefs.nsp
	echo "i2c" >> dist/atmosphere/contents/$(I2C_MITM_TID)/mitm.lst
	echo "i2c:pcv" >> dist/atmosphere/contents/$(I2C_MITM_TID)/mitm.lst

	mkdir -p dist/atmosphere/contents/$(I2C_MITM_TID)/flags
	touch dist/atmosphere/contents/$(I2C_MITM_TID)/flags/boot2.flag

	cd dist; zip -r $(PROJECT_NAME).zip ./*; cd ../;

.PHONY: all clean dist $(TARGETS)
