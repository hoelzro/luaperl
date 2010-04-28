perl.so: perl.o
	gcc -g -o perl.so -shared perl.o -llua -lm -ldl `perl -MExtUtils::Embed -e ldopts`

perl.o: perl.c
	gcc -g -c -fPIC perl.c `perl -MExtUtils::Embed -e ccopts`
clean:
	rm -f *.o *.so
test:
	lua -lperl -e 'print(package.loaded.perl)'
