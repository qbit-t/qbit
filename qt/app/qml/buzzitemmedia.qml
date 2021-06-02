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

	property int calculatedHeight: 0 //400
	property int calculatedWidth: 500
	property int calculatedWidthInternal: 500
	property var buzzId_: buzzId
	property var buzzMedia_: buzzMedia
	property var controller_: controller

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

	Component.onCompleted: {
	}

	function initialize(key) {
		if (key !== undefined) pkey_ = key;
		mediaList.prepare();
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

		onContentXChanged: {
			mediaIndicator.currentIndex = indexAt(contentX, 1);
			//
			var lItem = mediaList.itemAtIndex(mediaIndicator.currentIndex);
			if (lItem) {
				//console.log("[onCurrentIndexChanged]: height = " + lItem.height);
				mediaList.height = lItem.height;
				buzzitemmedia_.height = mediaList.height;
				calculatedHeight = mediaList.height;
			}
		}

		add: Transition {
			NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 400 }
		}

		model: ListModel { id: mediaModel }

		onWidthChanged: {
			if (currentItem && currentItem.mediaItem) {
				currentItem.mediaItem.adjust();
			}
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

			BuzzerCommands.DownloadMediaCommand {
				id: downloadCommand
				preview: true
				skipIfExists: true // (pkey_ !== "" || pkey_ !== undefined) ? false : true
				url: url_
				localFile: key_
				pkey: pkey_

				property int tryCount_: 0;

				onProgress: {
					//
					mediaLoading.progress(pos, size);
				}

				onProcessed: {
					// tx, previewFile, originalFile, orientation, duration, size, type
					// console.log(tx + ", " + previewFile + ", " + originalFile + ", " + orientation + ", " + duration + ", " + size + ", " + type);

					// stop timer
					downloadWaitTimer.stop();
					// set file
					if (preview) path_ = "file://" + previewFile;
					else path_ = "file://" + originalFile;
					// set original orientation
					orientation_ = orientation;
					// set duration
					duration_ = duration;
					// set size
					size_ = size;
					// set size
					media_ = type;
					// stop spinning
					mediaLoading.visible = false;

					//
					var lComponent;
					var lSource = "qrc:/qml/buzzitemmedia-image.qml";
					if (type === "audio")
						lSource = "qrc:/qml/buzzitemmedia-audio.qml";
					lComponent = Qt.createComponent(lSource);

					mediaFrame.mediaItem = lComponent.createObject(mediaFrame);

					mediaFrame.mediaItem.width = mediaList.width;
					mediaFrame.mediaItem.mediaList = mediaList;
					mediaFrame.mediaItem.buzzitemmedia_ = buzzitemmedia_;

					if (index === 0) mediaFrame.height = mediaFrame.mediaItem.height;
					mediaFrame.width = mediaList.width;

					// reset height
					if (index === 0) buzzitemmedia_.calculatedHeight = mediaFrame.height;
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
				downloadWaitTimer.start();
				downloadCommand.process();
			}
		}

		function addMedia(url, file) {
			mediaModel.append({
				key_: file,
				url_: url,
				path_: "",
				media_: "unknown",
				size_: 0,
				duration_: 0,
				orientation_: 0,
				loaded_: false });
		}

		function prepare() {
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
		visible: buzzMedia_ ? buzzMedia_.length > 1 : false

		Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
		Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
		Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
		Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
		Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");
	}
}
