crn-normalizer.exe: main.o normalizer.o ../common/crn.o ../common/math.o
	g++ $^ -lpng -lz -static-libgcc -static-libstdc++ -o $@

%.o: %.cpp
	g++ -std=c++2a -I ../lib -c $< -o $@

.PHONY: clean debug

clean:
	rm *.o ../common/*.o
