SRC_ROOT = ./
BUILD_ROOT = ./build

CXX ?= g++

CXXFLAGS ?= -Wall -Wextra -g
LDFLAGS ?= 
LIBS = 
# 1. 递归查找src下所有.cpp（Make 4.0+ 支持**递归通配符）
SRCS := $(shell find $(SRC_ROOT) -name "*.cpp" -type f)
SRCS += ../../src/common.cpp
SRCS += ../../src/LexicalAnalysize.cpp

# 2. 映射src路径到build，.cpp替换为.o（与原逻辑一致）
OBJS := $(patsubst $(SRC_ROOT)/%.cpp, $(BUILD_ROOT)/%.o, $(SRCS) )
# 3. ./src/*.cpp => ./build/*.d
DEPS = $(patsubst $(SRC_ROOT)/%.cpp, $(BUILD_ROOT)/%.d, $(SRCS) )
# 4. 提取所有.o文件的目录（去重，用于自动创建目录）
OBJ_DIRS := $(sort $(dir $(OBJS) ) )
ALL_DIRS := $(BUILD_ROOT) $(OBJ_DIRS)

# 5.1 递归查找src 下所有子目录（Make 4.0+ 用**）
SRC_SUBDIRS := $(wildcard $(SRC_ROOT)/**/)
SRC_SUBDIRS += ../../src
# 5.2 为每个子目录添加-I 前缀（生成-I./src -I./src/util -I./src/core 等）
INC_PATHS := $(addprefix -I, $(SRC_ROOT) $(SRC_SUBDIRS) )

# 5.3 将递归的头文件搜索路径合并到CXXFLAGS
CXXFLAGS += $(INC_PATHS)

# 定义最终生成的可执行文件名称为test.out
TARGET = $(BUILD_ROOT)/test.out
# 标识伪目标
.PHONY: all clean
# 默认目标：优先执行all（依赖TARGET）
all: $(ALL_DIRS) $(TARGET)
# 链接（模式）规则：将所有.o 文件链接为可执行文件
$(TARGET): $(OBJS)
	#@echo "SRC_ROOT: $(SRC_ROOT)"
	#@echo "SRCS: $(SRCS)"
	#@echo "OBJS: $(OBJS)"
	@echo "Linking $@..."
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

# 创建build目录及build下的子目录（仅顺序依赖）
$(ALL_DIRS):
	@mkdir -p $@
	@echo "create dir: $@"

# 生成.d 的（模式）规则
# | 符号用于定义仅顺序依赖（Order-only Prerequisites），
# | 右侧：是仅顺序依赖——仅保证 在生成目标之前，右侧依赖必须先存在/被构建，但右侧文件的更新 不会触发目标的重新生成
$(BUILD_ROOT)/%.d: $(SRC_ROOT)/%.cpp | $(OBJ_DIRS)
	@echo "Generating .h dependencies for $<..."
	rm -f $@; \
	$(CXX) $(CXXFLAGS) -MM $< | sed 's,\($*\)\.o[ :]*,$(BUILD_ROOT)/\1.o $@ : ,g' > $@; \

# 编译（模式）规则: %.o: %.c 表示“任意xxx.o 文件依赖于对应的xxx.c 文件”
$(BUILD_ROOT)/%.o: $(SRC_ROOT)/%.cpp
	@echo "building $@..."
	$(CXX) $(CXXFLAGS) -c $< -o $@ 

clean:
	@echo "clean..."
	rm -rf $(BUILD_ROOT)

# 引入所有.d-include $(DEPS)