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
	property string mediaViewTheme: "Darkmatter"
	property string mediaViewSelector: "dark"

	//
	readonly property bool playable: true
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
		totalSizeControl.setTotalSize(size_);
	}

	onOriginalDurationChanged: {
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

	function unbindCommonControls() {
		//
		if (player && (playing || downloadCommand.processing)) {
			//
			// reset playback
			videoFrame.sharedMediaPlayer_.continuePlayback = false;
			videoFrame.sharedMediaPlayer_.playbackController = null;
			videoFrame.sharedMediaPlayer_.popVideoInstance();
			unlink();
			//
			mediaStopped();
			//
			videoOut = null;
			player = null;
		}
	}

	function checkPlaying() {
		//
		if (player) {
			// reset continue playback
			videoFrame.sharedMediaPlayer_.continuePlayback = false;
			//
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

		anchors.fill: parent

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
			//
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

	Timer {
		id: hideControlsTimer
		interval: 3000
		repeat: false
		running: false

		onTriggered: {
			menuControl.enforceVisible = false;
			if (menuControl.enforceVisible) videoFrame.sharedMediaPlayer_.showCurrentPlayer();
			else videoFrame.sharedMediaPlayer_.hideCurrentPlayer();
		}
	}


	MouseArea {
		id: dragArea
		hoverEnabled: true
		x: 0
		y: 0
		width: parent.width
		height: parent.height - 50
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
			//
			if (videoFrame.sharedMediaPlayer_.isCurrentInstance(path_)) {
				hideControlsTimer.stop();
				//
				menuControl.enforceVisible = !menuControl.enforceVisible; // || frameContainer.scale > 1.0;
				if (menuControl.enforceVisible) videoFrame.sharedMediaPlayer_.showCurrentPlayer();
				else videoFrame.sharedMediaPlayer_.hideCurrentPlayer();
			}

			//
			if (frameContainer.scale != 1.0) {
				frameContainer.scale = 1.0;
			}

			frameContainer.x = frameContainer.getX();
			frameContainer.y = frameContainer.getY();
		}
	}

	BuzzerComponents.ImageQx {
		id: previewImageVideo
		autoTransform: true
		asynchronous: true

		x: getX()
		y: getY()

		function getX() {
			return (mediaList ? mediaList.width / 2 : parent.width / 2) - width / 2;
		}

		function getY() {
			//return (mediaList ? mediaList.height / 2 : parent.height / 2) - height / 2;
			return calculatedHeight / 2 - height / 2;
		}

		function adjustDimensions() {
			//
			var lRatioH = (originalHeight * 1.0) / (originalWidth * 1.0);
			var lRatioW = (originalWidth * 1.0) / (originalHeight * 1.0);
			//
			if (calculatedHeight > calculatedWidth && lRatioH < 2 || lRatioW > 2 /*&& originalHeight < calculatedHeight * 2*/) {
				var lCoeffW = (calculatedWidth * 1.0) / (originalWidth * 1.0)
				var lWidth = originalWidth * 1.0;
				width = lWidth * lCoeffW;

				//var lCoeffH = (calculatedHeight * 1.0) / (originalHeight * 1.0)
				var lHeightW = originalHeight * 1.0;
				height = lHeightW * lCoeffW;
			} else {
				var lCoeffH = (calculatedHeight * 1.0) / (originalHeight * 1.0)
				var lHeight = originalHeight * 1.0;
				height = lHeight * lCoeffH;

				var lWidthH = originalWidth * 1.0;
				width = lWidthH * lCoeffH;
			}
		}

		function adjustView() {
			if (previewImageVideo.status === Image.Ready && mediaList && buzzitemmedia_) {
				//
				adjustDimensions();
				//
				x = getX();
				controlsBack.adjustX();

				y = getY();
				controlsBack.adjustY();

				adjustFrameContainer();
			}

			/*
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
			*/
		}

		function adjustFrameContainer() {
			if (videoOut) {
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
				// force to reload
				console.log("[buzzitemmediaview-video/onStatusChanged]: forcing reload of " + preview_ + ", error = " + errorString);
				// force to reload
				source = "qrc://images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "broken.media.cover");
				//downloadCommand
				errorLoading();
			}
		}

		onWidthChanged: {
			//
			y = getY();
			frameContainer.y = y;
			frameContainer.height = height;
			controlsBack.adjustY();

			adjustFrameContainer();

			adjustHeight(height);
			/*
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
			*/
		}

		onHeightChanged: {
			x = getX();
			frameContainer.x = x;
			frameContainer.width = width;
			controlsBack.adjustX();

			adjustFrameContainer();

			adjustHeight(height);
			/*
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
			*/
		}
	}

	QuarkToolButton {
		id: menuControl
		x: previewImageVideo.visible ? previewImageVideo.x + previewImageVideo.width - width - spaceItems_:
									   frameContainer.x + frameContainer.width - width - spaceItems_
		y: previewImageVideo.visible ? previewImageVideo.y + spaceItems_ :
									   frameContainer.y + spaceItems_
		symbol: buzzerApp.isDesktop ? Fonts.shevronDownSym : Fonts.elipsisVerticalSym
		visible: !pinchHandler.active && enforceVisible
		labelYOffset: /*buzzerApp.isDesktop ? 0 :*/ 3
		Layout.alignment: Qt.AlignHCenter
		opacity: 0.6
		symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 14) : symbolFontPointSize

		symbolColor: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.menu.foreground")
		Material.background: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.menu.background")

		property bool enforceVisible: true

		onClicked: {
			if (headerMenu.visible) headerMenu.close();
			else { headerMenu.prepare(); headerMenu.open(); }
		}
	}

	QuarkPopupMenu {
		id: headerMenu
		x: (menuControl.x + menuControl.width) - width // - spaceRight_
		y: menuControl.y + menuControl.height // + spaceItems_
		width: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 200) : 200
		visible: false

		property int request_NO_RESPONSE: -1

		Material.background: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Page.background")
		menuHighlightColor: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.menu.highlight")
		menuBackgroundColor: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.menu.background")
		menuForegroundColor: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.menu.foreground")

		model: ListModel { id: menuModel }

		onAboutToShow:prepare()

		onClick: {
			//
			if (key === "reload") {
				//
				if (!downloadCommand.processing) {
					downloadCommand.downloaded = false;
					downloadCommand.cleanUp();
					downloadCommand.process();
				}

			} else if (key === "share") {
				//
				var lMimeType = "*/*";
				if (mimeType_.startsWith("audio")) lMimeType = "audio/*";
				else if (mimeType_.startsWith("video")) lMimeType = "video/*";

				shareUtils.sendFile(originalPath_, "Send file", lMimeType, request_NO_RESPONSE, false);
			} else if (key === "copyToDownload") {
				//
				buzzerApp.checkPermission();
				buzzerApp.copyFile(originalPath_, buzzerApp.downloadsPath() + "/" + buzzerApp.getFileName(originalPath_));
			}
		}

		function prepare() {
			//
			menuModel.clear();

			menuModel.append({
				key: "reload",
				keySymbol: Fonts.arrowDownHollowSym,
				name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.gallery.media.reload")});

			if (buzzerApp.getFileSize(originalPath_) > 0) {
				menuModel.append({
					key: "share",
					keySymbol: Fonts.shareSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.gallery.media.share")});

				menuModel.append({
					key: "copyToDownload",
					keySymbol: Fonts.downloadSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.gallery.media.copyToDownload")});
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

	function tryPlay() {
		if (actionButton.needDownload && !downloadCommand.downloaded && !downloadCommand.processing) {
			// start
			actionButton.symbol = Fonts.cancelSym;
			mediaLoading.start();
			videoFrame.sharedMediaPlayer_.playbackDownloadStarted();
			downloadCommand.process();
		} else if (actionButton.needDownload && !downloadCommand.downloaded && downloadCommand.processing) {
			// cancel
			actionButton.symbol = Fonts.arrowDownHollowSym;
			mediaLoading.visible = false;
			downloadCommand.terminate();
			videoFrame.sharedMediaPlayer_.playbackDownloadCompleted();
		} else if (!videoFrame.playing) {
			hideControlsTimer.start();
			videoFrame.play();
		} else {
			if (videoFrame.player)
				videoFrame.player.pause();
		}
	}

	function play(existingPlayer) {
		//
		if (existingPlayer) {
			// controller
			var lSingleVideoOut = videoFrame.sharedMediaPlayer_.createInstance(videoFrame, frameContainer, buzzitemmedia_, true);
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
			if (videoOut && player && player.paused) {
				//videoFrame.sharedMediaPlayer_.linkInstance(videoOut, buzzitemmedia_);
				player.play();
			} else {
				// controller
				var lVideoOut = videoFrame.sharedMediaPlayer_.createInstance(videoFrame, frameContainer, buzzitemmedia_);
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
			}
		}
	}

	function mediaPlaying() {
		//if (!videoFrame) return;
		console.log("[buzzmediaview/mediaPlaying]: playing...");
		playing = true;
		forceVisible = false;
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
		forceVisible = true;
		actionButton.adjust();
		frameContainer.disableScene();
		//elapsedTime.setTime(0);
		//player = null;
		//buzzerApp.wakeRelease();
	}

	function playerStatusChanged(status) {
		if (!videoFrame || !videoFrame.player) return;
		switch(videoFrame.player.status) {
			case MediaPlayer.Buffered:
				totalTimeControl.setTotalTime(videoFrame.player.duration ? videoFrame.player.duration : duration_);
				totalSizeControl.setTotalSize(size_);
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
		downloadCommand.cleanUp();
	}

	function playerPositionChanged(position) {
		if (videoFrame && videoFrame.player) {
			if (videoFrame.player.position >= 500 && previewImageVideo.visible) frameContainer.enableScene();
		}
	}

	property bool scaled: frameContainer.scale == 1.0
	property bool forceVisible: true

	//
	Rectangle {
		id: controlsBack
		width: Math.max(captionTextMetrics.width + spaceItems_, totalTimeControl.width + totalSizeControl.width) + 2 * spaceItems_
		height: captionControl.visible ? captionControl.height + spaceItems_ + totalTimeControl.height + 2 * spaceItems_ :
										 totalTimeControl.height + 2 * spaceItems_
		color: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.disabledHidden.uni")
		radius: 4

		visible: forceVisible && scaled

		TextMetrics	{
			id: captionTextMetrics
			elide: Text.ElideRight
			text: caption_
			elideWidth: (previewImageVideo.width ? previewImageVideo.width : parent.width) - 4 * spaceItems_
			font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : 11
		}

		QuarkLabel {
			id: captionControl
			x: spaceItems_
			y: spaceItems_
			width: (previewImageVideo.width ? previewImageVideo.width : parent.width) - 4 * spaceItems_
			elide: Text.ElideRight
			font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize)) : 11
			text: captionTextMetrics.elidedText +
				  (captionTextMetrics.elidedText !== caption_ && buzzerApp.isDesktop ? "..." : "")
			visible: caption_ != "none" && forceVisible && scaled && caption_ != ""
			color: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.menu.foreground")
		}

		QuarkLabel {
			id: totalTimeControl
			x: spaceItems_
			y: captionControl.visible ? captionControl.y + captionControl.height + spaceItems_ :
								 spaceItems_
			font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize)) : 11
			text: duration_ ? (DateFunctions.msToTimeString(duration_)) : "00:00"
			color: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.menu.foreground")

			visible: forceVisible && scaled

			function setTotalTime(ms) {
				text = DateFunctions.msToTimeString(ms);
			}
		}

		QuarkLabel {
			id: totalSizeControl
			x: totalTimeControl.x + totalTimeControl.width
			y: totalTimeControl.y
			font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize)) : 11
			color: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.menu.foreground")
			text: ", 0k"

			visible: forceVisible && scaled && size_ !== 0

			function setTotalSize(mediaSize) {
				//
				if (mediaSize < 1000) text = ", " + mediaSize + "b";
				else text = ", " + NumberFunctions.numberToCompact(mediaSize);
			}
		}

		function adjustX() {
			//x = frameContainer.visible ? (frameContainer.x + spaceItems_) : (previewImageVideo.x + spaceItems_);
			//width = frameContainer.visible ? (frameContainer.width - (2 * spaceItems_)) : (previewImageVideo.width - (2 * spaceItems_));

			x = previewImageVideo.x + spaceItems_;
			//width = frameContainer.visible ? (frameContainer.width - (2 * spaceItems_)) : (previewImageVideo.width - (2 * spaceItems_));
		}

		function adjustY() {
			//y = frameContainer.visible ? (frameContainer.y + frameContainer.height - (actionButton.height + 3 * spaceItems_)) :
			//							(previewImageVideo.y + previewImageVideo.height - (actionButton.height + 3 * spaceItems_));

			//y = frameContainer.visible ? (frameContainer.y + spaceItems_) :
			//							(previewImageVideo.y + spaceItems_);

			y = previewImageVideo.y + spaceItems_;
		}
	}

	//
	QuarkRoundSymbolButton {
		id: actionButton
		//x: controlsBack.x + spaceItems_
		//y: controlsBack.y + spaceItems_

		anchors.centerIn: parent
		spaceLeft: symbol === Fonts.arrowDownHollowSym || symbol === Fonts.pauseSym || symbol === Fonts.cancelSym ? 0 :
					   (symbol === Fonts.playSym ? 3 : 0)
		spaceTop: 2

		symbol: needDownload && !downloadCommand.downloaded ? Fonts.arrowDownHollowSym :
									(videoFrame.playing ? Fonts.pauseSym : Fonts.playSym)
		fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 17)) : 27
		radius: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultRadius * 1.5)) : defaultRadius * 1.5
		color: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.menu.highlight")
		Material.background: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.menu.background");

		property bool needDownload: size_ && /*size_ > 1024*200 &&*/ !downloadCommand.downloaded

		opacity: 0.8

		visible: forceVisible && scaled

		onClick: {
			process();
		}

		function process() {
			//
			tryPlay();
		}

		function notLoaded() {
			symbol = Fonts.cancelSym;
			textColor = buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Buzzer.event.like");
		}

		function loaded() {
			symbol = Fonts.playSym;
			textColor = buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.menu.foreground")
		}

		function adjust() {
			symbol = needDownload && !downloadCommand.downloaded ? Fonts.arrowDownHollowSym :
										(videoFrame.playing ? Fonts.pauseSym : Fonts.playSym);
		}
	}

	QuarkRoundProgress {
		id: mediaLoading
		x: actionButton.x - 2
		y: actionButton.y - 2
		size: buzzerClient.scaleFactor * (actionButton.width + 4)
		colorCircle: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.link");
		colorBackground: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.link");
		arcBegin: 0
		arcEnd: 0
		lineWidth: buzzerClient.scaleFactor * 2
		visible: false
		animationDuration: 50

		function start() {
			//
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
		//property bool processing: false;
		property bool downloaded: false;

		onProgress: {
			//
			mediaLoading.progress(pos, size);
			videoFrame.sharedMediaPlayer_.playbackDownloading(pos, size);
		}

		onProcessed: {
			// tx, previewFile, originalFile, orientation, duration, size, type
			console.log(tx + ", " + previewFile + ", " + originalFile + ", " + orientation + ", " + duration + ", " + size + ", " + type);

			//
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
			// signal
			videoFrame.sharedMediaPlayer_.playbackDownloadCompleted();

			// adjust button
			actionButton.adjust();

			// autoplay
			if (!videoFrame.playing) {
				hideControlsTimer.start();
				videoFrame.play();
			}
		}

		onError: {
			if (code == "E_MEDIA_HEADER_NOT_FOUND") {
				//
				downloaded = false;
				//
				tryCount_++;
				mediaLoading.visible = false;
				mediaLoading.notLoaded();
			}
		}
	}
}
