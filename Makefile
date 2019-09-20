

all: a

a: main.cpp systime.c
	gcc -llibgdi32.a -llibuser32.a -lkernel32.a -lcomctl32.a $^ -o $@

m: metric.c
	gcc -DmatridTest=main $^ -o $@
	$@

