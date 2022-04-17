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
import QtMultimedia 5.15

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
	property var frameColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background")
	property var fillColor: "transparent"
	property var sharedMediaPlayer_

	//
	property var buzzitemmedia_;
	property var mediaList;

	signal adjustHeight(var proposed);
	signal errorLoading();

	//
	property var currentOrientation_: orientation_;

	function terminate() {
		//
		console.log("[itemVideo]: terminate");
		if (videoFrame.player) videoFrame.player.pause();
	}

	onCurrentOrientation_Changed: {
		//
		if (buzzerApp.isDesktop && videoOut) {
			if (currentOrientation_ == 6) videoOut.orientation = -90;
			else if (currentOrientation_ == 3) videoOut.orientation = -180;
			else if (currentOrientation_ == 8) videoOut.orientation = 90;
			else videoOut.orientation = 0;
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

	function forceVisibilityCheck(isFullyVisible) {
		//
		if (player && playing) {
			//
			if (!isFullyVisible) {
				videoFrame.sharedMediaPlayer_.showCurrentPlayer();
			} else {
				videoFrame.sharedMediaPlayer_.hideCurrentPlayer();
			}
		}
	}

	function adjust() {
		frameContainer.adjustView();
		previewImage.adjustView();
	}

	//
	color: "transparent"

	//
	Item {
		id: frameContainer
		//x: videoOut ? videoOut.contentRect.x : 0
		//y: videoOut ? videoOut.contentRect.y : 0
		//width: previewImage.visible ? previewImage.width : parent.width
		//height: previewImage.visible ? previewImage.height : parent.height

		width: videoOut ? videoOut.contentRect.width : parent.width
		height: videoOut ? videoOut.contentRect.height : parent.height

		visible: false

		property var correctedHeight: 0

		function adjustView() {
			//if (previewImage.visible) return;
			if (!buzzitemmedia_) return;
			if (mediaList) width = mediaList.width - 2 * spaceItems_;

			mediaList.height = Math.max(buzzitemmedia_.calculatedHeight, previewImage.visible ? 0 : calculatedHeight);
			// buzzitemmedia_.calculatedHeight = correctedHeight ? correctedHeight : mediaList.height;

			height = previewImage.visible ? previewImage.height : parent.height;
			//console.log("[frameContainer/adjustView]: height = " + height + ", parent.height = " + parent.height + ", buzzitemmedia_.calculatedHeight = " + buzzitemmedia_.calculatedHeight + ", correctedHeight = " + correctedHeight);

			adjustHeight(correctedHeight ? correctedHeight : mediaList.height);
		}

		function enableScene() {
			//
			//console.log("[onContentRectChanged(0)]: videoOutput.contentRect = " + videoOut.contentRect + ", playing = " + playing + ", calculatedHeight = " + calculatedHeight);
			//
			if (videoFrame.playing && videoFrame.player && videoFrame.player.hasVideo /*videoOut.contentRect.height > 0 &&
													videoOut.contentRect.height < calculatedHeight &&
														(orientation != 90 && orientation != -90)*/) {
				//console.log("[onContentRectChanged(1)]: videoOutput.contentRect = " + videoOut.contentRect);
				previewImage.visible = false;
				frameContainer.visible = true;
				height = videoOut.contentRect.height;
				adjustHeight(height);
				correctedHeight = height;
			}
		}

		function disableScene() {
			previewImage.visible = true;
			frameContainer.visible = false;
			previewImage.adjustView();
		}

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
	}

	BuzzerComponents.ImageQx {
		id: previewImage
		asynchronous: true
		radius: 8

		function adjustView() {
			if (previewImage.status === Image.Ready && mediaList && buzzitemmedia_) {
				//console.log("[onHeightChanged(1)/image]: height = " + height + ", width = " + width + ", implicitHeight = " + previewImage.implicitHeight);
				width = mediaList.width - 2*spaceItems_;
				adjustHeight(height);
				parent.height = height;
			}
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
		}

		onWidthChanged: {
			//
			if (width != originalWidth) {
				var lCoeff = (width * 1.0) / (originalWidth * 1.0)
				var lHeight = originalHeight * 1.0;
				height = lHeight * lCoeff;

				//console.log("[onHeightChanged/image/new height]: height = " + lHeight + ", lCoeff = " + lCoeff + ", width = " + width + ", originalWidth = " + originalWidth);
			}
		}

		source: preview_
		fillMode: BuzzerComponents.ImageQx.PreserveAspectFit
		mipmap: true

		visible: true //actionButton.needDownload || (!playing && ((player && player.position === 1) || !player) && buzzerApp.isDesktop)

		MouseArea {
			x: 0
			y: 0
			width: previewImage.width
			height: previewImage.height
			enabled: true
			cursorShape: Qt.PointingHandCursor

			ItemDelegate {
				x: 1
				y: 1
				width: previewImage.width - 2
				height: previewImage.height - 2
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

	property var player: null
	property var videoOut: null
	property bool playing: false

	function play() {
		//
		if (!player) {
			// controller
			var lVideoOut = videoFrame.sharedMediaPlayer_.createInstance(videoFrame, frameContainer);
			//
			if (lVideoOut) {
				lVideoOut.player.onPlaying.connect(mediaPlaying);
				lVideoOut.player.onPaused.connect(mediaPaused);
				lVideoOut.player.onStopped.connect(mediaStopped);
				lVideoOut.player.onStatusChanged.connect(playerStatusChanged);
				lVideoOut.player.onPositionChanged.connect(playerPositionChanged);
				lVideoOut.player.onError.connect(playerError);
				lVideoOut.linkActivated.connect(frameContainer.clickActivated);
				lVideoOut.onContentRectChanged.connect(frameContainer.enableScene);

				videoOut = lVideoOut;
				player = lVideoOut.player;

				player.source = path_;
				player.play();
			}
		} else {
			if (player.stopped)
				videoFrame.sharedMediaPlayer_.linkInstance(videoOut);
			player.play();
		}
	}

	function mediaPlaying() {
		if (!videoFrame) return;
		playing = true;
		frameContainer.enableScene();
		actionButton.adjust();
		//previewImage.visible = false;
	}

	function mediaPaused() {
		if (!videoFrame) return;
		playing = false;
		actionButton.adjust();
	}

	function mediaStopped() {
		if (!videoFrame) return;
		videoFrame.playing = false;
		actionButton.adjust();
		frameContainer.disableScene();
		elapsedTime.setTime(0);
		playSlider.value = 0;
	}

	function playerStatusChanged(status) {
		if (!videoFrame || !videoFrame.player) return;
		switch(videoFrame.player.status) {
			case MediaPlayer.Buffered:
				totalTime.setTotalTime(videoFrame.player.duration ? videoFrame.player.duration : duration_);
				totalSize.setTotalSize(size_);
				playSlider.to = videoFrame.player.duration ? videoFrame.player.duration : duration_;
				videoOut.fillMode = VideoOutput.PreserveAspectFit;
				frameContainer.adjustView();

				//console.log("[onStatusChanged/buffered/inner]: status = " + videoFrame.player.status + ", duration = " + videoFrame.player.duration);
			break;
		}
	}

	function playerError(error, errorString) {
		console.log("[onErrorStringChanged]: " + errorString);
		// in case of error
		downloadCommand.downloaded = false;
		downloadCommand.processing = false;
		downloadCommand.cleanUp();
	}

	function playerPositionChanged(position) {
		if (videoFrame && videoFrame.player) {
			elapsedTime.setTime(videoFrame.player.position);
			playSlider.value = videoFrame.player.position;
		}
	}

	//
	Rectangle {
		id: controlsBack
		x: frameContainer.visible ? (frameContainer.x + spaceItems_) : (previewImage.x + spaceItems_)
		y: frameContainer.visible ? (frameContainer.y + frameContainer.height - (actionButton.height + 3 * spaceItems_)) :
									(previewImage.y + previewImage.height - (actionButton.height + 3 * spaceItems_))
		width: frameContainer.visible ? (frameContainer.width - (2 * spaceItems_)) : (previewImage.width - (2 * spaceItems_))
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
									(videoFrame.playing ? Fonts.pauseSym : Fonts.playSym)
		fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 7)) : 18
		radius: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultRadius)) : defaultRadius
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.highlight")

		property bool needDownload: size_ && size_ > 1024*200 && !downloadCommand.downloaded

		opacity: 0.6

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
			} else if (!videoFrame.playing) {
				videoFrame.play();
			} else {
				if (videoFrame.player)
					videoFrame.player.pause();
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
										(videoFrame.playing ? Fonts.pauseSym : Fonts.playSym);
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
			if (mediaSize < 1000) text = ", " + mediaSize + "b";
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
			if (videoFrame.player) videoFrame.player.seek(value);
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
		animationDuration: 50

		function start() {
			beginAnimation = false;
			endAnimation = false;

			visible = true;
			arcBegin = 0;
			arcEnd = 0;

			beginAnimation = true;
			endAnimation = true;
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
			if (!videoFrame.playing) {
				videoFrame.play();
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
