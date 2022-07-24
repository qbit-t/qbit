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
	id: mediaPlayer

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
	readonly property real defaultFontSize: buzzerApp.defaultFontSize()

	property var mediaPlayerController
	property var player
	property bool closed: true
	property bool showOnChanges: false
	property var overlayParent
	property bool gallery: false
	property string galleryTheme: "Darkmatter"
	property string gallerySelector: "dark"
	property bool instanceLinked: false	
	property var overlayRect;
	property bool inverse: false
	property var key: null

	signal instanceBounded();

	Component.onCompleted: {
		controllerAssigned();
	}

	onMediaPlayerControllerChanged: {
		controllerAssigned();
	}

	onPlayerChanged: {
		//console.log("[onPlayerChanged]: player === null");
		if (player === null /*&& !mediaPlayer.closed*/) {
			console.log("[onPlayerChanged]: player === null");
			mediaStopped();
		}
	}

	function controllerAssigned() {
		//
		if (!mediaPlayerController || instanceLinked) return;
		console.log("[onMediaPlayerControllerChanged]: link to instance, self = " + mediaPlayer);
		instanceLinked = true;
		mediaPlayerController.newInstanceCreated.connect(playerNewInstanceCreated);
		mediaPlayerController.showCurrentPlayer.connect(playerShow);
		mediaPlayerController.hideCurrentPlayer.connect(playerHide);
		mediaPlayerController.playbackDownloadStarted.connect(playerDownloadStarted);
		mediaPlayerController.playbackDownloading.connect(playerDownloading);
		mediaPlayerController.playbackDownloadCompleted.connect(playerDownloadCompleted);
		mediaPlayerController.playbackDurationChanged.connect(playerDurationChanged);
		mediaPlayerController.toggleCurrentPlayer.connect(playerToggle);

		// try to connect
		link();
	}

	function unlink() {
		if (mediaPlayerController && instanceLinked) {
			console.log("[buzzitemmedia-player/unlink]: unlinking player = " + player);
			instanceLinked = false;
			mediaPlayerController.newInstanceCreated.disconnect(playerNewInstanceCreated);
			mediaPlayerController.showCurrentPlayer.disconnect(playerShow);
			mediaPlayerController.hideCurrentPlayer.disconnect(playerHide);
			mediaPlayerController.playbackDownloadStarted.disconnect(playerDownloadStarted);
			mediaPlayerController.playbackDownloading.disconnect(playerDownloading);
			mediaPlayerController.playbackDownloadCompleted.disconnect(playerDownloadCompleted);
			mediaPlayerController.playbackDurationChanged.disconnect(playerDurationChanged);
			mediaPlayerController.toggleCurrentPlayer.disconnect(playerToggle);

			if (player) {
				player.onPlaying.disconnect(mediaPlaying);
				player.onPaused.disconnect(mediaPaused);
				player.onStopped.disconnect(mediaStopped);
				player.onStatusChanged.disconnect(playerStatusChanged);
				player.onPositionChanged.disconnect(playerPositionChanged);
				player.onError.disconnect(playerError);
				player = null;
			}
		}
	}

	function link() {
		if (mediaPlayerController && mediaPlayerController.lastInstancePlayer) {
			console.log("[buzzitemmedia-player/link]: linking to player = " + mediaPlayerController.lastInstancePlayer);
			// just connect
			playerNewInstanceCreated(mediaPlayerController.lastInstance, null);
			// attach properties
				if (player) {
					totalTime.setTotalTime(player.duration);
					totalSize.setTotalSize(player.size);
					playSlider.to = player.duration;
			}
		}
	}

	function playerNewInstanceCreated(instance, prev) {
		//
		if (instance === prev) return;
		//
		console.log("[playerNewInstanceCreated]: instance = " + instance + ", player = " + instance.player + ", prev = " + prev);

		if (prev && prev.player) {
			//prev.player.stop();
			prev.player.onPlaying.disconnect(mediaPlaying);
			prev.player.onPaused.disconnect(mediaPaused);
			prev.player.onStopped.disconnect(mediaStopped);
			prev.player.onStatusChanged.disconnect(playerStatusChanged);
			prev.player.onPositionChanged.disconnect(playerPositionChanged);
			prev.player.onError.disconnect(playerError);
		} else if (player) {
			//player.stop();
			player.onPlaying.disconnect(mediaPlaying);
			player.onPaused.disconnect(mediaPaused);
			player.onStopped.disconnect(mediaStopped);
			player.onStatusChanged.disconnect(playerStatusChanged);
			player.onPositionChanged.disconnect(playerPositionChanged);
			player.onError.disconnect(playerError);
			//player = null;
		}

		if (!instance.player) return;

		instance.player.onPlaying.connect(mediaPlaying);
		instance.player.onPaused.connect(mediaPaused);
		instance.player.onStopped.connect(mediaStopped);
		instance.player.onStatusChanged.connect(playerStatusChanged);
		instance.player.onPositionChanged.connect(playerPositionChanged);
		instance.player.onError.connect(playerError);

		player = instance.player; // new instance

		instanceBounded();

		//if (showOnChanges) playerShow();
	}

	function playerShow(key) {
		//
		if (!mediaPlayer) return;
		//
		var lProcess = !key && !mediaPlayer.key || key && mediaPlayer.key === key;
		if (lProcess && player) {
			console.log("[playerShow]: player = " + mediaPlayer);
			mediaPlayer.visible = true;
		}
	}

	function playerHide(key) {
		//
		if (!mediaPlayer) return;
		//
		var lProcess = !key && !mediaPlayer.key || key && mediaPlayer.key === key;
		if (lProcess) {
			console.log("[playerHide]: player = " + mediaPlayer);
			mediaPlayer.visible = false;
		}
	}

	function playerToggle(key) {
		//
		if (!mediaPlayer) return;
		//
		var lProcess = !key && !mediaPlayer.key || key && mediaPlayer.key === key;
		if (lProcess && player) {
			console.log("[playerToggle]: mediaPlayer.visible = " + mediaPlayer.visible + ", instance = " + mediaPlayer);
			mediaPlayer.visible = !mediaPlayer.visible;
		}
	}

	function mediaPlaying() {
		//console.log("[mediaPlaying]: playing...");
		buzzerApp.wakeLock();
		actionButton.adjust();
	}

	function mediaPaused() {
		console.log("[mediaPaused]: paused...");
		buzzerApp.wakeRelease();
		actionButton.adjust();
	}

	function mediaStopped() {
		console.log("[mediaStopped]: clearing...");
		actionButton.adjust();
		elapsedTime.setTime(0);
		playSlider.value = 0;

		if (mediaPlayerController && mediaPlayerController.continuePlayback && mediaPlayerController.playbackController) {
			//console.log("[mediaStopped]: continue playback...");
			//mediaPlayerController.continuePlayback = mediaPlayerController.playbackController.mediaContainer.playNext();
		} else {
			if (!mediaPlayerController)
				console.log("[mediaStopped]: releasing wake lock, mediaPlayerController = " + mediaPlayerController);

			buzzerApp.wakeRelease();

			if (mediaPlayerController) {
				console.log("[mediaStopped]: releasing wake lock, mediaPlayerController.continuePlayback = " + mediaPlayerController.continuePlayback);
				mediaPlayerController.continuePlayback = false;
			}

			if (!showOnChanges) {
				mediaPlayer.visible = false;
				mediaPlayer.closed = true;
			}
		}
	}

	function playerDurationChanged(duration) {
		if (!player) return;
		console.info("[playerDurationChanged]: duration = " + duration);
		totalTime.setTotalTime(duration);
		playSlider.to = duration;

		if (showOnChanges) playerShow(key);
	}

	function playerStatusChanged(status) {
		if (!player) return;
		switch(player.status) {
			case MediaPlayer.Buffered:
				console.info("[playerStatusChanged]: player.duration = " + player.duration + ", player.size = " + player.size);
				if (player.duration > 0) {
					totalTime.setTotalTime(player.duration);
					playSlider.to = player.duration;
				}

				totalSize.setTotalSize(player.size);

				if (showOnChanges) playerShow(key);
			break;
		}
	}

	function playerError(error, errorString) {
	}

	function playerPositionChanged(position) {
		if (player) {
			//console.log("[playerPositionChanged]: position = " + player.position + ", instance = " + mediaPlayer + ", visible = " + mediaPlayer.visible);
			elapsedTime.setTime(player.position);
			playSlider.value = player.position;
		}
	}

	function terminate() {
		//
		if (player && !mediaPlayer.closed) {
			mediaPlayer.closed = true;
			player.stop();
		}
	}

	function playerDownloadStarted() {
		//
		if (mediaLoading) {
			console.log("[playerDownloadStarted]: ....");
			mediaLoading.start();
			mediaLoading.visible = true;
			//
			actionButton.symbol = Fonts.cancelSym;
		}
	}

	function playerDownloading(pos, size) {
		//
		if (mediaLoading) {
			mediaLoading.progress(pos, size);
		}
	}

	function playerDownloadCompleted() {
		//
		if (mediaLoading) {
			console.log("[playerDownloadCompleted]: ....");
			mediaLoading.visible = false;
			//
			actionButton.symbol = Fonts.playSym;
		}
	}

	visible: false

	color: "transparent"
	width: parent.width
	height: actionButton.height + 2 * spaceItems_

	property var playListAvailableCondition: mediaPlayerController && mediaPlayerController.playbackController &&
											  mediaPlayerController.playbackController.mediaCount > 1
	property bool playListAvailable: playListAvailableCondition !== undefined ? (playListAvailableCondition === true) : false

	Item {
		anchors.fill: parent

		ShaderEffectSource {
			id: effectSource
			sourceItem: overlayParent
			anchors.fill: parent
			sourceRect: overlayRect ? overlayRect : Qt.rect(mediaPlayer.x, mediaPlayer.y - (overlayParent ? overlayParent.y : 0), mediaPlayer.width, mediaPlayer.height)
			mipmap: true
		}

		GaussianBlur { //
		  id: blur
		  anchors.fill: parent
		  source: effectSource
		  radius: 18
		  samples: 32
		  cached: true
		}

		rotation: inverse ? 180 : 0
	}

	//
	Rectangle {
		id: controlsBack
		x: 0
		y: 0
		width: parent.width
		height: parent.height
		//
		color: buzzerApp.getColor(gallery ? galleryTheme : buzzerClient.theme, gallery ? gallerySelector : buzzerClient.themeSelector, "Material.disabledHidden.alt")
		opacity: overlayParent ? 0.8 : 0.5
		radius: 0
		visible: true
	}

	//
	QuarkRoundSymbolButton {
		id: prevButton
		x: controlsBack.x + 3 * spaceItems_
		y: controlsBack.y + spaceItems_ + (actionButton.height / 2 - height / 2)
		spaceLeft: 0
		spaceTop: 0
		symbol: Fonts.backwardSym
		fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 4)) : (buzzerApp.defaultFontSize() + 3)
		radius: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultRadius - 10)) : defaultRadius - 10
		color: buzzerApp.getColor(gallery ? galleryTheme : buzzerClient.theme, gallery ? gallerySelector : buzzerClient.themeSelector, "Material.menu.highlight")
		opacity: 1.0

		visible: playListAvailable

		onClick: {
			//
			if (mediaPlayerController && mediaPlayerController.playbackController) {
				mediaPlayerController.playbackController.mediaContainer.playPrev();
			}
		}
	}

	//
	QuarkRoundSymbolButton {
		id: actionButton
		x: prevButton.visible ? prevButton.x + prevButton.width + spaceItems_: controlsBack.x + 3 * spaceItems_
		y: controlsBack.y + spaceItems_

		/*
		spaceLeft: buzzerClient.isDesktop ? (symbol === Fonts.playSym ? (buzzerClient.scaleFactor * 2) : 0) :
					(symbol === Fonts.arrowDownHollowSym || symbol === Fonts.pauseSym) ?
						2 : (symbol === Fonts.playSym || symbol === Fonts.cancelSym ? 3 : 0)
		spaceTop: buzzerApp.isDesktop ? 0 : 2
		*/

		symbol: player ? (player.playing ? Fonts.pauseSym : Fonts.playSym) : Fonts.playSym
		fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 7)) : (buzzerApp.defaultFontSize() + 7)
		radius: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultRadius)) : defaultRadius
		color: buzzerApp.getColor(gallery ? galleryTheme : buzzerClient.theme, gallery ? gallerySelector : buzzerClient.themeSelector, "Material.menu.highlight")
		opacity: 1.0

		onSymbolChanged: {
			if (symbol !== Fonts.playSym) spaceLeft = 0;
			else spaceLeft = buzzerClient.scaleFactor * 2;
		}

		onClick: {
			//
			if (!player.playing) {
				player.play();
			} else {
				player.pause();
			}
		}

		function notLoaded() {
			symbol = Fonts.cancelSym;
			textColor = buzzerApp.getColor(gallery ? galleryTheme : buzzerClient.theme, gallery ? gallerySelector : buzzerClient.themeSelector, "Buzzer.event.like");
		}

		function loaded() {
			symbol = Fonts.playSym;
			textColor = buzzerApp.getColor(gallery ? galleryTheme : buzzerClient.theme, gallery ? gallerySelector : buzzerClient.themeSelector, "Material.menu.foreground")
		}

		function adjust() {
			symbol = (player && player.playing ? Fonts.pauseSym : Fonts.playSym);
		}
	}

	//
	QuarkRoundSymbolButton {
		id: nextButton
		x: actionButton.x + actionButton.width + spaceItems_
		y: prevButton.y
		spaceLeft: 0
		spaceTop: 0
		symbol: Fonts.forwardSym
		fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 4)) : (buzzerApp.defaultFontSize() + 3)
		radius: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultRadius - 10)) : defaultRadius - 10
		color: buzzerApp.getColor(gallery ? galleryTheme : buzzerClient.theme, gallery ? gallerySelector : buzzerClient.themeSelector, "Material.menu.highlight")
		opacity: 1.0

		visible: playListAvailable

		onClick: {
			//
			if (mediaPlayerController && mediaPlayerController.playbackController) {
				mediaPlayerController.playbackController.mediaContainer.playNext();
			}
		}
	}

	QuarkRoundSymbolButton {
		id: closeButton
		x: controlsBack.width - (closeButton.width + spaceItems_)
		y: controlsBack.y + spaceItems_
		spaceLeft: buzzerClient.isDesktop ? buzzerClient.scaleFactor : 2
		spaceTop: buzzerClient.isDesktop ? buzzerClient.scaleFactor : 2
		symbol: Fonts.cancelSym
		fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 10)) : (buzzerApp.defaultFontSize() + 7)
		radius: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultRadius)) : defaultRadius
		color: "transparent" //buzzerApp.getColor(gallery ? galleryTheme : buzzerClient.theme, gallery ? gallerySelector : buzzerClient.themeSelector, "Material.menu.highlight")
		textColor: buzzerApp.getColorStatusBar(gallery ? galleryTheme : buzzerClient.theme, gallery ? gallerySelector : buzzerClient.themeSelector, "Material.foreground")
		opacity: 1.0

		onClick: {
			//
			mediaPlayer.closed = true;
			if (mediaPlayerController) {
				mediaPlayerController.popVideoInstance();
				mediaPlayerController.continuePlayback = false;
			}

			//
			if (player) { player.stop(); /*player.destroy(5000);*/ }
			mediaPlayer.visible = false;
		}
	}

	QuarkLabel {
		id: caption
		x: nextButton.visible ? nextButton.x + nextButton.width + 2*spaceItems_ :
								actionButton.x + actionButton.width + 2*spaceItems_
		y: actionButton.y + 1
		width: playSlider.width
		elide: Text.ElideRight
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize)) : buzzerApp.defaultFontSize()
		text: player ? player.caption : "none"
		visible: text != "none" && text != ""
		opacity: 1.0
		color: buzzerApp.getColor(gallery ? galleryTheme : buzzerClient.theme, gallery ? gallerySelector : buzzerClient.themeSelector, "Material.foreground")
	}

	QuarkLabel {
		id: elapsedTime
		x: nextButton.visible ? (nextButton.x + nextButton.width + 2*spaceItems_ + (caption.visible ? 3 : 0)) :
								(actionButton.x + actionButton.width + 2*spaceItems_ + (caption.visible ? 3 : 0))
		y: actionButton.y + (caption.visible ? caption.height + 3 : spaceItems_)
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - (caption.visible ? 3 : 0))) : buzzerApp.defaultFontSize()
		text: "00:00"
		opacity: 1.0
		color: buzzerApp.getColor(gallery ? galleryTheme : buzzerClient.theme, gallery ? gallerySelector : buzzerClient.themeSelector, "Material.foreground")

		function setTime(ms) {
			text = DateFunctions.msToTimeString(ms);
		}
	}

	QuarkLabel {
		id: totalTime
		x: elapsedTime.x + elapsedTime.width
		y: elapsedTime.y
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - (caption.visible ? 3 : 0))) : buzzerApp.defaultFontSize()
		text: player ? (player.duration ? ("/" + DateFunctions.msToTimeString(player.duration)) : "/00:00") : "/00:00"
		opacity: 1.0
		color: buzzerApp.getColor(gallery ? galleryTheme : buzzerClient.theme, gallery ? gallerySelector : buzzerClient.themeSelector, "Material.foreground")

		function setTotalTime(ms) {
			text = "/" + DateFunctions.msToTimeString(ms);
		}
	}

	QuarkLabel {
		id: totalSize
		x: totalTime.x + totalTime.width
		y: elapsedTime.y
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - (caption.visible ? 3 : 0))) : buzzerApp.defaultFontSize()
		text: ", 0k"
		visible: player && player.size !== 0 || false
		opacity: 1.0
		color: buzzerApp.getColor(gallery ? galleryTheme : buzzerClient.theme, gallery ? gallerySelector : buzzerClient.themeSelector, "Material.foreground")

		function setTotalSize(mediaSize) {
			//
			if (mediaSize < 1000) text = ", " + mediaSize + "b";
			else text = ", " + NumberFunctions.numberToCompact(mediaSize);
		}
	}

	Slider {
		id: playSlider
		x: nextButton.visible ? nextButton.x + nextButton.width + spaceItems_ :
								actionButton.x + actionButton.width + spaceItems_
		y: actionButton.y + actionButton.height - (height - 3 * spaceItems_) + (buzzerApp.isDesktop && caption.visible ? 3 : 0)
		from: 0
		to: 0
		orientation: Qt.Horizontal
		stepSize: 0.1
		width: closeButton.x - (nextButton.visible ? nextButton.x + nextButton.width : actionButton.x + actionButton.width)
		opacity: 1.0

		onMoved: {
			player.seek(value);
			elapsedTime.setTime(value);
		}

		onToChanged: {
			console.log("[buzzplayer/onToChanged]: to = " + to);
		}
	}

	QuarkRoundProgress {
		id: mediaLoading
		x: actionButton.x - buzzerClient.scaleFactor * 2
		y: actionButton.y - buzzerClient.scaleFactor * 2
		size: actionButton.width + buzzerClient.scaleFactor * 4
		colorCircle: buzzerApp.getColor(gallery ? galleryTheme : buzzerClient.theme, gallery ? gallerySelector : buzzerClient.themeSelector, "Material.link");
		colorBackground: buzzerApp.getColor(gallery ? galleryTheme : buzzerClient.theme, gallery ? gallerySelector : buzzerClient.themeSelector, "Material.link");
		arcBegin: 0
		arcEnd: 0
		lineWidth: buzzerClient.scaleFactor * 2
		visible: false
		animationDuration: 50
		opacity: 1.0

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
}

