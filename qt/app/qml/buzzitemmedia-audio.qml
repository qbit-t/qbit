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
import "qrc:/lib/dateFunctions.js" as DateFunctions

Rectangle {
	//
	id: audioFrame

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
	property bool mediaView: false
	property int originalDuration: duration_
	property int totalSize_: size_
	property var frameColor: buzzerApp.getColor(mediaView ? mediaViewTheme : buzzerClient.theme, mediaView ? mediaViewSelector : buzzerClient.themeSelector, "Page.background")
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
	x: mediaView ? getX() : 0
	y: mediaView ? getY() : 0

	width: mediaList ? mediaList.width - (mediaView ? spaceItems_ * 2 : 0) : parent.width - (mediaView ? spaceItems_ * 2 : 0)
	height: actionButton.height + spaceTop_ + spaceBottom_
	border.color: buzzerApp.getColor(mediaView ? mediaViewTheme : buzzerClient.theme, mediaView ? mediaViewSelector : buzzerClient.themeSelector, "Material.disabledHidden")
	color: fillColor
	radius: 8

	onHeightChanged: adjustHeight(height)

	onMediaViewChanged: {
		adjust();
	}

	onTotalSize_Changed: {
		totalSize.setTotalSize(size_);
	}

	function forceVisibilityCheck(isFullyVisible) {
		//
		if (player && (playing || downloadCommand.processing)) {
			//
			if (!isFullyVisible) {
				audioFrame.sharedMediaPlayer_.showCurrentPlayer(playerKey_);
			} else {
				audioFrame.sharedMediaPlayer_.hideCurrentPlayer(playerKey_);
			}
		}
	}

	function unbindCommonControls() {
		//
		if (player && (playing || downloadCommand.processing)) {
			//
			// reset playback
			audioFrame.sharedMediaPlayer_.continuePlayback = false;
			audioFrame.sharedMediaPlayer_.playbackController = null;
			audioFrame.sharedMediaPlayer_.popVideoInstance();
			unlink();
		}
	}

	function checkPlaying() {
		//
		if (player) {
			// reset continue playback
			audioFrame.sharedMediaPlayer_.continuePlayback = false;
			//
			if (audioFrame.sharedMediaPlayer_.isCurrentInstancePaused()) player.stop();
			else if (playing || downloadCommand.processing) audioFrame.sharedMediaPlayer_.showCurrentPlayer(null);
		}
	}

	function syncPlayer(mediaPlayer) {
		//
		if (mediaPlayer) play(mediaPlayer);
	}

	function terminate() {
		//
		player.pause();
	}

	function adjust() {
		width = mediaList ? mediaList.width - (mediaView ? spaceItems_ * 2 : 0) : parent.width - (mediaView ? spaceItems_ * 2 : 0);
		x = mediaView ? getX() : 0;
		y = mediaView ? getY() : 0;

		console.log("[buzzitemmedia-audio/adjust]: mediaView = " + mediaView + ", y = " + y + ", mediaList.height = " + mediaList.height +
					", height = " + height);
	}

	function getX() {
		return (mediaList ? mediaList.width / 2 : parent.width / 2) - width / 2;
	}

	function getY() {
		return (mediaList ? mediaList.height / 2 : parent.height / 2) - height / 2;
	}

	function reset() {
	}

	//
	MouseArea {
		id: linkClick
		x: 0
		y: 0
		width: parent.width
		height: parent.height
		enabled: !mediaView
		cursorShape: Qt.PointingHandCursor

		ItemDelegate {
			id: linkClicked
			x: 0
			y: 0
			width: parent.width
			height: parent.height
			enabled: !mediaView

			onClicked: {
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
	}

	layer.enabled: true
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
			return parent.width;
		}

		function getHeight() {
			return parent.height;
		}
	}

	Item {
		id: audioOut

		property var player;
	}

	property var player: null
	property var videoOut: null
	property bool playing: false

	function unlink() {
		player.onPlaying.disconnect(mediaPlaying);
		player.onPaused.disconnect(mediaPaused);
		player.onStopped.disconnect(mediaStopped);
		player.onStatusChanged.disconnect(playerStatusChanged);
		player.onPositionChanged.disconnect(playerPositionChanged);
		player.onError.disconnect(playerError);
	}

	function tryPlay() {
		if (actionButton.needDownload && !downloadCommand.downloaded && !downloadCommand.processing) {
			// start
			actionButton.symbol = Fonts.cancelSym;
			mediaLoading.start();
			audioFrame.sharedMediaPlayer_.playbackDownloadStarted();
			downloadCommand.process();
		} else if (actionButton.needDownload && !downloadCommand.downloaded && downloadCommand.processing) {
			// cancel
			actionButton.symbol = Fonts.arrowDownHollowSym;
			mediaLoading.visible = false;
			downloadCommand.processing = false;
			downloadCommand.terminate();
			audioFrame.sharedMediaPlayer_.playbackDownloadCompleted();
		} else if (!audioFrame.playing) {
			actionButton.symbol = Fonts.cancelSym;
			audioFrame.play();
		} else {
			if (audioFrame.player)
				audioFrame.player.pause();
		}
	}

	function play(existingPlayer) {
		//
		if (existingPlayer) {
			var lSingleAudioOut = audioFrame.sharedMediaPlayer_.createAudioInstance(audioFrame, audioOut, buzzitemmedia_, true);
			lSingleAudioOut.onPlaying.connect(mediaPlaying);
			lSingleAudioOut.onPaused.connect(mediaPaused);
			lSingleAudioOut.onStopped.connect(mediaStopped);
			lSingleAudioOut.onStatusChanged.connect(playerStatusChanged);
			lSingleAudioOut.onPositionChanged.connect(playerPositionChanged);
			lSingleAudioOut.onError.connect(playerError);

			player = lSingleAudioOut;

			if (!player.playing) {
				downloadCommand.downloaded = true;
				player.play();
			} else {
				downloadCommand.downloaded = true;
				mediaPlaying();
			}
		} else {
			if (player && player.paused) {
				// audioFrame.sharedMediaPlayer_.linkInstance(audioOut, buzzitemmedia_);
				player.play();
			} else {
				// controller
				var lAudioOut = audioFrame.sharedMediaPlayer_.createAudioInstance(audioFrame, audioOut, buzzitemmedia_);
				//
				if (lAudioOut) {
					lAudioOut.player.description = description_;
					lAudioOut.player.caption = caption_;
					lAudioOut.player.onPlaying.connect(mediaPlaying);
					lAudioOut.player.onPaused.connect(mediaPaused);
					lAudioOut.player.onStopped.connect(mediaStopped);
					lAudioOut.player.onStatusChanged.connect(playerStatusChanged);
					lAudioOut.player.onPositionChanged.connect(playerPositionChanged);
					lAudioOut.player.onError.connect(playerError);

					player = lAudioOut.player;

					player.source = path_;
					player.play();
				}
			}
		}
	}

	function mediaPlaying() {
		if (!audioFrame) return;
		playing = true;
		actionButton.adjust();
	}

	function mediaPaused() {
		if (!audioFrame) return;
		playing = false;
		actionButton.adjust();
	}

	function mediaStopped() {
		if (!audioFrame) return;
		audioFrame.playing = false;
		actionButton.adjust();
		elapsedTime.setTime(0);
		playSlider.value = 0;
	}

	function playerStatusChanged(status) {
		if (!audioFrame || !audioFrame.player) return;
		switch(audioFrame.player.status) {
			case MediaPlayer.Buffered:
				totalTime.setTotalTime(audioFrame.player.duration ? audioFrame.player.duration : duration_);
				totalSize.setTotalSize(size_);
				playSlider.to = audioFrame.player.duration ? audioFrame.player.duration : duration_;
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
		if (audioFrame && audioFrame.player) {
			elapsedTime.setTime(audioFrame.player.position);
			playSlider.value = audioFrame.player.position;
		}
	}

	//
	QuarkRoundSymbolButton {
		id: actionButton
		x: spaceLeft_
		y: spaceTop_
		/*
		spaceLeft: symbol === Fonts.arrowDownHollowSym || symbol === Fonts.pauseSym ?
						(buzzerClient.scaleFactor * 2) :
							(symbol === Fonts.playSym || symbol === Fonts.cancelSym ?
								(buzzerClient.scaleFactor * 3) : 0)
		spaceTop: buzzerClient.scaleFactor * 2
		*/
		symbol: needDownload && !downloadCommand.downloaded ? Fonts.arrowDownHollowSym :
									(player.playing ? Fonts.pauseSym : Fonts.playSym)
		fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 7)) : 18
		radius: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultRadius)) : defaultRadius
		color: buzzerApp.getColor(mediaView ? mediaViewTheme : buzzerClient.theme, mediaView ? mediaViewSelector : buzzerClient.themeSelector, "Material.menu.highlight")
		Material.background: buzzerApp.getColor(mediaView ? mediaViewTheme : buzzerClient.theme, mediaView ? mediaViewSelector : buzzerClient.themeSelector, "Material.menu.background");

		property bool needDownload: size_ /*&& size_ > 1024*200*/ && !downloadCommand.downloaded

		onSymbolChanged: {
			if (symbol !== Fonts.playSym) spaceLeft = 0;
			else spaceLeft = buzzerClient.scaleFactor * 2;
		}

		onClick: {
			//
			tryPlay();
		}

		function notLoaded() {
			symbol = Fonts.cancelSym;
			textColor = buzzerApp.getColor(mediaView ? mediaViewTheme : buzzerClient.theme, mediaView ? mediaViewSelector : buzzerClient.themeSelector, "Buzzer.event.like");
		}

		function loaded() {
			symbol = Fonts.playSym;
			textColor = buzzerApp.getColor(mediaView ? mediaViewTheme : buzzerClient.theme, mediaView ? mediaViewSelector : buzzerClient.themeSelector, "Material.menu.foreground")
		}

		function adjust() {
			symbol = needDownload && !downloadCommand.downloaded ? Fonts.arrowDownHollowSym :
										(audioFrame.playing ? Fonts.pauseSym : Fonts.playSym);
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
		//color: buzzerApp.getColor(mediaView ? mediaViewTheme : buzzerClient.theme, mediaView ? mediaViewSelector : buzzerClient.themeSelector, "Material.menu.foreground")
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
	}

	QuarkLabel {
		id: elapsedTime
		x: actionButton.x + actionButton.width + spaceItems_ + (caption.visible ? 3 : 0)
		y: actionButton.y + (caption.visible ? caption.height + 3 : spaceItems_)
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize)) : 11
		text: "00:00"
		//color: buzzerApp.getColor(mediaView ? mediaViewTheme : buzzerClient.theme, mediaView ? mediaViewSelector : buzzerClient.themeSelector, "Material.menu.foreground")
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")

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
		//color: buzzerApp.getColor(mediaView ? mediaViewTheme : buzzerClient.theme, mediaView ? mediaViewSelector : buzzerClient.themeSelector, "Material.menu.foreground")
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")

		function setTotalTime(ms) {
			text = "/" + DateFunctions.msToTimeString(ms);
		}
	}

	QuarkLabel {
		id: totalSize
		x: totalTime.x + totalTime.width
		y: elapsedTime.y
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize)) : 11
		//color: buzzerApp.getColor(mediaView ? mediaViewTheme : buzzerClient.theme, mediaView ? mediaViewSelector : buzzerClient.themeSelector, "Material.menu.foreground")
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
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
		to: duration_ ? duration_ : 1
		orientation: Qt.Horizontal
		stepSize: 0.1
		width: parent.width - (actionButton.x + actionButton.width + 2 * spaceItems_)

		onMoved: {
			if (player) {
				player.seek(value);
			}

			elapsedTime.setTime(value);
		}
	}

	QuarkRoundProgress {
		id: mediaLoading
		x: actionButton.x - buzzerClient.scaleFactor * 2
		y: actionButton.y - buzzerClient.scaleFactor * 2
		size: actionButton.width + buzzerClient.scaleFactor * 4
		colorCircle: buzzerApp.getColor(mediaView ? mediaViewTheme : buzzerClient.theme, mediaView ? mediaViewSelector : buzzerClient.themeSelector, "Material.link");
		colorBackground: buzzerApp.getColor(mediaView ? mediaViewTheme : buzzerClient.theme, mediaView ? mediaViewSelector : buzzerClient.themeSelector, "Material.link");
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
			audioFrame.sharedMediaPlayer_.playbackDownloading(pos, size);
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
			// signal
			audioFrame.sharedMediaPlayer_.playbackDownloadCompleted();

			// adjust button
			actionButton.adjust();

			// autoplay
			if (!audioFrame.playing) audioFrame.play();
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
