all: perl.so luaperl_dummy.so

perl.so: perl.o
	gcc -g -o perl.so -shared perl.o -llua -lm -ldl `perl -MExtUtils::Embed -e ldopts`

perl.o: perl.c
	gcc -g -c -fPIC perl.c `perl -MExtUtils::Embed -e ccopts`

luaperl_dummy.so:
	gcc -o luaperl_dummy.so -shared `perl -MExtUtils::Embed -e ldopts`
clean:
	rm -f *.o *.so
test:
	lua -lperl -e 'print(package.loaded.perl)'
