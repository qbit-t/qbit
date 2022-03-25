#ifndef VERSION_H
#define VERSION_H

static const int PROTOCOL_VERSION = 0xb1;
static const int QBIT_VERSION = ((((QBIT_VERSION_MAJOR << 16) + QBIT_VERSION_MINOR) << 8) + QBIT_VERSION_REVISION);

#define UNPACK_MAJOR(v) v >> 24
#define UNPACK_MINOR(v) ((v >> 8) << 16) >> 16
#define UNPACK_REVISION(v) (v << 24) >> 24

#endif // VERSION_H
