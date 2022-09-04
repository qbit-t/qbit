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
	readonly property real defaultFontSize: buzzerApp.defaultFontSize()

	property var infoDialog;
	property var controller;
	property var mediaPlayerController;
	property bool listen: false;

	property var buzzfeedModel_;
	property var modelLoader_;

	property var globalBuzzfeedModel_: buzzerClient.getGlobalBuzzfeedList();
	property var tagBuzzfedModel_: buzzerClient.createTagBuzzfeedList();
	property var buzzerBuzzfedModel_: buzzerClient.createBuzzerBuzzfeedList();

	function externalPull() {
		list.unbind();
		modelLoader_.restart();
	}

	function externalTop() {
		list.positionViewAtBeginning();
	}

	function start(force) {
		//
		if (!buzzerApp.isDesktop) {
			// search.setText("");
		} else {
			disconnect();
			controller.mainToolBar.searchTextEdited.connect(buzzfeed_.startSearch);
			controller.mainToolBar.searchTextCleared.connect(buzzfeed_.searchTextCleared);
			controller.mainToolBar.setSearchText("", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.global.search"));
		}

		//
		// TODO: switch off and rebind?
		//
		if (!list.model || force) {
			modelLoader_ = globalModelLoader;
			buzzfeedModel_ = globalBuzzfeedModel_;
			list.model = globalBuzzfeedModel_;
			switchDataTimer.start();
		} else {
			if (list.atTheTop()) switchDataTimer.start();
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
			list.unbind();
			buzzfeedModel_.clear();
		}
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
			list.reuseItems = true;
		}

		onError: {
			// code, message
			console.log("[globalfeed/error]: " + code + " | " + message);
			dataReceived = false;
			dataRequested = false;
			waitDataTimer.done();
			list.reuseItems = true;
			controller.showError(message);
		}

		function start() {
			//
			if (!dataReceived && !dataRequested) {
				list.reuseItems = false;
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
			list.reuseItems = true;
		}

		onError: {
			// code, message
			console.log("[globalfeed/tag/error]: " + code + " | " + message);
			dataReceived = false;
			dataRequested = false;
			waitDataTimer.done();
			list.reuseItems = true;
			controller.showError(message);
		}

		function start() {
			//
			if (!dataReceived && !dataRequested) {
				list.reuseItems = false;
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
			list.reuseItems = true;
		}

		onError: {
			// code, message
			console.log("[globalfeed/buzzer/error]: " + code + " | " + message);
			dataReceived = false;
			dataRequested = false;
			waitDataTimer.done();
			list.reuseItems = true;
			controller.showError(message);
		}

		function start() {
			//
			if (!dataReceived && !dataRequested) {
				list.reuseItems = false;
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
		width: buzzerApp.isDesktop ? parent.width - x - 5 : (parent.width - x + (Qt.platform.os === "ios" ? 8 : 0))
		placeHolder: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.global.search")
		fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 1)) : defaultFontPointSize
		visible: !buzzerApp.isDesktop

		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.statusBar")

		Material.theme: buzzerClient.statusBarTheme == "dark" ? Material.Dark : Material.Light;
		Material.accent: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
		Material.background: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
		Material.foreground: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
		Material.primary: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

		cancelSymbolColor: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		placeholderTextColor: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")

		x: 0 // 15
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
		start(true);
	}

	QuarkHLine {
		id: bottomLine
		x1: 0
		y1: search.y + search.height
		x2: parent.width
		y2: search.y + search.height
		penWidth: 1
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Panel.bottom.separator")
		visible: !buzzerApp.isDesktop
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
		cacheBuffer: 500
		reuseItems: true

		function adjust() {
			//
			for (var lIdx = 0; lIdx < list.count; lIdx++) {
				var lItem = list.itemAtIndex(lIdx);
				if (lItem) {
					lItem.width = list.width;
				}
			}
		}

		function atTheTop() {
			if (list.count > 0) {
				var lItem = list.itemAtIndex(0);
				if (lItem && lItem.y === contentY) return true;
				return false;
			}

			return true;
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
			modelLoader_.feed();
		}

		delegate: ItemDelegate {
			id: itemDelegate

			property var buzzItem;
			property bool commonType: false;

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
				buzzItem.buzzfeedModel_ = list.model;
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
	BuzzItemMediaPlayer {
		id: player
		x: 0
		y: (list.y + list.height) - height // buzzerApp.isDesktop ? 0 : search.y + search.calculatedHeight
		width: parent.width
		mediaPlayerController: buzzfeed_.mediaPlayerController
		overlayParent: list
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
