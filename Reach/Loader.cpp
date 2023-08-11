#include "Reach.h"

//Example on How To Use

int main() {
	timeBeginPeriod(1);
	std::thread(ReachScanner).detach();

	while (true) {
		ChangeReach(600, 600, 100, false);
	}
}