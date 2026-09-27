CXX = g++
CXXFLAGS = -IC:/msys64/mingw64/include -LC:/msys64/mingw64/lib -I/mingw64/include/ncursesw
LIBS = -lavformat -lavcodec -lavutil -lswresample -lncursesw

player:
	$(CXX) $(shell ls *.cpp | grep -v test.cpp) -o player $(CXXFLAGS) $(LIBS)

test:
	$(CXX) $(shell ls *.cpp | grep -v main.cpp) -o test $(CXXFLAGS) $(LIBS)

clean:
	rm -f player test *.o
cleantest:
	rm -f test *.o
cleandata:
	rm -f user_data.bin
