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
	id: buzzfeedtag_
	key: "buzzfeedtag"

	property var infoDialog;
	property var controller;
	property bool listen: false;
	property var buzzesThread_;

	readonly property int spaceLeft_: 15
	readonly property int spaceTop_: 12
	readonly property int spaceRight_: 15
	readonly property int spaceBottom_: 12
	readonly property int spaceItems_: 5
	readonly property int toolbarTotalHeight_: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 50) : 45

	function start(tag) {
		//
		buzzesThread_ = buzzerClient.createTagBuzzfeedList(); // buzzfeed regietered here to receive buzzes updates (likes, rebuzzes)
		modelLoader.tag = tag;
		//
		switchDataTimer.start();
	}

	Component.onCompleted: {
		closePageHandler = closePage;
		activatePageHandler = activatePage;
	}

	function closePage() {
		stopPage();
		controller.popPage(buzzfeedtag_);
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
		if (buzzesThread_)
			buzzerClient.unregisterBuzzfeed(buzzesThread_); // we should explicitly unregister buzzfeed
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
			if (listen && buzzerClient.buzzerDAppReady) {
				modelLoader.start();
				listen = false;
			}
		}

		function onThemeChanged() {
			buzzesThread_.resetModel();
		}
	}

	BuzzerCommands.LoadBuzzfeedByTagCommand {
		id: modelLoader
		model: buzzesThread_

		property bool dataReceived: false
		property bool dataRequested: false

		onProcessed: {
			console.log("[buzzfeed]: loaded");
			dataReceived = true;
			dataRequested = false;
			waitDataTimer.done();
		}

		onError: {
			// code, message
			console.log("[buzzfeed/error]: " + code + " | " + message);
			dataReceived = false;
			dataRequested = false;
			waitDataTimer.done();
			controller.showError(message);
		}

		function start() {
			//
			if (!dataReceived && !dataRequested) {
				dataRequested = true;
				waitDataTimer.start();
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
	// toolbar
	//

	QuarkToolBar {
		id: buzzThreadToolBar
		height: toolbarTotalHeight_
		width: list.width

		property int totalHeight: height

		Component.onCompleted: {
		}

		QuarkToolButton	{
			id: cancelButton
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
			id: tagControl
			x: cancelButton.x + cancelButton.width + spaceItems_
			y: parent.height / 2 - height / 2
			width: parent.width - (x /*+ spaceRightMenu_*/)
			elide: Text.ElideRight
			text: modelLoader.tag
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
	}

	//
	// thread
	//

	QuarkListView {
		id: list
		x: 0
		y: buzzThreadToolBar.y + buzzThreadToolBar.height
		width: parent.width
		height: parent.height - (buzzThreadToolBar.y + buzzThreadToolBar.height)
		usePull: true
		clip: true

		model: buzzesThread_

		// TODO: consumes a lot RAM
		cacheBuffer: 10000
		displayMarginBeginning: 5000
		displayMarginEnd: 5000

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

		/*
		headerPositioning: ListView.PullBackHeader
		header: ItemDelegate {
		}
		*/

		delegate: ItemDelegate {
			//
			id: itemDelegate

			property var buzzItem;

			onClicked: {
				// open thread
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

				buzzItem.width = list.width;
				buzzItem.controller_ = buzzfeedtag_.controller;
				buzzItem.buzzfeedModel_ = buzzesThread_;
				buzzItem.listView_ = list;
				buzzItem.rootId_ = modelLoader.buzzId;

				itemDelegate.height = buzzItem.calculateHeight();
				itemDelegate.width = list.width;

				buzzItem.calculatedHeightModified.connect(itemDelegate.calculatedHeightModified);
			}

			function calculatedHeightModified(value) {
				itemDelegate.height = value;
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
				lPage.initializeBuzz(modelLoader.tag);

				addPage(lPage);
			}
		}
	}

	//
	// support
	//

	QuarkPopupMenu {
		id: headerMenu
		x: parent.width - width - spaceRight_
		y: toolbarTotalHeight_ + spaceItems_
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
				modelLoader.restart();
			} else {
				listen = true;
			}
		}
	}

	Timer {
		id: waitDataTimer
		interval: 1000
		repeat: false
		running: false

		onTriggered: {
			waitIndicator.running = true;
		}

		function done() {
			stop();
			waitIndicator.running = false;
		}
	}

	QuarkBusyIndicator {
		id: waitIndicator
		anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter; }
	}
}
