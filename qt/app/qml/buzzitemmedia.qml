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
	property var buzzerAlias_: buzzerAlias
	property var buzzMedia_: buzzMedia
	property var buzzBody_: buzzBodyFlat
	property var controller_: controller
	property var frameColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background")
	property var fillColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background.simple")
	property var sharedMediaPlayer_
	property var playerKey_

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

	//
	// playback controller
	property alias mediaCount: mediaList.mediaModel.count
	property alias mediaContainer: mediaList
	//
	//

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

	function unbindCommonControls() {
		//
		mediaList.unbindCommonControls();
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
		//cacheBuffer: 100000

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
				if (lItem && lItem.mediaItem) {
					lItem.mediaItem.forceVisibilityCheck(fullyVisible);
				}
			}
		}

		function unbindCommonControls() {
			//
			for (var lIdx = 0; lIdx < mediaList.count; lIdx++) {
				var lItem = mediaList.itemAtIndex(lIdx);
				if (lItem && lItem.mediaItem) {
					lItem.mediaItem.unbindCommonControls();
				}
			}
		}

		function playNext() {
			//
			for (var lIdx = mediaIndicator.currentIndex + 1; lIdx < buzzMedia_.length; lIdx++) {
				var lItem = mediaList.itemAtIndex(lIdx);
				if (lItem && lItem.mediaItem && lItem.mediaItem.playable) {
					mediaList.currentIndex = lIdx;
					lItem.mediaItem.tryPlay();
					sharedMediaPlayer_.showCurrentPlayer(null);
					return true;
				} else if (lItem && !lItem.mediaItem) {
					// absent
					lItem.forceDownloadAndTryPlay();
					return true;
				} else if (!lItem) {
					// absent at all
					var lMedia = buzzMedia_[lIdx];
					mediaList.addMedia(lMedia.url, buzzerClient.getTempFilesPath() + "/" + lMedia.tx, true /*force play when downloaded*/);
					//console.info("mediaList.count = " + mediaList.count + ", lIdx = " + lIdx);
					//var lNewItem = mediaList.itemAtIndex(lIdx);
					//lNewItem.forceDownloadAndTryPlay();
					return true;
				}
			}

			return false;
		}

		function playPrev() {
			//
			for (var lIdx = mediaIndicator.currentIndex - 1; lIdx >= 0; lIdx--) {
				var lItem = mediaList.itemAtIndex(lIdx);
				if (lItem && lItem.mediaItem && lItem.mediaItem.playable) {
					mediaList.currentIndex = lIdx;
					lItem.mediaItem.tryPlay();
					sharedMediaPlayer_.showCurrentPlayer(null);
					return true;
				} else if (lItem && !lItem.mediaItem) {
					// absent
					lItem.forceDownloadAndTryPlay();
					return true;
				}
			}

			return false;
		}

		//model: ListModel { id: mediaModel }
		property ListModel mediaModel: ListModel { id: media }

		onContentXChanged: {
			//
			var lLastIndex = mediaIndicator.currentIndex;
			var lNewIndex = lLastIndex;

			//
			if (contentX == 0) lNewIndex = 0;
			else {
				var lOffset = mediaList.width/2;
				if (lLastIndex + 1 >= mediaList.mediaModel.count - 1) lOffset = mediaList.width * 0.90;
				lNewIndex = mediaList.indexAt(mediaList.contentX + lOffset, 0);
			}

			//
			if (lNewIndex === -1 && lLastIndex < buzzMedia_.length - 1) {
				var lMedia = buzzMedia_[lLastIndex + 1];
				mediaList.addMedia(lMedia.url, buzzerClient.getTempFilesPath() + "/" + lMedia.tx);
				lNewIndex = lLastIndex + 1;
			}

			//
			var lItem = mediaList.itemAtIndex(lNewIndex);
			if (lItem && lItem.mediaItem) {
				//
				//console.info("[onContentXChanged]: mediaList.contentX = " + mediaList.contentX +
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
					var lModelItem = mediaList.mediaModel.get(lNewIndex);
					lModelItem.preview_ = lModelItem.previewSource_;
					//
					mediaList.height = lItem.mediaItem.height;
					buzzitemmedia_.height = mediaList.height;
					calculatedHeight = mediaList.height;
					//
					prevIndex = lNewIndex;
				}
			} else if (lItem && !lItem.mediaItem) {
				// absent
				lItem.forceDownload();
			}

			if (lNewIndex !== -1) mediaIndicator.currentIndex = lNewIndex;
		}

		onWidthChanged: {
			adjustItems();
		}

		delegate: Rectangle {
			//
			id: mediaFrame
			color: "transparent"
			width: mediaList.width //mediaItem ? mediaItem.width + 2 * spaceItems_ : 100 //mediaList.width
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
					//console.info("[buzzitemmedia/adjustHeight]: proposed = " + proposed + ", buzzitemmedia_.calculatedHeight = " + buzzitemmedia_.calculatedHeight);
					buzzitemmedia_.calculatedHeight = proposed;
					mediaList.height = proposed;
				}
			}

			function forceDownload() {
				//
				if (!downloadCommand.processed_) {
					downloadWaitTimer.start();
					downloadCommand.process();
				}
			}

			function forceDownloadAndTryPlay() {
				//
				if (!downloadCommand.processed_) {
					downloadWaitTimer.start();
					downloadCommand.tryPlay_ = true;
					downloadCommand.process();
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
				property bool tryPlay_: false;

				function errorMediaLoading() {
					//
					tryReloadCount_++;
					if (tryReloadCount_ > 2) return;
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
					var lPSize = buzzerApp.getFileSize(previewFile);
					var lOSize = buzzerApp.getFileSize(originalFile);
					console.info("index = " + index + ", " + tx + ", " + previewFile + " - [" + lPSize + "], " + originalFile + " - [" + lOSize + "], " + orientation + ", " + duration + ", " + size + ", " + type);

					// stop timer
					downloadWaitTimer.stop();
					// set preview
					if (previewFile === "<stub>") preview_ = "qrc://images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "default.media.cover");
					else preview_ = "file://" + previewFile; // ONLY: preview binding here, path_ is for the inner component
					//
					if (previewFile === "<stub>") previewSource_ = preview_;
					else previewSource_ = "file://" + previewFile;
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
					// mime type
					mimeType_ = mimeType;
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
						console.info(lComponent.errorString());
					}

					mediaFrame.mediaItem = lComponent.createObject(mediaFrame);
					mediaFrame.mediaItem.adjustHeight.connect(mediaFrame.adjustHeight);
					mediaFrame.mediaItem.errorLoading.connect(errorMediaLoading);

					mediaFrame.mediaItem.frameColor = buzzitemmedia_.frameColor;
					mediaFrame.mediaItem.fillColor = buzzitemmedia_.fillColor;
					mediaFrame.mediaItem.width = mediaList.width;
					mediaFrame.mediaItem.mediaList = mediaList;
					mediaFrame.mediaItem.playerKey_ = playerKey_;
					mediaFrame.mediaItem.pkey_ = pkey_;
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

					if (tryPlay_) {
						//
						if (mediaFrame.mediaItem.playable) {
							console.info("trying to play...");
							mediaList.currentIndex = index; // move to
							mediaFrame.mediaItem.tryPlay();
							sharedMediaPlayer_.showCurrentPlayer(null);
							tryPlay_ = false;
						} else {
							// release lock in case if lock was take place
							buzzerApp.wakeRelease();
						}
					}
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
				interval: (!index || mediaList.mediaModel.count < 4 ? 2000 : 1000)
				repeat: false
				running: false

				onTriggered: {
					downloadCommand.process();
				}
			}

			Timer {
				id: downloadWaitTimer
				interval: (!index || mediaList.mediaModel.count < 4 ? 2000 : 1000)
				repeat: false
				running: false

				onTriggered: {
					mediaLoading.visible = true;
				}
			}

			Component.onCompleted: {
				if (!downloadCommand.processed_ && (!index || forcePlay_ || mediaList.mediaModel.count < 4)) {
					downloadWaitTimer.start();
					if (forcePlay_) downloadCommand.tryPlay_ = forcePlay_;
					downloadCommand.process();
				}
			}
		}

		function addMedia(url, file, forcePlay) {
			mediaList.mediaModel.append({
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
				previewSource_: "",
				mimeType_: "UNKNOWN",
				forcePlay_: (forcePlay ? true : false)});
		}

		function prepare() {
			if (mediaList.count) return;
			for (var lIdx = 0; lIdx < buzzMedia_.length && lIdx < 1 /*just first*/; lIdx++) {
				var lMedia = buzzMedia_[lIdx];
				addMedia(lMedia.url, buzzerClient.getTempFilesPath() + "/" + lMedia.tx);
			}

			mediaList.model = mediaList.mediaModel;
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
	}

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

	QuarkLabel {
		id: mediaPagesIndicator
		x: calculatedWidthInternal / 2 - width / 2
		y: spaceStats_ - (height + 3)
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - 2)) : defaultFontSize + 1
		text: "0/0"
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		visible: count > 5

		property var count: buzzMedia_ ? buzzMedia_.length : 0
		property var currentIndex: mediaIndicator.currentIndex

		onCurrentIndexChanged: {
			text = (currentIndex + 1) + "/" + count;
		}
	}

	/*
	QuarkLabel {
		id: mediaIndicator
		x: calculatedWidthInternal - (width + 3 * spaceItems_)
		y: spaceStats_ - (height + 3)
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - 2)) : defaultFontSize + 1
		text: "0/0"
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		visible: count > 1

		property var count: buzzMedia_ ? buzzMedia_.length : 0
		property var currentIndex: mediaList.currentIndex

		onCurrentIndexChanged: {
			text = (currentIndex + 1) + "/" + count;
		}
	}
	*/
}
