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

import app.buzzer.components 1.0 as BuzzerComponents
import app.buzzer.commands 1.0 as BuzzerCommands

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

Item
{
	id: conversationsfeed_

	property var infoDialog;
	property var controller;
	property var mediaPlayerController;

	property var conversationModel_: buzzerClient.getConversationsList();

	function externalPull() {
		conversationsModelLoader.restart();
	}

	function start() {
		restart();
		switchDataTimer.start();

		var lPlayerController = buzzerApp.sharedMediaPlayerController();
		if (lPlayerController && lPlayerController.isCurrentInstancePlaying()) {
			lPlayerController.showCurrentPlayer();
		}
	}

	function restart() {
		if (!buzzerApp.isDesktop) {
			search.prevText_ = "";
			search.setText("");
		} else {
			controller.mainToolBar.setSearchText("", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.global.search.add"));
		}
	}

	function disconnect() {
		if (buzzerApp.isDesktop) {
			console.log("[conversations/disconnect]: disconnecting");
			controller.mainToolBar.searchTextEdited.disconnect(conversationsfeed_.startSearch);
			controller.mainToolBar.searchTextCleared.disconnect(conversationsfeed_.searchTextCleared);
		}
	}

	function resetModel() {
		//
		if (buzzerApp.isDesktop){
			controller.mainToolBar.searchTextEdited.connect(conversationsfeed_.startSearch);
			controller.mainToolBar.searchTextCleared.connect(conversationsfeed_.searchTextCleared);
			controller.mainToolBar.setSearchText("", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.global.search.add"));
		}

		conversationModel_.resetModel();
	}

	Component.onCompleted: {
		//
		if (buzzerClient.buzzerDAppReady) {
			switchDataTimer.start();
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
				conversationsModelLoader.start();
			}
		}

		function onBuzzerDAppResumed() {
			if (buzzerClient.buzzerDAppReady) {
				conversationsModelLoader.processAndMerge(true); // preserve data
			}
		}

		function onThemeChanged() {
			if (conversationModel_) conversationModel_.resetModel();
		}

		function onBuzzerChanged() {
			if (buzzerClient.buzzerDAppReady) {
				conversationsModelLoader.restart();
			}
		}
	}

	BuzzerCommands.LoadConversationsCommand {
		id: conversationsModelLoader
		model: conversationModel_

		property bool dataReceived: false
		property bool dataRequested: false

		onProcessed: {
			console.log("[conversations]: loaded");
			dataReceived = true;
			dataRequested = false;
			waitDataTimer.done();
		}

		onError: {
			// code, message
			console.log("[conversations/error]: " + code + " | " + message);
			dataReceived = false;
			dataRequested = false;
			waitDataTimer.done();
			controller.showError(message);
		}

		function start() {
			//
			if (!dataReceived && !dataRequested) {
				dataRequested = true;
				conversationsModelLoader.process(false);
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
				conversationsModelLoader.process(true);
			}
		}
	}

	BuzzerCommands.CreateConversationCommand {
		id: createConversation

		onError: {
			// code, message
			handleError(code, message);
		}
	}

	QuarkSearchField {
		id: search
		width: parent.width - x //- 14
		placeHolder: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.global.search.add")
		visible: !buzzerApp.isDesktop

		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.statusBar");

		Material.theme: buzzerClient.statusBarTheme == "dark" ? Material.Dark : Material.Light;
		Material.accent: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
		Material.background: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
		Material.foreground: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
		Material.primary: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

		cancelSymbolColor: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		placeholderTextColor: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")

		x: 0 //15
		y: 0

		spacingLeft: 15
		spacingRight: 14

		property var prevText_: ""

		onSearchTextChanged: {
			//
			if (prevText_ !== searchText) {
				prevText_ = searchText;
				startSearch(searchText);
			}

			if (searchText === "")
				searchTextCleared();
		}

		onInnerTextChanged: {
			if (text === "") searchTextCleared();
		}

		onTextCleared: {
			searchTextCleared();
		}
	}

	QuarkHLine {
		id: bottomLine
		x1: 0
		y1: search.y + search.height
		x2: parent.width
		y2: search.y + search.height
		penWidth: 1
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Panel.bottom.separator")
		visible: true
	}

	function startSearch(searchText) {
		if (searchText[0] === '@') {
			// search buzzers on network
			searchBuzzers.process(searchText);
			// filter local feed
			buzzerClient.getConversationsList().setFilter(searchText);
		} else {
			buzzerClient.getConversationsList().resetFilter();
			start();
		}
	}

	function searchTextCleared() {
		buzzerClient.getConversationsList().resetFilter();
		//start();
	}

	QuarkListView {
		id: list
		x: 0
		y: buzzerApp.isDesktop ? 0 : search.y + search.calculatedHeight
		width: parent.width
		height: parent.height - (buzzerApp.isDesktop ? 0 : search.calculatedHeight)
		usePull: true
		clip: true
		model: conversationModel_

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
				conversationsModelLoader.restart();
			}
		}

		onFeedReady: {
			//
			conversationsModelLoader.feed();
		}

		delegate: ItemDelegate {
			id: itemDelegate

			property var conversationItem;

			onWidthChanged: {
				if (conversationItem) {
					conversationItem.width = list.width;
					itemDelegate.height = conversationItem.calculateHeight();
				}
			}

			onClicked: {
				//
				controller.openConversation(conversationId, self, conversationModel_);
			}

			Component.onCompleted: {
				var lSource = "qrc:/qml/conversationitem.qml";
				var lComponent = Qt.createComponent(lSource);
				if (lComponent.status === Component.Error) {
					controller.showError(lComponent.errorString());
				} else {
					conversationItem = lComponent.createObject(itemDelegate);

					conversationItem.width = list.width;
					conversationItem.controller_ = conversationsfeed_.controller;
					conversationItem.conversationsModel_ = conversationModel_; //list.model;
					conversationItem.listView_ = list;

					itemDelegate.height = conversationItem.calculateHeight();
					itemDelegate.width = list.width;

					conversationItem.initialize();

					conversationItem.calculatedHeightModified.connect(itemDelegate.calculatedHeightModified);
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
		mediaPlayerController: conversationsfeed_.mediaPlayerController
		overlayParent: list
	}

	Timer {
		id: switchDataTimer
		interval: 500
		repeat: false
		running: false

		onTriggered: {
			if (buzzerClient.buzzerDAppReady) {
				conversationsModelLoader.restart();
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

	//
	// backend
	//

	QuarkPopupMenu {
		id: buzzersList
		width: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 170) : 170
		visible: false

		model: ListModel { id: buzzersModel }

		property var matched;

		onClick: {
			//
			conversationsfeed_.restart();
			conversationModel_.resetFilter();

			//
			var lId = conversationModel_.locateConversation(key);
			// conversation found
			if (lId !== "") {
				//
				var lConversation = buzzerClient.locateConversation(lId);
				if (lConversation) {
					controller.openConversation(lId, lConversation, buzzerClient.getConversationsList());
				}
			} else {
				// try to create conversation
				createConversation.process(key);
			}
		}

		function popup(match, buzzers) {
			//
			if (buzzers.opened) buzzers.close();

			//
			if (buzzers.length === 0) return;
			if (buzzers.length === 1 && match === buzzers[0]) return;
			//
			matched = match;
			//
			buzzersModel.clear();

			for (var lIdx = 0; lIdx < buzzers.length; lIdx++) {
				if (buzzers[lIdx] !== buzzerClient.name)
					buzzersModel.append({
						key: buzzers[lIdx],
						keySymbol: "",
						name: buzzers[lIdx]});
			}

			x = (search.x + search.width) - width;
			y = !buzzerApp.isDesktop ? (search.y + search.calculatedHeight) : 0;

			open();
		}
	}

	BuzzerCommands.SearchEntityNamesCommand {
		id: searchBuzzers

		onProcessed: {
			// pattern, entities
			buzzersList.popup(pattern, entities);
		}

		onError: {
			controller.showError(message);
		}
	}

	function handleError(code, message) {
		if (code === "E_CHAINS_ABSENT") return;
		if (message === "UNKNOWN_REFTX" || code === "E_TX_NOT_SENT") {
			//buzzerClient.resync();
			controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"));
		} else if (code === "E_SENT_TX") {
			controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_SENT_TX"));
		} else {
			controller.showError(message);
		}
	}
}
