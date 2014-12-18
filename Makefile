CXXFLAGS =	-O2 -g -Wall -fmessage-length=0

OBJS =		webclient.o tcp_client.o

LIBS =

TARGET =	webclient

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
