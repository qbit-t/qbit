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

Rectangle {
	//
	id: imageFrame

	//
	readonly property int spaceLeft_: 15
	readonly property int spaceTop_: 12
	readonly property int spaceRight_: 15
	readonly property int spaceBottom_: 12
	readonly property int spaceAvatarBuzz_: 10
	readonly property int spaceItems_: 5
	readonly property int spaceMedia_: 10
	readonly property int spaceSingleMedia_: 8
	readonly property int spaceMediaIndicator_: 15
	readonly property int spaceHeader_: 7
	readonly property int spaceRightMenu_: 15
	readonly property int spaceStats_: -5
	readonly property int spaceLine_: 4
	readonly property int spaceThreaded_: 33
	readonly property int spaceThreadedItems_: 4
	readonly property real defaultFontSize: 11
	property var frameColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background")

	//
	property var mediaList;
	property var mediaBox;

	onMediaBoxChanged: {
		mediaImage.adjustView();
	}

	function adjustOrientation(orientation) {
	}

	function terminate() {
		//
	}

	//
	color: "transparent"
	width: mediaImage.width + 2 * spaceItems_
	height: mediaList.height // mediaImage.height

	Image {
		id: mediaImage
		autoTransform: true

		x: getX()
		y: getY()

		fillMode: Image.PreserveAspectFit
		mipmap: true

		source: path

		Component.onCompleted: {
		}

		function getX() {
			return parent.width / 2 - width / 2;
		}

		function getY() {
			return 0;
		}

		function adjustView() {
			width = mediaList.width - 2 * spaceItems_;
			mediaList.height = Math.max(mediaBox.calculatedHeight, height);
			mediaBox.calculatedHeight = mediaList.height;
		}

		onStatusChanged: {
			adjustView();
		}

		layer.enabled: true
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
				return mediaImage.width / 2 - mediaImage.paintedWidth / 2;
			}

			function getY() {
				return mediaImage.height / 2 - mediaImage.paintedHeight / 2;
			}

			function getWidth() {
				return mediaImage.paintedWidth;
			}

			function getHeight() {
				return mediaImage.paintedHeight;
			}
		}
	}

	QuarkToolButton	{
		id: removeButton

		x: mediaImage.width - (width + spaceItems_)
		y: spaceItems_
		// Material.background: "transparent"
		visible: true
		labelYOffset: 3
		symbolColor: (!sending || uploaded) ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
										  buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		symbol: !uploaded ? Fonts.cancelSym : Fonts.checkedSym

		onClicked: {
			if (!sending) {
				mediaList.removeMedia(index);
			}
		}
	}

	QuarkRoundProgress {
		id: mediaUploadProgress
		x: removeButton.x - 1
		y: removeButton.y - 1
		size: removeButton.width + 2
		colorCircle: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link");
		colorBackground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link");
		arcBegin: 0
		arcEnd: progress
		lineWidth: 3
	}
}
