import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Dialogs 1.1
import QtGraphicalEffects 1.0
import Qt.labs.folderlistmodel 2.11
import QtMultimedia 5.8

import app.buzzer.components 1.0 as BuzzerComponents
import app.buzzer.commands 1.0 as BuzzerCommands

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

Item
{
	id: peerslist_

	property var peersModel;
	property var controller;

	x: 0
	y: 0
	width: parent.width
	height: parent.height

	onPeersModelChanged: {
		list.model = peersModel;
	}

	function disableAnmation() {
		if (list) {
			if (list.add) list.add.enabled = false;
			if (list.remove) list.remove.enabled = false;
			if (list.displaced) list.displaced.enabled = false;
		}
	}

	function enableAnmation() {
		if (list.add) list.add.enabled = true;
		if (list.remove) list.remove.enabled = true;
		if (list.displaced) list.displaced.enabled = true;
		adjustData();
	}

	function stop() {
		pollTimer.running = false;
		pollTimer.stop();
	}

	Timer {
		id: pollTimer
		interval: 2000
		repeat: true
		running: true

		onTriggered: {
			peersModel.update();
		}
	}

	QuarkListView {
		id: list
		focus: true
		currentIndex: -1
		x: 0
		y: 0

		width: parent.width
		height: parent.height
		clip: true

		property int fontPointSize: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 12 : 15;

		onWidthChanged: {
		}

		add: Transition {
			NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 400 }
			NumberAnimation { property: "height"; from: 0; /*to: list.totalItemHeight;*/ duration: 400 }
		}

		remove: Transition {
			NumberAnimation { property: "opacity"; from: 1.0; to: 0; duration: 400 }
			NumberAnimation { property: "height"; /*from: list.totalItemHeight;*/ to: 0; duration: 400 }
		}

		displaced: Transition {
			NumberAnimation { properties: "x,y"; duration: 400; easing.type: Easing.OutBounce }
		}

		onFeedReady: {
		}

		delegate: ItemDelegate {
			id: itemDelegate

			property var peerItem;

			Component.onCompleted: {
				var lSource = "qrc:/qml/peeritem.qml";
				if (status === buzzerClient.peer_BANNED() ||
						status === buzzerClient.peer_POSTPONED() ||
						status === buzzerClient.peer_PENDING_STATE() ||
						status === buzzerClient.peer_UNDEFINED()) {
					lSource = "qrc:/qml/peerinactiveitem.qml";
				}

				var lComponent = Qt.createComponent(lSource);
				if (lComponent.status === Component.Error) {
					controller.showError(lComponent.errorString());
				} else {
					peerItem = lComponent.createObject(itemDelegate);

					peerItem.width = list.width;
					peerItem.controller_ = controller;
					peerItem.listView_ = list;

					itemDelegate.height = peerItem.calculateHeight();
					itemDelegate.width = list.width;

					peerItem.calculatedHeightModified.connect(itemDelegate.calculatedHeightModified);
				}
			}

			function calculatedHeightModified(value) {
				itemDelegate.height = value;
			}
		}
	}
}

