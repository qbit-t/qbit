#ifndef QUARKROUNDFRAME_H
#define QUARKROUNDFRAME_H

#include <QPainter>
#include <QQuickPaintedItem>
#include <QRegion>
#include <qqml.h>

namespace buzzer {

class QuarkRoundFrame : public QQuickPaintedItem
{
    Q_OBJECT
	Q_DISABLE_COPY(QuarkRoundFrame)

public:
	QuarkRoundFrame()
    {
        // Important, otherwise the paint method is never called
        this->setFlag(ItemHasContents, true);
    }

    void paint(QPainter *painter)
    {
		QRegion r1(QRect(x(), y(), width(), height()),    // r1: elliptic region
				   QRegion::Ellipse);
		//QRegion r2(x(), y(), width(), height());    // r2: rectangular region
		//QRegion r3 = r1.subtracted(r2);        // r3: substraction

		painter->setClipRegion(r1);

		qInfo() << "[QuarkRoundFrame]: clipped";
    }

public:
    static void declare()
    {
		qmlRegisterType<buzzer::QuarkRoundFrame>("app.buzzer.components", 1, 0, "RoundFrame");
    }
};

} // buzzer

#endif // QUARKROUNDFRAME_H
