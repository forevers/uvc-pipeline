# some dependency file references ...
# https://www.gnu.org/software/make/manual/html_node/Automatic-Prerequisites.html#Automatic-Prerequisites
# http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/#gcc
# https://codereview.stackexchange.com/questions/2547/makefile-dependency-generation/11109#11109
# https://stackoverflow.com/questions/38368531/create-dependency-files-in-makefile

SRCS = ./src/main.cpp 
SRCS += ./src/uvc_worker.cpp 
SRCS += ./src/render_ui.cpp
SRCS += ./src/libuvc_acquisition.cpp
SRCS += ./src/v4l2_acquisition.cpp 
SRCS += ./src/image_scaled.cpp

OBJS = $(SRCS:.cpp=.o)

# dependency file list
DEPS = $(SRCS:.cpp=.d)

PRODUCT := uvc_pipeline

CC := g++

C_INCLUDES = -I./libuvc/include -I./libuvc/build/include `pkg-config --cflags-only-I libusb-1.0` -I./src -I./src/include
C_INCLUDES += -I../../android/Camera_OpenCV_Pipeline/libWebCam/src/main/cpp/camera_pipeline_modules/webcam_include
PTHREAD =-pthread
CFLAGS    := 
CPPFLAGS := $(DEBUG) $(OPT) $(WARN) $(PTHREAD) -std=c++11 -pipe `pkg-config gtkmm-3.0 --cflags` -DPULL_MODEL
GTKLIB =`pkg-config --cflags --libs gtk+-3.0` `pkg-config gtkmm-3.0 --libs`
LDFLAGS := $(PTHREAD) $(GTKLIB) -export-dynamic -Wl,-R./libuvc/build -L./libuvc/build -luvc
LD := g++

all: $(PRODUCT)

clean:
	$(RM) $(DEPS) $(OBJS) $(PRODUCT) $(DEPS)

# dependency rule
%.d: %.cpp
	mkdir -p $(@D)
	$(CC) $(C_INCLUDES) $(CFLAGS) $(CPPFLAGS) -M $< | \
	sed 's,\($(notdir $*)\.o\) *:,$(dir $@)\1 $@: ,' > $@.tmp
	mv $@.tmp $@

# build target
$(PRODUCT): $(OBJS)
	$(LD) $^ -o $@ $(LDFLAGS)

# include generated dependencies
include $(SRCS:.cpp=.d)

# object rule
%.o: %.cpp
	$(CC) $(C_INCLUDES) -c $(CPPFLAGS) -o $@ $(<:.d=.cpp)

