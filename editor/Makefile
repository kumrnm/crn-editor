crn-editor.exe: $(patsubst %.cpp,%.o,$(wildcard *.cpp ../common/*.cpp)) resource.o
	g++ -mwindows $^ -lgdiplus -lshlwapi -o $@

resource.o: resource.rc CRN.ico
	windres $< -o $@

%.o: %.cpp
	g++ -std=c++2a -DUNICODE -c $< -o $@

.PHONY: clean debug

clean:
	rm *.o ../common/*.o

debug: crn-editor.exe
	crn-editor.exe "..\\temp"
