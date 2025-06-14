TARGET = app
COMPILER_TOOLS = -B/usr/lib/x86_64-linux-gnu

app: main.cpp resFile.cpp
	g++ main.cpp resFile.cpp -o $(TARGET) $(COMPILER_TOOLS) io/libio.a -lglfw -lglad

clean:
	rm -f $(TARGET)