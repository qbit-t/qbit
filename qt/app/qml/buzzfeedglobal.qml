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

	//
	readonly property int spaceLeft_: 15
	readonly property int spaceTop_: 12
	readonly property int spaceRight_: 15
	readonly property int spaceBottom_: 12
	readonly property int spaceAvatarBuzz_: 10
	readonly property int spaceItems_: 5
	readonly property int spaceMedia_: 10
	readonly property int spaceSingleMedia_: 8
	readonly property int spaceMediaIndicator_: 15
	readonly property int spaceHeader_: 7
	readonly property int spaceRightMenu_: 15
	readonly property int spaceStats_: -5
	readonly property int spaceLine_: 4
	readonly property int spaceThreaded_: 33
	readonly property int spaceThreadedItems_: 4
	readonly property real defaultFontSize: 11

	property var infoDialog;
	property var controller;
	property var mediaPlayerControler;
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
		//
		if (!buzzerApp.isDesktop) search.setText("");
		else {
			disconnect();
			controller.mainToolBar.searchTextEdited.connect(buzzfeed_.startSearch);
			controller.mainToolBar.searchTextCleared.connect(buzzfeed_.searchTextCleared);
			controller.mainToolBar.setSearchText("", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.global.search"));
		}

		if (!list.model) {
			modelLoader_ = globalModelLoader;
			buzzfeedModel_ = globalBuzzfeedModel_;
			list.model = globalBuzzfeedModel_;
			switchDataTimer.start();
		}
	}

	function disconnect() {
		if (buzzerApp.isDesktop) {
			controller.mainToolBar.searchTextEdited.disconnect(buzzfeed_.startSearch);
			controller.mainToolBar.searchTextCleared.disconnect(buzzfeed_.searchTextCleared);
		}
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

	Connections {
		target: buzzerClient

		function onBuzzerDAppReadyChanged() {
			if (listen && buzzerClient.buzzerDAppReady) {
				modelLoader_.start();
				listen = false;
			}
		}

		function onThemeChanged() {
			//
			player.terminate();
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

	function startSearch(searchText) {
		if (searchText[0] === '#') {
			buzzersList.close();
			searchTags.process(searchText);
		} else if (searchText[0] === '@') {
			tagsList.close();
			searchBuzzers.process(searchText);
		} else {
			start();
		}
	}

	function searchTextCleared() {
		start();
	}

	//
	QuarkListView {
		id: list
		x: 0
		y: buzzerApp.isDesktop ? 0 : search.y + search.calculatedHeight
		width: parent.width
		height: parent.height - (buzzerApp.isDesktop ? 0 : search.calculatedHeight)
		usePull: true
		clip: true

		// TODO: consumes a lot RAM
		cacheBuffer: 10000
		displayMarginBeginning: 5000
		displayMarginEnd: 5000

		//property real localVelocity: maximumFlickVelocity
		//maximumFlickVelocity: 3000

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
			//console.log("maximumFlickVelocity = " + maximumFlickVelocity);
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

			onWidthChanged: {
				if (buzzItem) {
					buzzItem.width = list.width;
					itemDelegate.height = buzzItem.calculateHeight();
				}
			}

			onClicked: {
				//
				if (!(type === buzzerClient.tx_BUZZER_ENDORSE_TYPE() || type === buzzerClient.tx_BUZZER_MISTRUST_TYPE())) {
					controller.openThread(buzzChainId, buzzId, buzzerAlias, buzzBodyFlat);
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

	//
	BuzzItemMediaPlayer {
		id: player
		x: 0
		y: buzzerApp.isDesktop ? 0 : search.y + search.calculatedHeight
		width: parent.width
		mediaPlayerControler: buzzfeed_.mediaPlayerControler
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
			if (!buzzerApp.isDesktop) {
				search.prevText_ = key;
				search.setText(key);
			} else {
				controller.mainToolBar.setSearchText(
					key,
					buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.global.search"));
			}

			startWithBuzzer(key);
		}

		function popup(match, buzzers) {
			//
			if (buzzersList.opened) buzzersList.close();

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
		width: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 170) : 170
		visible: false

		model: ListModel { id: tagsModel }

		property var matched;

		onClick: {
			//
			if (!buzzerApp.isDesktop) {
				search.prevText_ = key;
				search.setText(key);
			} else {
				controller.mainToolBar.setSearchText(
					key,
					buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.global.search"));
			}

			startWithTag(key);
		}

		function popup(match, tags) {
			//
			if (tagsList.opened) tagsList.close();

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
