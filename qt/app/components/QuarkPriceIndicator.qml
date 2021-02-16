// QuarkPriceIndicator.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4

Canvas
{
    id: triangle
    x: 0
    y: 0
    width: 8
    height: 12
    contextType: "2d"
    property string color: "";

    onPaint:
    {
        context.reset();
        context.moveTo(width, 0);
        context.lineTo(width, height);
        context.lineTo(0, height/2);
        context.closePath();
        context.fillStyle = color;
        context.fill();
    }
}

