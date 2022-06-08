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
	id: imageFrameFeed

	//
	readonly property bool playable: false
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
	property var frameColor: "transparent" //buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background")
	property var fillColor: "transparent"
	property var sharedMediaPlayer_
	property var mediaIndex_: 0
	property var controller_
	property var playerKey_
	property var pkey_

	//
	property var buzzitemmedia_;
	property var mediaList;

	signal adjustHeight(var proposed);
	signal errorLoading();

	onMediaListChanged: {
		mediaImageFeed.adjustView();
	}

	onBuzzitemmedia_Changed: {
		mediaImageFeed.adjustView();
	}

	onHeightChanged: {
		//console.log("[image/onHeightChanged]: height = " + height);
	}

	function adjust() {
		mediaImageFeed.adjustView();
	}

	function terminate() {
		//
	}

	function forceVisibilityCheck(isFullyVisible) {
		//
	}

	function unbindCommonControls() {
		//
	}

	//
	color: "transparent"
	width: mediaImageFeed.width + 2 * spaceItems_
	height: 1 //calculatedHeight
	//radius: 8

	BuzzerComponents.ImageQx {
		id: mediaImageFeed
		autoTransform: true
		asynchronous: true
		radius: 8

		x: spaceItems_
		y: 0

		function adjustView() {
			if (mediaImageFeed.status === Image.Ready && mediaList && buzzitemmedia_) {
				width = mediaList.width - spaceItems_;
				parent.height = height;
				//mediaList.height = height;
				adjustHeight(height);
			}
		}

		fillMode: BuzzerComponents.ImageQx.PreserveAspectFit
		mipmap: buzzerApp.isDesktop

		source: preview_

		Component.onCompleted: {
		}

		onHeightChanged: {
			if (buzzitemmedia_) {
				parent.height = height;
				adjustHeight(height);
			}
		}

		onStatusChanged: {
			adjustView();

			if (status == Image.Error) {
				// force to reload
				console.log("[buzzmedia/onStatusChanged]: forcing reload of " + preview_ + ", errorString = " + errorString);
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

				//adjustHeight(height);
			}
		}

		MouseArea {
			id: linkClick
			x: 1
			y: 1
			width: mediaImageFeed.width - 1
			height: mediaImageFeed.height - 1
			enabled: true
			cursorShape: Qt.PointingHandCursor

			ItemDelegate {
				id: linkClicked
				x: 0
				y: 0
				width: mediaImageFeed.width
				height: mediaImageFeed.height
				enabled: true

				onClicked: {
					// expand
					controller_.openMedia(pkey_, mediaIndex_, buzzitemmedia_.buzzMedia_, sharedMediaPlayer_, null,
										  buzzitemmedia_.buzzId_, buzzitemmedia_.buzzerAlias_, buzzitemmedia_.buzzBody_);
				}
			}
		}
	}
}
