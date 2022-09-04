#ifndef MACXUTILS_H
#define MACXUTILS_H

#include <QString>
#include <QQuickWindow>

class MacXUtils {
public:
	static void setStatusBarColor(QQuickWindow* window, QString color);
};

#endif // MACXUTILS_H
