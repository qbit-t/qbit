// QuarkHLine.qml

import QtQuick 2.15
import QtQuick.Shapes 1.15

Shape {
	property real x1: 0
	property real y1: 0
	property real x2: 0
	property real y2: 0
	property real penWidth: 1
	property color color: "white"

	asynchronous: true

	ShapePath {
		strokeWidth: penWidth
		strokeColor: color
		startX: x1
		startY: y1
		PathLine { x: x2; y: y2 }
	}
}

