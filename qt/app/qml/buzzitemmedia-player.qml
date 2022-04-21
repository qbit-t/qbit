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
	readonly property real defaultFontSize: 11

	property var mediaPlayerControler
	property var player
	property bool closed: true

	signal instanceBounded();

	onMediaPlayerControlerChanged: {
		//
		if (!mediaPlayerControler) return;
		mediaPlayerControler.newInstanceCreated.connect(playerNewInstanceCreated);
		mediaPlayerControler.showCurrentPlayer.connect(playerShow);
		mediaPlayerControler.hideCurrentPlayer.connect(playerHide);
	}

	onPlayerChanged: {
		//console.log("[onPlayerChanged]: player === null");
		if (player === null /*&& !mediaPlayer.closed*/) {
			console.log("[onPlayerChanged]: player === null");
			mediaStopped();
		}
	}

	function playerNewInstanceCreated(instance, prev) {
		//
		console.log("[playerNewInstanceCreated]: instance = " + instance + ", player = " + instance.player + ", prev = " + prev);

		if (prev && prev.player) {
			prev.player.stop();
			prev.player.onPlaying.disconnect(mediaPlaying);
			prev.player.onPaused.disconnect(mediaPaused);
			prev.player.onStopped.disconnect(mediaStopped);
			prev.player.onStatusChanged.disconnect(playerStatusChanged);
			prev.player.onPositionChanged.disconnect(playerPositionChanged);
			prev.player.onError.disconnect(playerError);
		} else if (player) {
			player.stop();
			player.onPlaying.disconnect(mediaPlaying);
			player.onPaused.disconnect(mediaPaused);
			player.onStopped.disconnect(mediaStopped);
			player.onStatusChanged.disconnect(playerStatusChanged);
			player.onPositionChanged.disconnect(playerPositionChanged);
			player.onError.disconnect(playerError);
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
	}

	function playerShow() {
		console.log("[playerShow]: ...");
		mediaPlayer.visible = true;
		mediaPlayer.closed = false;
	}

	function playerHide() {
		console.log("[playerHide]: ...");
		mediaPlayer.visible = false;
		mediaPlayer.closed = true;
	}

	function mediaPlaying() {
		buzzerApp.wakeLock();
		actionButton.adjust();
	}

	function mediaPaused() {
		buzzerApp.wakeRelease();
		actionButton.adjust();
	}

	function mediaStopped() {
		buzzerApp.wakeRelease();
		actionButton.adjust();
		elapsedTime.setTime(0);
		playSlider.value = 0;
		mediaPlayer.visible = false;
		mediaPlayer.closed = true;

		//if (player) player.destroy(1000);
	}

	function playerStatusChanged(status) {
		if (!player) return;
		switch(player.status) {
			case MediaPlayer.Buffered:
				totalTime.setTotalTime(player.duration);
				totalSize.setTotalSize(player.size);
				playSlider.to = player.duration;
			break;
		}
	}

	function playerError(error, errorString) {
	}

	function playerPositionChanged(position) {
		if (player) {
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

	visible: false

	//
	color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background")
	width: parent.width
	height: frameContainer.height

	//
	QuarkRoundRectangle {
		id: frameContainer
		x: 0
		y: 0
		width: parent.width
		height: actionButton.height + 2 * spaceItems_

		color: "transparent"
		backgroundColor: "transparent"

		//radius: 8
		penWidth: 1

		//visible: true
	}

	//
	Rectangle {
		id: controlsBack
		x: frameContainer.x // + 2 * spaceItems_
		y: frameContainer.y // + frameContainer.height - (actionButton.height + 4 * spaceItems_)
		width: frameContainer.width //- (4 * spaceItems_)
		height: actionButton.height + 2 * spaceItems_
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		opacity: 0.3
		radius: 0
	}

	//
	QuarkRoundSymbolButton {
		id: actionButton
		x: controlsBack.x + 3 * spaceItems_
		y: controlsBack.y + spaceItems_
		spaceLeft: symbol === Fonts.arrowDownHollowSym || symbol === Fonts.pauseSym ? 2 :
					   (symbol === Fonts.playSym || symbol === Fonts.cancelSym ? 3 : 0)
		spaceTop: 2
		symbol: player ? (player.playing ? Fonts.pauseSym : Fonts.playSym) : Fonts.playSym
		fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 7)) : 18
		radius: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultRadius)) : defaultRadius
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.highlight")

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
			textColor = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.event.like");
		}

		function loaded() {
			symbol = Fonts.playSym;
			textColor = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")
		}

		function adjust() {
			symbol = (player && player.playing ? Fonts.pauseSym : Fonts.playSym);
		}
	}

	QuarkRoundSymbolButton {
		id: closeButton
		x: frameContainer.width - (closeButton.width + spaceItems_)
		y: controlsBack.y + spaceItems_
		spaceLeft: 0
		spaceTop: 0
		symbol: Fonts.cancelSym
		fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 10)) : 18
		radius: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultRadius)) : defaultRadius
		color: "transparent" //buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.highlight")
		textColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")

		onClick: {
			//
			mediaPlayer.closed = true;
			if (player) { player.stop(); player.destroy(5000); }
			else {
				mediaPlayer.visible = false;
			}
		}
	}

	QuarkLabel {
		id: elapsedTime
		x: actionButton.x + actionButton.width + 2*spaceItems_
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
		text: player ? (player.duration ? ("/" + DateFunctions.msToTimeString(player.duration)) : "/00:00") : "/00:00"

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
		visible: player && player.size !== 0 || false

		function setTotalSize(mediaSize) {
			//
			if (mediaSize < 1000) text = ", " + mediaSize + "b";
			else text = ", " + NumberFunctions.numberToCompact(mediaSize);
		}
	}

	Slider {
		id: playSlider
		x: actionButton.x + actionButton.width + spaceItems_
		y: actionButton.y + actionButton.height - (height - 3 * spaceItems_)
		from: 0
		to: 1
		orientation: Qt.Horizontal
		stepSize: 0.1
		width: frameContainer.width - (actionButton.width + 4 * spaceItems_ + closeButton.width)

		onMoved: {
			player.seek(value);
			elapsedTime.setTime(value);
		}

		onToChanged: {
			console.log("[onToChanged]: to = " + to);
		}
	}
}

