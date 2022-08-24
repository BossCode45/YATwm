.PHONY: clean
CXX := g++
CXXFLAGS := -g `pkg-config --cflags x11`
LINKFLAGS := `pkg-config --libs x11`
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

#Files to be compiled
$(OBJS_DIR)/main.o: $(SOURCE_FILES) $(SOURCE_HEADERS)

clean:
	rm $(OBJS_DIR)/*.o 
	rm $(OUT_DIR)/$(EXEC)
