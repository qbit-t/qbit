import QtQuick 2.15
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.15
import QtMultimedia 5.15

import app.buzzer.components 1.0 as BuzzerComponents

VideoOutput {
	id: videoOut

	property var previewImage: null;
	property var player: null;
	property bool allowClick: true
	property var pixelFormat: null

	signal linkActivated();

	anchors.fill: parent

	Component.onCompleted: {
		if (player && !pushed) { player.pushSurface(videoSurface); pushed = true; }
		else {
			console.log("[VideoOutput/Component.onCompleted]: player is NULL");
		}
	}

	property bool pushed: false

	onPlayerChanged: {
		if (player && !pushed) { player.pushSurface(videoSurface); pushed = true; }
	}

	MouseArea {
		id: linkClick
		x: 0
		y: 0
		width: videoOut.width
		height: videoOut.height
		enabled: allowClick
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
	}
}
