import QtQuick 2.15
import QtQuick.Controls 2.2
import QtMultimedia 5.15

import app.buzzer.components 1.0 as BuzzerComponents
import app.buzzer.commands 1.0 as BuzzerCommands

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

import "qrc:/lib/numberFunctions.js" as NumberFunctions
import "qrc:/lib/dateFunctions.js" as DateFunctions

Item {
	//
	id: playerController

	property var controller;
	property var playbackController: null;
	property bool continuePlayback: true
	property var lastInstance: null;
	property var prevInstance: null;

	Component.onCompleted: buzzerApp.setSharedMediaPlayerController(playerController);

	//
	signal newInstanceCreated(var instance, var prev);
	signal showCurrentPlayer();
	signal hideCurrentPlayer();
	signal toggleCurrentPlayer();

	signal playbackDownloadStarted();
	signal playbackDownloading(var pos, var size);
	signal playbackDownloadCompleted();

	//
	function continueCreateInstance() {
	}

	function createInstance(playerPlceholder, videooutPlaceholder, playback, continuous) {
		//
		var lPlayer = null;
		var lVideoOutput = null;

		//
		var lPosition = 0;
		var lSource;

		//
		continuePlayback = false;

		//
		if (lastInstance && lastInstance.player) {
			//
			console.log("[createInstance]: lastInstance = " + lastInstance + ", lastInstance.player = " + lastInstance.player);
			lSource = lastInstance.player.source;
			lPosition = lastInstance.player.position;
			lastInstance.player.stop();
		} else {
			if (!lastInstance) console.log("[createInstance]: lastInstance = " + lastInstance);
			else if (lastInstance.player) console.log("[createInstance]: lastInstance.player = " + lastInstance.player);
		}

		// make new
		var lComponent = Qt.createComponent("qrc:/qml/buzzitemmedia-mediaplayer.qml");
		if (lComponent.status === Component.Error) {
			controller.showError(lComponent.errorString());
		} else {
			lPlayer = lComponent.createObject(playerController /*playerPlceholder*/);

			var lComponentOut = Qt.createComponent("qrc:/qml/buzzitemmedia-videooutput.qml");
			if (lComponentOut.status === Component.Error) {
				controller.showError(lComponentOut.errorString());
			} else {
				lVideoOutput = lComponentOut.createObject(videooutPlaceholder);
				lVideoOutput.player = lPlayer;

				if (continuous && lSource) {
					// TODO: ?
					console.log("[createInstance]: source = " + lSource + ", position = " + lPosition);
					lPlayer.source = lSource;
					lPlayer.seek(lPosition);
					lPlayer.play();
				}

				prevInstance = lastInstance;
				lastInstance = lVideoOutput;
				playbackController = playback;
				continuePlayback = true; // possible

				newInstanceCreated(lVideoOutput, prevInstance);

				return lastInstance;
			}
		}

		return null;
	}

	function createVideoInstance(videooutPlaceholder, player) {
		//
		var lPlayer = player;
		var lVideoOutput = null;

		console.log("[createVideoInstance]: creating...");
		 var lComponentOut = Qt.createComponent("qrc:/qml/buzzitemmedia-videooutput.qml");
		 if (lComponentOut.status === Component.Error) {
			 console.log("[createVideoInstance]: " + lComponentOut.errorString());
			 controller.showError(lComponentOut.errorString());
		 } else {
			 lVideoOutput = lComponentOut.createObject(videooutPlaceholder);
			 lVideoOutput.player = lPlayer;

			 return lVideoOutput;
		 }

		 return null;
	}

	function disableContinousPlayback() {
		//
		console.log("[disableContinousPlayback]: disabling");
		playbackController = null;
		continuePlayback = false;
	}

	function popVideoInstance(player) {
		//
		var lPlayer = player;
		if (!lastInstance) return;
		//
		if (!lPlayer) lPlayer = lastInstance.player;
		if (!lPlayer) return;
		//if (!lPlayer.videoOutput) return;

		lPlayer.popSurface();
	}

	function popAllVideoInstances(player) {
		//
		var lPlayer = player;
		if (!lastInstance) return;
		//
		if (!lPlayer) lPlayer = lastInstance.player;
		if (!lPlayer) return;
		//if (!lPlayer.videoOutput) return;

		lPlayer.clearSurfaces();
	}

	function createAudioInstance(playerPlceholder, root, playback, continuous) {
		//
		var lPlayer = null;
		var lVideoOutput = null;

		//
		var lPosition = 0;
		var lSource;

		//
		continuePlayback = false;

		// clean-up
		if (lastInstance && lastInstance.player) {
			//
			lSource = lastInstance.player.source;
			lPosition = lastInstance.player.position;
			lastInstance.player.stop();
		}

		//
		var lComponent = Qt.createComponent("qrc:/qml/buzzitemmedia-mediaplayer.qml");
		if (lComponent.status === Component.Error) {
			controller.showError(lComponent.errorString());
		} else {
			lPlayer = lComponent.createObject(playerController /*playerPlceholder*/);
			root.player = lPlayer;

			if (continuous && lSource) {
				// TODO: ?
				console.log("[createAudioInstance]: source = " + lSource + ", position = " + lPosition);
				lPlayer.source = lSource;
				lPlayer.seek(lPosition);
				lPlayer.play();
			}

			prevInstance = lastInstance;
			lastInstance = root;
			playbackController = playback;
			continuePlayback = true;
			newInstanceCreated(root, prevInstance);

			return lastInstance;
		}

		return null;
	}

	function linkInstance(instance, playback) {
		//
		if (instance !== lastInstance) {
			prevInstance = lastInstance;
			lastInstance = instance;
			playbackController = playback;
		}

		newInstanceCreated(instance, prevInstance);
	}

	function isCurrentInstancePlaying() {
		if (lastInstance && lastInstance.player) {
			return lastInstance.player.playing;
		}

		return false;
	}

	function isCurrentInstanceStopped() {
		if (lastInstance && lastInstance.player) {
			return lastInstance.player.stopped;
		}

		return false;
	}

	function isCurrentInstancePaused() {
		if (lastInstance && lastInstance.player) {
			return lastInstance.player.paused;
		}

		return false;
	}
}
