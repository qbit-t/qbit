import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import Qt.labs.qmlmodels 1.0
import QtQuick.Dialogs 1.1
import QtGraphicalEffects 1.0
import Qt.labs.folderlistmodel 2.11
import QtMultimedia 5.8
import QtQuick.Window 2.15

import app.buzzer.components 1.0 as BuzzerComponents
import app.buzzer.commands 1.0 as BuzzerCommands

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

import "qrc:/lib/numberFunctions.js" as NumberFunctions

QuarkPage {
	id: buzzfeedbuzzer_
	key: "buzzfeedbuzzer"

	property var infoDialog;
	property var controller;
	property bool listen: false;
	property var buzzfeedModel_;
	property var mediaPlayerController: buzzerApp.sharedMediaPlayerController()

	readonly property int spaceLeft_: 15
	readonly property int spaceTop_: 12
	readonly property int spaceRight_: 15
	readonly property int spaceBottom_: 12
	readonly property int spaceItems_: 5

	function start(buzzer) {
		//
		buzzfeedModel_ = buzzerClient.createBuzzerBuzzfeedList(); // buzzfeed regietered here to receive buzzes updates (likes, rebuzzes)
		//
		buzzerModelLoader.buzzer = buzzer;
		list.model = buzzfeedModel_;
		list.headerItem.initialize(buzzer);

		switchDataTimer.start();
	}

	Component.onCompleted: {
		closePageHandler = closePage;
		activatePageHandler = activatePage;
	}

	function closePage() {
		stopPage();
		controller.popPage(buzzfeedbuzzer_);
		destroy(1000);
	}

	function activatePage() {
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
	}

	function onErrorCallback(error)	{
		controller.showError(error);
	}

	function stopPage() {
		// unregister
		if (buzzfeedModel_)
			buzzerClient.unregisterBuzzfeed(buzzfeedModel_); // we should explicitly unregister buzzfeed
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
			buzzerThreadToolBar.adjust();
			list.adjust();
		}
	}

	Connections {
		target: buzzerClient

		function onBuzzerDAppReadyChanged() {
			if (listen && buzzerClient.buzzerDAppReady) {
				buzzerModelLoader.start();
				listen = false;
			}
		}

		function onBuzzerDAppResumed() {
			if (buzzerClient.buzzerDAppReady) {
				buzzerModelLoader.processAndMerge();
			}
		}

		function onThemeChanged() {
			if (buzzfeedModel_) buzzfeedModel_.resetModel();
		}
	}

	BuzzerCommands.LoadBuzzfeedByBuzzerCommand {
		id: buzzerModelLoader
		model: buzzfeedModel_

		property bool dataReceived: false
		property bool dataRequested: false

		onProcessed: {
			console.log("[globalfeed/buzzer]: loaded");
			dataReceived = true;
			dataRequested = false;
			waitDataTimer.done();
		}

		onError: {
			// code, message
			console.log("[globalfeed/buzzer/error]: " + code + " | " + message);
			waitDataTimer.done();

			if (code !== "E_LOAD_BUZZFEED_BUZZER_TIMESTAMP") {
				//
				dataReceived = false;
				dataRequested = false;
				controller.showError(message);
			} else {
				dataReceived = true;
				dataRequested = false;
			}
		}

		function start() {
			//
			if (!dataReceived && !dataRequested) {
				dataRequested = true;
				buzzerModelLoader.process(false);
				waitDataTimer.start();
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
				buzzerModelLoader.process(true);
			}
		}
	}

	//
	// toolbar
	//

	QuarkToolBar {
		id: buzzerThreadToolBar
		height: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 50) : 45
		width: parent.width
		//backgroundColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background")

		property int totalHeight: height

		function adjust() {
			if (buzzerApp.isDesktop) return;

			if (parent.width > parent.height) {
				visible = false;
				height = 0;
			} else {
				visible = true;
				height = 45;
			}
		}

		Component.onCompleted: {
		}

		QuarkToolButton	{
			id: cancelButton
			y: parent.height / 2 - height / 2
			Material.background: "transparent"
			visible: true
			labelYOffset: buzzerApp.isDesktop ? 0 : 3
			symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.leftArrowSym
			symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : symbolFontPointSize

			onClicked: {
				closePage();
			}
		}

		QuarkLabelRegular {
			id: buzzerControl
			x: cancelButton.x + cancelButton.width + spaceItems_
			y: parent.height / 2 - height / 2
			width: parent.width - (x /*+ spaceRightMenu_*/)
			elide: Text.ElideRight
			text: buzzerModelLoader.buzzer
			font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : 18
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link")
		}

		QuarkToolButton {
			id: menuControl
			x: parent.width - width //- spaceItems_
			y: parent.height / 2 - height / 2
			Material.background: "transparent"
			symbol: Fonts.elipsisVerticalSym
			visible: buzzerApp.isDesktop
			labelYOffset: buzzerApp.isDesktop ? 0 : 3
			symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
			Layout.alignment: Qt.AlignHCenter
			symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : symbolFontPointSize

			onClicked: {
				if (headerMenu.visible) headerMenu.close();
				else { headerMenu.prepare(); headerMenu.open(); }
			}
		}

		QuarkHLine {
			id: bottomLine
			x1: 0
			y1: parent.height
			x2: parent.width
			y2: parent.height
			penWidth: 1
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
			visible: true
		}

		ProgressBar {
			id: waitIndicator

			x: 0
			y: parent.height - height
			width: parent.width
			visible: false
			value: 0.0
			indeterminate: true

			Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
			Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
			Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
			Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
			Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");
		}
	}

	//
	// thread
	//

	QuarkListView {
		id: list
		x: 0
		y: buzzerThreadToolBar.y + buzzerThreadToolBar.height
		width: parent.width
		height: parent.height - (buzzerThreadToolBar.y + buzzerThreadToolBar.height)
		usePull: true
		clip: true

		//cacheBuffer: 10000
		displayMarginBeginning: 1000
		displayMarginEnd: 1000

		function adjust() {
			//
			for (var lIdx = 0; lIdx < list.count; lIdx++) {
				var lItem = list.itemAtIndex(lIdx);
				if (lItem) {
					lItem.width = list.width;
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
						lProcessable = (lBackItem.y + lBackItem.height) < list.contentY && list.contentY - (lBackItem.y + lBackItem.height) > displayMarginBeginning;
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
						lProcessable = (lForwardItem.y + lForwardItem.height) > list.contentY + list.height && (lForwardItem.y + lForwardItem.height) - (list.contentY + list.height) > displayMarginEnd;
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
				buzzerModelLoader.restart();
			}
		}

		onFeedReady: {
			//
			buzzerModelLoader.feed();
		}

		header: BuzzerItem {
			id: buzzerItem
			x: 0
			y: 0
			width: parent.width
			height: buzzerItem.calculatedHeight
			controller_: controller
		}

		headerPositioning: ListView.InlineHeader

		delegate: ItemDelegate {
			id: itemDelegate

			property var buzzItem;

			onClicked: {
				//
				controller.openThread(buzzChainId, buzzId, buzzerAlias, buzzBodyFlat);
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

				buzzItem.sharedMediaPlayer_ = buzzfeedbuzzer_.mediaPlayerController;
				buzzItem.width = list.width;
				buzzItem.controller_ = buzzfeedbuzzer_.controller;
				buzzItem.buzzfeedModel_ = buzzfeedModel_;
				buzzItem.listView_ = list;

				itemDelegate.height = buzzItem.calculateHeight();
				itemDelegate.width = list.width;

				buzzItem.calculatedHeightModified.connect(itemDelegate.calculatedHeightModified);
			}

			function calculatedHeightModified(value) {
				itemDelegate.height = value;
			}

			function forceVisibilityCheck(check) {
				if (buzzItem) {
					buzzItem.forceVisibilityCheck(check);
				}
			}

			function unbindCommonControls() {
				if (buzzItem) {
					buzzItem.unbindCommonControls();
				}
			}
		}
	}

	QuarkToolButton {
		id: createBuzz
		x: parent.width - (width + 15)
		y: parent.height - (height + 15)
		width: 55
		height: width
		visible: true
		Layout.alignment: Qt.AlignHCenter
		radius: width / 2

		enabled: true

		Image {
			id: buzzImage
			anchors.fill: parent
			source: "../images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector,
					buzzerApp.isDesktop ? "buzzer.round.full" : "buzzer.round")
			fillMode: Image.PreserveAspectFit
		}

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
				lPage.initializeBuzz(buzzerModelLoader.buzzer);

				addPage(lPage);
			}
		}

		//
		BuzzItemMediaPlayer {
			id: player
			x: 0
			y: (list.y + list.height) - height // bottomLine.y1 + 1
			width: parent.width
			mediaPlayerController: buzzfeedbuzzer_.mediaPlayerController
			overlayParent: list

			onVisibleChanged: {
				if (visible) {
					createBuzz.y = (parent.height - (height + 15)) - player.height;
				} else {
					createBuzz.y = parent.height - (height + 15);
				}
			}
		}
	}

	//
	// support
	//

	QuarkPopupMenu {
		id: headerMenu
		x: parent.width - width - spaceRight_
		y: menuControl.y + menuControl.height + spaceItems_
		width: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 350) : 350
		visible: false

		model: ListModel { id: menuModel }

		Component.onCompleted: prepare()

		onClick: {
			// key, activate
			controller.activatePage(key);
		}

		function prepare() {
			//
			menuModel.clear();

			//
			var lArray = controller.enumStakedPages();
			for (var lI = 0; lI < lArray.length; lI++) {
				//
				menuModel.append({
					key: lArray[lI].key,
					keySymbol: "",
					name: lArray[lI].alias + " // " + lArray[lI].caption.substring(0, 100)});
			}
		}
	}

	Timer {
		id: switchDataTimer
		interval: 500
		repeat: false
		running: false

		onTriggered: {
			if (buzzerClient.buzzerDAppReady) {
				buzzerModelLoader.restart();
			} else {
				listen = true;
			}
		}
	}

	Timer {
		id: waitDataTimer
		interval: 100
		repeat: false
		running: false

		onTriggered: {
			waitIndicator.visible = true;
		}

		function done() {
			stop();
			waitIndicator.visible = false;
		}
	}
}
