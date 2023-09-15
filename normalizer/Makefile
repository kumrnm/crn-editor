crn-normalizer.exe: main.o normalizer.o ../common/crn.o ../common/math.o
	g++ $^ -lpng -lz -o $@

%.o: %.cpp
	g++ -std=c++2a -I ../lib -c $< -o $@

.PHONY: clean debug

clean:
	rm *.o ../common/*.o
