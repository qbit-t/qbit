// QuarkRoundRectangle.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1

import QtQuick 2.0
import QtQml 2.2

Item {
	Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
    Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
    Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
    Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
    Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

	id: root

	property int radius: 10
	property bool topLeftCorner: true
	property bool topRightCorner: true
	property bool bottomRightCorner: true
	property bool bottomLeftCorner: true
	property color color: "black"
	property color backgroundColor: "transparent"
	property int penWidth: 1

	onTopLeftCornerChanged: draw.requestPaint()
	onTopRightCornerChanged: draw.requestPaint()
	onBottomRightCornerChanged: draw.requestPaint()
	onBottomLeftCornerChanged: draw.requestPaint()
	onColorChanged: draw.requestPaint()
	onRadiusChanged: draw.requestPaint()

	Canvas {
		id: draw
		anchors.fill: parent

		onPaint: {
			var lContext = getContext("2d");
			lContext.reset();
			lContext.beginPath();

			//Start position
			lContext.moveTo(0, height / 2)

			//topLeftCorner
			if(topLeftCorner){
				lContext.lineTo(0, radius);
				lContext.arcTo(0, 0, radius, 0, radius);
			} else {
				lContext.lineTo(0, 0);
			}

			//topRightCorner
			if(topRightCorner){
				lContext.lineTo(width - radius, 0);
				lContext.arcTo(width, 0, width, radius, radius);
			} else {
				lContext.lineTo(width, 0);
			}

			//bottomRightCorner
			if(bottomRightCorner) {
				lContext.lineTo(width, height-radius);
				lContext.arcTo(width, height, width - radius, height, radius);
			} else {
				lContext.lineTo(width, height);
			}

			//bottomLeftCorner
			if(bottomLeftCorner) {
				lContext.lineTo(radius, height);
				lContext.arcTo(0, height, 0, height - radius, radius);
			} else {
				lContext.lineTo(0, height);
			}

			//Close path
			lContext.lineTo(height / 2);
			lContext.closePath();

			//Draw border
			lContext.lineWidth = penWidth;
			lContext.strokeStyle = color;
			lContext.stroke();

			//Draw background
			lContext.fillStyle = backgroundColor;
			lContext.fill();
		}
	}
}
