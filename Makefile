CC = g++
CFLAGS = -I ./boost_1_58_0 
LDFLAGS = -L./boost_1_58_0/build/lib
LBOOSTFLAGS = -lboost_system -lboost_date_time

router: fileio router.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -o router router.cpp $(LBOOSTFLAGS)

fileio: fileio.cpp
	$(CC) -o fileio fileio.cpp

clean:
	rm -rf router fileio details*.txt

