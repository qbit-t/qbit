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

	function externalTop() {
		list.positionViewAtBeginning();
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
			if (buzzerClient.buzzerDAppReady && !modelLoader.initialized()) {
				modelLoader.start();
			}
		}

		function onBuzzerDAppResumed() {
			if (buzzerClient.buzzerDAppReady) {
				console.log("[events/onBuzzerDAppResumed]: resuming...");
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
			list.reuseItems = true;
		}

		onError: {
			// code, message
			console.log("[eventsfeed/error]: " + code + " | " + message);
			dataReceived = false;
			dataRequested = false;
			requestProcessed = true;
			list.reuseItems = true;

			// just restart
			if (code == "E_LOAD_EVENTSFEED") {
				switchDataTimer.start();
			}
		}

		function start() {
			//
			if (!dataReceived && !dataRequested) {
				list.reuseItems = false;
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

		function initialized() { return dataReceived && dataRequested; }
	}

	Timer {
		id: switchDataTimer
		interval: 500
		repeat: false
		running: false

		onTriggered: {
			if (buzzerClient.buzzerDAppReady) {
				modelLoader.restart();
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
		cacheBuffer: 500
		reuseItems: true

		model: buzzerClient.getEventsfeedList()

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
			property int buzzType: 0;

			ListView.onPooled: {
				unbindItem();
				unbindCommonControls();
			}

			ListView.onReused: {
				// replace inner instance
				if (buzzItem && buzzType !== type) {
					// create and replace
					buzzItem.destroy();
					createItem();
				} else {
					// just rebind
					bindItem();
				}
			}

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
				if (itemDelegate.buzzItem) {
					var lHeight = itemDelegate.buzzItem.calculateHeight();
					itemDelegate.buzzItem.width = list.width;
					itemDelegate.height = lHeight;
					itemDelegate.buzzItem.height = lHeight;
				}
			}

			Component.onCompleted: createItem()

			function createItem() {
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

				var lType = type;
				buzzType = lType; // avoid binding

				var lComponent = Qt.createComponent(lSource);
				if (lComponent.status === Component.Error) {
					eventsfeed_.controller.showError(lComponent.errorString());
				} else {
					buzzItem = lComponent.createObject(itemDelegate);
					bindItem();
				}
			}

			function bindItem() {
				//
				buzzItem.calculatedHeightModified.connect(itemDelegate.calculatedHeightModified);
				buzzItem.bindItem();
				//
				buzzItem.width = list.width;
				buzzItem.controller_ = eventsfeed_.controller;
				buzzItem.eventsfeedModel_ = buzzerClient.getEventsfeedList();
				buzzItem.listView_ = list;

				itemDelegate.height = buzzItem.calculateHeight();
				itemDelegate.width = list.width;
			}

			function calculatedHeightModified(value) {
				itemDelegate.height = value;
				itemDelegate.buzzItem.height = value;
			}

			function unbindItem() {
				if (buzzItem)
					buzzItem.calculatedHeightModified.disconnect(itemDelegate.calculatedHeightModified);
			}

			function unbindCommonControls() {
				if (buzzItem)
					buzzItem.unbindCommonControls();
			}

			function forceVisibilityCheck(check) {
				if (buzzItem) {
					buzzItem.forceVisibilityCheck(check);
				}
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
