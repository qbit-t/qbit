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
	property int calculatedHeight: parent.height
	property int calculatedWidth: 500
	property var createViewHandler: null
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

	//
	property var buzzitemmedia_;
	property var mediaList;

	signal errorLoading();

	onMediaListChanged: {
		mediaImage.adjustView();
	}

	onBuzzitemmedia_Changed: {
		mediaImage.adjustView();
	}

	onCalculatedWidthChanged: {
		mediaImage.adjustView();
	}

	function adjust() {
		mediaImage.adjustView();
	}

	function showLoading() {
		mediaLoading.visible = true;
	}

	function hideLoading() {
		mediaLoading.visible = false;
	}

	function loadingProgress(pos, size) {
		mediaLoading.progress(pos, size);
	}

	//
	color: "transparent"
	//border.color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
	width: mediaImage.width + 2 * spaceItems_
	height: calculatedHeight // mediaImage.height
	radius: 8

	Image {
		id: mediaImage
		autoTransform: true
		asynchronous: true

		x: getX()
		y: getY()

		fillMode: Image.PreserveAspectFit
		mipmap: true
		source: usePreview_ ? preview_ : path_

		property int widthEvents: calculatedWidth

		function getX() {
			return (mediaList ? mediaList.width / 2 : parent.width / 2) - width / 2;
		}

		function getY() {
			return (mediaList ? mediaList.height / 2 : parent.height / 2) - height / 2;
		}

		function adjustView() {
			if (mediaImage.status === Image.Ready) {
				//
				reloadControl.visible = true;
				waitControl.visible = false;
				//
				if (calculatedWidth > calculatedHeight && mediaImage.sourceSize.height > mediaImage.sourceSize.width ||
						(mediaImage.sourceSize.height > mediaImage.sourceSize.width && !orientation_)) {
					height = calculatedHeight - 20;
					y = getY();
					x = getX();
				} else {
					width = mediaList.width - 2*spaceItems_;
					y = getY();
					x = getX();
				}
			}
		}

		onSourceChanged: {
			//
			if (!usePreview_) waitControl.visible = true;
		}

		onStatusChanged: {
			//
			adjustView();
			//
			if (status == Image.Error) {
				//
				reloadControl.visible = true;

				// force to reload
				console.log("[onStatusChanged]: forcing reload of " + path_);
				//downloadCommand
				errorLoading();
			}
		}

		MouseArea {
			id: linkClick
			x: 0
			y: 0
			width: mediaImage.width
			height: mediaImage.height
			enabled: true
			cursorShape: Qt.PointingHandCursor

			ItemDelegate {
				id: linkClicked
				x: 0
				y: 0
				width: mediaImage.width
				height: mediaImage.height
				enabled: true

				onClicked: {
					//
					if (createViewHandler) createViewHandler();
				}
			}
		}

		layer.enabled: true
		layer.effect: OpacityMask {
			id: roundEffect
			maskSource: Item {
				width: mediaImage.width // roundEffect.getWidth()
				height: mediaImage.height // roundEffect.getHeight()

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

	QuarkToolButton {
		id: reloadControl
		x: mediaImage.x + mediaImage.width - width - spaceItems_
		y: mediaImage.y + spaceItems_
		symbol: Fonts.arrowDownHollowSym
		visible: false
		labelYOffset: buzzerApp.isDesktop ? 1 : 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")
		Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.background");
		Layout.alignment: Qt.AlignHCenter
		opacity: 0.6
		symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 14) : symbolFontPointSize

		onClicked: {
			//
			errorLoading();
		}
	}

	QuarkSymbolLabel {
		id: waitControl
		x: parent.width / 2 - width / 2
		y: parent.height / 2 - height / 2
		symbol: Fonts.clockSym
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (mediaLoading.size-10)) : (mediaLoading.size-10)
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link")
		visible: false
	}

	QuarkRoundProgress {
		id: mediaLoading
		x: parent.width / 2 - width / 2
		y: parent.height / 2 - height / 2
		size: buzzerClient.scaleFactor * 50
		colorCircle: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link")
		colorBackground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link")
		arcBegin: 0
		arcEnd: 0
		lineWidth: buzzerClient.scaleFactor * 3
		visible: false
		animationDuration: 50

		QuarkSymbolLabel {
			id: waitSymbol
			anchors.fill: parent
			symbol: Fonts.clockSym
			font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (mediaLoading.size-10)) : (mediaLoading.size-10)
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link")
			visible: mediaLoading.visible
		}

		function wait() {
			beginAnimation = false;
			endAnimation = false;

			visible = true;
			waitSymbol.visible = true;
			arcBegin = 0;
			arcEnd = 0;

			beginAnimation = true;
			endAnimation = true;
		}

		function hide() {
			mediaLoading.visible = false;
		}

		function progress(pos, size) {
			//
			waitSymbol.visible = false;
			//
			var lPercent = (pos * 100) / size;
			arcEnd = (360 * lPercent) / 100;
		}
	}

}
