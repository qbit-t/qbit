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
import app.buzzer.components 1.0 as BuzzerComponents

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

Item
{
	id: buzzfeed_

	property var infoDialog;
	property var controller;
	property var mediaPlayerController;

	function externalPull() {
		switchDataTimer.start();
	}

	function externalTop() {
		list.positionViewAtBeginning();
	}

	Component.onCompleted: {
		//
		switchDataTimer.start();
	}

	onWidthChanged: {
		orientationChangedTimer.start();
	}

	onMediaPlayerControllerChanged: {
		//
		buzzerApp.sharedMediaPlayerController(mediaPlayerController);
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

	Timer {
		id: switchDataTimer
		interval: 500
		repeat: false
		running: false

		onTriggered: {
			//
			if (mediaPlayerController) {
				mediaPlayerController.disableContinousPlayback();
				mediaPlayerController.popVideoInstance();
			}

			//
			if (buzzerClient.buzzerDAppReady) {
				modelLoader.restart();
			}
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
			//
			buzzfeedPlayer.terminate();
			buzzerClient.getBuzzfeedList().resetModel();
		}

		function onBuzzerChanged() {
			if (buzzerClient.buzzerDAppReady) {
				console.log("[buzzfeed/onBuzzerChanged]: name = " + buzzerClient.name);
				//modelLoader.restart();
				switchDataTimer.start();
			}
		}
	}

	BuzzerCommands.LoadBuzzfeedCommand	{
		id: modelLoader
		model: buzzerClient.getBuzzfeedList()

		property bool dataReceived: false
		property bool dataRequested: false
		property bool requestProcessed: true
		property bool feeding: false

		onProcessed: {
			console.log("[buzzfeed]: loaded");
			dataReceived = true;
			dataRequested = false;
			requestProcessed = true;
			list.reuseItems = true;

			if (!feeding) {
				// goto begin unconditionally
				list.positionViewAtBeginning();
			} else feeding = false;
		}

		onError: {
			// code, message
			console.log("[buzzfeed/error]: " + code + " | " + message);
			dataReceived = false;
			dataRequested = false;
			requestProcessed = true;

			// just restart
			if (code == "E_LOAD_BUZZFEED") {
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
				feeding = true;
				modelLoader.process(true);
			}
		}
	}

	//
	QuarkListView {
		id: list
		x: 0
		y: 0
		width: parent.width
		height: parent.height
		usePull: true
		clip: true
		highlightFollowsCurrentItem: false
		cacheBuffer: 500
		reuseItems: true

		model: buzzerClient.getBuzzfeedList()

		function adjust() {
			//
			for (var lIdx = 0; lIdx < list.count; lIdx++) {
				var lItem = list.itemAtIndex(lIdx);
				if (lItem) {
					lItem.width = list.width;
				}
			}
		}

		function unbind() {
			//
			var lVisible;
			var lProcessable;
			var lForwardItem;
			// trace forward
			for (var lForwardIdx = 0; lForwardIdx < list.count; lForwardIdx++) {
				//
				lForwardItem = list.itemAtIndex(lForwardIdx);
				if (lForwardItem) {
					lVisible = lForwardItem.y >= list.contentY && lForwardItem.y + lForwardItem.height < list.contentY + list.height;
					lProcessable = (lForwardItem.y + lForwardItem.height) > list.contentY + list.height && (lForwardItem.y + lForwardItem.height) - (list.contentY + list.height) >= (cacheBuffer * 0.7);

					if (!lProcessable || lVisible) {
						lForwardItem.unbindCommonControls();
					}
				}
			}
		}

		onContentYChanged: {
			//
			var lVisible;
			var lProcessable;
			var lBackItem;
			var lForwardItem;
			var lBeginIdx = list.indexAt(1, contentY);
			//
			if (lBeginIdx > -1) {
				// trace back
				for (var lBackIdx = lBeginIdx; lBackIdx >= 0; lBackIdx--) {
					//
					lBackItem = list.itemAtIndex(lBackIdx);
					if (lBackItem) {
						lVisible = lBackItem.y >= list.contentY && lBackItem.y + lBackItem.height < list.contentY + list.height;
						lProcessable = (lBackItem.y + lBackItem.height) < list.contentY && list.contentY - (lBackItem.y + lBackItem.height) >= (cacheBuffer * 0.7);
						if (!lProcessable) {
							lBackItem.forceVisibilityCheck(lVisible);
						}

						if (lProcessable) {
							// stop it
							lBackItem.unbindCommonControls();
							break;
						}
					}
				}

				// trace forward
				for (var lForwardIdx = lBeginIdx; lForwardIdx < list.count; lForwardIdx++) {
					//
					lForwardItem = list.itemAtIndex(lForwardIdx);
					if (lForwardItem) {
						lVisible = lForwardItem.y >= list.contentY && lForwardItem.y + lForwardItem.height < list.contentY + list.height;
						lProcessable = (lForwardItem.y + lForwardItem.height) > list.contentY + list.height && (lForwardItem.y + lForwardItem.height) - (list.contentY + list.height) >= (cacheBuffer * 0.7);
						if (!lProcessable) {
							lForwardItem.forceVisibilityCheck(lVisible);
						}

						if (lProcessable) {
							// stop it
							lForwardItem.unbindCommonControls();
							// we are done
							break;
						}
					}
				}
			}
		}

		onDragStarted: {
		}

		onDraggingVerticallyChanged: {
		}

		onDragEnded: {
			if (list.pullReady) {
				list.unbind();
				switchDataTimer.start();
			}
		}

		onFeedReady: {
			//
			modelLoader.feed();
		}

		delegate: ItemDelegate {
			id: itemDelegate

			property var buzzItem;
			property bool commonType: false;
			property var adjustValue: []

			ListView.onPooled: {
				unbindItem();
				unbindCommonControls();
			}

			ListView.onReused: {
				// replace inner instance
				if (buzzItem && ((commonType && (type === buzzerClient.tx_BUZZER_MISTRUST_TYPE() ||
												 type === buzzerClient.tx_BUZZER_ENDORSE_TYPE())) ||
								 !commonType && (type !== buzzerClient.tx_BUZZER_MISTRUST_TYPE() &&
												 type !== buzzerClient.tx_BUZZER_ENDORSE_TYPE()))) {
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
				if (buzzfeed_.mediaPlayerController &&
						(buzzfeed_.mediaPlayerController.isCurrentInstancePlaying() ||
										   buzzfeed_.mediaPlayerController.isCurrentInstancePaused())) {
					buzzfeed_.mediaPlayerController.showCurrentPlayer(null);
				}
			}

			function forceChildLink() {
				buzzItem.childLink_ = true;
			}

			function forceParentLink() {
				buzzItem.parentLink_ = true;
			}

			onClicked: {
				//
				if (!(type === buzzerClient.tx_BUZZER_ENDORSE_TYPE() || type === buzzerClient.tx_BUZZER_MISTRUST_TYPE())) {
					controller.openThread(buzzChainId, buzzId, buzzerAlias, buzzBodyFlat);
				}
			}

			onWidthChanged: {
				/*
				if (itemDelegate.buzzItem) {
					var lHeight = itemDelegate.buzzItem.calculateHeight();
					itemDelegate.buzzItem.width = list.width;
					itemDelegate.height = lHeight;
					itemDelegate.buzzItem.height = lHeight;
				}
				*/
				if (buzzItem) {
					buzzItem.width = list.width;
					itemDelegate.height = buzzItem.calculateHeight();
				}
			}

			Component.onCompleted: createItem()

			function createItem() {
				var lSource = "qrc:/qml/buzzitem.qml";
				if (type === buzzerClient.tx_BUZZER_ENDORSE_TYPE() ||
						type === buzzerClient.tx_BUZZER_MISTRUST_TYPE()) {
					lSource = "qrc:/qml/buzzendorseitem.qml";
					commonType = false;
				} else commonType = true;

				var lComponent = Qt.createComponent(lSource);
				buzzItem = lComponent.createObject(itemDelegate);

				bindItem();
			}

			function calculatedHeightModified(value) {
				/*
				itemDelegate.height = value;
				itemDelegate.buzzItem.height = value;
				itemDelegate.adjustValue.push(value);
				*/
				itemDelegate.height = value;
				buzzItem.height = value;
			}

			function bindItem() {
				//
				buzzItem.calculatedHeightModified.connect(itemDelegate.calculatedHeightModified);
				buzzItem.bindItem();
				//
				buzzItem.sharedMediaPlayer_ = buzzfeed_.mediaPlayerController;
				buzzItem.width = list.width;
				buzzItem.controller_ = buzzfeed_.controller;
				buzzItem.buzzfeedModel_ = buzzerClient.getBuzzfeedList();
				buzzItem.listView_ = list;

				itemDelegate.height = buzzItem.calculateHeight();
				itemDelegate.width = list.width;
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
	QuarkRoundSymbolButton {
		id: createBuzz
		x: parent.width - (width + 15)
		y: parent.height - (height + 15)
		width: 55
		height: width
		visible: true
		radius: width / 2
		enableShadow: true
		elevation: 10
		outerPercent: 3
		color: "transparent"

		enabled: true

		onClick: {
			//
			controller.openBuzzEditor();
		}
	}

	Image {
		id: buzzImage
		x: createBuzz.x
		y: createBuzz.y
		width: createBuzz.width
		height: createBuzz.height
		mipmap: true

		source: "../images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector,
				//buzzerApp.isDesktop ? "buzzer.round.full" : "buzzer.round")
				"buzzer.round.full")
		fillMode: Image.PreserveAspectFit
		property var radius: width / 2

		layer.enabled: true
		layer.effect: OpacityMask {
			maskSource: Item {
				width: 2 * buzzImage.radius
				height: 2 * buzzImage.radius

				Rectangle {
					anchors.centerIn: parent
					width: 2 * buzzImage.radius
					height: 2 * buzzImage.radius
					radius: buzzImage.radius
				}
			}
		}
	}

	QuarkBusyIndicator {
		anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter; }
		running: !modelLoader.requestProcessed
	}

	//
	BuzzItemMediaPlayer {
		id: buzzfeedPlayer
		x: 0
		y: (list.y + list.height) - height // 0
		width: parent.width
		mediaPlayerController: buzzfeed_.mediaPlayerController
		overlayParent: list

		onVisibleChanged: {
			if (visible) {
				createBuzz.y = (parent.height - (height + buzzerClient.scaleFactor*15)) - buzzfeedPlayer.height;
			} else {
				createBuzz.y = parent.height - (height + buzzerClient.scaleFactor*15);
			}
		}
	}
}
