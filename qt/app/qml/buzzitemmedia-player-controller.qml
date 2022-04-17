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
	function createMediaPlayerInstance(requester) {
	}

	function createInstance(playerPlceholder, videooutPlaceholder) {
		//
		var lPlayer = null;
		var lVideoOutput = null;

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
				lVideoOutput.source = lPlayer;

				prevInstance = lastInstance;
				lastInstance = lVideoOutput;
				newInstanceCreated(lVideoOutput, prevInstance);

				return lastInstance;
			}
		}

		return null;
	}

	function linkInstance(instance) {
		//
		prevInstance = lastInstance;
		lastInstance = instance;
		newInstanceCreated(instance, prevInstance);
	}

	function isCurrentInstancePlaying() {
		if (lastInstance && lastInstance.player) {
			return lastInstance.player.playing;
		}

		return false;
	}
}
