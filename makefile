.PHONY: clean
CXX := g++
CXXFLAGS := -std=c++17 -Iinclude `pkg-config --cflags --libs libnotify`# -g -fsanitize=address -fno-omit-frame-pointer
LINKFLAGS := -lX11 -lXrandr
OBJS_DIR := .
OUT_DIR := .
SOURCE_DIR := .
EXEC := YATwm
SOURCE_FILES := $(wildcard $(SOURCE_DIR)/*.cpp)
SOURCE_HEADERS := $(wildcard $(SOURCE_DIR)/*.h)
OBJS := $(subst $(SOURCE_DIR),$(OBJS_DIR), $(patsubst %.cpp,%.o,$(SOURCE_FILES)))
INSTALL_DIR = /

$(EXEC): $(OBJS)
	$(CXX) $(OBJS) $(CXXFLAGS) $(LINKFLAGS) -o $(OUT_DIR)/$(EXEC)

$(OBJS_DIR)/%.o : $(SOURCE_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

i: $(EXEC)
	sudo install $(EXEC) $(INSTALL_DIR)usr/bin
	sudo install yat.desktop $(INSTALL_DIR)usr/share/xsessions
	sudo install config.toml $(INSTALL_DIR)etc/YATwm
install: i
r:
	sudo rm $(INSTALL_DIR)usr/bin/$(EXEC)
	sudo rm $(INSTALL_DIR)usr/share/xsessions/yat.desktop
	sudo rm -rf $(INSTALL_DIR)etc/YATwm
remove: r

#Files to be compiled
$(OBJS_DIR)/main.o: $(SOURCE_FILES) $(SOURCE_HEADERS)
$(OBJS_DIR)/ewmh.o: $(SOURCE_DIR)/ewmh.cpp $(SOURCE_DIR)/ewmh.h
$(OBJS_DIR)/util.o: $(SOURCE_DIR)/util.cpp $(SOURCE_DIR)/util.h
$(OBJS_DIR)/config.o: $(SOURCE_DIR)/config.cpp $(SOURCE_DIR)/config.h $(OBJS_DIR)/util.o

clean:
	rm $(OBJS_DIR)/*.o 
	rm $(OUT_DIR)/$(EXEC)
