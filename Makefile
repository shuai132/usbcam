OBJS = jpgd.o jpge.o webcam.o grab.o mytools.o
target  = v4l2
CXXFLAG = -std=c++11 -Wall
LDFLAGS = -pthread

$(target):$(OBJS)
	@echo "linking $@ ..."
	@g++ $(LDFLAGS) $(OBJS) -o $(target)

$(OBJS):%o:%cpp
	@echo "compiling $@ ..."
	@g++ $(CXXFLAG) -c $< -o $@ -I.

clean:
	rm *.o -f
	rm $(target) -f
