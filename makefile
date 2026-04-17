CXX = g++

all:
	$(CXX) badapple_V2.cpp -o badapple -I/usr/include/opencv4 -lopencv_core -lopencv_imgproc -lopencv_videoio -lopencv_highgui