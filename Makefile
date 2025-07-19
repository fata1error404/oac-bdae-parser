TARGET = app

LIB_SOURCES = libs/glad/glad.c \
			  libs/imgui/imgui.cpp \
              libs/imgui/imgui_draw.cpp \
              libs/imgui/imgui_tables.cpp \
              libs/imgui/imgui_widgets.cpp \
              libs/imgui/imgui_impl_glfw.cpp \
              libs/imgui/imgui_impl_opengl3.cpp \
			  libs/imgui/ImGuiFileDialog.cpp

app: main.cpp resFile.cpp $(LIB_SOURCES)
	g++ main.cpp resFile.cpp $(LIB_SOURCES) -o $(TARGET) libs/io/libio.a -lglfw

clean:
	rm -f $(TARGET)