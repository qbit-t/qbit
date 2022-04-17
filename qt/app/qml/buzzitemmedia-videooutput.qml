import QtQuick 2.15
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.15
import QtMultimedia 5.15

VideoOutput {
	id: videoOut

	property var previewImage: null;
	property var player: null;

	signal linkActivated();

	anchors.fill: parent

	onContentRectChanged: {
	}

	layer.enabled: buzzerApp.isDesktop
	layer.effect: OpacityMask {
		id: roundEffect
		maskSource: Item {
			width: roundEffect.getWidth()
			height: roundEffect.getHeight()

			Rectangle {
				x: roundEffect.getX()
				y: roundEffect.getY()
				width: roundEffect.getWidth()
				height: roundEffect.getHeight()
				radius: 8
			}
		}

		function getX() {
			return 0;
		}

		function getY() {
			return 0;
		}

		function getWidth() {
			return previewImage.width;
		}

		function getHeight() {
			return previewImage.height;
		}
	}

	MouseArea {
		id: linkClick
		x: 0
		y: 0
		width: videoOut.width
		height: videoOut.height
		enabled: true
		cursorShape: Qt.PointingHandCursor

		function clickActivated() {
			linkActivated();
		}

		onClicked: {
			//
			if (!buzzerApp.isDesktop) {
				linkClick.clickActivated();
			}
		}

		ItemDelegate {
			id: linkClicked
			x: 0
			y: 0
			width: videoOut.width
			height: videoOut.height
			enabled: buzzerApp.isDesktop

			onClicked: {
				linkClick.clickActivated();
			}
		}
	}
}
