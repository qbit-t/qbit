import QtQuick 2.9
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
	property var mediaPlayerControler;

	function externalPull() {
		//modelLoader.restart();
		switchDataTimer.start();
	}

	Component.onCompleted: {
		//
		if (buzzerClient.buzzerDAppReady) {
			modelLoader.start();
		}
	}

	onWidthChanged: {
		orientationChangedTimer.start();
	}

	onMediaPlayerControlerChanged: {
		//
		buzzerApp.sharedMediaPlayerController(mediaPlayerControler);
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
			if (mediaPlayerControler) mediaPlayerControler.popVideoInstance();
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

		onProcessed: {
			console.log("[buzzfeed]: loaded");
			dataReceived = true;
			dataRequested = false;
			requestProcessed = true;
		}

		onError: {
			// code, message
			console.log("[buzzfeed/error]: " + code + " | " + message);
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

	//
	QuarkListView {
		id: list
		x: 0
		y: 0
		width: parent.width
		height: parent.height
		usePull: true
		clip: true

		model: buzzerClient.getBuzzfeedList()

		// TODO: consumes a lot RAM
		//cacheBuffer: 10000
		displayMarginBeginning: 500
		displayMarginEnd: 500

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
				//modelLoader.restart();
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

			property bool isFullyVisible: itemDelegate.y >= list.contentY && itemDelegate.y + height < list.contentY + list.height

			onIsFullyVisibleChanged: {
				if (itemDelegate !== null && itemDelegate.buzzItem !== null && itemDelegate.buzzItem !== undefined) {
					try {
						itemDelegate.buzzItem.forceVisibilityCheck(itemDelegate.isFullyVisible);
					} catch (err) {
						console.log("[onIsFullyVisibleChanged]: " + err + ", itemDelegate.buzzItem = " + itemDelegate.buzzItem);
					}
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
				if (buzzItem) {
					buzzItem.width = list.width;
					itemDelegate.height = buzzItem.calculateHeight();
				}
			}

			Component.onCompleted: {
				var lSource = "qrc:/qml/buzzitem.qml";
				if (type === buzzerClient.tx_BUZZER_ENDORSE_TYPE() ||
						type === buzzerClient.tx_BUZZER_MISTRUST_TYPE()) {
					lSource = "qrc:/qml/buzzendorseitem.qml";
				}

				var lComponent = Qt.createComponent(lSource);
				buzzItem = lComponent.createObject(itemDelegate);

				buzzItem.sharedMediaPlayer_ = buzzfeed_.mediaPlayerControler;
				buzzItem.width = list.width;
				buzzItem.controller_ = buzzfeed_.controller;
				buzzItem.buzzfeedModel_ = buzzerClient.getBuzzfeedList();
				buzzItem.listView_ = list;

				itemDelegate.height = buzzItem.calculateHeight();
				itemDelegate.width = list.width;

				buzzItem.calculatedHeightModified.connect(itemDelegate.calculatedHeightModified);
			}

			function calculatedHeightModified(value) {
				itemDelegate.height = value;
			}
		}
	}

	//
	BuzzItemMediaPlayer {
		id: buzzfeedPlayer
		x: 0
		y: 0
		width: parent.width
		mediaPlayerControler: buzzfeed_.mediaPlayerControler
	}

	QuarkToolButton {
		id: createBuzz
		x: parent.width - (width + 15)
		y: parent.height - (height + 15)
		width: 65
		height: width
		visible: true
		//symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		//Layout.alignment: Qt.AlignHCenter
		radius: (width + 30) / 2
		//clip: true

		enabled: true

		onClicked: {
			//
			var lComponent = null;
			var lPage = null;

			lComponent = buzzerApp.isDesktop ? Qt.createComponent("qrc:/qml/buzzeditor-desktop.qml") :
											   Qt.createComponent("qrc:/qml/buzzeditor.qml");
			if (lComponent.status === Component.Error) {
				showError(lComponent.errorString());
			} else {
				lPage = lComponent.createObject(controller);
				lPage.controller = controller;

				addPage(lPage);
			}
		}
	}

	BuzzerComponents.ImageQx {
		id: buzzImage
		x: createBuzz.x + 5
		y: createBuzz.y + 5
		width: createBuzz.width - 10
		height: createBuzz.height - 10
		mipmap: true

		source: "qrc://images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector,
				//buzzerApp.isDesktop ? "buzzer.round.full" : "buzzer.round")
				"buzzer.round.full")
		fillMode: BuzzerComponents.ImageQx.PreserveAspectFit
		radius: width / 2
	}

	QuarkBusyIndicator {
		anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter; }
		running: !modelLoader.requestProcessed
	}
}
