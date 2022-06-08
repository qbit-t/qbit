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
	id: videoFrameFeed

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
	property bool mediaView: false
	property int originalDuration: duration_
	property int totalSize_: size_
	property int calculatedHeight: 500
	property var frameColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background")
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

	//
	property var currentOrientation_: orientation_;

	function terminate() {
		//
		console.log("[itemVideo]: terminate");
		if (videoFrameFeed.player) videoFrameFeed.player.pause();
	}

	onCurrentOrientation_Changed: {
		checkOrientation();
	}

	onVideoOutChanged: {
		checkOrientation();
	}

	function checkOrientation() {
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
		//console.log("[item/onHeightChanged]: height = " + height);
	}

	function forceVisibilityCheck(isFullyVisible) {
		//
		if (player && (playing || downloadCommand.processing)) {
			//
			if (!isFullyVisible) {
				videoFrameFeed.sharedMediaPlayer_.showCurrentPlayer(playerKey_);
			} else {
				videoFrameFeed.sharedMediaPlayer_.hideCurrentPlayer(playerKey_);
			}
		}
	}

	function unbindCommonControls() {
		//
		if (player && (playing || downloadCommand.processing)) {
			//
			// reset playback
			videoFrameFeed.sharedMediaPlayer_.continuePlayback = false;
			videoFrameFeed.sharedMediaPlayer_.playbackController = null;
			videoFrameFeed.sharedMediaPlayer_.popVideoInstance();
			unlink();
			//
			mediaStopped();
			//
			videoOut = null;
			player = null;
		}
	}

	function adjust() {
		frameContainer.adjustView();
		previewImage.adjustView();
	}

	//
	color: "transparent"
	width: parent.width
	height: 1

	//
	Item {
		id: frameContainer

		visible: false

		property var correctedHeight: 0

		function adjustView() {
			//if (previewImage.visible) return;
			if (!buzzitemmedia_) return;
			if (mediaList) width = mediaList.width - 2 * spaceItems_;
			adjustHeight(height);
		}

		function enableScene() {
			if (videoFrameFeed.playing && videoFrameFeed.player && videoFrameFeed.player.hasVideo && videoFrameFeed.player.position > 1) {
				previewImage.visible = false;
				frameContainer.visible = true;

				previewImage.adjustFrameContainer();

				//height = videoOut.contentRect.height;
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
			controller_.openMedia(pkey_, mediaIndex_, buzzitemmedia_.buzzMedia_, sharedMediaPlayer_, !sharedMediaPlayer_.isCurrentInstanceStopped() ? player : null,
								  buzzitemmedia_.buzzId_, buzzitemmedia_.buzzerAlias_, buzzitemmedia_.buzzBody_);
			/*
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
				lMedia.mediaPlayerController = sharedMediaPlayer_;
				lMedia.initialize(pkey_, mediaIndex_, !sharedMediaPlayer_.isCurrentInstanceStopped() ? player : null, description_);
				controller_.addPage(lMedia);
			}
			*/
		}
	}

	BuzzerComponents.ImageQx {
		id: previewImage
		autoTransform: true
		asynchronous: true
		radius: 8

		function adjustView() {
			if (previewImage.status === Image.Ready && mediaList && buzzitemmedia_) {
				width = mediaList.width - 2*spaceItems_;
				if (height > 0) {
					//mediaList.height = height;
					parent.height = height;
					adjustHeight(height);
				}
			}
		}

		function adjustFrameContainer() {
			if (videoOut && !previewImage.visible && videoFrameFeed.playing && videoFrameFeed.player && videoFrameFeed.player.hasVideo && videoFrameFeed.player.position > 1) {
				var lContentCoeff = (videoOut.sourceRect.width  * 1.0) / (videoOut.sourceRect.height * 1.0);
				var lImageCoeff = (width * 1.0) / (height * 1.0);

				//console.log("[adjustFrameContainer]: lContentCoeff = " + lContentCoeff + ", lImageCoeff = " + lImageCoeff);
				if (Math.abs(lContentCoeff - lImageCoeff) > 0.001) {
					// own
					var lCoeff = (frameContainer.width * 1.0) / (videoOut.sourceRect.width * 1.0);
					//var lCoeff = (frameContainer.width * 1.0) * (videoOut.sourceRect.height * 1.0) / (videoOut.sourceRect.width * 1.0);
					var lHeight = videoOut.sourceRect.height * 1.0;

					frameContainer.height = lHeight * lCoeff;
					parent.height = frameContainer.height;
					//mediaList.height = frameContainer.height;

					//console.log("[adjustFrameContainer]: frameContainer.height = " + frameContainer.height + ", parent.height = " + parent.height + ", coeff = " + lCoeff);
				} else {
					frameContainer.height = previewImage.visible ? previewImage.height : parent.height; //
				}
			}
		}

		onHeightChanged: {
			if (buzzitemmedia_) {
				//console.log("[onHeightChanged(2)/image]: height = " + height + ", width = " + width + ", implicitHeight = " + previewImage.implicitHeight);
				parent.height = height;
				adjustHeight(height);
			}
		}

		onStatusChanged: {
			adjustView();

			if (status == Image.Error) {
				// force to reload
				console.log("[buzzitemmedia-video/onStatusChanged]: forcing reload of " + preview_ + ", error = " + errorString);
				// force to reload
				source = "qrc://images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "default.media.cover");
				//"qrc://images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "broken.media.cover");

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
				//console.log("[onHeightChanged/image/new height]: height = " + lHeight + ", lCoeff = " + lCoeff + ", width = " + width + ", originalWidth = " + originalWidth);
			}
		}

		source: preview_ == "none" ? "qrc://images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "default.media.cover") :
									 preview_
		fillMode: BuzzerComponents.ImageQx.PreserveAspectFit
		mipmap: buzzerApp.isDesktop

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
					controller_.openMedia(pkey_, mediaIndex_, buzzitemmedia_.buzzMedia_, sharedMediaPlayer_, !sharedMediaPlayer_.isCurrentInstanceStopped() ? player : null,
										  buzzitemmedia_.buzzId_, buzzitemmedia_.buzzerAlias_, buzzitemmedia_.buzzBody_);

					/*
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
						lMedia.mediaPlayerController = sharedMediaPlayer_;
						lMedia.initialize(pkey_, mediaIndex_, !sharedMediaPlayer_.isCurrentInstanceStopped() ? player : null, description_);
						controller_.addPage(lMedia);
					}
					*/
				}
			}
		}
	}

	property var player: null
	property var videoOut: null
	property bool playing: false

	function unlink() {
		//
		if (videoOut) {
			console.log("[buzzmedia/unlink]: unlinking video instance...");
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
			videoFrameFeed.sharedMediaPlayer_.playbackDownloadStarted();
			downloadCommand.process();
		} else if (actionButton.needDownload && !downloadCommand.downloaded && downloadCommand.processing) {
			// cancel
			actionButton.symbol = Fonts.arrowDownHollowSym;
			mediaLoading.visible = false;
			downloadCommand.processing = false;
			downloadCommand.terminate();
			videoFrameFeed.sharedMediaPlayer_.playbackDownloadCompleted();
		} else if (!videoFrameFeed.playing) {
			videoFrameFeed.play();
		} else {
			if (videoFrameFeed.player)
				videoFrameFeed.player.pause();
		}
	}

	function play() {
		//
		if (videoOut && player && player.paused) {
			// videoFrameFeed.sharedMediaPlayer_.linkInstance(videoOut, buzzitemmedia_);
			player.play();
		} else {
			// controller
			var lVideoOut = videoFrameFeed.sharedMediaPlayer_.createInstance(videoFrameFeed, frameContainer, buzzitemmedia_);
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

				videoOut = lVideoOut;
				player = lVideoOut.player;

				player.source = path_;
				player.play();
			}
		}
	}

	function mediaPlaying() {
		if (!videoFrameFeed) return;
		playing = true;
		frameContainer.enableScene();
		actionButton.adjust();
		//previewImage.visible = false;
	}

	function mediaPaused() {
		if (!videoFrameFeed) return;
		playing = false;
		actionButton.adjust();
	}

	function mediaStopped() {
		console.log("[buzzmedia/mediaStopped]: stopped...");
		if (!videoFrameFeed) return;
		videoFrameFeed.playing = false;
		actionButton.adjust();
		frameContainer.disableScene();
		elapsedTime.setTime(0);
		playSlider.value = 0;
		//player = null;
	}

	function playerStatusChanged(status) {
		if (!videoFrameFeed || !videoFrameFeed.player) return;
		switch(videoFrameFeed.player.status) {
			case MediaPlayer.Buffered:
				totalTime.setTotalTime(videoFrameFeed.player.duration ? videoFrameFeed.player.duration : duration_);
				totalSize.setTotalSize(size_);
				playSlider.to = videoFrameFeed.player.duration ? videoFrameFeed.player.duration : duration_;
				videoOut.fillMode = VideoOutput.PreserveAspectFit;
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
		// in case of error: for now do not do anything
		//downloadCommand.downloaded = false;
		//downloadCommand.processing = false;
		//downloadCommand.cleanUp();
	}

	function playerPositionChanged(position) {
		if (videoFrameFeed && videoFrameFeed.player) {
			elapsedTime.setTime(videoFrameFeed.player.position);
			playSlider.value = videoFrameFeed.player.position;

			//console.log("[playerPositionChanged]: videoFrame.player.position = " + videoFrame.player.position + ", position = " + position);
			if (videoFrameFeed.player.position >= 500 && previewImage.visible) frameContainer.enableScene();
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
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden.uni")
		opacity: 0.9
		radius: 8
	}

	//
	QuarkRoundSymbolButton {
		id: actionButton
		x: controlsBack.x + spaceItems_
		y: controlsBack.y + spaceItems_
		/*
		spaceLeft: buzzerClient.isDesktop ? (symbol === Fonts.playSym ? (buzzerClient.scaleFactor * 2) : 0) :
					(symbol === Fonts.arrowDownHollowSym || symbol === Fonts.pauseSym) ?
						2 : (symbol === Fonts.playSym || symbol === Fonts.cancelSym ? 3 : 0)
		spaceTop: buzzerApp.isDesktop ? 0 : 2
		*/
		symbol: needDownload && !downloadCommand.downloaded ? Fonts.arrowDownHollowSym :
									(videoFrameFeed.playing ? Fonts.pauseSym : Fonts.playSym)
		fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 7)) : 18
		radius: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultRadius)) : defaultRadius
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.highlight")

		onSymbolChanged: {
			if (symbol !== Fonts.playSym) spaceLeft = 0;
			else spaceLeft = buzzerClient.scaleFactor * 2;
		}

		property bool needDownload: size_ && /*size_ > 1024*200 &&*/ !downloadCommand.downloaded

		opacity: 0.8

		onClick: {
			//
			tryPlay();
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
										(videoFrameFeed.playing ? Fonts.pauseSym : Fonts.playSym);
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
		visible: caption_ != "none" && caption_ != ""
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")

		//onTextChanged: console.log("[onTextChanged]: caption_ = " + caption_);
	}

	QuarkLabel {
		id: elapsedTime
		x: actionButton.x + actionButton.width + spaceItems_ + (caption.visible ? 3 : 0)
		y: actionButton.y + (caption.visible ? caption.height + (buzzerApp.isDesktop ? 0 : 3) : spaceItems_)
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - (caption.visible ? 3 : 0))) : 11
		text: "00:00"
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")

		function setTime(ms) {
			text = DateFunctions.msToTimeString(ms);
		}
	}

	QuarkLabel {
		id: totalTime
		x: elapsedTime.x + elapsedTime.width
		y: elapsedTime.y
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - (caption.visible ? 3 : 0))) : 11
		text: duration_ ? ("/" + DateFunctions.msToTimeString(duration_)) : "/00:00"
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")

		function setTotalTime(ms) {
			text = "/" + DateFunctions.msToTimeString(ms);
		}
	}

	QuarkLabel {
		id: totalSize
		x: totalTime.x + totalTime.width
		y: elapsedTime.y
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - (caption.visible ? 3 : 0))) : 11
		text: ", 0k"
		visible: size_ !== 0
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")

		function setTotalSize(mediaSize) {
			//
			if (mediaSize < 1000) text = ", " + mediaSize + "b";
			else text = ", " + NumberFunctions.numberToCompact(mediaSize);
		}
	}

	Slider {
		id: playSlider
		x: actionButton.x + actionButton.width // + spaceItems_
		y: actionButton.y + actionButton.height - (height - 3 * spaceItems_) + (buzzerApp.isDesktop && caption.visible ? 3 : 0)
		from: 0
		to: 1
		orientation: Qt.Horizontal
		stepSize: 0.1
		//width: frameContainer.width - (actionButton.width + 3 * spaceItems_)
		width: frameContainer.visible ? (frameContainer.width - (actionButton.width + 5 * spaceItems_)) :
										(previewImage.width - (actionButton.width + 4 * spaceItems_))

		onMoved: {
			if (videoFrameFeed.player) {
				videoFrameFeed.player.seek(value);
			}

			elapsedTime.setTime(value);
		}
	}

	QuarkRoundProgress {
		id: mediaLoading
		x: actionButton.x - buzzerClient.scaleFactor * 2
		y: actionButton.y - buzzerClient.scaleFactor * 2
		size: actionButton.width + buzzerClient.scaleFactor * 4
		colorCircle: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link");
		colorBackground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link");
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
		property bool processing: false;
		property bool downloaded: false;

		onProgress: {
			//
			processing = true;
			mediaLoading.progress(pos, size);
			videoFrameFeed.sharedMediaPlayer_.playbackDownloading(pos, size);
		}

		onProcessed: {
			// tx, previewFile, originalFile, orientation, duration, size, type
			var lPSize = buzzerApp.getFileSize(previewFile);
			var lOSize = buzzerApp.getFileSize(originalFile);
			console.log("[buzzmedia-video/downloadCommand]: " + "index = " + index + ", " + tx + ", " + previewFile + " - [" + lPSize + "], " + originalFile + " - [" + lOSize + "], " + orientation + ", " + duration + ", " + size + ", " + type);

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
			// signal
			videoFrameFeed.sharedMediaPlayer_.playbackDownloadCompleted();

			// adjust button
			actionButton.adjust();

			// autoplay
			if (!videoFrameFeed.playing) {
				videoFrameFeed.play();
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
