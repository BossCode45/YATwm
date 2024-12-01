.PHONY: clean
CXX := g++
CXXFLAGS := -std=c++17 `pkg-config --cflags --libs libnotify`# -g -fsanitize=address -fno-omit-frame-pointer
LINKFLAGS := -lX11 -lXrandr
OBJS_DIR := build
OUT_DIR := out
SOURCE_DIR := src
EXEC := YATwm
SOURCE_FILES := $(wildcard $(SOURCE_DIR)/*.cpp)
SOURCE_HEADERS := $(wildcard $(SOURCE_DIR)/*.h)
OBJS := $(subst $(SOURCE_DIR),$(OBJS_DIR), $(patsubst %.cpp,%.o,$(SOURCE_FILES)))
INSTALL_DIR = /

$(EXEC): $(OBJS)
	$(CXX) $(OBJS) $(CXXFLAGS) $(LINKFLAGS) -o $(EXEC)

$(OBJS_DIR)/%.o : $(SOURCE_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

i: $(EXEC)
	sudo install -D -m 755 $(EXEC) $(INSTALL_DIR)usr/bin/$(EXEC)
	sudo install -D -m 644 yat.desktop $(INSTALL_DIR)usr/share/xsessions/yat.desktop
	sudo install -D -m 644 config $(INSTALL_DIR)etc/YATwm/config
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
$(OBJS_DIR)/commands.o: $(SOURCE_DIR)/commands.cpp $(SOURCE_DIR)/commands.h
$(OBJS_DIR)/config.o: $(SOURCE_DIR)/config.cpp $(SOURCE_DIR)/config.h $(SOURCE_DIR)/commands.h
$(OBJS_DIR)/keybinds.o: $(SOURCE_DIR)/keybinds.cpp $(SOURCE_DIR)/keybinds.h $(SOURCE_DIR)/commands.h
$(OBJS_DIR)/ipc.o: $(SOURCE_DIR)/ipc.cpp $(SOURCE_DIR)/ipc.h $(SOURCE_DIR)/commands.h $(SOURCE_DIR)/ewmh.h

clean:
	rm $(OBJS_DIR)/*.o 
	rm $(EXEC)
