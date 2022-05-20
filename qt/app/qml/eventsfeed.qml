﻿import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Dialogs 1.1
import QtGraphicalEffects 1.0
import Qt.labs.folderlistmodel 2.11
import QtMultimedia 5.8
import QtQuick.Window 2.15

import app.buzzer.commands 1.0 as BuzzerCommands

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

Item
{
	id: eventsfeed_

	property var infoDialog;
	property var controller;
	property var mediaPlayerController;

	function externalPull() {
		modelLoader.restart();
	}

	Component.onCompleted: {
		//
		if (buzzerClient.buzzerDAppReady) {
			modelLoader.restart();
		}
	}

	onWidthChanged: {
		orientationChangedTimer.start();
	}

	// to adjust model
	Timer {
		id: orientationChangedTimer
		interval: 100
		repeat: false
		running: false

		onTriggered: {
			list.adjust();
		}
	}

	Connections {
		target: buzzerClient

		function onBuzzerDAppReadyChanged() {
			if (buzzerClient.buzzerDAppReady) {
				modelLoader.start();
			}
		}

		function onBuzzerDAppResumed() {
			if (buzzerClient.buzzerDAppReady) {
				modelLoader.processAndMerge();
			}
		}

		function onThemeChanged() {
			buzzerClient.getEventsfeedList().resetModel();
		}

		function onBuzzerChanged() {
			if (buzzerClient.buzzerDAppReady) {
				modelLoader.restart();
			}
		}
	}

	BuzzerCommands.LoadEventsfeedCommand {
		id: modelLoader
		model: buzzerClient.getEventsfeedList()

		property bool dataReceived: false
		property bool dataRequested: false
		property bool requestProcessed: true

		onProcessed: {
			console.log("[eventsfeed]: loaded");
			dataReceived = true;
			dataRequested = false;
			requestProcessed = true;
		}

		onError: {
			// code, message
			console.log("[eventsfeed/error]: " + code + " | " + message);
			dataReceived = false;
			dataRequested = false;
			requestProcessed = true;
			controller.showError(message);
		}

		function start() {
			//
			if (!dataReceived && !dataRequested) {
				dataRequested = true;
				requestProcessed = false;
				modelLoader.process(false);
				return true;
			}

			return false;
		}

		function restart() {
			dataReceived = false;
			dataRequested = false;
			start();
		}

		function feed() {
			//
			if (!start() && !model.noMoreData && !dataRequested) {
				dataRequested = true;
				modelLoader.process(true);
			}
		}
	}

	QuarkListView {
		id: list
		x: 0
		y: 0
		width: parent.width
		height: parent.height
		usePull: true
		clip: true

		model: buzzerClient.getEventsfeedList()

		add: Transition {
			enabled: true
			NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 400 }
		}

		function adjust() {
			//
			for (var lIdx = 0; lIdx < list.count; lIdx++) {
				var lItem = list.itemAtIndex(lIdx);
				if (lItem) {
					lItem.width = list.width;
				}
			}
		}

		onDragStarted: {
		}

		onDraggingVerticallyChanged: {
		}

		onDragEnded: {
			if (list.pullReady) {
				modelLoader.restart();
			}
		}

		onFeedReady: {
			//
			modelLoader.feed();
		}

		delegate: ItemDelegate {
			id: itemDelegate

			property var buzzItem;

			hoverEnabled: buzzerApp.isDesktop
			onHoveredChanged: {
				if (eventsfeed_.mediaPlayerController &&
						(eventsfeed_.mediaPlayerController.isCurrentInstancePlaying() ||
										   eventsfeed_.mediaPlayerController.isCurrentInstancePaused())) {
					eventsfeed_.mediaPlayerController.showCurrentPlayer(null);
				}
			}

			onClicked: {
				// open thread
				var lComponent = null;
				var lPage = null;

				if (type === buzzerClient.tx_BUZZER_SUBSCRIBE_TYPE() ||
					type === buzzerClient.tx_BUZZER_ENDORSE_TYPE() ||
					type === buzzerClient.tx_BUZZER_MISTRUST_TYPE()) {
					//
					controller.openBuzzfeedByBuzzer(getBuzzerName());
				} else if (type === buzzerClient.tx_BUZZER_CONVERSATION() ||
							type === buzzerClient.tx_BUZZER_ACCEPT_CONVERSATION() ||
							type === buzzerClient.tx_BUZZER_DECLINE_CONVERSATION() ||
							type === buzzerClient.tx_BUZZER_MESSAGE() ||
							type === buzzerClient.tx_BUZZER_MESSAGE_REPLY()) {
					//
					var lConversationId = buzzId;
					if (type === buzzerClient.tx_BUZZER_MESSAGE() || type === buzzerClient.tx_BUZZER_MESSAGE_REPLY()) {
						if (eventInfos.length) {
							var lInfo = eventInfos[0];
							lConversationId = lInfo.eventId;
						}
					}
					//
					var lConversation = buzzerClient.locateConversation(lConversationId);
					if (lConversation) {
						controller.openConversation(lConversationId, lConversation, buzzerClient.getConversationsList(), buzzId);
					}
				} else {
					//
					controller.openThread(buzzChainId, buzzId, getBuzzerAlias(), buzzBodyFlat);
				}
			}

			function getBuzzerName() {
				if (eventInfos.length > 0) {
					return buzzerClient.getBuzzerName(eventInfos[0].buzzerInfoId);
				}

				return buzzerClient.getBuzzerName(publisherInfoId);
			}

			function getBuzzerAlias() {
				if (eventInfos.length > 0) {
					return buzzerClient.getBuzzerAlias(eventInfos[0].buzzerInfoId);
				}

				return buzzerClient.getBuzzerAlias(publisherInfoId);
			}

			onWidthChanged: {
				if (buzzItem) {
					buzzItem.width = list.width;
					itemDelegate.height = buzzItem.calculateHeight();
				}
			}

			Component.onCompleted: {
				var lSource = "";
				if (type === buzzerClient.tx_BUZZER_SUBSCRIBE_TYPE()) {
					lSource = "qrc:/qml/eventsubscribeitem.qml";
				} else if (type === buzzerClient.tx_BUZZ_LIKE_TYPE() ||
							type === buzzerClient.tx_REBUZZ_TYPE()) {
					lSource = "qrc:/qml/eventlikerebuzzitem.qml";
				} else if (type === buzzerClient.tx_BUZZ_REWARD_TYPE() ||
							type === buzzerClient.tx_BUZZER_ENDORSE_TYPE() ||
							type === buzzerClient.tx_BUZZER_MISTRUST_TYPE()) {
					lSource = "qrc:/qml/eventdonateendorsemistrustitem.qml";
				} else if (type === buzzerClient.tx_BUZZ_REPLY_TYPE() ||
										type === buzzerClient.tx_REBUZZ_REPLY_TYPE()) {
					lSource = "qrc:/qml/eventreplyrebuzzitem.qml";
				} else if (type === buzzerClient.tx_BUZZER_CONVERSATION() ||
							type === buzzerClient.tx_BUZZER_ACCEPT_CONVERSATION() ||
							type === buzzerClient.tx_BUZZER_DECLINE_CONVERSATION() ||
							type === buzzerClient.tx_BUZZER_MESSAGE() ||
							type === buzzerClient.tx_BUZZER_MESSAGE_REPLY()) {
					lSource = "qrc:/qml/eventconversationitem.qml";
				}

				var lComponent = Qt.createComponent(lSource);
				if (lComponent.status === Component.Error) {
					eventsfeed_.controller.showError(lComponent.errorString());
				} else {
					buzzItem = lComponent.createObject(itemDelegate);

					buzzItem.width = list.width;
					buzzItem.controller_ = eventsfeed_.controller;
					buzzItem.eventsfeedModel_ = buzzerClient.getEventsfeedList();
					buzzItem.listView_ = list;

					itemDelegate.height = buzzItem.calculateHeight();
					itemDelegate.width = list.width;

					buzzItem.calculatedHeightModified.connect(itemDelegate.calculatedHeightModified);
				}
			}

			function calculatedHeightModified(value) {
				itemDelegate.height = value;
			}
		}
	}

	//
	BuzzItemMediaPlayer {
		id: player
		x: 0
		y: (list.y + list.height) - height // buzzerApp.isDesktop ? 0 : search.y + search.calculatedHeight
		width: parent.width
		mediaPlayerController: eventsfeed_.mediaPlayerController
		overlayParent: list
	}

	QuarkBusyIndicator {
		anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter; }
		running: !modelLoader.requestProcessed
	}
}
