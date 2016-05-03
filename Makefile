all: iostream swig;

iostream: python/iostream.py python/_iostream.so;
python/iostream.py: swig/iostream.py; @cp swig/iostream.py python/iostream.py
swig/iostream.py swig/iostream_wrap.cxx: swig/iostream.i include/iostream; swig -Wall -python -c++ swig/iostream.i
python/_iostream.so: include/debug include/iostream include/alloc swig/iostream_wrap.cxx
	g++-4.8 -Wall -fPIC -shared -O3 -g -std=c++11 -I/usr/include/python2.7 -o python/_iostream.so swig/iostream_wrap.cxx

swig: python/swig.py python/_swig.so;
python/swig.py: swig/swig.py; @cp swig/swig.py python/swig.py
swig/swig.py swig/swig_wrap.cxx: swig/swig.i include/swig; swig -Wall -python -c++ swig/swig.i
python/_swig.so: include/swig swig/swig_wrap.cxx
	g++-4.8 -Wall -fPIC -shared -O3 -g -std=c++11 -I/usr/include/python2.7 -o python/_swig.so swig/swig_wrap.cxx


clean:; rm -rf swig/*.o swig/*.py swig/*_wrap.cxx python/iostream.py python/_iostream.so
