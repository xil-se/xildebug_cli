SRCS := \
	main.c

INCLUDES := \

DEFS := \
	_GNU_SOURCE

CFLAGS += \
	-Wall -Wextra -Wpedantic -Wno-unused-parameter -std=c99 -g \
	$(shell pkg-config --cflags libusb-1.0) \
	$(addprefix -D, $(DEFS)) \
	$(addprefix -I, $(INCLUDES))

LIBS := \
	$(shell pkg-config --libs libusb-1.0) -lreadline

OBJS := $(patsubst %.c,out/obj/%.o, $(filter %.c, $(SRCS)))
DEPS := $(patsubst %.o,%.d,$(OBJS))

all: out/xildebug
.PHONY: all

out/obj/%.o: %.c
	@echo "CC $<"
	@mkdir -p $(dir $@)
	@$(CC) -MM -MF $(subst .o,.d,$@) -MP -MT "$@ $(subst .o,.d,$@)" $(CFLAGS) $<
	@$(CC) $(CFLAGS) -c -o $@ $<

out/xildebug: $(OBJS)
	@echo "LD $@"
	@$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	@echo "Cleaning"
	@rm -rf out/
.PHONY: clean

ifneq ("$(MAKECMDGOALS)","clean")
-include $(DEPS)
endif
