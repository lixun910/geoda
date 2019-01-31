
include ../GeoDamake.opt

#include ./file.lst

CPPFLAGS 	:=	$(CPPFLAGS)
CXXFLAGS 	:=	$(CXXFLAGS)

CXX_SRCS := uiRoadDownload.cpp TravelTool.cpp Downloader.cpp Roads.cpp RoadUtils.cpp oclDijkstraKernel.cpp
OBJ := ${CXX_SRCS:.cpp=.o}

default: $(O_OBJ:.o=.$(OBJ_EXT))

clean:
	rm -f *.o 

