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

$(EXEC): $(OBJS)
	$(CXX) $(OBJS) $(CXXFLAGS) $(LINKFLAGS) -o $(OUT_DIR)/$(EXEC)

$(OBJS_DIR)/%.o : $(SOURCE_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

i: $(EXEC)
	sudo mv $(EXEC) /usr/bin
	sudo cp yat.desktop /usr/share/xsessions
	sudo mkdir /etc/YATwm
	sudo cp config.toml /etc/YATwm
install: i
r:
	sudo rm /usr/bin/$(EXEC)
	sudo rm /usr/share/xsessions/yat.desktop
	sudo rm -rf /etc/YATwm
remove: r

#Files to be compiled
$(OBJS_DIR)/main.o: $(SOURCE_FILES) $(SOURCE_HEADERS)
$(OBJS_DIR)/ewmh.o: $(SOURCE_DIR)/ewmh.cpp $(SOURCE_DIR)/ewmh.h
$(OBJS_DIR)/util.o: $(SOURCE_DIR)/util.cpp $(SOURCE_DIR)/util.h
$(OBJS_DIR)/config.o: $(SOURCE_DIR)/config.cpp $(SOURCE_DIR)/config.h $(OBJS_DIR)/util.o

clean:
	rm $(OBJS_DIR)/*.o 
	rm $(OUT_DIR)/$(EXEC)
