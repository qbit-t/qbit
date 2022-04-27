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
	property var lastInstance: null;
	property var prevInstance: null;

	Component.onCompleted: buzzerApp.setSharedMediaPlayerController(playerController);

	//
	signal newInstanceCreated(var instance, var prev);
	signal showCurrentPlayer();
	signal hideCurrentPlayer();

	//
	function continueCreateInstance() {
	}

	function createInstance(playerPlceholder, videooutPlaceholder, continuous) {
		//
		var lPlayer = null;
		var lVideoOutput = null;

		//
		var lPosition = 0;
		var lSource;

		//
		if (lastInstance && lastInstance.player) {
			//
			lSource = lastInstance.player.source;
			lPosition = lastInstance.player.position;
			lastInstance.player.stop();
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

	function popVideoInstance(player) {
		//
		var lPlayer = player;
		if (!lastInstance) return;
		//
		if (!lPlayer) lPlayer = lastInstance.player;
		if (!lPlayer) return;
		if (!lPlayer.videoOutput) return;

		lPlayer.popSurface();
	}

	function popAllVideoInstances(player) {
		var lPlayer = player;
		if (!lastInstance) return;
		//
		if (!lPlayer) lPlayer = lastInstance.player;
		if (!lPlayer) return;
		if (!lPlayer.videoOutput) return;

		lPlayer.clearSurfaces();
	}

	function createAudioInstance(playerPlceholder, root) {
		//
		var lPlayer = null;
		var lVideoOutput = null;

		// clean-up
		if (lastInstance && lastInstance.player) {
			lastInstance.player.stop();
			popAllVideoInstances();
		}

		//
		var lComponent = Qt.createComponent("qrc:/qml/buzzitemmedia-mediaplayer.qml");
		if (lComponent.status === Component.Error) {
			controller.showError(lComponent.errorString());
		} else {
			lPlayer = lComponent.createObject(playerController /*playerPlceholder*/);

			root.player = lPlayer;

			prevInstance = lastInstance;
			lastInstance = root;
			newInstanceCreated(root, prevInstance);

			return lastInstance;
		}

		return null;
	}

	function linkInstance(instance) {
		//
		if (instance !== lastInstance) {
			prevInstance = lastInstance;
			lastInstance = instance;
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
