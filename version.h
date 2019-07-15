#ifndef VERSION_H
#define VERSION_H

static const int PROTOCOL_VERSION = 0xa1;

static const int CLIENT_VERSION =
						   1000000 * CLIENT_VERSION_MAJOR
						 +   10000 * CLIENT_VERSION_MINOR
						 +     100 * CLIENT_VERSION_REVISION
						 +       1 * CLIENT_VERSION_BUILD;

#endif // VERSION_H
