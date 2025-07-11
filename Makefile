TARGET = app
COMPILER_TOOLS = -B/usr/lib/x86_64-linux-gnu

IMGUI_SOURCES = libs/imgui/imgui.cpp \
            	libs/imgui/imgui_draw.cpp \
            	libs/imgui/imgui_tables.cpp \
            	libs/imgui/imgui_widgets.cpp \
            	libs/imgui/imgui_impl_glfw.cpp \
            	libs/imgui/imgui_impl_opengl3.cpp \
				libs/imgui/ImGuiFileDialog.cpp

IMGUI_HEADERS = -Ilibs/imgui

app: main.cpp resFile.cpp $(IMGUI_SOURCES)
	g++ $(CXXFLAGS) main.cpp resFile.cpp $(IMGUI_SOURCES) -o $(TARGET) $(COMPILER_TOOLS) $(INCLUDES) libs/io/libio.a -lglfw -lglad

clean:
	rm -f $(TARGET)