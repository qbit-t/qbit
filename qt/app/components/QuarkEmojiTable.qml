// QuarkEmojiTable.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4
import QtQuick 2.15

import app.buzzer.commands 1.0 as BuzzerCommands

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

TableView {
	columnSpacing: 1
	rowSpacing: 1
	clip: true

	signal emojiSelected(var emoji);

	delegate: ItemDelegate {
		// color: "transparent"
		implicitHeight: buzzerClient.scaleFactor * 60
		implicitWidth: buzzerClient.scaleFactor * 60

		onClicked: {
			emojiSelected(symbol);
		}

		QuarkLabel {
			id: emoji
			font.pointSize: buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 12)
			text: symbol ? symbol : ""
			anchors.centerIn: parent

			property bool tooltipVisible: false
			ToolTip.text: caption ? caption : ""
			ToolTip.visible: tooltipVisible

			MouseArea {
				hoverEnabled: true
				anchors.fill: parent

				onEntered: emoji.tooltipVisible = true;
				onExited: emoji.tooltipVisible = false;

				onClicked: {
					// save fav
					buzzerClient.setFavEmoji(name);
					// notify
					emojiSelected(symbol);
				}
			}
		}
	}

	ScrollIndicator.vertical: ScrollIndicator { }
}

