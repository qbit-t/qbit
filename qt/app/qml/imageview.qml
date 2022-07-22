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

QuarkPage {
	id: imageview_
	key: "imageview"
	stacked: false

	// mandatory
	property var file_;

	Component.onCompleted: {
		closePageHandler = closePage;
		activatePageHandler = activatePage;
	}

	function closePage() {
		stopPage();
		controller.popPage();
		destroy(1000);
	}

	function activatePage() {
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
	}

	function onErrorCallback(error)	{
		controller.showError(error);
	}

	function stopPage() {
		pageClosed();
	}

	function initialize(file, c) {
		file_ = file;
		controller = c;
	}

	onWidthChanged: {
		orientationChangedTimer.start();
	}

	// to adjust model
	Timer {
		id: orientationChangedTimer
		interval: 100
		repeat: false
		running: false

		onTriggered: {
			mediaContainer.adjustOrientation();
		}
	}

	// spacing
	readonly property int spaceLeft_: 15
	readonly property int spaceTop_: 12
	readonly property int spaceRight_: 15
	readonly property int spaceBottom_: 12
	readonly property int spaceAvatarBuzz_: 10
	readonly property int spaceItems_: 5
	readonly property int spaceHeader_: 5
	readonly property int spaceRightMenu_: 15
	readonly property int spaceStats_: -5
	readonly property int spaceLine_: 4
	readonly property int spaceMedia_: 20

	//
	// toolbar
	//
	QuarkToolBar {
		id: buzzMediaToolBar
		height: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 50) : 45
		width: parent.width

		property int totalHeight: height

		PropertyAnimation {
			id: toolBarAnimation
			target: buzzMediaToolBar
			property: "height"
			to: 0
			duration: 100
		}

		function setHeight(value) {
			toolBarAnimation.to = value;
			toolBarAnimation.running = true;
		}

		Component.onCompleted: {
		}

		QuarkToolButton	{
			id: cancelButton
			Material.background: "transparent"
			visible: true
			labelYOffset: buzzerApp.isDesktop ? 0 : 3
			symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.cancelSym
			symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : defaultSymbolFontPointSize

			onClicked: {
				closePage();
			}
		}

		QuarkToolButton {
			id: autoFitButton
			Material.background: "transparent"
			visible: true
			labelYOffset: buzzerApp.isDesktop ? 0 : 3
			symbolColor: activated ?
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.autoFitSym
			symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : defaultSymbolFontPointSize

			x: moveButton.x - width - spaceItems_
			y: topOffset

			property bool activated: true

			onClicked: {
				activated = !activated;
			}

			function activate() {
				activated = true;
			}
		}

		QuarkToolButton {
			id: moveButton
			Material.background: "transparent"
			visible: true
			labelYOffset: buzzerApp.isDesktop ? 0 : 3
			symbolColor: activated ?
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.moveSym
			symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : defaultSymbolFontPointSize

			x: rotateButton.x - width - spaceItems_
			y: topOffset

			property bool activated: false

			onClicked: {
				activated = !activated;

				if (rotateButton.activated && activated) rotateButton.activated = false;
				if (zoomButton.activated && activated) zoomButton.activated = false;
			}

			function activate() {
				activated = true;

				if (rotateButton.activated && activated) rotateButton.activated = false;
				if (zoomButton.activated && activated) zoomButton.activated = false;
			}
		}

		QuarkToolButton {
			id: rotateButton
			Material.background: "transparent"
			visible: true
			labelYOffset: buzzerApp.isDesktop ? 0 : 3
			symbolColor: activated ?
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.rotateSym
			symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : defaultSymbolFontPointSize

			x: zoomButton.x - width - spaceItems_
			y: topOffset

			property bool activated: false

			onClicked: {
				activated = !activated;

				if (moveButton.activated && activated) moveButton.activated = false;
			}
		}

		QuarkToolButton {
			id: zoomButton
			Material.background: "transparent"
			visible: true
			labelYOffset: buzzerApp.isDesktop ? 0 : 3
			symbolColor: activated ?
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.zoomSym
			symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : defaultSymbolFontPointSize

			x: hideBarButton.x - width - spaceItems_
			y: topOffset

			property bool activated: true

			onClicked: {
				activated = !activated;

				if (moveButton.activated && activated) moveButton.activated = false;
			}
		}

		QuarkToolButton {
			id: hideBarButton
			Material.background: "transparent"
			visible: true
			labelYOffset: buzzerApp.isDesktop ? 0 : 3
			symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.caretUpSym
			symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : defaultSymbolFontPointSize

			x: parent.width - width - 8
			y: topOffset

			onClicked: {
				//
				buzzMediaToolBar.visible = false;
				buzzMediaToolBar.setHeight(0);
				moveButton.activate();
			}
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

	//
	Rectangle {
		id: flick
		x: 0
		y: buzzMediaToolBar.height
		width: parent.width
		height: parent.height - buzzMediaToolBar.height
		color: "transparent"
		clip: true

		Rectangle {
			id: mediaContainer

			x: 0
			y: 0
			width: flick.width
			height: flick.height
			scale: getScale()

			function getScale() {
				//
				if (autoFitButton.activated) {
					if (parent.width > parent.height)
						return height / mediaImage.sourceSize.height;
					return width / mediaImage.sourceSize.width;
				}

				return mediaContainer.scale;
			}

			color: "transparent"
			smooth: true
			antialiasing: true

			Behavior on scale { NumberAnimation { duration: 100 } }

			function adjustOrientation() {
			}

			Image {
				id: mediaImage
				autoTransform: true
				asynchronous: true

				anchors.centerIn: parent
				fillMode: Image.PreserveAspectFit
				mipmap: true
				antialiasing: true

				source: (imageview_.file_ ? imageview_.file_ : "")
			}
		}

		PinchArea {
			x: 0
			y: 0
			width: parent.width
			height: parent.height
			pinch.target: mediaContainer
			pinch.minimumRotation: rotateButton.activated ? -360 : mediaContainer.rotation
			pinch.maximumRotation: rotateButton.activated ? 360  : mediaContainer.rotation
			pinch.minimumScale: zoomButton.activated ? getMinScale() ://width / Math.max(mediaImage.sourceSize.width, mediaImage.sourceSize.height) :
													   mediaContainer.scale
			pinch.maximumScale: zoomButton.activated ? 10.0 :
													   mediaContainer.scale
			pinch.dragAxis: Pinch.XAndYAxis

			enabled: zoomButton.activated || rotateButton.activated

			function getMinScale() {
				if (parent.width > parent.height)
					return height / mediaImage.sourceSize.height;
				return width / mediaImage.sourceSize.width;
			}

			onPinchFinished: {
				//
				if (mediaContainer.x < dragArea.drag.minimumX)
					mediaContainer.x = dragArea.drag.minimumX;
				else if (mediaContainer.x > dragArea.drag.maximumX)
					mediaContainer.x = dragArea.drag.maximumX;

				if (mediaContainer.y < dragArea.drag.minimumY)
					mediaContainer.y = dragArea.drag.minimumY;
				else if (mediaContainer.y > dragArea.drag.maximumY)
					mediaContainer.y = dragArea.drag.maximumY;
			}
		}

		MouseArea {
			id: dragArea
			hoverEnabled: true
			x: 0
			y: 0
			width: parent.width
			height: parent.height
			drag.target: mediaContainer
			cursorShape: Qt.PointingHandCursor

			drag.minimumX: getMinX()
			function getMinX() {
				if (parent.width > mediaImage.width * mediaContainer.scale) return 0;
				return (parent.width - mediaImage.width * mediaContainer.scale) / 2.0;

			}

			drag.maximumX: getMaxX()
			function getMaxX() {
				if (mediaImage.width * mediaContainer.scale < parent.width) return 0;
				return (mediaImage.width * mediaContainer.scale - parent.width) / 2.0;
			}

			drag.minimumY: getMinY()
			function getMinY() {
				if (mediaImage.height * mediaContainer.scale < parent.height) return 0;
				return (parent.height - mediaImage.height * mediaContainer.scale) / 2.0;

			}

			drag.maximumY: getMaxY()
			function getMaxY() {
				if (mediaImage.height * mediaContainer.scale < parent.height) return 0;
				return (mediaImage.height * mediaContainer.scale - parent.height) / 2.0;
			}

			scrollGestureEnabled: false  // 2-finger-flick gesture should pass through to the Flickable
			enabled: moveButton.activated

			onClicked: {
				buzzMediaToolBar.visible = !buzzMediaToolBar.visible;
				if (!buzzMediaToolBar.visible)
					buzzMediaToolBar.setHeight(0);
				else
					buzzMediaToolBar.setHeight(45);
			}
		}
	}
}
