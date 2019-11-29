

all: a

a: main.cpp systime.c
	gcc -llibgdi32.a -llibuser32.a -lkernel32.a -lcomctl32.a $^ -o $@

m: metric.c
	gcc -DmatridTest=main $^ -o $@
	$@

sym: func.sym
casm.sym: bin/Debug/fwmonitor.exe
	objdump.exe -S $^ > $@

#use sed -e 's/<_Z[0-9]\+//g'	to delete mingW64's function prefix _Z[0-9]\+
#use sed -e 's/<_ZL[0-9]\+//g'	to delete mingW64's local function prefix _ZL[0-9]\+ 
#use sed -e 's/<_Z[L]\?[0-9]\+//g'	to delete both of above 2 cases.
#sed also use [[:digit:]] as [0-9]
func.sym: casm.sym
	grep "<_.*>:" casm.sym |sed -e '/<__fu.*__/d' -e '/<__GLOBAL__I_.*/d' -e 's/<_Z[L]\?[0-9]\+//g' -e 's/<_//g' -e 's/>://g' | sort >func.sym

