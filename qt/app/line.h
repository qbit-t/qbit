#ifndef QUARKLINE_H
#define QUARKLINE_H

#include <QPainter>
#include <QQuickPaintedItem>
#include <qqml.h>

namespace buzzer {

class QuarkLine : public QQuickPaintedItem
{
    Q_OBJECT
    Q_DISABLE_COPY(QuarkLine)

    Q_PROPERTY(int x1 READ x1 WRITE setX1 NOTIFY x1Changed)
    Q_PROPERTY(int y1 READ y1 WRITE setY1 NOTIFY y1Changed)
    Q_PROPERTY(int x2 READ x2 WRITE setX2 NOTIFY x2Changed)
    Q_PROPERTY(int y2 READ y2 WRITE setY2 NOTIFY y2Changed)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(int penWidth READ penWidth WRITE setPenWidth NOTIFY penWidthChanged)
	Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)

public:
    QuarkLine() : m_x1(0), m_y1(0), m_x2(0), m_y2(0), m_color(Qt::black), m_penWidth(1)
    {
        // Important, otherwise the paint method is never called
        this->setFlag(ItemHasContents, true);
    }

    void paint(QPainter *painter)
    {
		if (m_visible) {
            QPen pen(m_color, m_penWidth);
            painter->setPen(pen);
            if(smooth() == true)
            {
                painter->setRenderHint(QPainter::Antialiasing, true);
            }
            int x = qMin(m_x1, m_x2) - m_penWidth/2;
            int y = qMin(m_y1, m_y2) - m_penWidth/2;
            painter->drawLine(m_x1 - x, m_y1 - y, m_x2 - x, m_y2 - y);
        }
    }

    // Get methods
    int x1() const { return m_x1; }
    int y1() const { return m_y1; }
    int x2() const { return m_x2; }
    int y2() const { return m_y2; }
    QColor color() const { return m_color; }
    int penWidth() const { return m_penWidth; }
	bool visible() const { return m_visible; }

    // Set methods
    void setX1(int x1)
    {
        if(m_x1 == x1) return;
        m_x1 = x1;
        updateSize();
        emit x1Changed();
        update();
    }
    void setY1(int y1)
    {
        if(m_y1 == y1) return;
        m_y1 = y1;
        updateSize();
        emit y1Changed();
        update();
    }
    void setX2(int x2)
    {
        if(m_x2 == x2) return;
        m_x2 = x2;
        updateSize();
        emit x2Changed();
        update();
    }
    void setY2(int y2)
    {
        if(m_y2 == y2) return;
        m_y2 = y2;
        updateSize();
        emit x2Changed();
        update();
    }
    void setColor(const QColor &color)
    {
        if(m_color == color) return;
        m_color = color;
        emit colorChanged();
        update();
    }
    void setPenWidth(int newWidth)
    {
        if(m_penWidth == newWidth) return;
        m_penWidth = newWidth;
        updateSize();
        emit penWidthChanged();
        update();
    }
	void setVisible(bool visible)
	{
		if(m_visible == visible) return;
		m_visible = visible;
		emit visibleChanged();
		update();
	}

signals:
    void x1Changed();
    void y1Changed();
    void x2Changed();
    void y2Changed();
    void colorChanged();
    void penWidthChanged();
	void visibleChanged();

protected:
    void updateSize()
    {
        setX(qMin(m_x1, m_x2) - m_penWidth/2);
        setY(qMin(m_y1, m_y2) - m_penWidth/2);
        setWidth(qAbs(m_x2 - m_x1) + m_penWidth);
        setHeight(qAbs(m_y2 - m_y1) + m_penWidth);
    }

protected:
    int m_x1;
    int m_y1;
    int m_x2;
    int m_y2;
    QColor m_color;
    int m_penWidth;
	bool m_visible = true;

public:
    static void declare()
    {
        qmlRegisterType<buzzer::QuarkLine>("app.buzzer.components", 1, 0, "Line");
    }
};

} // buzzer

#endif // QUARKLINE_H
