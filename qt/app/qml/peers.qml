﻿import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Dialogs 1.1

import app.buzzer.components 1.0 as BuzzerComponents
import app.buzzer.commands 1.0 as BuzzerCommands

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

QuarkPage
{
	id: peers_
	key: "peers"

	readonly property int spaceLeft_: 15
	readonly property int spaceTop_: 12
	readonly property int spaceRight_: 15
	readonly property int spaceBottom_: 12
	readonly property int spaceAvatarBuzz_: 10
	readonly property int spaceItems_: 5
	readonly property int spaceMedia_: 10
	readonly property int spaceSingleMedia_: 8
	readonly property int spaceMediaIndicator_: 15
	readonly property int spaceHeader_: 5
	readonly property int spaceRightMenu_: 15
	readonly property int spaceStats_: -5
	readonly property int spaceLine_: 4
	readonly property int spaceThreaded_: 33
	readonly property int spaceThreadedItems_: 4

	property var activePeersModel;
	property var networkPeersModel;
	property var manualPeersModel;

	Component.onCompleted: {
		buzzerApp.lockPortraitOrientation();
		closePageHandler = closePage;
		activatePageHandler = activatePage;

		waitDataTimer.start();
	}

	function closePage() {
		//
		buzzerApp.unlockOrientation();
		//
		stopPage();
		destroy(1000);
		controller.popPage();
	}

	function stopPage() {
		peersActive.stop();
		peersNetwork.stop();
		peersManual.stop();

		activePeersModel = undefined;
		networkPeersModel = undefined;
		manualPeersModel = undefined;
	}

	function activatePage() {
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
		toolBar.activate();
	}

	function onErrorCallback(error) {
		controller.showError(error);
	}

	Timer {
		id: waitDataTimer
		interval: 1000
		repeat: false
		running: false

		onTriggered: {
			activePeersModel = buzzerClient.getPeersActive();
			activePeersModel.feed(true);

			networkPeersModel = buzzerClient.getPeersAll();
			networkPeersModel.feed(true);

			manualPeersModel = buzzerClient.getPeersAdded();
			manualPeersModel.feed(true);
		}
	}

	//
	// toolbar
	//

	QuarkToolBar {
		id: toolBar
		height: 45
		width: parent.width

		property int totalHeight: height

		Component.onCompleted: {
		}

		QuarkToolButton	{
			id: cancelButton
			Material.background: "transparent"
			visible: true
			labelYOffset: 3
			symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.leftArrowSym

			onClicked: {
				closePage();
			}
		}

		QuarkLabel {
			id: buzzerControl
			x: cancelButton.x + cancelButton.width + 5
			y: parent.height / 2 - height / 2
			width: parent.width - (x)
			elide: Text.ElideRight
			text: "Peers"
			font.pointSize: 18
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link")
		}

		QuarkHLine {
			id: bottomLine
			x1: 0
			y1: parent.height
			x2: parent.width
			y2: parent.height
			penWidth: 1
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
			visible: true
		}
	}

	TabBar {
		id: peersBar
		x: 0
		y: toolBar.y + toolBar.height
		width: parent.width
		currentIndex: 0
		position: TabBar.Header

		TabButton {
			QuarkLabel {
				x: parent.width / 2 - width / 2
				y: parent.height / 2 - height / 2
				text: buzzerApp.getLocalization(buzzerClient.locale, "Peers.active")
			}
		}
		TabButton {
			QuarkLabel {
				x: parent.width / 2 - width / 2
				y: parent.height / 2 - height / 2
				text: buzzerApp.getLocalization(buzzerClient.locale, "Peers.network")
			}
		}
		TabButton {
			QuarkLabel {
				x: parent.width / 2 - width / 2
				y: parent.height / 2 - height / 2
				text: buzzerApp.getLocalization(buzzerClient.locale, "Peers.manual")
			}
		}

		onCurrentIndexChanged: {
		}

		Component.onCompleted: {
		}

		Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Market.tabActive");
		Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Market.tabInactive");
	}

	StackLayout {
		id: peerPages
		currentIndex: peersBar.currentIndex

		PeersList {
			id: peersActive
			x: 0
			y: peersBar.y + peersBar.height
			width: peers_.width
			height: peers_.height - (peersBar.y + 50)

			peersModel: activePeersModel
			controller: peers_.controller

			Component.onCompleted: {
			}
		}
		PeersList {
			id: peersNetwork
			x: 0
			y: peersBar.y + peersBar.height
			width: peers_.width
			height: peers_.height - (peersBar.y + 50)

			peersModel: networkPeersModel
			controller: peers_.controller

			Component.onCompleted: {
			}
		}
		PeersManualList {
			id: peersManual
			x: 0
			y: peersBar.y + peersBar.height
			width: peers_.width
			height: peers_.height - (peersBar.y + 50)

			peersModel: manualPeersModel
			controller: peers_.controller

			Component.onCompleted: {
			}
		}
	}
}