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
	property int calculatedHeight: 400
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

	//
	property var buzzitemmedia_;
	property var mediaList;

	signal adjustHeight(var proposed);

	onMediaListChanged: {
		mediaImage.adjustView();
	}

	onBuzzitemmedia_Changed: {
		mediaImage.adjustView();
	}

	function adjust() {
		mediaImage.adjustView();
	}

	//
	color: "transparent"
	// border.color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
	width: mediaImage.width + 2 * spaceItems_
	height: calculatedHeight // mediaImage.height
	radius: 8

	Image {
		id: mediaImage
		autoTransform: true
		asynchronous: true

		x: spaceItems_
		y: 0

		function adjustView() {
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

		width: mediaList.width - spaceItems_
		height: calculatedHeight
		fillMode: Image.PreserveAspectCrop
		mipmap: true

		source: path_

		Component.onCompleted: {
		}

		onStatusChanged: {
			//
			/*
			if (status == Image.Error) {
				// force to reload
				console.log("[onStatusChanged]: forcing reload of " + path_);
				path_ = "";
				downloadCommand.process(true);
			}
			*/
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

		layer.enabled: true
		layer.effect: OpacityMask {
			id: roundEffect
			maskSource: Item {
				width: roundEffect.getWidth()
				height: roundEffect.getHeight()

				Rectangle {
					//anchors.centerIn: parent
					x: roundEffect.getX()
					y: roundEffect.getY()
					width: roundEffect.getWidth()
					height: roundEffect.getHeight()
					radius: 8
				}
			}

			function getX() {
				return 0;
			}

			function getY() {
				return 0;
			}

			function getWidth() {
				return mediaImage.width;
			}

			function getHeight() {
				return mediaImage.height;
			}
		}
	}
}
