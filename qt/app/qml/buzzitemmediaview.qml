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

	property int calculatedHeight: 400
	property int calculatedWidth: 500
	property var buzzMedia_: buzzMedia
	property var controller_: controller
	//property bool preview_: true

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
	onCalculatedHeightChanged: { // calculatedHeightModified(calculatedHeight);
		if (!imageView_ /*&& !buzzerApp.isDesktop*/) {
			mediaList.clear();
			mediaList.prepare();
		}
	}

	Component.onCompleted: {
	}

	function initialize(pkey) {
		if (key !== undefined) pkey_ = pkey;
		mediaList.prepare();
	}

	property var imageView_: undefined
	property int imageViewIndex_: 0

	function imageViewClosed() {
		imageView_ = undefined;
		mediaList.clear();
		mediaList.prepare();
		mediaList.currentIndex = imageViewIndex_;
	}

	onCalculatedWidthChanged: {
		if (!imageView_ /*&& !buzzerApp.isDesktop*/) {
			mediaList.clear();
			mediaList.prepare();
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

		onContentXChanged: {
			mediaIndicator.currentIndex = indexAt(contentX, 1);
		}

		add: Transition {
			NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 400 }
		}

		model: ListModel { id: mediaModel }

		delegate: Rectangle {
			//
			id: mediaFrame
			color: "transparent"
			width: mediaModel.count > 1 ?
					   (mediaImage.width ? mediaImage.width : mediaList.width - 2 * spaceItems_) + 2 * spaceItems_ :
					   calculatedWidth
			height: calculatedHeight

			property var mediaItem;

			function createImageView() {
				if (!buzzerApp.isDesktop) {
					// expand
					var lSource;
					var lComponent;

					// viewer
					lSource = buzzerApp.isDesktop ? "qrc:/qml/imageview-desktop.qml" :
													"qrc:/qml/imageview.qml";
					lComponent = Qt.createComponent(lSource);

					if (lComponent.status === Component.Error) {
						controller_.showError(lComponent.errorString());
					} else {
						imageViewIndex_ = index;
						imageView_ = lComponent.createObject(controller_);
						imageView_.pageClosed.connect(buzzitemmediaview_.imageViewClosed);
						imageView_.initialize(path_, controller_);
						controller_.addPage(imageView_);
					}
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

				function errorMediaLoading() {
					//
					tryReloadCount_++;
					if (tryReloadCount_ > 3) return;
					// cleaning up
					downloadCommand.cleanUp();
					// re-process
					if (mediaFrame.mediaItem && media_ === "image")
						mediaFrame.mediaItem.showLoading();
					downloadCommand.process();
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
					console.log("[buzzitemmediaview]: tx = " + tx + ", " + previewFile + " - [" + lPSize + "], " + originalFile + " - [" + lOSize + "], " + orientation + ", " + duration + ", " + size + ", " + type);

					// stop timer
					downloadWaitTimer.stop();
					// set file
					if (originalFile.length) path_ = "file://" + originalFile;
					// set file
					preview_ = "file://" + previewFile;
					// set original orientation
					orientation_ = orientation;
					// set duration
					duration_ = duration;
					// set size
					size_ = size;
					// set size
					media_ = type;
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

							mediaFrame.mediaItem.width = mediaList.width;
							mediaFrame.mediaItem.mediaList = mediaList;
							mediaFrame.mediaItem.buzzitemmedia_ = buzzitemmediaview_;

							mediaFrame.height = mediaFrame.mediaItem.height;
							mediaFrame.width = mediaList.width;

							mediaFrame.mediaItem.mediaView = true;
						} else if (type === "image") {
							//
							lSource = "qrc:/qml/buzzitemmediaview-image.qml";
							lComponent = Qt.createComponent(lSource);

							mediaFrame.mediaItem = lComponent.createObject(mediaFrame);
							mediaFrame.mediaItem.errorLoading.connect(errorMediaLoading);

							mediaFrame.mediaItem.width = mediaList.width - 2 * spaceItems_;
							mediaFrame.mediaItem.calculatedWidth = calculatedWidth;
							mediaFrame.mediaItem.mediaList = mediaList;
							mediaFrame.mediaItem.buzzitemmedia_ = buzzitemmediaview_;
							mediaFrame.mediaItem.createViewHandler = mediaFrame.createImageView;

							mediaFrame.height = mediaFrame.mediaItem.height;
							mediaFrame.width = mediaList.width;

							// stop spinning
							mediaFrame.mediaItem.hideLoading();

							// re-process
							if (preview) {
								preview = false;
								mediaFrame.mediaItem.showLoading();
								downloadCommand.process();
							}
						} else if (type === "video") {
							//
							lSource = "qrc:/qml/buzzitemmediaview-video.qml";
							lComponent = Qt.createComponent(lSource);
							if (lComponent.status === Component.Error) {
								window.showError(lComponent.errorString());
							} else {
								mediaFrame.mediaItem = lComponent.createObject(mediaFrame);

								mediaFrame.mediaItem.width = mediaList.width - 2 * spaceItems_;
								mediaFrame.mediaItem.calculatedWidth = calculatedWidth;
								mediaFrame.mediaItem.mediaList = mediaList;
								mediaFrame.mediaItem.buzzitemmedia_ = buzzitemmediaview_;

								mediaFrame.height = mediaFrame.mediaItem.height;
								mediaFrame.width = mediaList.width;
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
				media_: "unknown",
				preview_: "",
				usePreview_: false,
				size_: 0,
				duration_: 0,
				orientation_: 0,
				loaded_: false });
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

		onCurrentIndexChanged: {
			mediaList.positionViewAtIndex(currentIndex, ListView.Beginning);
		}

		x: calculatedWidth / 2 - width / 2
		y: spaceStats_ - height
		visible: buzzMedia_ ? buzzMedia_.length > 1 : false

		Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
		Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
		Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
		Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
		Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");
	}
}
