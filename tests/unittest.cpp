#include "unittest.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>

#if defined(__linux__)
#include <sys/time.h>
#endif

#include <iostream>

#define longlong_t long long

longlong_t _get_time()
{
#ifdef WIN32
	FILETIME ft;
	ULARGE_INTEGER ui;

	GetSystemTimeAsFileTime(&ft);
	ui.LowPart=ft.dwLowDateTime;
	ui.HighPart=ft.dwHighDateTime;

	return ui.QuadPart / 10000;
#elif defined(__linux__)
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	return now.tv_sec * 1000 + now.tv_nsec/1000000;
#endif
	return 0;
}

using namespace qbit;
using namespace tests;

void TestSuit::execute() {

	for(std::list<Unit*>::iterator lUnit = tests_.begin(); lUnit != tests_.end(); lUnit++) {

		std::cout << (*lUnit)->name_ << ": ";

		longlong_t lBeginTime = _get_time();		
		try {
			if (!(*lUnit)->execute()) (*lUnit)->state_ = UnitState::ERROR;
			else (*lUnit)->state_ = UnitState::SUCCESS;
		}
		catch(std::exception& ex) {
			(*lUnit)->state_ = UnitState::ERROR;
			(*lUnit)->error_ = ex.what();
		}
		catch(...) {
			(*lUnit)->state_ = UnitState::ERROR;
			(*lUnit)->error_ = "Undefined error";
		}
		longlong_t lEndTime = _get_time();
		(*lUnit)->time_ = lEndTime-lBeginTime;

		if ((*lUnit)->state_ == UnitState::SUCCESS) std::cout << "OK / "; 
		else if ((*lUnit)->state_ == UnitState::ERROR) std::cout << "ERROR (" << (*lUnit)->error_ << ") / "; 

		std::cout << (*lUnit)->time_ << "ms" << std::endl; 
	}
} 