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
	id: buzzitemmedia_

	property real calculatedHeight: 0 //400
	property int calculatedWidth: 500
	property int calculatedWidthInternal: 500
	property var buzzId_: buzzId
	property var buzzMedia_: buzzMedia
	property var buzzBody_: buzzBodyFlat
	property var controller_: controller
	property var frameColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background")
	property var fillColor: "transparent"
	property var sharedMediaPlayer_

	readonly property int maxCalculatedWidth_: 600
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
	readonly property real defaultFontSize: 11

	property var pkey_: ""

	signal calculatedHeightModified(var value);
	
	onCalculatedHeightChanged: calculatedHeightModified(calculatedHeight);

	onCalculatedWidthChanged: {
		//
		if (calculatedWidth > maxCalculatedWidth_) {
			calculatedWidth = maxCalculatedWidth_;
		}

		calculatedWidthInternal = calculatedWidth;
	}

	function forceVisibilityCheck(isFullyVisible) {
		//
		mediaList.setFullyVisible(isFullyVisible);
	}

	Component.onCompleted: {
	}

	function initialize(key) {
		if (key !== undefined) pkey_ = key;
		mediaList.prepare();
	}

	function cleanUp() {
		mediaList.cleanUp();
	}

	//
	// media list
	//

	QuarkListView {
		id: mediaList
		x: 0
		y: 0
		width: calculatedWidthInternal
		height: calculatedHeight
		clip: true
		orientation: Qt.Horizontal
		layoutDirection:  Qt.LeftToRight
		snapMode: ListView.SnapOneItem
		highlightFollowsCurrentItem: true
		highlightMoveDuration: -1
		highlightMoveVelocity: -1

		property var prevIndex: 0

		function adjustItems() {
			//
			for (var lIdx = 0; lIdx < mediaList.count; lIdx++) {
				var lItem = mediaList.itemAtIndex(lIdx);
				if (lItem) {
					lItem.width = mediaList.width;
					lItem.mediaItem.width = mediaList.width;
					lItem.mediaItem.adjust();
				}
			}
		}

		function cleanUp() {
			//
			for (var lIdx = 0; lIdx < mediaList.count; lIdx++) {
				var lItem = mediaList.itemAtIndex(lIdx);
				if (lItem) {
					lItem.mediaItem.terminate();
				}
			}
		}

		function setFullyVisible(fullyVisible) {
			//
			for (var lIdx = 0; lIdx < mediaList.count; lIdx++) {
				var lItem = mediaList.itemAtIndex(lIdx);
				if (lItem) {
					lItem.mediaItem.forceVisibilityCheck(fullyVisible);
				}
			}
		}

		add: Transition {
			NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 400 }
		}

		model: ListModel { id: mediaModel }

		onContentXChanged: {
			//
			if (contentX == 0) mediaIndicator.currentIndex = 0;
			else mediaIndicator.currentIndex = mediaList.indexAt(mediaList.contentX, 0);
			//
			var lItem = mediaList.itemAtIndex(mediaIndicator.currentIndex);
			if (lItem && lItem.mediaItem) {
				//
				//console.log("[onContentXChanged]: mediaList.contentX = " + mediaList.contentX +
				//			", mediaList.indexAt(mediaList.contentX, 0) = " + mediaList.indexAt(mediaList.contentX, 0) +
				//			", lItem.mediaItem.height = " + lItem.mediaItem.height);
				//
				if (lItem.x === mediaList.contentX) {
					//
					var lPrevItem = mediaList.itemAtIndex(prevIndex);
					if (lPrevItem && lItem.mediaItem.height === 1) {
						lItem.mediaItem.height = lPrevItem.mediaItem.height;
					}
					//
					var lModelItem = mediaModel.get(mediaIndicator.currentIndex);
					lModelItem.preview_ = lModelItem.previewSource_;
					//
					mediaList.height = lItem.mediaItem.height;
					buzzitemmedia_.height = mediaList.height;
					calculatedHeight = mediaList.height;
					//
					prevIndex = mediaIndicator.currentIndex;
				}
			}
		}

		onWidthChanged: {
			adjustItems();
		}

		delegate: Rectangle {
			//
			id: mediaFrame
			color: "transparent"
			//width: mediaItem ? mediaItem.width + 2 * spaceItems_ : 100 //mediaList.width
			height: mediaItem ? mediaItem.height : 100

			property var mediaItem;

			QuarkRoundProgress {
				id: mediaLoading
				x: mediaList.width / 2 - width / 2
				y: mediaList.height / 2 - height / 2
				size: buzzerClient.scaleFactor * 50
				colorCircle: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link");
				colorBackground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link");
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

				function progress(pos, size) {
					//
					waitSymbol.visible = false;
					//
					var lPercent = (pos * 100) / size;
					arcEnd = (360 * lPercent) / 100;
				}
			}

			function adjustHeight(proposed) {
				//
				if (index === mediaIndicator.currentIndex && proposed > 0 && buzzitemmedia_.calculatedHeight !== proposed) {
					//
					//console.log("[buzzitemmedia/adjustHeight]: proposed = " + proposed + ", buzzitemmedia_.calculatedHeight = " + buzzitemmedia_.calculatedHeight);
					buzzitemmedia_.calculatedHeight = proposed;
					mediaList.height = proposed;
				}
			}

			BuzzerCommands.DownloadMediaCommand {
				id: downloadCommand
				preview: true
				skipIfExists: true // (pkey_ !== "" || pkey_ !== undefined) ? false : true
				url: url_
				localFile: key_
				pkey: pkey_

				property int tryCount_: 0;
				property int tryReloadCount_: 0;
				property bool processed_: false;

				function errorMediaLoading() {
					//
					tryReloadCount_++;
					if (tryReloadCount_ > 3) return;
					// cleaning up
					downloadCommand.cleanUp();
					// re-process
					downloadCommand.process();
				}

				onProgress: {
					//
					mediaLoading.progress(pos, size);
				}

				onProcessed: {
					//
					processed_ = true;
					// tx, previewFile, originalFile, orientation, duration, size, type
					//var lPSize = buzzerApp.getFileSize(previewFile);
					//var lOSize = buzzerApp.getFileSize(originalFile);
					//console.log("index = " + index + ", " + tx + ", " + previewFile + " - [" + lPSize + "], " + originalFile + " - [" + lOSize + "], " + orientation + ", " + duration + ", " + size + ", " + type);

					// stop timer
					downloadWaitTimer.stop();
					// set preview
					/*if (index == 0)*/ preview_ = "file://" + previewFile; // ONLY: preview binding here, path_ is for the inner component
					previewSource_ = "file://" + previewFile;
					// set original orientation
					orientation_ = orientation;
					// set duration
					duration_ = duration;
					// set size
					size_ = size;
					// set size
					media_ = type;
					// caption
					caption_ = description;
					// stop spinning
					mediaLoading.visible = false;

					//
					var lComponent;
					var lSource = "qrc:/qml/buzzitemmedia-image.qml";
					if (type === "audio")
						lSource = "qrc:/qml/buzzitemmedia-audio.qml";
					else if (type === "video")
						lSource = "qrc:/qml/buzzitemmedia-video.qml";

					lComponent = Qt.createComponent(lSource);
					if (lComponent.status === Component.Error) {
						console.log(lComponent.errorString());
					}

					mediaFrame.mediaItem = lComponent.createObject(mediaFrame);
					mediaFrame.mediaItem.adjustHeight.connect(mediaFrame.adjustHeight);
					mediaFrame.mediaItem.errorLoading.connect(errorMediaLoading);

					mediaFrame.mediaItem.frameColor = buzzitemmedia_.frameColor;
					mediaFrame.mediaItem.fillColor = buzzitemmedia_.fillColor;
					mediaFrame.mediaItem.width = mediaList.width;
					mediaFrame.mediaItem.mediaList = mediaList;
					mediaFrame.mediaItem.buzzitemmedia_ = buzzitemmedia_;
					mediaFrame.mediaItem.sharedMediaPlayer_ = buzzitemmedia_.sharedMediaPlayer_;
					mediaFrame.mediaItem.mediaIndex_ = index;
					mediaFrame.mediaItem.controller_ = controller_;

					if (index === 0) mediaFrame.height = mediaFrame.mediaItem.height;
					mediaFrame.width = mediaList.width;

					// reset height
					if (index === 0 && !buzzitemmedia_.calculatedHeight) {
						// mediaList.height = mediaFrame.mediaItem.height;
						buzzitemmedia_.calculatedHeight = mediaFrame.height;
					}

					//mediaFrame.mediaItem.adjust();
				}

				onError: {
					if (code == "E_MEDIA_HEADER_NOT_FOUND") {
						//
						tryCount_++;

						if (tryCount_ < 15) {
							downloadTimer.start();
						} else {
							mediaLoading.visible = false;
						}
					}
				}
			}

			Timer {
				id: downloadTimer
				interval: 2000
				repeat: false
				running: false

				onTriggered: {
					downloadCommand.process();
				}
			}

			Timer {
				id: downloadWaitTimer
				interval: 1000
				repeat: false
				running: false

				onTriggered: {
					mediaLoading.visible = true;
				}
			}

			Component.onCompleted: {
				if (!downloadCommand.processed_) {
					downloadWaitTimer.start();
					downloadCommand.process();
				}
			}
		}

		function addMedia(url, file) {
			mediaModel.append({
				key_: file,
				url_: url,
				path_: "",
				preview_: "",
				media_: "unknown",
				size_: 0,
				duration_: 0,
				orientation_: 0,
				loaded_: false,
				description_: (buzzBody_ ? buzzBody_ : ""),
				caption_: "",
				previewSource_: ""});
		}

		function prepare() {
			if (mediaList.count) return;
			for (var lIdx = 0; lIdx < buzzMedia_.length; lIdx++) {
				var lMedia = buzzMedia_[lIdx];
				addMedia(lMedia.url, buzzerClient.getTempFilesPath() + "/" + lMedia.tx);
			}
		}
	}

	PageIndicator {
		id: mediaIndicator
		count: buzzMedia_ ? buzzMedia_.length : 0
		currentIndex: mediaList.currentIndex

		x: calculatedWidthInternal / 2 - width / 2
		y: spaceStats_ - height
		visible: buzzMedia_ ? buzzMedia_.length > 1 && buzzMedia_.length <= 5 : false

		Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
		Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
		Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
		Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
		Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

		/*
		delegate: Rectangle {
			implicitWidth: 6
			implicitHeight: 6

			radius: width / 2
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")

			opacity: index === mediaIndicator.currentIndex ? 0.95 : pressed ? 0.7 : 0.45

			Behavior on opacity {
				OpacityAnimator {
					duration: 100
				}
			}
		}
		*/
	}

	QuarkLabel {
		id: mediaPagesIndicator
		x: calculatedWidthInternal - width
		y: spaceStats_ - (height + 3)
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 1)) : defaultFontSize + 1
		text: "0/0"
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		visible: count > 5

		property var count: buzzMedia_ ? buzzMedia_.length : 0
		property var currentIndex: mediaIndicator.currentIndex

		onCurrentIndexChanged: {
			text = (currentIndex + 1) + "/" + count;
		}
	}
}
