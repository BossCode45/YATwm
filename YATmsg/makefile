.PHONY: clean
CXX := g++
CXXFLAGS := -std=c++17 # -g -fsanitize=address -fno-omit-frame-pointer
LINKFLAGS := -lX11
OBJS_DIR := ../build
OUT_DIR := ../out/
SOURCE_DIR := ./
EXEC := YATmsg
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
install: i
r:
	sudo rm $(INSTALL_DIR)usr/bin/$(EXEC)
remove: r

#Files to be compiled
$(OBJS_DIR)/YATmsg.o: $(SOURCE_FILES) $(SOURCE_HEADERS)

clean:
	rm $(OBJS_DIR)/*.o 
	rm $(EXEC)
