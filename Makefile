CXX = g++
CPPFLAGS = -std=c++14 -MMD -MP -g
PROTOBUF = `pkg-config --cflags protobuf`
PROTOBUFL = `pkg-config --libs protobuf`
ROOTCONFIG =
ROOTCONFIG2 = `root-config --cflags --glibs`
BLASDIR = /usr/local/opt/openblas/lib
BLASFLAG = -L${BLASDIR} -lopenblas
SRC = ${wildcard *.cxx}

prototype: ${SRC:%.cxx=%.o}
	${CXX} -o prototype $^ ${CPPFLAGS} $(BLASFLAG) $(ROOTCONFIG) $(PROTOBUFL)


-include $(SRC:%.cxx=%.d)

%.o: %.cxx
	${CXX} ${CPPFLAGS} -c $< `root-config --cflags` $(PROTOBUF)

.phony: clean
clean:
	-rm *.d
	-rm *.o
