import QtQuick 2.15
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Dialogs 1.1
//import QtGraphicalEffects 1.0
import QtGraphicalEffects 1.15
import Qt.labs.folderlistmodel 2.11
import QtMultimedia 5.8

import app.buzzer.components 1.0 as BuzzerComponents
import app.buzzer.commands 1.0 as BuzzerCommands

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

import "qrc:/lib/numberFunctions.js" as NumberFunctions
import "qrc:/lib/dateFunctions.js" as DateFunctions

Rectangle {
	//
	id: videoFrame

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
	property bool mediaView: false
	property int originalDuration: duration_
	property int totalSize_: size_
	property int calculatedHeight: 600

	//
	property var buzzitemmedia_;
	property var mediaList;

	signal adjustHeight(var proposed);

	//
	property var currentOrientation_: orientation_;

	onCurrentOrientation_Changed: {
		//
		if (buzzerApp.isDesktop) {
			if (currentOrientation_ == 6) videoOutput.orientation = -90;
			else if (currentOrientation_ == 3) videoOutput.orientation = -180;
			else if (currentOrientation_ == 8) videoOutput.orientation = 90;
			else videoOutput.orientation = 0;
		}
	}

	onTotalSize_Changed: {
		totalSize.setTotalSize(size_);
	}

	onOriginalDurationChanged: {
		playSlider.to = duration_;
	}

	onMediaListChanged: {
		adjust();
	}

	onBuzzitemmedia_Changed: {
		adjust();
	}

	onHeightChanged: {
		/*
		if (buzzitemmedia_)
			console.log("[onHeightChanged]: height = " + height + ", parent.height = " + parent.height + ", buzzitemmedia_.calculatedHeight = " + buzzitemmedia_.calculatedHeight);
		*/
		adjust();
	}

	function adjust() {
		videoOutput.adjustView();
		previewImage.adjustView();
	}

	//
	color: "transparent"
	width: player.width + 2 * spaceItems_

	//
	VideoOutput {
		id: videoOutput

		x: getX()
		y: getY()

		property var correctedHeight: 0

		function getX() {
			return 0;
		}

		function getY() {
			return 0;
		}

		function adjustView() {
			if (previewImage.visible) return;
			if (!buzzitemmedia_) return;
			if (mediaList) width = mediaList.width - 2 * spaceItems_;

			//console.log("[adjustView]: height = " + height + ", parent.height = " + parent.height + ", buzzitemmedia_.calculatedHeight = " + buzzitemmedia_.calculatedHeight);

			mediaList.height = Math.max(buzzitemmedia_.calculatedHeight, previewImage.visible ? 0 : calculatedHeight);
			// buzzitemmedia_.calculatedHeight = correctedHeight ? correctedHeight : mediaList.height;
			height = previewImage.visible ? previewImage.height : parent.height;
			adjustHeight(correctedHeight ? correctedHeight : mediaList.height);
		}

		width: parent.width
		height: previewImage.visible ? previewImage.height : parent.height
		visible: !actionButton.needDownload

		source: player

		fillMode: VideoOutput.PreserveAspectFit

		onSourceRectChanged: {
		}

		onContentRectChanged: {
			//
			if (!previewImage.visible && videoOutput.contentRect.height > 0 &&
													videoOutput.contentRect.height < calculatedHeight &&
														(orientation != 90 && orientation != -90)) {
				console.log("[onContentRectChanged]: videoOutput.contentRect = " + videoOutput.contentRect);
				height = videoOutput.contentRect.height;
				adjustHeight(height);
				correctedHeight = height;
			}
		}

		layer.enabled: buzzerApp.isDesktop
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
				return previewImage.width;
			}

			function getHeight() {
				return previewImage.height;
			}
		}

		MouseArea {
			id: linkClick
			x: 0
			y: 0
			width: videoOutput.width
			height: videoOutput.height
			enabled: true
			cursorShape: Qt.PointingHandCursor

			function clickActivated() {
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

			onClicked: {
				//
				if (!buzzerApp.isDesktop) {
					linkClick.clickActivated();
				}
			}

			ItemDelegate {
				id: linkClicked
				x: 0
				y: 0
				width: videoOutput.width
				height: videoOutput.height
				enabled: buzzerApp.isDesktop

				onClicked: {
					linkClick.clickActivated();
				}
			}
		}
	}

	Image {
		id: previewImage
		asynchronous: true

		function adjustView() {
			if (previewImage.status === Image.Ready && mediaList && buzzitemmedia_) {
				width = mediaList.width - 2*spaceItems_;
				adjustHeight(height);
				parent.height = height;
			}
		}

		onHeightChanged: {
			if (buzzitemmedia_) {
				adjustHeight(height);
			}
		}

		onStatusChanged: {
			adjustView();
		}

		source: preview_
		fillMode: VideoOutput.PreserveAspectFit
		mipmap: true

		visible: actionButton.needDownload || (!player.playing && player.position == 1 && buzzerApp.isDesktop)

		layer.enabled: true
		layer.effect: OpacityMask {
			id: roundEffect1
			maskSource: Item {
				width: roundEffect1.getWidth()
				height: roundEffect1.getHeight()

				Rectangle {
					//anchors.centerIn: parent
					x: roundEffect1.getX()
					y: roundEffect1.getY()
					width: roundEffect1.getWidth()
					height: roundEffect1.getHeight()
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
				return previewImage.width;
			}

			function getHeight() {
				return previewImage.height;
			}
		}

		MouseArea {
			x: 0
			y: 0
			width: previewImage.width
			height: previewImage.height
			enabled: true
			cursorShape: Qt.PointingHandCursor

			ItemDelegate {
				x: 0
				y: 0
				width: previewImage.width
				height: previewImage.height
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

	//
	QuarkRoundRectangle {
		id: frameContainer
		x: videoOutput.contentRect.x /*+ spaceItems_*/ - 1
		y: videoOutput.contentRect.y - 1
		width: videoOutput.contentRect.width + 2
		height: videoOutput.contentRect.height + 2

		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background")
		backgroundColor: "transparent"
		radius: 14
		penWidth: 9

		visible: !buzzerApp.isDesktop && !actionButton.needDownload
	}

	//
	MediaPlayer {
		id: player
		source: path_

		property bool playing: false;

		onPlaying: { playing = true; actionButton.adjust(); }
		onPaused: { playing = false; actionButton.adjust(); }
		onStopped: {
			playing = false;
			actionButton.adjust();
			player.seek(1);
		}

		onStatusChanged: {
			//
			switch(status) {
				case MediaPlayer.Buffered:
					totalTime.setTotalTime(duration ? duration : duration_);
					totalSize.setTotalSize(size_);
					playSlider.to = duration ? duration : duration_;
					//player.seek(1);
					videoOutput.fillMode = VideoOutput.PreserveAspectFit;
				break;
			}

			console.log("[onStatusChanged]: status = " + status + ", duration = " + duration);

			videoOutput.adjustView();
		}

		onErrorStringChanged: {
			console.log("[onErrorStringChanged]: " + errorString);
		}

		onPositionChanged: {
			elapsedTime.setTime(position);
			playSlider.value = position;
		}

		onPlaybackStateChanged: {
		}
	}

	//
	Rectangle {
		id: controlsBack
		x: frameContainer.visible ? (frameContainer.x + 2 * spaceItems_) : (previewImage.x + spaceItems_)
		y: frameContainer.visible ? (frameContainer.y + frameContainer.height - (actionButton.height + 4 * spaceItems_)) :
									(previewImage.y + previewImage.height - (actionButton.height + 3 * spaceItems_))
		width: frameContainer.visible ? (frameContainer.width - (4 * spaceItems_)) : (previewImage.width - (2 * spaceItems_))
		height: actionButton.height + 2 * spaceItems_
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		opacity: 0.3
		radius: 8
	}

	//
	QuarkRoundSymbolButton {
		id: actionButton
		x: controlsBack.x + spaceItems_
		y: controlsBack.y + spaceItems_
		spaceLeft: symbol === Fonts.arrowDownHollowSym || symbol === Fonts.pauseSym ? 2 :
					   (symbol === Fonts.playSym || symbol === Fonts.cancelSym ? 3 : 0)
		spaceTop: 2
		symbol: needDownload && !downloadCommand.downloaded ? Fonts.arrowDownHollowSym :
									(player.playing ? Fonts.pauseSym : Fonts.playSym)
		fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 7)) : 18
		radius: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultRadius)) : defaultRadius

		property bool needDownload: size_ && size_ > 1024*200 && !downloadCommand.downloaded

		onClick: {
			//
			if (needDownload && !downloadCommand.downloaded && !downloadCommand.processing) {
				// start
				symbol = Fonts.cancelSym;
				mediaLoading.start();
				downloadCommand.process();
			} else if (needDownload && !downloadCommand.downloaded && downloadCommand.processing) {
				// cancel
				symbol = Fonts.arrowDownHollowSym;
				mediaLoading.visible = false;
				downloadCommand.processing = false;
				downloadCommand.terminate();
			} else if (!player.playing) {
				player.play();
			} else {
				player.pause();
			}
		}

		function notLoaded() {
			symbol = Fonts.cancelSym;
			textColor = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.event.like");
		}

		function loaded() {
			symbol = Fonts.playSym;
			textColor = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")
		}

		function adjust() {
			symbol = needDownload && !downloadCommand.downloaded ? Fonts.arrowDownHollowSym :
										(player.playing ? Fonts.pauseSym : Fonts.playSym);
		}
	}

	QuarkLabel {
		id: elapsedTime
		x: actionButton.x + actionButton.width + spaceItems_
		y: actionButton.y + spaceItems_
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize)) : 11
		text: "00:00"

		function setTime(ms) {
			text = DateFunctions.msToTimeString(ms);
		}
	}

	QuarkLabel {
		id: totalTime
		x: elapsedTime.x + elapsedTime.width
		y: actionButton.y + spaceItems_
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize)) : 11
		text: duration_ ? ("/" + DateFunctions.msToTimeString(duration_)) : "/00:00"

		function setTotalTime(ms) {
			text = "/" + DateFunctions.msToTimeString(ms);
		}
	}

	QuarkLabel {
		id: totalSize
		x: totalTime.x + totalTime.width
		y: actionButton.y + spaceItems_
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize)) : 11
		text: ", 0k"
		visible: size_ !== 0

		function setTotalSize(mediaSize) {
			//
			if (mediaSize < 1000) text = ", " + size + "b";
			else text = ", " + NumberFunctions.numberToCompact(mediaSize);
		}
	}

	Slider {
		id: playSlider
		x: actionButton.x + actionButton.width // + spaceItems_
		y: actionButton.y + actionButton.height - (height - 3 * spaceItems_)
		from: 0
		to: 1
		orientation: Qt.Horizontal
		stepSize: 0.1
		//width: frameContainer.width - (actionButton.width + 3 * spaceItems_)
		width: frameContainer.visible ? (frameContainer.width - (actionButton.width + 5 * spaceItems_)) :
										(previewImage.width - (actionButton.width + 4 * spaceItems_))

		onMoved: {
			player.seek(value);
			elapsedTime.setTime(value);
		}

		onToChanged: {
			console.log("[onToChanged]: to = " + to);
		}
	}

	QuarkRoundProgress {
		id: mediaLoading
		x: actionButton.x - 2
		y: actionButton.y - 2
		size: buzzerClient.scaleFactor * (actionButton.width + 4)
		colorCircle: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link");
		colorBackground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link");
		arcBegin: 0
		arcEnd: 0
		lineWidth: buzzerClient.scaleFactor * 2
		visible: false

		function start() {
			visible = true;
			arcBegin = 0;
			arcEnd = 0;
		}

		function progress(pos, size) {
			//
			var lPercent = (pos * 100) / size;
			arcEnd = (360 * lPercent) / 100;
		}
	}

	BuzzerCommands.DownloadMediaCommand {
		id: downloadCommand
		preview: false
		skipIfExists: true
		url: url_
		localFile: key_
		pkey: pkey_

		property int tryCount_: 0;
		property bool processing: false;
		property bool downloaded: false;

		onProgress: {
			//
			processing = true;
			mediaLoading.progress(pos, size);
		}

		onProcessed: {
			// tx, previewFile, originalFile, orientation, duration, size, type
			console.log(tx + ", " + previewFile + ", " + originalFile + ", " + orientation + ", " + duration + ", " + size + ", " + type);

			//
			processing = false;
			downloaded = true;

			// set original orientation
			orientation_ = orientation;
			// set duration
			duration_ = duration;
			// set size
			size_ = size;
			// set size
			media_ = type;
			// set file
			path_ = "file://" + originalFile;
			// stop spinning
			mediaLoading.visible = false;

			// adjust button
			actionButton.adjust();

			// autoplay
			if (!player.playing) {
				player.play();
			}
		}

		onError: {
			if (code == "E_MEDIA_HEADER_NOT_FOUND") {
				//
				processing = false;
				downloaded = false;
				//
				tryCount_++;
				mediaLoading.visible = false;
				mediaLoading.notLoaded();
			}
		}
	}
}
