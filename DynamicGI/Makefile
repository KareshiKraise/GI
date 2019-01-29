.SUFFIXES: .c .cc .cpp .cxx .h

# compiler names:
CXX		= g++ 
CC		= gcc

# flags for C++ compiler:
CFLAGS		= -g 
CXXFLAGS	= -g -std=c++11

# libraries to link with:

INC_PATH		=	
LIB_PATH 		=	-L/usr/lib/nvidia-375/ -L/usr/lib/x86_64-linux-gnu/

GL_LDFLAGS 		=	-lGL -lGLU -lGLEW
GLFW_LDFLAGS 	=	-lglfw
GLUT_LDFLAGS 	=	-lglut
ASSIMP_LDFLAGS 	=	-lassimp
SOIL_LDFLAGS	= 	-lSOIL

OBJFILES 		=	src/framebuffer.o src/main.o src/mesh_loader.o src/model.o src/window.o \
					src/techniques.o src/third_party/imgui.o src/third_party/imgui_impl_glfw.o \
					src/third_party/imgui_impl_opengl3.o src/third_party/imgui_draw.o \
					src/third_party/imgui_widgets.o

# compile rules
.c.o:	$*.h
	@echo "Compiling C code ...."
	$(CC) -o $*.o -c $(CXXFLAGS) $(INC_PATH) $*.c

.cpp.o:	$*.h
	@echo "Compiling C++ code ...."
	$(CXX) -o $*.o -c $(CXXFLAGS) $(INC_PATH) $*.cpp

# ***********************************************************************************
all:	main

main:  $(OBJFILES)
	@echo "Linking ...."
	$(CXX) $^ -o $@ $(LIB_PATH) $(GL_LDFLAGS) $(GLFW_LDFLAGS) $(ASSIMP_LDFLAGS)
		
clean:	
	@echo "Clearing ..."
	rm -f *.o core *.*~ *~ main src/*.o src/third_party/*.o
