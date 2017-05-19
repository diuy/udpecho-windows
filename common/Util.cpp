#include "Util.h"
#include <time.h>
#include <sstream>

using namespace std;

std::string nowTimeStr() {
	char str[255];
	time_t t = time(NULL);
	tm* t2;
	t2 = localtime(&t);
	strftime(str, sizeof(str), "%Y-%m-%d %H:%M:%S", t2);
	return std::string(str);
}

std::string nowDateStr() {
	char str[255];
	time_t t = time(NULL);
	tm* t2;
	t2 = localtime(&t);
	strftime(str, sizeof(str), "%Y%m%d", t2);
	return std::string(str);
}


int StringToInt(const char * str, int defaultValue) {
	istringstream is(str);
	int k = defaultValue;
	is >> k;
	return k;
}