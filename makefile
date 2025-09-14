# 检测操作系统
ifeq ($(OS),Windows_NT)
    # Windows 系统
    RM = del /Q
    EXECUTABLE = build.exe
    LOG_FILES = *.log
else
    # Unix-like 系统 (Linux, macOS)
    RM = rm -f
    EXECUTABLE = build
    LOG_FILES = *.log
endif

# windows build:  g++ -o build.exe main.cpp ylog.cpp -pthread

# 主构建规则
$(EXECUTABLE): main.cpp Ylog.cpp
	g++ -o $@ $^ -pthread -std=c++17

# 清理规则
clean:
	$(RM) $(EXECUTABLE)
	$(RM) $(LOG_FILES)

# 伪目标声明
.PHONY: clean