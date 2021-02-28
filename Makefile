CXX = g++
CPPFLAGS = -std=c++14 -MMD -MP -g
PROTOBUF = `pkg-config --cflags protobuf`
PROTOBUFL = `pkg-config --libs protobuf`
LD_FLAGS = -L/usr/local/lib
ROOTCONFIG =
ROOTCONFIG2 = `root-config --cflags --glibs`
BLASDIR = /usr/include/openblas
BLASFLAG = -L${BLASDIR} -lopenblas
SRC = ${wildcard *.cxx}

prototype: ${SRC:%.cxx=%.o}
	${CXX} -o prototype $^ ${CPPFLAGS} $(BLASFLAG) $(ROOTCONFIG) $(PROTOBUFL) $(LD_FLAGS)


-include $(SRC:%.cxx=%.d)

%.o: %.cxx
	${CXX} ${CPPFLAGS} -c $< `root-config --cflags` $(PROTOBUF)

.phony: clean
clean:
	-rm *.d
	-rm *.o
