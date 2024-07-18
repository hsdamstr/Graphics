
.PHONY: all
all: cmake
	${MAKE} -j 3 --quiet -C ..


.PHONY: cmake
cmake:
	cd .. && cmake .


%: cmake
	${MAKE} -j 3 --quiet -C .. ${@}

Makefile:
	
