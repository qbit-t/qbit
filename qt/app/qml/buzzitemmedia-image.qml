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
	property int calculatedHeight: 500 // 400?
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
	property var fillColor: "transparent"
	property var sharedMediaPlayer_

	//
	property var buzzitemmedia_;
	property var mediaList;

	signal adjustHeight(var proposed);
	signal errorLoading();

	onMediaListChanged: {
		mediaImage.adjustView();
	}

	onBuzzitemmedia_Changed: {
		mediaImage.adjustView();
	}

	function adjust() {
		mediaImage.adjustView();
	}

	function terminate() {
		//
	}

	function forceVisibilityCheck(isFullyVisible) {
		//
	}

	//
	color: "transparent"
	// border.color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
	width: mediaImage.width + 2 * spaceItems_
	height: calculatedHeight // mediaImage.height
	radius: 8

	BuzzerComponents.ImageQx {
		id: mediaImage
		autoTransform: true
		asynchronous: true
		radius: 8

		x: spaceItems_
		y: 0

		function adjustView() {
			if (mediaImage.status === Image.Ready && mediaList && buzzitemmedia_) {
				//console.log("[onHeightChanged(1)/image]: height = " + height + ", width = " + width + ", implicitHeight = " + previewImage.implicitHeight);
				width = mediaList.width - spaceItems_;
				adjustHeight(height);
				parent.height = height;
			}
		}

		function adjustView2() {
			//
			if (!mediaList || !buzzitemmedia_) return;
			//
			width = mediaList.width - spaceItems_;
			//height = calculatedHeight;
			//mediaList.height = Math.max(buzzitemmedia_.calculatedHeight, height);
			//buzzitemmedia_.calculatedHeight = mediaList.height;

			if (calculatedHeight < mediaImage.paintedHeight) {
				calculatedHeight = mediaImage.paintedHeight;
			}
		}

		//width: (mediaList ? mediaList.width : imageFrame.width) - spaceItems_
		//height: calculatedHeight
		fillMode: BuzzerComponents.ImageQx.PreserveAspectFit //Image.PreserveAspectFit
		mipmap: true

		//Layout.fillWidth: true
		//Layout.fillHeight: true

		source: preview_

		Component.onCompleted: {
		}

		onHeightChanged: {
			if (buzzitemmedia_) {
				//console.log("[onHeightChanged(2)/image]: height = " + height + ", width = " + width + ", implicitHeight = " + previewImage.implicitHeight);
				adjustHeight(height);
				parent.height = height;
			}
		}

		onStatusChanged: {
			adjustView();

			if (status == Image.Error) {
				// force to reload
				console.log("[onStatusChanged]: forcing reload of " + preview_);
				//downloadCommand
				errorLoading();
			}
		}

		onWidthChanged: {
			//
			if (width != originalWidth) {
				var lCoeff = (width * 1.0) / (originalWidth * 1.0)
				var lHeight = originalHeight * 1.0;
				height = lHeight * lCoeff;

				adjustHeight(height);
			}
		}

		MouseArea {
			id: linkClick
			x: 1
			y: 1
			width: mediaImage.width - 1
			height: mediaImage.height - 1
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
					// expand
					var lSource;
					var lComponent;

					// gallery
					lSource = "qrc:/qml/buzzmedia.qml";
					lComponent = Qt.createComponent(lSource);

					if (lComponent.status === Component.Error) {
						controller_.showError(lComponent.errorString());
					} else {
						var lMedia = lComponent.createObject(controller_);
						lMedia.controller = controller_;
						lMedia.buzzMedia_ = buzzitemmedia_.buzzMedia_;
						lMedia.initialize(pkey_);
						controller_.addPage(lMedia);
					}
				}
			}
		}
	}
}
