export CC	:=	gcc
export CXX	:=	g++

CFLAGS		:=	$(CFLAGS) -DNCGC_PLATFORM_TEST
CXXFLAGS	:=	$(CXXFLAGS) -DNCGC_PLATFORM_TEST

ifeq ($(OS),Windows_NT)
	TARGET := out/$(PLATFORM)/test.exe
else
	TARGET := out/$(PLATFORM)/test
endif

testbin: $(TARGET)

.SECONDEXPANSION:
$(TARGET): $$(OBJFILES) obj/$(PLATFORM)/ntr_blowfish.bin.o obj/$(PLATFORM)/test.c.o obj/$(PLATFORM)/test.cpp.o
	@if [ $$(sha256sum ntr_blowfish.bin | cut -d ' ' -f 1) != \
		"bedd20bd7f9cac742ad760e2448d4043e0d37121b67a1be3a6b8afbb8a34f08e" ]; then \
		echo "Invalid ntr_blowfish.bin"; \
		exit 1; \
	fi
	@mkdir -p $(dir $@)
	@echo $^ =\> $@
	@$(CXX) $^ -o $@

-include obj/$(PLATFORM)/test.c.d obj/$(PLATFORM)/test.cpp.d
