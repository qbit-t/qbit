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
	id: peerinactiveitem_

	property int calculatedHeight: calculateHeightInternal()

	property var peerId_: peerId;
	property var endpoint_: endpoint;
	property var status_: status;
	property var time_: time;
	property var localDateTime_: localDateTime;
	property var latency_: latency;
	property var inQueue_: inQueue;
	property var outQueue_: outQueue;
	property var pendingQueue_: pendingQueue;
	property var receivedCount_: receivedCount;
	property var receivedBytes_: receivedBytes;
	property var sentCount_: sentCount;
	property var sentBytes_: sentBytes;

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

	QuarkLabel {
		id: endpointLabel
		x: serverSymbol.x + serverSymbol.width + spaceItems_
		y: spaceTop_
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

	QuarkLabel {
		id: timeLabel
		x: timeSymbol.x + timeSymbol.width + spaceItems_
		y: timeSymbol.y
		text: localDateTime_ + " - " + latency_ + " ms"
		font.pointSize: listView_.fontPointSize
		elide: Text.ElideRight
		width: parent.width - (x + spaceRight_)
	}

	// received

	QuarkSymbolLabel {
		id: receivedSymbol
		x: spaceLeft_
		y: timeSymbol.y + timeSymbol.height + spaceItems_
		font.pointSize: listView_.fontPointSize
		symbol: Fonts.inboxInSym
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.peer.active");
	}

	QuarkNumberLabel {
		id: receivedLabel
		x: receivedSymbol.x + receivedSymbol.width + spaceItems_
		y: receivedSymbol.y

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
		y: sentSymbol.y

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

	QuarkLabel {
		id: queuesLabel
		x: queuesSymbol.x + queuesSymbol.width + spaceItems_
		y: queuesSymbol.y
		text: "" + inQueue_ + " / " + outQueue_ + " / " + pendingQueue_
		font.pointSize: listView_.fontPointSize
		elide: Text.ElideRight
		width: (parent.width - (spaceLeft_ + spaceRight_)) / 3
	}

	//
	// bottom line
	//

	QuarkHLine {
		id: bottomLine
		x1: 0
		y1: queuesSymbol.y + queuesSymbol.height + spaceBottom_
		x2: parent.width
		y2: queuesSymbol.y + queuesSymbol.height + spaceBottom_
		penWidth: 1
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		visible: true

		onY1Changed: {
			calculateHeight();
		}
	}
}
