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
	id: peersmanuallist_

	property var peersModel;
	property var controller;

	x: 0
	y: 0
	width: parent.width
	height: parent.height

	readonly property int spaceLeft_: 15
	readonly property int spaceTop_: 12
	readonly property int spaceRight_: 15
	readonly property int spaceBottom_: 12
	readonly property int spaceAvatarBuzz_: 10
	readonly property int spaceItems_: 7
	readonly property int spaceMedia_: 10
	readonly property int spaceMediaIndicator_: 15
	readonly property int spaceHeader_: 5
	readonly property int spaceRightMenu_: 15
	readonly property int spaceStats_: -5
	readonly property int spaceLine_: 4

	property var menuHighlightColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.highlight")
	property var menuBackgroundColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.background")
	property var menuForegroundColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")

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
		pollTimer.stop();
	}

	Timer {
		id: pollTimer
		interval: 2000
		repeat: true
		running: false

		onTriggered: {
			// peersModel.update();
		}
	}

	QuarkEditBox {
		id: peerEditBox
		x: spaceLeft_
		y: spaceTop_
		width: parent.width - (spaceLeft_ + spaceTop_)
		symbol: Fonts.globeSym
		clipboardButton: false
		helpButton: true
		addButton: true
		placeholderText: buzzerApp.getLocalization(buzzerClient.locale, "Peers.manual.add")

		onTextChanged: {
		}

		onAddClicked: {
			//
			if (text !== "" && text.indexOf(":") >= 0) {
				//
				if (buzzerClient.addPeerExplicit(text)) {
					peersModel.update();
					text = "";
				}
			}
		}

		onHelpClicked: {
			var lMsgs = [];
			lMsgs.push(buzzerApp.getLocalization(buzzerClient.locale, "Peers.manual.add.help"));
			infoDialog.show(lMsgs);
		}

		onHeadClicked: {
		}
	}

	Rectangle {
		id: backRect
		x: spaceLeft_ + 1
		y: peerEditBox.y + peerEditBox.height
		height: parent.height - (y + spaceBottom_)
		width: parent.width - (spaceLeft_ + spaceRight_)
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.background");
	}

	QuarkListView {
		id: list
		focus: true
		currentIndex: -1
		x: backRect.x
		y: backRect.y

		width: backRect.width
		height: backRect.height
		clip: true

		property int fontPointSize: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 11 : 15;

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

		delegate: SwipeDelegate
		{
			id: listDelegate
			width: list.width
			height: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 30 : 40
			leftPadding: 0
			rightPadding: 0
			topPadding: 0
			bottomPadding: 0

			onClicked: {
				//
				swipe.open(SwipeDelegate.Right);
			}

			Binding {
				target: background
				property: "color"
				value: listDelegate.highlighted ?
						   menuHighlightColor:
						   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.background");
			}

			contentItem: Rectangle	{
				id: backgroundPane
				height: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 30 : 40
				width: list.width
				color: "transparent"
				QuarkText {
					text: endpoint
					x: 10
					y: parent.height / 2 - height / 2
					font.pointSize: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 12 : 16
				}
			}

			swipe.right: Rectangle {
				id: swipeRect
				width: listDelegate.height
				height: listDelegate.height
				color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.downBackground")

				QuarkSymbolLabel {
					symbol: Fonts.trashSym
					font.pointSize: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 12 : 16
					color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")

					x: parent.width / 2 - width / 2
					y: parent.height / 2 - height / 2
				}

				MouseArea {
					anchors.fill: swipeRect
					onPressed: {
						swipeRect.color = Qt.darker(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.downBackground"), 1.1);
					}
					onReleased: {
						swipeRect.color = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.downBackground");
					}
					onClicked: {
						listDelegate.swipe.close();
						buzzerClient.removePeer(endpoint); // remove from backend
						peersModel.remove(index); // trick
					}
				}

				anchors.right: parent.right
			}
		}
	}

	InfoDialog {
		id: infoDialog
		bottom: 50
	}
}

