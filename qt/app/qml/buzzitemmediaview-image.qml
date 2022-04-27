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
	property var sharedMediaPlayer_

	//
	property var buzzitemmedia_;
	property var mediaList;

	signal adjustHeight(var proposed);
	signal errorLoading();
	signal previewLoaded();

	onMediaListChanged: {
		mediaImage.adjustView();
	}

	onBuzzitemmedia_Changed: {
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

	function binfMedia(mediaSource) {
		mediaImage.source = mediaSource;
	}

	function loadingProgress(pos, size) {
		mediaLoading.progress(pos, size);
	}

	function forceVisibilityCheck(isFullyVisible) {
	}

	function checkPlaying() {
	}

	function syncPlayer(mediaPlayer) {
		//
	}

	function reset() {
		//
		mediaImage.scale = 1.0;
		mediaImage.x = mediaImage.getX();
		mediaImage.y = mediaImage.getY();
	}

	//
	color: "transparent"
	width: parent.width
	height: parent.height
	radius: 8

	BuzzerComponents.ImageQx {
		id: mediaImage
		autoTransform: true
		asynchronous: true
		//radius: 8

		x: getX()
		y: getY()

		function getX() {
			return (mediaList ? mediaList.width / 2 : parent.width / 2) - width / 2;
		}

		function getY() {
			//return (mediaList ? mediaList.height / 2 : parent.height / 2) - height / 2;
			return calculatedHeight / 2 - height / 2;
		}

		function adjustDimensions() {
			//
			var lRatioH = (originalHeight * 1.0) / (originalWidth * 1.0);
			var lRatioW = (originalWidth * 1.0) / (originalHeight * 1.0);
			//
			if (calculatedHeight > calculatedWidth && lRatioH < 2 || lRatioW > 2 /*&& originalHeight < calculatedHeight * 2*/) {
				var lCoeffW = (calculatedWidth * 1.0) / (originalWidth * 1.0)
				var lWidth = originalWidth * 1.0;
				width = lWidth * lCoeffW;

				//var lCoeffH = (calculatedHeight * 1.0) / (originalHeight * 1.0)
				var lHeightW = originalHeight * 1.0;
				height = lHeightW * lCoeffW;
			} else {
				var lCoeffH = (calculatedHeight * 1.0) / (originalHeight * 1.0)
				var lHeight = originalHeight * 1.0;
				height = lHeight * lCoeffH;

				var lWidthH = originalWidth * 1.0;
				width = lWidthH * lCoeffH;
			}
		}

		function adjustView() {
			if (mediaImage.status === Image.Ready && mediaList && buzzitemmedia_) {
				//
				reloadControl.forceVisible = true;
				waitControl.visible = false;
				//
				adjustDimensions();
				//
				y = getY();
				x = getX();
			}
		}

		onYChanged: {
			//console.log("[onYChanged]: y = " + y + ", height = " + height + ", scale = " + scale);
		}

		fillMode: BuzzerComponents.ImageQx.PreserveAspectFit
		mipmap: true

		source: usePreview_ ? preview_ : path_

		Component.onCompleted: {
		}

		onSourceChanged: {
			//
			if (!usePreview_) waitControl.visible = true;
		}

		onStatusChanged: {
			adjustView();
			//
			if (status == Image.Error) {
				//
				reloadControl.forceVisible = true;

				// force to reload
				console.log("[onStatusChanged]: forcing reload of " + path_);
				//downloadCommand
				errorLoading();
			} else if (status == Image.Ready) {
				if (usePreview_) {
					console.log("[onStatusChanged]: usePreview_ = " + usePreview_ + ", preview_ = " + preview_);
					previewLoaded();
				} else {
					console.log("[onStatusChanged]: usePreview_ = " + usePreview_ + ", path_ = " + path_);
				}
			}
		}

		onWidthChanged: {
			//console.log("[onWidthChanged]: height = " + height + ", width = " + width +
			//							", originalWidth = " + originalWidth + ", originalHeight = " + originalHeight);
			adjustHeight(height);
		}

		onHeightChanged: {
			//console.log("[onHeightChanged]: width = " + width + ", height = " + height +
			//							", originalHeight = " + originalHeight + ", originalWidth = " + originalWidth);
			adjustHeight(height);
		}

		onScaleChanged: {
			mediaList.interactive = scale == 1.0;
		}
	}

	PinchHandler {
		id: pinchHandler
		minimumRotation: 0
		maximumRotation: 0
		minimumScale: 1.0
		maximumScale: 10.0
		target: mediaImage

		onActiveChanged: {
			dragArea.enabled = !active;
		}
	}

	MouseArea {
		id: dragArea
		//hoverEnabled: true
		x: 0
		y: 0
		width: parent.width
		height: parent.height
		drag.target: mediaImage
		drag.threshold: 20
		cursorShape: Qt.PointingHandCursor

		drag.onActiveChanged: {
		}

		drag.minimumX: getMinX()
		function getMinX() {
			if (parent.width > mediaImage.width * mediaImage.scale) return mediaImage.getX();
			return (parent.width - mediaImage.width * mediaImage.scale) / 2.0;

		}

		drag.maximumX: getMaxX()
		function getMaxX() {
			if (mediaImage.width * mediaImage.scale < parent.width) return 0;
			return (mediaImage.width * mediaImage.scale - parent.width) / 2.0;
		}

		enabled: false

		onPressedChanged: {
		}

		onClicked: {
			mediaImage.scale = 1.0;
			mediaImage.x = mediaImage.getX();
			mediaImage.y = mediaImage.getY();
		}
	}

	QuarkToolButton {
		id: reloadControl
		x: (mediaImage.width > width * 2) ? mediaImage.x + mediaImage.width - width - spaceItems_ :
											mediaImage.x + mediaImage.width + spaceItems_
		y: mediaImage.y + spaceItems_
		symbol: Fonts.arrowDownHollowSym

		property bool scaled: mediaImage.scale == 1.0
		property bool forceVisible: false

		visible: forceVisible && scaled

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
