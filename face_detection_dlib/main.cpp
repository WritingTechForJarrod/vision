#include "stdafx.h"
#include <ctime>
#include <vector>
#include <string>

int main() {
	std::vector<double> history;
	const double p(CLOCKS_PER_SEC);
	const int n(2);

	history.push_back(std::clock());
	std::cout << "Face detector initializing" << std::endl;
	history.push_back(std::clock());
	init();
	history.push_back(std::clock());
	std::cout << "Initialized in " << (history[2] - history[1])/p << " seconds" << std::endl;
	history.push_back(std::clock());
	for (int i(0); i < n; ++i)
		std::cout << detect() << " faces detected";
	history.push_back(std::clock());
	std::cout << "Face detector took " << (history[2+n] - history[2])/n/p << std::endl;
	std::string s;
	std::cin >> s;
	std::cout << "You typed " << s;
	return 0;
}