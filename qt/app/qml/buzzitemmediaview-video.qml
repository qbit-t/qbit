import QtQuick 2.15
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Dialogs 1.1
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
	property int originalDuration: duration_
	property int totalSize_: size_
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

	function forceVisibilityCheck(isFullyVisible) {
		//
		if (player && (playing || downloadCommand.processing)) {
			//
			if (!isFullyVisible && frameContainer.scale == 1.0) {
				videoFrame.sharedMediaPlayer_.showCurrentPlayer();
			} else {
				videoFrame.sharedMediaPlayer_.hideCurrentPlayer();
			}
		}
	}

	function checkPlaying() {
		//
		if (player) {
			if (videoFrame.sharedMediaPlayer_.isCurrentInstancePaused()) player.stop();
			else if (playing || downloadCommand.processing) videoFrame.sharedMediaPlayer_.showCurrentPlayer();

			unlink();
		}
	}

	function adjust() {
		frameContainer.adjustView();
		previewImageVideo.adjustView();
	}

	function syncPlayer(mediaPlayer) {
		//
		if (mediaPlayer) play(mediaPlayer);
	}

	function reset() {
		//
		frameContainer.scale = 1.0;
		frameContainer.x = frameContainer.getX();
		frameContainer.y = frameContainer.getY();
	}

	//
	color: "transparent"
	width: parent.width
	height: parent.height

	//
	Item {
		id: frameContainer
		visible: false

		function getX() {
			return (mediaList ? mediaList.width / 2 : parent.width / 2) - width / 2;
		}

		function getY() {
			return (mediaList ? mediaList.height / 2 : parent.height / 2) - height / 2;
		}

		property var correctedHeight: 0

		function adjustView() {
		}

		onScaleChanged: {
			mediaList.interactive = scale == 1.0;
		}

		function enableScene() {
			//
			// console.log("[onContentRectChanged(0)]: videoOutput.contentRect = " + videoOut.contentRect + ", videoFrame.playing = " + videoFrame.playing + ", videoFrame.player = " + videoFrame.player + ", videoFrame.player.hasVideo = " + videoFrame.player.hasVideo);
			if (videoFrame.playing && videoFrame.player && videoFrame.player.hasVideo && videoFrame.player.position > 1) {
				previewImageVideo.visible = false;
				frameContainer.visible = true;
				pinchHandler.enabled = true;

				previewImageVideo.adjustFrameContainer();

				controlsBack.adjustX();
				controlsBack.adjustY();
			}
		}

		function disableScene() {
			previewImageVideo.visible = true;
			frameContainer.visible = false;
			pinchHandler.enabled = false;
			//previewImageVideo.adjustView();
		}

		function clickActivated() {
		}
	}

	PinchHandler {
		id: pinchHandler
		minimumRotation: 0
		maximumRotation: 0
		minimumScale: 1.0
		maximumScale: 10.0
		target: frameContainer
		enabled: !previewImageVideo.visible

		onActiveChanged: {
		}
	}

	MouseArea {
		id: dragArea
		hoverEnabled: true
		x: 0
		y: 0
		width: parent.width
		height: parent.height
		drag.target: frameContainer
		drag.threshold: 20
		cursorShape: Qt.PointingHandCursor

		drag.onActiveChanged: {
		}

		drag.minimumX: (parent.width - frameContainer.width * frameContainer.scale) / 2.0
		drag.maximumX: (frameContainer.width * frameContainer.scale - parent.width) / 2.0

		enabled: true

		onPressedChanged: {
		}

		onClicked: {
			if (frameContainer.scale == 1.0) {
				forceVisible = !forceVisible;
				frameContainer.x = frameContainer.getX();
				frameContainer.y = frameContainer.getY();
			} else {
				frameContainer.scale = 1.0;
				frameContainer.x = frameContainer.getX();
				frameContainer.y = frameContainer.getY();
			}
		}
	}

	BuzzerComponents.ImageQx {
		id: previewImageVideo
		autoTransform: true
		asynchronous: true
		//radius: 8

		x: getX()
		y: getY()

		function getX() {
			return (mediaList ? mediaList.width / 2 : parent.width / 2) - width / 2;
		}

		function getY() {
			return (mediaList ? mediaList.height / 2 : parent.height / 2) - height / 2;
		}

		function adjustView() {
			if (previewImageVideo.status === Image.Ready && mediaList && buzzitemmedia_) {
				//
				if (calculatedWidth > height || originalHeight > calculatedHeight * 2)
					height = calculatedHeight - 20;
				else
					width = calculatedWidth - 2*spaceItems_;

				x = getX();
				controlsBack.adjustX();

				y = getY();
				//controlsBack.adjustY();

				adjustFrameContainer();
			}
		}

		function adjustFrameContainer() {
			if (videoOut) {
				var lContentCoeff = (videoOut.sourceRect.width  * 1.0) / (videoOut.sourceRect.height * 1.0);
				var lImageCoeff = (width * 1.0) / (height * 1.0);

				if (lContentCoeff !== lImageCoeff) {
					// own
					var lCoeff = (frameContainer.width * 1.0) / (videoOut.sourceRect.width * 1.0)
					var lHeight = videoOut.sourceRect.height * 1.0;

					if (lHeight * lCoeff > calculatedHeight - 20)
						frameContainer.height = calculatedHeight - 20;
					else
						frameContainer.height = lHeight * lCoeff;

					frameContainer.y = frameContainer.getY();

					//
					lCoeff = (frameContainer.height * 1.0) / (videoOut.sourceRect.height * 1.0)
					var lWidth = videoOut.sourceRect.width * 1.0;
					if (lWidth * lCoeff > calculatedWidth - 2*spaceItems_)
						frameContainer.width = calculatedWidth - 2*spaceItems_;
					else
						frameContainer.width = lWidth * lCoeff;

					frameContainer.x = frameContainer.getX();
				}
			}
		}

		fillMode: BuzzerComponents.ImageQx.PreserveAspectFit
		mipmap: true

		source: preview_

		Component.onCompleted: {
		}

		onSourceChanged: {
		}

		onStatusChanged: {
			adjustView();
			//
			if (status == Image.Error) {
				//
				// force to reload
				console.log("[onStatusChanged]: forcing reload of " + path_);
				//downloadCommand
				errorLoading();
			}
		}

		onWidthChanged: {
			//
			if (width != originalWidth) {
				var lCoeff = (width * 1.0) / (originalWidth * 1.0)
				var lHeight = originalHeight * 1.0;

				if (lHeight * lCoeff > calculatedHeight - 20)
					height = calculatedHeight - 20;
				else
					height = lHeight * lCoeff;

				y = getY();
				frameContainer.y = y;
				frameContainer.height = height;
				controlsBack.adjustY();

				adjustFrameContainer();
			}
		}

		onHeightChanged: {
			if (height != originalHeight) {
				var lCoeff = (height * 1.0) / (originalHeight * 1.0)
				var lWidth = originalWidth * 1.0;
				if (lWidth * lCoeff > calculatedWidth - 2*spaceItems_)
					width = calculatedWidth - 2*spaceItems_;
				else
					width = lWidth * lCoeff;

				x = getX();
				frameContainer.x = x;
				frameContainer.width = width;
				controlsBack.adjustX();

				adjustFrameContainer();
			}
		}
	}

	property var player: null
	property var videoOut: null
	property bool playing: false

	function unlink() {
		//
		if (videoOut) {
			console.log("[buzzmediaview/unlink]: unlinking video instance...");
			videoOut.player.onPlaying.disconnect(mediaPlaying);
			videoOut.player.onPaused.disconnect(mediaPaused);
			videoOut.player.onStopped.disconnect(mediaStopped);
			videoOut.player.onStatusChanged.disconnect(playerStatusChanged);
			videoOut.player.onPositionChanged.disconnect(playerPositionChanged);
			videoOut.player.onError.disconnect(playerError);
			videoOut.player.onHasVideoChanged.disconnect(playerHasVideo);
			videoOut.linkActivated.disconnect(frameContainer.clickActivated);
			videoOut.onContentRectChanged.disconnect(frameContainer.enableScene);
		}
	}

	function play(existingPlayer) {
		//
		if (existingPlayer) {
			// controller
			//var singleVideoOut = videoFrame.sharedMediaPlayer_.createVideoInstance(frameContainer, existingPlayer);
			var lSingleVideoOut = videoFrame.sharedMediaPlayer_.createInstance(videoFrame, frameContainer, true);
			console.log("[buzzmediaview/play]: video instance created...");
			lSingleVideoOut.player.description = description_;
			lSingleVideoOut.player.caption = caption_;
			lSingleVideoOut.player.onPlaying.connect(mediaPlaying);
			lSingleVideoOut.player.onPaused.connect(mediaPaused);
			lSingleVideoOut.player.onStopped.connect(mediaStopped);
			lSingleVideoOut.player.onStatusChanged.connect(playerStatusChanged);
			lSingleVideoOut.player.onPositionChanged.connect(playerPositionChanged);
			lSingleVideoOut.player.onError.connect(playerError);
			lSingleVideoOut.player.onHasVideoChanged.connect(playerHasVideo);
			lSingleVideoOut.linkActivated.connect(frameContainer.clickActivated);
			lSingleVideoOut.onContentRectChanged.connect(frameContainer.enableScene);
			lSingleVideoOut.allowClick = false;

			videoOut = lSingleVideoOut;
			player = lSingleVideoOut.player;

			videoOut.fillMode = VideoOutput.PreserveAspectFit;
			videoOut.anchors.fill = frameContainer;
			frameContainer.adjustView();

			if (!player.playing) {
				downloadCommand.downloaded = true;
				player.play();
			} else {
				downloadCommand.downloaded = true;
				mediaPlaying();
			}
		} else {
			if (!player) {
				// controller
				var lVideoOut = videoFrame.sharedMediaPlayer_.createInstance(videoFrame, frameContainer);
				//
				if (lVideoOut) {
					lVideoOut.player.description = description_;
					lVideoOut.player.caption = caption_;
					lVideoOut.player.onPlaying.connect(mediaPlaying);
					lVideoOut.player.onPaused.connect(mediaPaused);
					lVideoOut.player.onStopped.connect(mediaStopped);
					lVideoOut.player.onStatusChanged.connect(playerStatusChanged);
					lVideoOut.player.onPositionChanged.connect(playerPositionChanged);
					lVideoOut.player.onError.connect(playerError);
					lVideoOut.player.onHasVideoChanged.connect(playerHasVideo);
					lVideoOut.linkActivated.connect(frameContainer.clickActivated);
					lVideoOut.onContentRectChanged.connect(frameContainer.enableScene);
					lVideoOut.allowClick = false;

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
	}

	function mediaPlaying() {
		//if (!videoFrame) return;
		console.log("[buzzmediaview/mediaPlaying]: playing...");
		playing = true;
		frameContainer.enableScene();
		actionButton.adjust();
		buzzerApp.wakeLock();
	}

	function mediaPaused() {
		console.log("[buzzmediaview/mediaPaused]: paused...");
		//if (!videoFrame) return;
		playing = false;
		actionButton.adjust();
		buzzerApp.wakeRelease();
	}

	function mediaStopped() {
		console.log("[buzzmediaview/mediaStopped]: stopped...");
		//if (!videoFrame) return;
		videoFrame.playing = false;
		actionButton.adjust();
		frameContainer.disableScene();
		elapsedTime.setTime(0);
		playSlider.value = 0;
		//player = null;
		buzzerApp.wakeRelease();
	}

	function playerStatusChanged(status) {
		if (!videoFrame || !videoFrame.player) return;
		switch(videoFrame.player.status) {
			case MediaPlayer.Buffered:
				totalTime.setTotalTime(videoFrame.player.duration ? videoFrame.player.duration : duration_);
				totalSize.setTotalSize(size_);
				playSlider.to = videoFrame.player.duration ? videoFrame.player.duration : duration_;
				videoOut.fillMode = VideoOutput.PreserveAspectFit;
				videoOut.anchors.fill = frameContainer;
				frameContainer.adjustView();

				//console.log("[onStatusChanged/buffered/inner]: status = " + videoFrame.player.status + ", duration = " + videoFrame.player.duration);
			break;
		}
	}

	function playerHasVideo() {
		frameContainer.enableScene();
	}

	function playerError(error, errorString) {
		console.log("[onErrorStringChanged]: error = " + error + " - " + errorString);
		// in case of error
		downloadCommand.downloaded = false;
		downloadCommand.processing = false;
		downloadCommand.cleanUp();
	}

	function playerPositionChanged(position) {
		if (videoFrame && videoFrame.player) {
			elapsedTime.setTime(videoFrame.player.position);
			playSlider.value = videoFrame.player.position;

			//console.log("[playerPositionChanged]: videoFrame.player.position = " + videoFrame.player.position + ", position = " + position);
			if (//videoFrame.player.position >= 1 &&
					videoFrame.player.position >= 500 && previewImageVideo.visible) frameContainer.enableScene();
		}
	}

	property bool scaled: frameContainer.scale == 1.0
	property bool forceVisible: true

	//
	Rectangle {
		id: controlsBack
		height: actionButton.height + 2 * spaceItems_
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		opacity: 0.8
		radius: 8

		visible: forceVisible && scaled

		function adjustX() {
			x = frameContainer.visible ? (frameContainer.x + spaceItems_) : (previewImageVideo.x + spaceItems_);
			width = frameContainer.visible ? (frameContainer.width - (2 * spaceItems_)) : (previewImageVideo.width - (2 * spaceItems_));
		}

		function adjustY() {
			y = frameContainer.visible ? (frameContainer.y + frameContainer.height - (actionButton.height + 3 * spaceItems_)) :
										(previewImageVideo.y + previewImageVideo.height - (actionButton.height + 3 * spaceItems_));
		}
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

		property bool needDownload: size_ && /*size_ > 1024*200 &&*/ !downloadCommand.downloaded

		opacity: 0.8

		visible: forceVisible && scaled

		onClick: {
			process();
		}

		function process() {
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
		id: caption
		x: actionButton.x + actionButton.width + spaceItems_
		y: actionButton.y + 1
		width: playSlider.width
		elide: Text.ElideRight
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize)) : 11
		text: caption_
		visible: caption_ != "none" && forceVisible && scaled
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")
	}

	QuarkLabel {
		id: elapsedTime
		x: actionButton.x + actionButton.width + spaceItems_ + (caption_ != "none" ? 3 : 0)
		y: actionButton.y + (caption_ != "none" ? caption.height + 3 : spaceItems_)
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize)) : 11
		text: "00:00"
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")

		visible: forceVisible && scaled

		function setTime(ms) {
			text = DateFunctions.msToTimeString(ms);
		}
	}

	QuarkLabel {
		id: totalTime
		x: elapsedTime.x + elapsedTime.width
		y: elapsedTime.y
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize)) : 11
		text: duration_ ? ("/" + DateFunctions.msToTimeString(duration_)) : "/00:00"
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")

		visible: forceVisible && scaled

		function setTotalTime(ms) {
			text = "/" + DateFunctions.msToTimeString(ms);
		}
	}

	QuarkLabel {
		id: totalSize
		x: totalTime.x + totalTime.width
		y: elapsedTime.y
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize)) : 11
		text: ", 0k"

		visible: forceVisible && scaled && size_ !== 0

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
		width: frameContainer.visible ? (frameContainer.width - (actionButton.width + 5 * spaceItems_)) :
										(previewImageVideo.width - (actionButton.width + 4 * spaceItems_))

		visible: forceVisible && scaled

		onMoved: {
			if (videoFrame.player) {
				videoFrame.player.seek(value);
			}

			elapsedTime.setTime(value);
		}

		onToChanged: {
			console.log("[buzzitemmediaview/onToChanged]: to = " + to);
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
