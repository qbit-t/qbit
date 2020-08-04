#include "leveldb.h"

#ifndef MOBILE_PLATFORM
void qbit::db::LevelDBLogger::Logv(const char * format, va_list ap) {
	if (!gLog().isEnabled(Log::DB)) {
		return;
	}

	char buffer[500];
	for (int iter = 0; iter < 2; iter++) {
		char* base;
		int bufsize;
		if (iter == 0) {
			bufsize = sizeof(buffer);
			base = buffer;
		}
		else {
			bufsize = 30000;
			base = new char[bufsize];
		}
		char* p = base;
		char* limit = base + bufsize;

		// Print the message
		if (p < limit) {
			va_list backup_ap;
			va_copy(backup_ap, ap);
			p += vsnprintf(p, limit - p, format, backup_ap);
			va_end(backup_ap);
		}

		// Truncate to available space if necessary
		if (p >= limit) {
			if (iter == 0) {
				continue;
			}
			else {
				p = limit - 1;
			}
		}

		// Add newline if necessary
		if (p == base || p[-1] != '\n') {
			*p++ = '\n';
		}

		assert(p <= limit);
		base[std::min(bufsize - 1, (int)(p - base))] = '\0';
		
		std::string lMsg = std::string("[leveldb]: ") + std::string(base);
		gLog().write(Log::DB, lMsg);
		
		if (base != buffer) {
			delete[] base;
		}
		break;
	}
}
#endif
