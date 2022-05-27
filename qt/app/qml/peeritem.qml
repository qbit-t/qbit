import QtQuick 2.15
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

import "qrc:/lib/numberFunctions.js" as NumberFunctions

Item {
	id: peeritem_

	property int calculatedHeight: calculateHeightInternal()

	property var peerId_: peerId;
	property var endpoint_: endpoint;
	property var status_: status;
	property var time_: time;
	property var localDateTime_: localDateTime;
	property bool outbound_: outbound;
	property var latency_: latency;
	property var roles_: roles;
	property var address_: address;
	property var inQueue_: inQueue;
	property var outQueue_: outQueue;
	property var pendingQueue_: pendingQueue;
	property var receivedCount_: receivedCount;
	property var receivedBytes_: receivedBytes;
	property var sentCount_: sentCount;
	property var sentBytes_: sentBytes;
	property var dapps_: dapps;
	property var chains_: chains;

	property var controller_: controller
	property var listView_

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

	signal calculatedHeightModified(var value);

	onCalculatedHeightChanged: {
		calculatedHeightModified(calculatedHeight);
	}

	function calculateHeightInternal() {
		return bottomLine.y1;
	}

	function calculateHeight() {
		calculatedHeight = calculateHeightInternal();
		return calculatedHeight;
	}

	//
	// item
	//

	// endpoint

	QuarkSymbolLabel {
		id: serverSymbol
		x: spaceLeft_
		y: spaceTop_
		font.pointSize: listView_.fontPointSize + 3
		symbol: Fonts.serverSym
		color: getColor()

		function getColor() {
			if (status_ === buzzerClient.peer_ACTIVE()) {
				return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.peer.active");
			} else if (status_ === buzzerClient.peer_PENDING_STATE() || status_ === buzzerClient.peer_POSTPONED() ||
					   status_ === buzzerClient.peer_QUARANTINE()) {
				return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.peer.pending");
			}

			return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.peer.banned");
		}
	}

	QuarkLabelRegular {
		id: endpointLabel
		x: serverSymbol.x + serverSymbol.width + spaceItems_
		y: serverSymbol.y + serverSymbol.height / 2 - height / 2
		text: endpoint_
		font.pointSize: listView_.fontPointSize + 3
		elide: Text.ElideRight
		width: parent.width - (x + spaceRight_)
	}

	QuarkVLine {
		id: vLine
		x1: serverSymbol.x + serverSymbol.width / 2
		y1: serverSymbol.y + serverSymbol.height + 2
		x2: x1
		y2: timeSymbol.y + timeSymbol.height / 2
		penWidth: 1
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledLine")
	}

	QuarkHLine {
		id: hLine
		x1: serverSymbol.x + serverSymbol.width / 2
		y1: timeSymbol.y + timeSymbol.height / 2
		x2: x1 + spaceLeft_
		y2: y1
		penWidth: 1
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledLine")
	}

	// time

	QuarkSymbolLabel {
		id: timeSymbol
		x: hLine.x2 + spaceItems_
		y: endpointLabel.y + endpointLabel.height + spaceItems_
		font.pointSize: listView_.fontPointSize
		symbol: Fonts.clockSym
	}

	QuarkLabelRegular {
		id: timeLabel
		x: timeSymbol.x + timeSymbol.width + spaceItems_
		y: timeSymbol.y + timeSymbol.height / 2 - height / 2
		text: localDateTime_ + " - " + latency_ + " ms"
		font.pointSize: listView_.fontPointSize
		elide: Text.ElideRight
		width: parent.width - (x + spaceRight_)
	}

	// address

	QuarkSymbolLabel {
		id: addressSymbol
		x: spaceLeft_
		y: timeSymbol.y + timeSymbol.height + spaceItems_
		font.pointSize: listView_.fontPointSize
		symbol: Fonts.tagSym
	}

	QuarkLabelRegular {
		id: addressLabel
		x: addressSymbol.x + addressSymbol.width + spaceItems_
		y: addressSymbol.y + addressSymbol.height / 2 - height / 2
		text: address_
		font.pointSize: listView_.fontPointSize
		elide: Text.ElideRight
		width: parent.width - (x + spaceRight_)
	}

	// id (peer id)

	QuarkSymbolLabel {
		id: peerIdSymbol
		x: spaceLeft_
		y: addressSymbol.y + addressSymbol.height + spaceItems_
		font.pointSize: listView_.fontPointSize
		symbol: Fonts.passportSym
	}

	QuarkLabelRegular {
		id: peerIdLabel
		x: addressSymbol.x + addressSymbol.width + spaceItems_
		y: peerIdSymbol.y + peerIdSymbol.height / 2 - height / 2
		text: peerId_
		font.pointSize: listView_.fontPointSize
		elide: Text.ElideRight
		width: parent.width - (x + spaceRight_)
	}

	// roles

	QuarkSymbolLabel {
		id: rolesSymbol
		x: spaceLeft_
		y: peerIdSymbol.y + peerIdSymbol.height + spaceItems_
		font.pointSize: listView_.fontPointSize
		symbol: Fonts.userTagSym
	}

	Rectangle {
		id: rolesFrame
		x: rolesLabel.x - 3
		y: rolesLabel.y - 2
		width: rolesLabel.width + 6
		height: rolesLabel.height + 4
		radius: 5
		color: getColor()

		function getColor() {
			//
			if (roles_.indexOf("MINER") >= 0) {
				return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.event.subscribe");
			} else if (roles_.indexOf("FULLNODE") >= 0) {
				return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.event.buzz");
			} else if (roles_.indexOf("NODE") >= 0) {
				return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.event.buzzer");
			}

			return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.event.undefined");
		}
	}

	QuarkLabel {
		id: rolesLabel
		x: addressSymbol.x + addressSymbol.width + spaceItems_ + 5
		y: rolesSymbol.y + rolesSymbol.height / 2 - height / 2
		text: roles_
		font.pointSize: listView_.fontPointSize
		font.capitalization: Font.AllLowercase
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground");
	}

	// received

	QuarkSymbolLabel {
		id: receivedSymbol
		x: spaceLeft_
		y: rolesSymbol.y + rolesSymbol.height + spaceItems_
		font.pointSize: listView_.fontPointSize
		symbol: Fonts.inboxInSym
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.peer.active");
	}

	QuarkNumberLabel {
		id: receivedLabel
		x: receivedSymbol.x + receivedSymbol.width + spaceItems_
		y: receivedSymbol.y + receivedSymbol.height / 2 - height / 2

		number: parseFloat(receivedBytes_)
		fillTo: 1
		useSign: false
		units: ""
		font.pointSize: listView_.fontPointSize
		mayCompact: true

		zeroesColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.zeroes")
	}

	Rectangle {
		id: point0

		x: receivedLabel.x + receivedLabel.calculatedWidth + spaceItems_
		y: receivedLabel.y + receivedLabel.calculatedHeight / 2 - height / 2
		width: spaceItems_
		height: spaceItems_
		radius: width / 2

		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		border.color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
	}

	// sent

	QuarkSymbolLabel {
		id: sentSymbol
		x: point0.x + point0.width + spaceItems_
		y: receivedSymbol.y
		font.pointSize: listView_.fontPointSize
		symbol: Fonts.inboxOutSym
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.peer.banned");
	}

	QuarkNumberLabel {
		id: sentLabel
		x: sentSymbol.x + sentSymbol.width + spaceItems_
		y: sentSymbol.y + sentSymbol.height / 2 - height / 2

		number: parseFloat(sentBytes_)
		fillTo: 1
		useSign: false
		units: ""
		font.pointSize: listView_.fontPointSize
		mayCompact: true

		zeroesColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.zeroes")
	}

	Rectangle {
		id: point1

		x: sentLabel.x + sentLabel.calculatedWidth + spaceItems_
		y: sentLabel.y + sentLabel.calculatedHeight / 2 - height / 2
		width: spaceItems_
		height: spaceItems_
		radius: width / 2

		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		border.color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
	}

	// queues

	QuarkSymbolLabel {
		id: queuesSymbol
		x: point1.x + point1.width + spaceItems_
		y: sentSymbol.y
		font.pointSize: listView_.fontPointSize
		symbol: Fonts.queuesSym
	}

	QuarkLabelRegular {
		id: queuesLabel
		x: queuesSymbol.x + queuesSymbol.width + spaceItems_
		y: queuesSymbol.y + queuesSymbol.height / 2 - height / 2
		text: "" + inQueue_ + " / " + outQueue_ + " / " + pendingQueue_
		font.pointSize: listView_.fontPointSize
		elide: Text.ElideRight
		width: (parent.width - (spaceLeft_ + spaceRight_)) / 3
	}

	//
	// chains
	//

	Rectangle {
		id: chainsFrame
		x: rolesSymbol.x
		y: queuesSymbol.y + queuesSymbol.height + spaceItems_
		width: parent.width - (x + spaceRight_)
		height: getHeight()
		radius: 5
		color: getColor()

		property bool collapsed: true
		property int pos_: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 36 : 36

		function getPos() { return pos_; }

		function getHeight() {
			//
			if (chainsFrame.collapsed) return pos_;
			return pos_ + chainsLabel.height + spaceBottom_;
		}

		function getColor() {
			//
			return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.peers.chains");
		}

		PropertyAnimation {
			id: chainsFrameAnimation
			target: chainsFrame
			property: "height"
			to: 0
			duration: 100
		}

		QuarkSymbolLabel {
			id: chainsSymbol
			x: spaceLeft_
			y: spaceItems_
			font.pointSize: listView_.fontPointSize + 4
			symbol: Fonts.chainsSym
		}

		QuarkSymbolLabel {
			id: chainOpenedSymbol
			x: chainsSymbol.x + chainsSymbol.width + spaceItems_
			y: spaceItems_
			font.pointSize: listView_.fontPointSize + 4
			symbol: chainsFrame.collapsed ? Fonts.collapsedSym : Fonts.expandedSym
		}

		QuarkLabel {
			id: chainsLabel
			x: spaceLeft_
			y: chainsFrame.getPos()
			text: chains_
			wrapMode: Text.Wrap
			font.pointSize: listView_.fontPointSize
			width: parent.width - (spaceRight_ + spaceLeft_)
			visible: !chainsFrame.collapsed

			Component.onCompleted: {
				if (buzzerApp.isDesktop) {
					if (Qt.platform.os == "windows") font.family = "Courier New";
					else font.family = "Noto Sans Mono";
				} else {
					font.family = "Noto Sans Mono"; // added into package
				}
			}
		}

		MouseArea {
			id: chainsClick
			x: 0
			y: 0
			width: parent.width
			height: parent.height

			onClicked: {
				//
				chainsFrame.collapsed = !chainsFrame.collapsed;
				chainsFrameAnimation.to = chainsFrame.getHeight();
				chainsFrameAnimation.running = true;
			}
		}

		onHeightChanged: {
			calculateHeight();
		}
	}

	//
	// bottom line
	//

	QuarkHLine {
		id: bottomLine
		x1: 0
		y1: chainsFrame.y + chainsFrame.height + spaceBottom_
		x2: parent.width
		y2: chainsFrame.y + chainsFrame.height + spaceBottom_
		penWidth: 1
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		visible: true

		onY1Changed: {
			calculateHeight();
		}
	}
}
