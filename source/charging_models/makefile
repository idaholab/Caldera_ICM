CXXFLAGS = -g -fPIC -std=c++11
OBJ = datatypes_global_SE_EV_definitions.o datatypes_global.o


libGlobal.so: $(OBJ)
	g++ -shared -o libGlobal.so $(OBJ)

clean:
	rm -f *.o libGlobal.so
    
#install: libcaldera.so
#	cp libcaldera.so /usr/local/lib
#	mkdir -p /usr/local/include/caldera
#	cp *.h /usr/local/include/caldera
#	ldconfig
	
#uninstall:
#	rm -f /usr/local/lib/libcaldera.so
#	rm -rf /usr/local/include/caldera
#	ldconfig
	


