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
	id: buzzfeed_

	property var infoDialog;
	property var controller;
	property bool listen: false;

	property var buzzfeedModel_;
	property var modelLoader_;

	property var globalBuzzfeedModel_: buzzerClient.getGlobalBuzzfeedList();
	property var tagBuzzfedModel_: buzzerClient.createTagBuzzfeedList();
	property var buzzerBuzzfedModel_: buzzerClient.createBuzzerBuzzfeedList();

	function externalPull() {
		modelLoader_.restart();
	}

	function start() {
		if (!buzzerApp.isDesktop) search.setText("");
		else {
			controller.mainToolBar.searchTextEdited.connect(startSearch);
			controller.mainToolBar.searchTextCleared.connect(searchTextCleared);
			controller.mainToolBar.setSearchText("", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.global.search"));
		}

		modelLoader_ = globalModelLoader;
		buzzfeedModel_ = globalBuzzfeedModel_;
		list.model = globalBuzzfeedModel_;
		switchDataTimer.start();
	}

	function startWithTag(tag) {
		//
		stopFeed();
		//
		tagModelLoader.tag = tag;
		modelLoader_ = tagModelLoader;
		buzzfeedModel_ = tagBuzzfedModel_;
		list.model = tagBuzzfedModel_;
		switchDataTimer.start();
	}

	function startWithBuzzer(buzzer) {
		//
		stopFeed();
		//
		buzzerModelLoader.buzzer = buzzer;
		modelLoader_ = buzzerModelLoader;
		buzzfeedModel_ = buzzerBuzzfedModel_;
		list.model = buzzerBuzzfedModel_;
		switchDataTimer.start();
	}

	function stopFeed() {
		// unregister
		if (buzzfeedModel_ && buzzfeedModel_ !== buzzerClient.getGlobalBuzzfeedList()) {
			buzzfeedModel_.clear();
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
			if (listen && buzzerClient.buzzerDAppReady) {
				modelLoader_.start();
				listen = false;
			}
		}

		function onThemeChanged() {
			if (buzzfeedModel_) buzzfeedModel_.resetModel();
		}
	}

	BuzzerCommands.LoadBuzzesGlobalCommand {
		id: globalModelLoader
		model: globalBuzzfeedModel_

		property bool dataReceived: false
		property bool dataRequested: false

		onProcessed: {
			console.log("[globalfeed]: loaded");
			dataReceived = true;
			dataRequested = false;
			waitDataTimer.done();
		}

		onError: {
			// code, message
			console.log("[globalfeed/error]: " + code + " | " + message);
			dataReceived = false;
			dataRequested = false;
			waitDataTimer.done();
			controller.showError(message);
		}

		function start() {
			//
			if (!dataReceived && !dataRequested) {
				dataRequested = true;
				globalModelLoader.process(false);
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
				globalModelLoader.process(true);
			}
		}
	}

	BuzzerCommands.LoadBuzzfeedByTagCommand {
		id: tagModelLoader
		model: tagBuzzfedModel_

		property bool dataReceived: false
		property bool dataRequested: false

		onProcessed: {
			console.log("[globalfeed/tag]: loaded");
			dataReceived = true;
			dataRequested = false;
			waitDataTimer.done();
		}

		onError: {
			// code, message
			console.log("[globalfeed/tag/error]: " + code + " | " + message);
			dataReceived = false;
			dataRequested = false;
			waitDataTimer.done();
			controller.showError(message);
		}

		function start() {
			//
			if (!dataReceived && !dataRequested) {
				dataRequested = true;
				tagModelLoader.process(false);
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
				tagModelLoader.process(true);
			}
		}
	}

	BuzzerCommands.LoadBuzzfeedByBuzzerCommand {
		id: buzzerModelLoader
		model: buzzerBuzzfedModel_

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
			dataReceived = false;
			dataRequested = false;
			waitDataTimer.done();
			controller.showError(message);
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

	QuarkSearchField {
		id: search
		width: buzzerApp.isDesktop ? parent.width - x - 5 : parent.width - x - 14
		placeHolder: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.global.search")
		fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 12) : fontPointSize
		visible: !buzzerApp.isDesktop

		x: 15
		y: 0

		onSearchTextChanged: {
			//
			startSearch(searchText);
		}

		onTextCleared: {
			searchTextCleared();
		}
	}

	function startSearch(searchText) {
		if (searchText[0] === '#') {
			searchTags.process(searchText);
		} else if (searchText[0] === '@') {
			searchBuzzers.process(searchText);
		} else {
			start();
		}
	}

	function searchTextCleared() {
		start();
	}

	QuarkListView {
		id: list
		x: 0
		y: buzzerApp.isDesktop ? 0 : search.y + search.calculatedHeight
		width: parent.width
		height: parent.height - (buzzerApp.isDesktop ? 0 : search.calculatedHeight)
		usePull: true
		clip: true

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
				modelLoader_.restart();
			}
		}

		onFeedReady: {
			//
			modelLoader_.feed();
		}

		delegate: ItemDelegate {
			id: itemDelegate

			property var buzzItem;

			onWidthChanged: {
				if (buzzItem) {
					buzzItem.width = list.width;
					itemDelegate.height = buzzItem.calculateHeight();
				}
			}

			onClicked: {
				//
				controller.openThread(buzzChainId, buzzId, buzzerAlias, buzzBodyFlat);
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
				buzzItem.controller_ = buzzfeed_.controller;
				buzzItem.buzzfeedModel_ = list.model;
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

	Timer {
		id: switchDataTimer
		interval: 500
		repeat: false
		running: false

		onTriggered: {
			if (buzzerClient.buzzerDAppReady) {
				modelLoader_.restart();
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

	BusyIndicator {
		id: waitIndicator
		anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter; }
	}

	//
	// backend
	//

	QuarkPopupMenu {
		id: buzzersList
		width: 170
		visible: false

		model: ListModel { id: buzzersModel }

		property var matched;

		onClick: {
			//
			if (!buzzerApp.isDesktop) search.setText(key);
			else {
				controller.mainToolBar.setSearchText(
					key,
					buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.global.search"));
			}

			startWithBuzzer(key);
		}

		function popup(match, buzzers) {
			//
			if (buzzers.length === 0) return;
			if (buzzers.length === 1 && match === buzzers[0]) return;
			//
			matched = match;
			//
			buzzersModel.clear();

			for (var lIdx = 0; lIdx < buzzers.length; lIdx++) {
				buzzersModel.append({
					key: buzzers[lIdx],
					keySymbol: "",
					name: buzzers[lIdx]});
			}

			if (!buzzerApp.isDesktop) {
				x = search.x;
				y = search.y + search.calculatedHeight;
			} else {
				x = 10;
				y = -10;
			}

			open();
		}
	}

	QuarkPopupMenu {
		id: tagsList
		width: 170
		visible: false

		model: ListModel { id: tagsModel }

		property var matched;

		onClick: {
			//
			search.setText(key);
			startWithTag(key);
		}

		function popup(match, tags) {
			//
			if (tags.length === 0) return;
			if (tags.length === 1 && match === tags[0]) return;
			//
			matched = match;
			//
			tagsModel.clear();

			for (var lIdx = 0; lIdx < tags.length; lIdx++) {
				tagsModel.append({
					key: tags[lIdx],
					keySymbol: "",
					name: tags[lIdx]});
			}

			if (!buzzerApp.isDesktop) {
				x = search.x;
				y = search.y + search.calculatedHeight;
			} else {
				x = 10;
				y = -10;
			}

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

	BuzzerCommands.LoadHashTagsCommand {
		id: searchTags

		onProcessed: {
			// pattern, tags
			tagsList.popup(pattern, tags);
		}

		onError: {
			controller.showError(message);
		}
	}
}
