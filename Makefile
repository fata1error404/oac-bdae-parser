TARGET = app

LIB_SOURCES = libs/glad/glad.c \
			  libs/imgui/imgui.cpp \
              libs/imgui/imgui_draw.cpp \
              libs/imgui/imgui_tables.cpp \
              libs/imgui/imgui_widgets.cpp \
              libs/imgui/imgui_impl_glfw.cpp \
              libs/imgui/imgui_impl_opengl3.cpp \
			  libs/imgui/ImGuiFileDialog.cpp

OS = $(shell uname -s)

ifeq ($(OS),Linux)
# Linux build
app: main.cpp resFile.cpp $(LIB_SOURCES)
	g++ main.cpp resFile.cpp $(LIB_SOURCES) -o $(TARGET) libs/io/libio_linux.a -lglfw
else
# Windows build
app: main.cpp resFile.cpp $(LIB_SOURCES)
	g++ main.cpp resFile.cpp $(LIB_SOURCES) aux_docs/resource.res -o $(TARGET) libs/io/libio_windows.a libs/GLFW/libglfw3.a -lgdi32
endif

clean:
	rm -f $(TARGET)