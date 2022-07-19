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
	readonly property real defaultFontSize: buzzerApp.defaultFontSize()
	property var frameColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background")

	//
	property string mediaViewTheme: "Darkmatter"
	property string mediaViewSelector: "dark"

	//
	property var mediaList;
	property var mediaBox;

	signal adjustHeight(var proposed);

	onMediaBoxChanged: {
		adjust();
	}

	onWidthChanged: {
		adjust();
	}

	function adjust() {
		mediaImage.adjustWidth();
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
	//height: 600 // mediaImage.height

	BuzzerComponents.ImageQx {
		id: mediaImage
		asynchronous: true
		autoTransform: true
		radius: 8

		function adjustView() {
			/*
			if (mediaImage.status === Image.Ready && mediaList) {
				if (originalWidth > originalHeight) {
					width = mediaList.width - 4*spaceItems_;
					parent.height = height;
				} else {
					height = parent.parent.height;
				}

				//adjustHeight(height);
			}
			*/

			if (mediaList) {
				//width = mediaList.width - 2 * spaceItems_;
				mediaList.height = Math.max(mediaBox.calculatedHeight, height);
				mediaBox.calculatedHeight = mediaList.height;
			}
		}

		function adjustWidth() {
			//
			var lCoeff;
			width = mediaList.width - 4*spaceItems_;
			lCoeff = (width * 1.0) / (originalWidth * 1.0);
			var lHeight = originalHeight * 1.0;
			height = lHeight * lCoeff;
			adjustHeight(height);
		}

		onHeightChanged: {
			parent.height = height;
		}

		onWidthChanged: {
			//
		}

		onStatusChanged: {
			if (status == Image.Ready) {
				adjustWidth();
				adjustView();
				controlsBack.visible = true;
				removeButton.visible = true;
			}

			if (status == Image.Error) {
				console.info("[onStatusChanged]: error = " + errorString);
			}
		}

		source: path
		fillMode: BuzzerComponents.ImageQx.PreserveAspectFit
		mipmap: true
		anchors.centerIn: parent
	}

	//
	Rectangle {
		id: controlsBack
		x: mediaImage.x + spaceItems_
		y: mediaImage.y + spaceItems_
		width: totalSizeControl.width + 2 * spaceItems_
		height: totalSizeControl.height + 2 * spaceItems_
		color: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.disabledHidden.uni")
		radius: 4

		visible: false

		Component.onCompleted: totalSizeControl.setTotalSize(size)

		QuarkLabel {
			id: totalSizeControl
			x: spaceItems_
			y: spaceItems_
			font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize)) : buzzerApp.defaultFontSize()
			color: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.menu.foreground")
			text: "0k"

			visible: true

			function setTotalSize(mediaSize) {
				//
				if (mediaSize < 1000) text = mediaSize + "b";
				else text = NumberFunctions.numberToCompact(mediaSize);
			}
		}

		function adjustX() {
			x = mediaImage.x + spaceItems_;
		}

		function adjustY() {
			y = mediaImage.y + spaceItems_;
		}
	}

	QuarkToolButton	{
		id: removeButton

		x: mediaImage.x + mediaImage.width - (width + 2*spaceItems_)
		y: 2 * spaceItems_
		// Material.background: "transparent"
		visible: false
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
