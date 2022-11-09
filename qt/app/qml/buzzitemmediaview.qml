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
	id: buzzitemmediaview_

	property string mediaViewTheme: "Darkmatter"
	property string mediaViewSelector: "dark"

	property int calculatedHeight: 400
	property int calculatedWidth: 500

	property var buzzMedia_: buzzMedia
	property var controller_: controller
	property var sharedMediaPlayer_

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
	readonly property real defaultFontSize: buzzerApp.defaultFontSize()
	property var pkey_: ""
	property var mediaIndex_: 0
	property var mediaPlayer_: null
	property var buzzId_: null
	property var buzzBody_: ""

	//
	// playback controller
	property alias mediaCount: mediaList.count
	property alias mediaContainer: mediaList
	//
	//

	Component.onCompleted: {
	}

	onSharedMediaPlayer_Changed: {
		mediaList.setSharedMediaPlayer(sharedMediaPlayer_);
		playerControl.mediaPlayerController = sharedMediaPlayer_;
	}

	onMediaIndex_Changed: {
		adjustPositionTimer.start();
	}

	onCalculatedWidthChanged: {
		mediaPagesIndicator.landscape = parent.width > parent.height;
		mediaPagesIndicator.adjust();
		mediaIndicator.landscape = mediaPagesIndicator.landscape;
		mediaIndicator.adjust();
	}

	onCalculatedHeightChanged: {
		mediaPagesIndicator.landscape = parent.width > parent.height;
		mediaPagesIndicator.adjust();
		mediaIndicator.landscape = mediaPagesIndicator.landscape;
		mediaIndicator.adjust();
	}

	function initialize(pkey, mediaIndex, player, buzzId, buzzBody) {
		if (key !== undefined) pkey_ = pkey;
		mediaPlayer_ = player;
		mediaIndex_ = mediaIndex;
		buzzBody_ = buzzBody;
		buzzId_ = buzzId;

		console.log("[initialize]: mediaIndex = " + mediaIndex + ", player = " + player + ", buzzId = " + buzzId);

		mediaList.prepare();
	}

	function adjust() {
		mediaList.adjust();
	}

	function checkPlaying() {
		mediaList.checkPlaying();
	}

	//
	// adjust index
	//

	Timer {
		id: adjustPositionTimer
		interval: 100
		repeat: false
		running: false

		onTriggered: {
			mediaList.preservedIndex = mediaIndex_;
			mediaIndicator.currentIndex = mediaList.preservedIndex;
			mediaList.currentIndex = mediaList.preservedIndex;

			if (mediaPlayer_ != null) {
				//
				mediaList.syncPlayer(mediaIndex_, mediaPlayer_);
			}
		}
	}

	//
	// media list
	//

	QuarkListView {
		id: mediaList
		x: 0
		y: 0
		width: calculatedWidth
		height: calculatedHeight
		clip: true
		orientation: Qt.Horizontal
		layoutDirection:  Qt.LeftToRight
		snapMode: ListView.SnapOneItem
		highlightFollowsCurrentItem: true
		highlightMoveDuration: -1
		highlightMoveVelocity: -1

		//displayMarginBeginning: 500
		//displayMarginEnd: 500
		cacheBuffer: 100000

		property var preservedIndex: -1

		function setSharedMediaPlayer(player) {
			//
			for (var lIdx = 0; lIdx < mediaList.count; lIdx++) {
				var lItem = mediaList.itemAtIndex(lIdx);
				if (lItem) {
					lItem.mediaItem.sharedMediaPlayer_ = player;
				}
			}
		}

		function adjust() {
			//
			preservedIndex = mediaList.currentIndex;
			//
			for (var lIdx = 0; lIdx < mediaList.count; lIdx++) {
				var lItem = mediaList.itemAtIndex(lIdx);
				if (lItem) {
					lItem.width = mediaList.width;
					lItem.height = mediaList.height;
					lItem.mediaItem.width = mediaList.width;
					if (lItem.mediaType !== "audio") lItem.mediaItem.height = mediaList.height;
					lItem.mediaItem.adjust();
				}
			}
		}

		function checkPlaying() {
			//
			for (var lIdx = 0; lIdx < mediaList.count; lIdx++) {
				var lItem = mediaList.itemAtIndex(lIdx);
				if (lItem && lItem.mediaItem) {
					lItem.mediaItem.checkPlaying();
				}
			}
		}

		function syncPlayer(mediaIndex, mediaPlayer) {
			//
			for (var lIdx = 0; lIdx < mediaList.count; lIdx++) {
				if (mediaIndex === lIdx) {
					var lItem = mediaList.itemAtIndex(lIdx);
					if (lItem && lItem.mediaItem) {
						lItem.mediaItem.syncPlayer(mediaPlayer);
					}
				}
			}
		}

		function playNext() {
			//
			console.log("[playNext]: mediaIndicator.currentIndex + 1 = " + (mediaIndicator.currentIndex + 1) + ", mediaList.currentIndex = " + mediaList.currentIndex);
			for (var lIdx = mediaIndicator.currentIndex + 1; lIdx < mediaList.count; lIdx++) {
				var lItem = mediaList.itemAtIndex(lIdx);
				if (lItem && lItem.mediaItem && lItem.mediaItem.playable) {
					preservedIndex = -1;
					mediaList.currentIndex = lIdx;
					lItem.mediaItem.tryPlay();
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
					preservedIndex = -1;
					mediaList.currentIndex = lIdx;
					lItem.mediaItem.tryPlay();
					return true;
				}
			}

			return false;
		}

		onContentXChanged: {
			//
			if (mediaList.count > 1) {
				//
				var lCurrentItem;
				if (preservedIndex != -1) {
					//
					if (mediaList.currentIndex >= 0) {
						lCurrentItem = mediaList.itemAtIndex(mediaList.currentIndex);
						if (lCurrentItem) lCurrentItem.mediaItem.reset();
					}

					mediaIndicator.currentIndex = preservedIndex;
					mediaList.currentIndex = preservedIndex;

					// try to track user attention
					if (buzzitemmediaview_.sharedMediaPlayer_.isCurrentInstancePlaying() ||
						buzzitemmediaview_.sharedMediaPlayer_.isCurrentInstancePaused()) {
						buzzitemmediaview_.sharedMediaPlayer_.showCurrentPlayer(null);
					}

					preservedIndex = -1;
				} else {
					var lIndex = mediaList.indexAt(mediaList.contentX, 0);
					if (lIndex >= 0) {
						//
						var lItem = mediaList.itemAtIndex(lIndex);
						if (lItem.x === mediaList.contentX) {
							//
							if (mediaList.currentIndex >= 0) {
								lCurrentItem = mediaList.itemAtIndex(mediaList.currentIndex);
								if (lCurrentItem) lCurrentItem.mediaItem.reset();
							}

							mediaIndicator.currentIndex = mediaList.indexAt(mediaList.contentX, 0);
							mediaList.currentIndex = mediaIndicator.currentIndex;

							// try to track user attention
							if (buzzitemmediaview_.sharedMediaPlayer_.isCurrentInstancePlaying() ||
								buzzitemmediaview_.sharedMediaPlayer_.isCurrentInstancePaused()) {
								buzzitemmediaview_.sharedMediaPlayer_.showCurrentPlayer(null);
							}
						}
					}
				}

				//
				var lNewCurrentItem = mediaList.itemAtIndex(mediaList.currentIndex);
				if (lNewCurrentItem) {
					lNewCurrentItem.tryDownload();
				}
			}
		}

		/*
		add: Transition {
			NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 400 }
		}
		*/

		model: ListModel { id: mediaModel }

		delegate: Rectangle {
			//
			id: mediaFrame
			color: "transparent"
			width: mediaList.width
			height: mediaList.height

			property var mediaItem;
			property var mediaType;

			function tryDownload() {
				downloadCommand.previewMediaLoaded();
			}

			//property bool isFullyVisible: mediaFrame.x >= mediaList.contentX && mediaFrame.x + mediaFrame.width <= mediaList.contentX + mediaList.width

			/*
			onIsFullyVisibleChanged: {
				if (mediaFrame !== null && mediaFrame.mediaItem !== null && mediaFrame.mediaItem !== undefined) {
					try {
						mediaFrame.mediaItem.forceVisibilityCheck(mediaFrame.isFullyVisible);
					} catch (err) {
						console.log("[onIsFullyVisibleChanged]: " + err + ", itemDelegate.buzzItem = " + mediaFrame.buzzItem);
					}
				}
			}
			*/

			BuzzerCommands.DownloadMediaCommand {
				id: downloadCommand
				preview: true
				skipIfExists: true // (pkey_ !== "" || pkey_ !== undefined) ? false : true
				url: url_
				localFile: key_
				pkey: pkey_

				property int tryCount_: 0;
				property int tryReloadCount_: 0;

				function errorMediaLoading() {
					//
					tryReloadCount_++;
					if (tryReloadCount_ > 2 || downloadCommand.processing) return;
					// cleaning up
					downloadCommand.cleanUp();
					// re-process
					if (mediaFrame.mediaItem && media_ === "image")
						mediaFrame.mediaItem.showLoading();
					downloadCommand.process();
				}

				function previewMediaLoaded() {
					// re-process
					if (preview && index == mediaList.currentIndex) {
						preview = false;
						if (mediaFrame.mediaItem && media_ === "image")
							mediaFrame.mediaItem.showLoading();
						downloadCommand.process();
					}
				}
				
				onProgress: {
					//
					if (media_ === "image") {
						mediaFrame.mediaItem.loadingProgress(pos, size);
					}
				}

				onProcessed: {
					// tx, previewFile, originalFile, orientation, duration, size, type
					var lPSize = buzzerApp.getFileSize(previewFile);
					var lOSize = buzzerApp.getFileSize(originalFile);
					console.info("[buzzitemmediaview]: tx = " + tx + ", " + previewFile + " - [" + lPSize + "], " + originalFile + " - [" + lOSize + "], " + orientation + ", " + duration + ", " + size + ", " + type + ", preview = " + preview);

					// stop timer
					downloadTimer.stop();
					// set file
					if (originalFile.length) {
						originalPath_ = originalFile;
						path_ = "file://" + originalFile;
					}
					// set file
					if (previewFile === "<stub>") preview_ = "qrc://images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "default.media.cover");
					else preview_ = "file://" + previewFile;
					// set original orientation
					orientation_ = orientation;
					// set duration
					duration_ = duration;
					// set size
					size_ = size;
					// common ty[e
					media_ = type;
					// description
					caption_ = description;
					// mime type
					mimeType_ = mimeType;
					// use preview
					usePreview_ = preview;

					if (mediaFrame.mediaItem && media_ === "image") {
						// stop spinning
						mediaFrame.mediaItem.hideLoading();
					}

					if (!mediaFrame.mediaItem) {
						//
						var lComponent;
						var lSource;

						if (type === "audio") {
							//
							lSource = "qrc:/qml/buzzitemmedia-audio.qml";
							lComponent = Qt.createComponent(lSource);

							mediaFrame.mediaItem = lComponent.createObject(mediaFrame);
							mediaFrame.mediaItem.errorLoading.connect(errorMediaLoading);

							mediaFrame.mediaItem.fillColor = buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.disabledHidden.uni")
							mediaFrame.mediaItem.width = mediaList.width;
							mediaFrame.mediaItem.mediaList = mediaList;
							mediaFrame.mediaItem.buzzitemmedia_ = buzzitemmediaview_;
							mediaFrame.mediaItem.sharedMediaPlayer_ = buzzitemmediaview_.sharedMediaPlayer_;

							mediaFrame.height = mediaFrame.mediaItem.height;
							mediaFrame.width = mediaList.width;

							mediaFrame.mediaType = "audio";

							mediaFrame.mediaItem.mediaView = true;
						} else if (type === "image") {
							//
							lSource = "qrc:/qml/buzzitemmediaview-image.qml";
							lComponent = Qt.createComponent(lSource);

							mediaFrame.mediaItem = lComponent.createObject(mediaFrame);
							mediaFrame.mediaItem.errorLoading.connect(errorMediaLoading);
							mediaFrame.mediaItem.previewLoaded.connect(previewMediaLoaded);

							mediaFrame.mediaItem.width = mediaList.width - 2 * spaceItems_;
							mediaFrame.mediaItem.height = mediaList.height;
							//mediaFrame.mediaItem.calculatedWidth = calculatedWidth;
							mediaFrame.mediaItem.mediaList = mediaList;
							mediaFrame.mediaItem.buzzitemmedia_ = buzzitemmediaview_;
							mediaFrame.mediaItem.sharedMediaPlayer_ = buzzitemmediaview_.sharedMediaPlayer_;

							mediaFrame.height = mediaList.height;
							mediaFrame.width = mediaList.width;

							mediaFrame.mediaType = "image";

							// stop spinning
							mediaFrame.mediaItem.hideLoading();

							// re-process
							/*
							if (preview) {
								preview = false;
								mediaFrame.mediaItem.showLoading();
								downloadCommand.process();
							}
							*/
						} else if (type === "video") {
							//
							lSource = "qrc:/qml/buzzitemmediaview-video.qml";
							lComponent = Qt.createComponent(lSource);
							if (lComponent.status === Component.Error) {
								window.showError(lComponent.errorString());
							} else {
								mediaFrame.mediaItem = lComponent.createObject(mediaFrame);
								mediaFrame.mediaItem.errorLoading.connect(errorMediaLoading);

								mediaFrame.mediaItem.width = mediaList.width;
								mediaFrame.mediaItem.height = mediaList.height;
								//mediaFrame.mediaItem.calculatedWidth = calculatedWidth;
								mediaFrame.mediaItem.mediaList = mediaList;
								mediaFrame.mediaItem.buzzitemmedia_ = buzzitemmediaview_;
								mediaFrame.mediaItem.sharedMediaPlayer_ = buzzitemmediaview_.sharedMediaPlayer_;

								mediaFrame.height = mediaList.height;
								mediaFrame.width = mediaList.width;

								mediaFrame.mediaType = "video";
							}
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
							// mediaLoading.visible = false;
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

			Component.onCompleted: {
				//mediaLoading.visible = true;
				downloadCommand.process();
			}
		}

		function addMedia(url, file) {
			mediaModel.append({
				key_: file,
				url_: url,
				path_: "",
				originalPath_: "",
				media_: "unknown",
				preview_: "",
				usePreview_: true,
				size_: 0,
				duration_: 0,
				orientation_: 0,
				loaded_: false,
				description_: (buzzBody_ ? buzzBody_ : ""),
				caption_: "",
				mimeType_: "UNKNOWN"});
		}

		function prepare() {
			for (var lIdx = 0; lIdx < buzzMedia_.length; lIdx++) {
				var lMedia = buzzMedia_[lIdx];
				addMedia(lMedia.url, buzzerClient.getTempFilesPath() + "/" + lMedia.tx);
				console.log("[prepare]: " + buzzMedia_.length + " / " + lMedia.url);
			}
		}

		function clear() {
			mediaModel.clear();
		}
	}

	PageIndicator {
		id: mediaIndicator
		count: buzzMedia_ ? buzzMedia_.length : 0
		currentIndex: mediaList.currentIndex
		interactive: buzzerApp.isDesktop

		property bool landscape: false

		x: calculatedWidth / 2 - width / 2
		y: spaceStats_ - (height + (landscape ? 0 : 5))
		visible: buzzMedia_ ? buzzMedia_.length > 1 && buzzMedia_.length <= 5 : false

		Material.theme: Material.Dark
		Material.accent: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.accent");
		Material.background: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.background");
		Material.foreground: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.foreground");
		Material.primary: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.primary");

		function adjust() {
			y = spaceStats_ - (height + (landscape ? 0 : 5));
		}
	}

	QuarkLabel {
		id: mediaPagesIndicator
		x: buzzerApp.isDesktop || (buzzerApp.isTablet && !buzzerApp.isPortrait()) ? calculatedWidth / 2 - width / 2 : (calculatedWidth - (width + 2*spaceItems_))
		y: spaceStats_ - (height + (landscape ? 0 : 5))
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 1)) : defaultFontSize + 1
		text: "0/0"
		color: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.foreground")
		visible: count > 5

		property bool landscape: false
		property var count: buzzMedia_ ? buzzMedia_.length : 0
		property var currentIndex: mediaIndicator.currentIndex

		onCurrentIndexChanged: {
			text = (currentIndex + 1) + "/" + count;
		}

		function adjust() {
			y = spaceStats_ - (height + (landscape ? 0 : 5));
		}
	}
}
