# Thanks to Job Vranish (https://spin.atomicobject.com/2016/08/26/makefile-c-projects/)
# Although this makefile is only loosely based on that
BUILD_DIR := ./build
SRC_DIR := ./src
EXAMPLE_DIR := ./examples

.PHONY: default lib clean
default:
	@echo "Usage: make lib, make clean, make build/*.example where *.c is in examples/"
lib: $(BUILD_DIR)/winglang.so
clean:
	rm -r $(BUILD_DIR)

SRCS := $(shell find $(SRC_DIR) -name "*.c")
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)
INC_DIRS := ./include
INC_FLAGS := $(addprefix -I,$(INC_DIRS))
DEP_NEEDED_FLAGS := $(INC_FLAGS) -MMD -MP

# Linking
$(BUILD_DIR)/%.example: $(BUILD_DIR)/$(EXAMPLE_DIR)/%.c.o $(OBJS)
	$(CC) $< $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/winglang.so: $(OBJS)
	$(CC) $(OBJS) -o $@ -shared $(LDFLAGS)

# Compiling with dependency files
$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(DEP_NEEDED_FLAGS) -fPIC $(CFLAGS) -c $< -o $@

# Include the .d makefiles. The - at the front suppresses the errors of missing
# Makefiles. Initially, all the .d files will be missing, and we don't want those
# errors to show up.
-include $(DEPS)
