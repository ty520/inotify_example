cc = g++
CXXFLAGS = -std=c++11
objects = sample.o inotify_app.o

file_monitor: $(objects)
	$(cc) $(CFLAGS) -o file_monitor $(objects) 

sample.o inotify_app.o: inotify_app.h

.PHONY : clean
clean :
	rm file_monitor $(objects)
