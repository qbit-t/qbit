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
	id: buzzfeedthread_
	key: "buzzfeedthread"

	property var infoDialog;
	property var controller;
	property bool listen: false;
	property var buzzesThread_;
	property var mediaPlayerController: buzzerApp.sharedMediaPlayerController()

	readonly property int spaceLeft_: 15
	readonly property int spaceTop_: 12
	readonly property int spaceRight_: 15
	readonly property int spaceBottom_: 12
	readonly property int spaceItems_: 5

	property bool sending: false
	property bool atTheBottom: false

	// adjust height by virtual keyboard
	followKeyboard: true

	Connections {
		target: buzzesThread_

		function onCountChanged() {
			if (atTheBottom) {
				list.positionViewAtEnd();
			}
		}
	}

	function start(chain, buzzId) {
		//
		buzzesThread_ = buzzerClient.createBuzzesBuzzfeedList(); // buzzfeed regietered here to receive buzzes updates (likes, rebuzzes)
		//
		modelLoader.buzzId = buzzId;
		buzzSubscribeCommand.chain = chain;
		buzzSubscribeCommand.buzzId = buzzId;

		console.log("[buzzfeedthread/start]: chain = " + chain + ", buzzId = " + buzzId);

		switchDataTimer.start();
	}

	Component.onCompleted: {
		closePageHandler = closePage;
		activatePageHandler = activatePage;
	}

	function closePage() {
		//
		if (mediaPlayerController && mediaPlayerController.isCurrentInstancePlaying()) {
			mediaPlayerController.disableContinousPlayback();
			mediaPlayerController.popVideoInstance();
			mediaPlayerController.showCurrentPlayer();
		}

		//
		stopPage();
		controller.popPage();
		destroy(1000);
	}

	function activatePage() {
		buzzText.external_ = false;
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Window.background"));
	}

	function onErrorCallback(error)	{
		controller.showError(error);
	}

	function stopPage() {
		// unsubscribe
		buzzUnsubscribeCommand.process();
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
				listen = false;
				modelLoader.start();
			}
		}

		function onBuzzerDAppResumed() {
			if (listen && buzzerClient.buzzerDAppReady) {
				listen = false;
				modelLoader.start();
			} else if (buzzerClient.buzzerDAppReady) {
				modelLoader.processAndMerge(true);
				buzzSubscribeCommand.process();
			}
		}
	}

	BuzzerCommands.BuzzSubscribeCommand {
		id: buzzSubscribeCommand
		model: buzzesThread_

		onProcessed: {
			//
			console.log("[buzzfeedthread/buzzSubscribeCommand]: processed");
			var lPeers = buzzSubscribeCommand.peers();
			for (var lIdx = 0; lIdx < lPeers.length; lIdx++) {
				console.log(lPeers[lIdx]);
			}
		}
	}

	BuzzerCommands.BuzzUnsubscribeCommand {
		id: buzzUnsubscribeCommand
		subscribeCommand: buzzSubscribeCommand
	}

	BuzzerCommands.LoadBuzzfeedByBuzzCommand {
		id: modelLoader
		model: buzzesThread_

		property bool dataReceived: false
		property bool dataRequested: false

		onProcessed: {
			console.log("[buzzfeedthread]: loaded");
			dataReceived = true;
			dataRequested = false;
			waitDataTimer.done();
		}

		onError: {
			// code, message
			console.log("[buzzfeedthread/error]: " + code + " | " + message);
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
		height: 45
		width: parent.width

		property int totalHeight: height

		Component.onCompleted: {
		}

		QuarkToolButton	{
			id: cancelButton
			Material.background: "transparent"
			visible: true
			labelYOffset: 3
			symbolColor: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.leftArrowSym

			onClicked: {
				closePage();
			}
		}

		QuarkHLine {
			id: bottomLine
			x1: 0
			y1: parent.height
			x2: parent.width
			y2: parent.height
			penWidth: 1
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Panel.bottom.separator") // Material.disabledHidden
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
		height: parent.height - (buzzThreadToolBar.y + buzzThreadToolBar.height + replyContainer.height - 1)
		//contentHeight: 1000
		usePull: true
		clip: true

		model: buzzesThread_

		// TODO: consumes a lot RAM
		cacheBuffer: 500
		//displayMarginBeginning: 1000
		//displayMarginEnd: 1000

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

		property int lastItemCount: -1

		onContentYChanged: {
			//
			if (lastItemCount == -1) lastItemCount = list.count;
			if (lastItemCount == list.count) {

				var lBottomItem = list.itemAtIndex(list.count - 1);
				if (lBottomItem !== null) {
					atTheBottom = lBottomItem.isFullyVisible;
				}
			}

			lastItemCount = list.count;

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
				modelLoader.restart();
			}
		}

		onFeedReady: {
			//
			modelLoader.feed();
		}

		DelegateChooser {
			id: itemChooser
			role: "index"
			DelegateChoice {
				index: 0
				Item {
					id: headDelegate

					property var buzzItem;

					property bool isFullyVisible: y >= list.contentY && y + height < list.contentY + list.height

					onWidthChanged: {
						if (buzzItem) {
							buzzItem.width = list.width;
							headDelegate.height = buzzItem.calculateHeight();
						}
					}

					onHeightChanged: {
						player.y = list.y + height + 1;
					}

					Component.onCompleted: {
						var lSource = "qrc:/qml/buzzitemhead.qml";
						var lComponent = Qt.createComponent(lSource);
						buzzItem = lComponent.createObject(headDelegate);

						buzzItem.sharedMediaPlayer_ = buzzfeedthread_.mediaPlayerController;
						buzzItem.width = list.width;
						buzzItem.controller_ = buzzfeedthread_.controller;
						buzzItem.buzzfeedModel_ = buzzesThread_;
						buzzItem.listView_ = list;
						buzzItem.rootId_ = modelLoader.buzzId;

						headDelegate.height = buzzItem.calculateHeight();
						headDelegate.width = list.width;

						buzzItem.calculatedHeightModified.connect(headDelegate.calculatedHeightModified);
					}

					function calculatedHeightModified(value) {
						headDelegate.height = value;
						if (atTheBottom)
							toTheTimer.start();
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

			DelegateChoice {
				ItemDelegate {
					id: itemDelegate

					property var buzzItem;

					property bool isFullyVisible: y >= list.contentY && y + height < list.contentY + list.height
					property bool isCompleted: false
					property bool isVisible: isCompleted && itemDelegate.y >= list.contentY && itemDelegate.y + height < list.contentY + list.height

					onIsVisibleChanged: {
						if (isCompleted && itemDelegate !== null && itemDelegate.buzzItem !== null && itemDelegate.buzzItem !== undefined) {
							try {
								itemDelegate.buzzItem.forceVisibilityCheck(isVisible);
							} catch (err) {
								console.log("[onIsVisibleChanged]: " + err + ", itemDelegate.buzzItem = " + itemDelegate.buzzItem);
							}
						}
					}

					onClicked: {
						//
						if (index === 0) return;

						// open thread
						var lComponent = null;
						var lPage = null;

						lComponent = Qt.createComponent("qrc:/qml/buzzfeedthread.qml");
						if (lComponent.status === Component.Error) {
							showError(lComponent.errorString());
						} else {
							lPage = lComponent.createObject(controller);
							lPage.controller = controller;
							addPage(lPage);

							lPage.start(buzzChainId, buzzId);
						}
					}

					onWidthChanged: {
						if (buzzItem) {
							buzzItem.width = list.width;
							itemDelegate.height = buzzItem.calculateHeight();
						}
					}

					Component.onDestruction: {
						isCompleted = false;
					}

					Component.onCompleted: {
						var lSource = "qrc:/qml/buzzitem.qml";
						if (type === buzzerClient.tx_BUZZER_ENDORSE_TYPE() ||
								type === buzzerClient.tx_BUZZER_MISTRUST_TYPE()) {
							lSource = "qrc:/qml/buzzendorseitem.qml";
						}

						var lComponent = Qt.createComponent(lSource);
						buzzItem = lComponent.createObject(itemDelegate);

						buzzItem.sharedMediaPlayer_ = buzzfeedthread_.mediaPlayerController;
						buzzItem.width = list.width;
						buzzItem.controller_ = buzzfeedthread_.controller;
						buzzItem.buzzfeedModel_ = buzzesThread_;
						buzzItem.listView_ = list;
						buzzItem.rootId_ = modelLoader.buzzId;

						itemDelegate.height = buzzItem.calculateHeight();
						itemDelegate.width = list.width;

						buzzItem.calculatedHeightModified.connect(itemDelegate.calculatedHeightModified);

						isCompleted = true;
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
		}

		delegate: itemChooser
	}

	//
	BuzzItemMediaPlayer {
		id: player
		x: 0
		y: (list.y + list.height) - height // bottomLine.y1 + 1
		width: parent.width
		mediaPlayerController: buzzfeedthread_.mediaPlayerController
		overlayParent: list
	}

	//
	// reply
	//

	Rectangle {
		id: replyContainer
		x: 0
		y: parent.height - (height) //buzzThreadToolBar.height +
		width: parent.width
		height: buzzText.contentHeight + spaceTop_ + spaceBottom_ + 2 * spaceItems_
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Market.tabBackground") // Page.background

		QuarkHLine {
			id: topLine
			x1: 0
			y1: 0
			x2: parent.width
			y2: 0
			penWidth: 1
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Panel.top.separator") // Material.disabledHidden
			visible: true
		}

		QuarkToolButton {
			id: richEditorButton
			Material.background: "transparent"
			visible: true
			labelYOffset: 2
			labelXOffset: -3
			symbolColor: !sending ?
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.richEditSym

			x: 0
			y: parent.height - (height + spaceItems_ - 2)

			onClicked: {
				//
				if (!sending && buzzesThread_.count > 0) {
					//
					var lComponent = null;
					var lPage = null;
					//
					buzzText.external_ = true;

					lComponent = Qt.createComponent("qrc:/qml/buzzeditor.qml");
					if (lComponent.status === Component.Error) {
						showError(lComponent.errorString());
					} else {
						lPage = lComponent.createObject(controller);
						lPage.controller = controller;
						lPage.initializeReply(buzzesThread_.self(0), buzzesThread_, buzzerClient.getPlainText(buzzText.textDocument));
						buzzText.clear();
						addPage(lPage);
					}
				}
			}
		}

		Rectangle {
			id: replyEditorContainer
			x: richEditorButton.x + richEditorButton.width
			y: spaceTop_
			height: buzzText.contentHeight + 2 * spaceItems_
			width: parent.width - (sendButton.width + richEditorButton.width + hiddenCountFrame.width + spaceItems_ + spaceRight_)
			//radius: 8
			border.color: "transparent" // buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
			color: "transparent"

			Rectangle {
				id: quasiCursor

				x: 2
				y: 3

				width: 1
				height: parent.height - 6

				color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")

				opacity: 1.0

				property bool toggleBlink: true

				NumberAnimation on opacity {
					id: blinkOutAnimation
					from: 1.0
					to: 0.0
					duration: 200
					running: quasiCursor.toggleBlink
				}

				NumberAnimation on opacity {
					id: blinkInAnimation
					from: 0.0
					to: 1.0
					duration: 200
					running: !quasiCursor.toggleBlink
				}

				Timer {
					id: blinkTimer
					interval: 500
					repeat: parent.visible
					running: parent.visible

					onTriggered: {
						quasiCursor.toggleBlink = !quasiCursor.toggleBlink
					}
				}
			}

			QuarkTextEdit {
				id: buzzText
				x: spaceItems_
				y: spaceItems_
				//height: 1000 //parent.height - spaceItems_
				width: parent.width - spaceItems_
				wrapMode: Text.Wrap
				textFormat: Text.RichText

				QuarkLabelRegular {
					id: placeHolder

					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
					anchors.fill: parent

					elide: Text.ElideRight

					text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.editor.message")
					color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.textDisabled")
				}

				//focus: true
				color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")

				property bool external_: false

				onActiveFocusChanged: {
					quasiCursor.visible = !activeFocus;
					placeHolder.visible = !activeFocus;
				}

				onLengthChanged: {
					// TODO: may be too expensive
					var lText = buzzerClient.getPlainText(buzzText.textDocument);
					countProgress.adjust(buzzerClient.getBuzzBodySize(lText) + preeditText.length);
					buzzersList.close();
					tagsList.close();
				}

				property bool buzzerStarted_: false;
				property bool tagStarted_: false;

				onPreeditTextChanged: {
					//
					if (preeditText == " ") {
						buzzerStarted_ = false;
						tagStarted_ = false;
						buzzersList.close();
						tagsList.close();
					} else if (preeditText == "@" || buzzerStarted_) {
						//
						buzzerStarted_ = true;
					} else if (preeditText == "#" || tagStarted_) {
						//
						tagStarted_ = true;
					}

					if (buzzerStarted_ || tagStarted_) {
						var lText = buzzerClient.getPlainText(buzzText.textDocument);
						highlighter.tryHighlightBlock(lText + preeditText, 0);
					}
				}

				onFocusChanged: {
					editBuzzTimer.start();
				}

				onEditingFinished: {
					if (!external_) buzzText.forceActiveFocus();
				}
			}

			layer.enabled: false
			layer.effect: OpacityMask {
				maskSource: Item {
					width: replyEditorContainer.width
					height: replyEditorContainer.height

					Rectangle {
						anchors.centerIn: parent
						width: replyEditorContainer.width
						height: replyEditorContainer.height
						radius: 8
					}
				}
			}
		}

		QuarkToolButton {
			id: sendButton
			Material.background: "transparent"
			visible: true
			labelYOffset: 2
			labelXOffset: -3
			symbolColor: !sending ?
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.sendSym
			symbolFontPointSize: 20

			x: hiddenCountFrame.x - width - spaceItems_
			y: parent.height - (height + spaceItems_ - 2)

			onClicked: {
				if (!sending) {
					//
					/*
					if (buzzesThread_.count > 0) {
						var lBottomItem = list.itemAtIndex(buzzesThread_.count - 1);
						if (lBottomItem !== null) {
							atTheBottom = (lBottomItem.y < list.contentY + list.contentHeight);
						} else {
							atTheBottom = false;
						}
					}
					*/

					//
					sending = true;

					var lBuzzId = buzzesThread_.buzzId(0);
					createReply(lBuzzId);
				}
			}
		}

		QuarkRoundState {
			id: hiddenCountFrame
			x: parent.width - (size + spaceRight_)
			y: parent.height - (size + spaceBottom_ + spaceItems_ - 2)
			size: 24
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.hiddenLight")
			background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Market.tabBackground")
		}

		QuarkRoundProgress {
			id: countProgress
			x: hiddenCountFrame.x
			y: hiddenCountFrame.y
			size: hiddenCountFrame.size
			colorCircle: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accentUltra");
			colorBackground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accentUltra");
			arcBegin: 0
			arcEnd: 0
			lineWidth: 3

			function adjust(length) {
				arcEnd = (length * 360) / buzzerClient.getBuzzBodyMaxSize();
			}
		}

		ProgressBar {
			id: createProgressBar

			x: 0
			y: 0
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

	Timer {
		id: editBuzzTimer
		interval: 500
		repeat: false
		running: false

		onTriggered: {
			list.positionViewAtEnd();
			atTheBottom = true;
		}
	}

	Timer {
		id: toTheTimer
		interval: 1000
		repeat: false
		running: false

		onTriggered: {
			list.positionViewAtEnd();
			atTheBottom = true;
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
				buzzSubscribeCommand.process();
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
		width: 170
		visible: false

		model: ListModel { id: buzzersModel }

		property var matched;

		onClick: {
			//
			var lPos = buzzText.cursorPosition; // current
			var lText = buzzerClient.getPlainText(buzzText.textDocument) + "";
			for (var lIdx = lPos; lIdx >= 0; lIdx--) {
				if (lText[lIdx] === '@') {
					break;
				}
			}

			var lNewText = lText.slice(0, lIdx) + key + lText.slice(lPos);

			buzzText.clear();
			buzzText.insert(0, lNewText);

			buzzText.cursorPosition = lIdx + key.length;
		}

		function popup(match, nx, ny, buzzers) {
			//
			if (buzzers.length === 0) return;
			if (buzzers.length === 1 && match === buzzers[0]) return;
			//
			matched = match;
			//
			if (nx + replyEditorContainer.x + width > replyContainer.width) x = replyContainer.width - width;
			else x = nx + replyEditorContainer.x;

			buzzersModel.clear();

			for (var lIdx = 0; lIdx < buzzers.length; lIdx++) {
				buzzersModel.append({
					key: buzzers[lIdx],
					keySymbol: "",
					name: buzzers[lIdx]});
			}

			var lNewY = ny + replyContainer.y + spaceTop_;
			y = lNewY - ((buzzers.length) * 50 + spaceTop_);

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
			var lPos = buzzText.cursorPosition; // current
			var lText = buzzerClient.getPlainText(buzzText.textDocument) + "";
			for (var lIdx = lPos; lIdx >= 0; lIdx--) {
				if (lText[lIdx] === '#') {
					break;
				}
			}

			var lNewText = lText.slice(0, lIdx) + key + lText.slice(lPos);

			buzzText.clear();
			buzzText.insert(0, lNewText);

			buzzText.cursorPosition = lIdx + key.length;
		}

		function popup(match, nx, ny, tags) {
			//
			if (tags.length === 0) return;
			if (tags.length === 1 && match === tags[0]) return;
			//
			matched = match;
			//
			if (nx + replyEditorContainer.x + width > replyContainer.width) x = replyContainer.width - width;
			else x = nx + replyEditorContainer.x;

			tagsModel.clear();

			for (var lIdx = 0; lIdx < tags.length; lIdx++) {
				tagsModel.append({
					key: tags[lIdx],
					keySymbol: "",
					name: tags[lIdx]});
			}

			var lNewY = ny + replyContainer.y + spaceTop_;
			y = lNewY - ((tags.length) * 50 + spaceTop_);

			open();
		}
	}

	BuzzerCommands.SearchEntityNamesCommand {
		id: searchBuzzers

		onProcessed: {
			// pattern, entities
			var lRect = buzzText.positionToRectangle(buzzText.cursorPosition);
			buzzersList.popup(pattern, lRect.x, lRect.y + lRect.height, entities);
		}

		onError: {
			controller.showError(message);
		}
	}

	BuzzerCommands.LoadHashTagsCommand {
		id: searchTags

		onProcessed: {
			// pattern, tags
			var lRect = buzzText.positionToRectangle(buzzText.cursorPosition);
			tagsList.popup(pattern, lRect.x, lRect.y + lRect.height, tags);
		}

		onError: {
			controller.showError(message);
		}
	}

	BuzzerComponents.BuzzTextHighlighter {
		id: highlighter
		textDocument: buzzText.textDocument

		onMatched: {
			// start, length, match
			if (match.length) {
				//
				var lPosition = buzzText.cursorPosition;
				if (/*(lPosition === start || lPosition === start + length) &&*/ buzzText.preeditText != " ") {
					if (match[0] === '@')
						searchBuzzers.process(match);
					else if (match[0] === '#')
						searchTags.process(match);
					else if (match.includes('/data/user/')) {
						var lParts = match.split(".");
						if (lParts.length) {
							if (lParts[lParts.length-1].toLowerCase() === "jpg" || lParts[lParts.length-1].toLowerCase() === "jpeg" ||
								lParts[lParts.length-1].toLowerCase() === "png") {
								// pop-up rich editor
								if (!sending) {
									//
									var lComponent = null;
									var lPage = null;

									lComponent = Qt.createComponent("qrc:/qml/buzzeditor.qml");
									if (lComponent.status === Component.Error) {
										showError(lComponent.errorString());
									} else {
										lPage = lComponent.createObject(controller);
										lPage.controller = controller;
										lPage.initializeReply(buzzesThread_.self(0), buzzesThread_, buzzerClient.getPlainText(buzzText.textDocument));
										buzzText.clear();
										addPage(lPage);
									}
								}
							}
						}
					}
				}
			}
		}
	}

	BuzzerCommands.ReplyCommand {
		id: replyCommand
		model: buzzesThread_

		onProcessed: {
			sending = false;
			createProgressBar.visible = false;
			buzzText.clear();
		}

		onRetry: {
			// was specific error, retrying...
			createProgressBar.indeterminate = true;
			retryReplyCommand.start();
		}

		onError: {
			sending = false;
			createProgressBar.visible = false;
			handleError(code, message);
		}

		onMediaUploaded: {
		}
	}

	Timer {
		id: retryReplyCommand
		interval: 1000
		repeat: false
		running: false

		onTriggered: {
			replyCommand.reprocess();
		}
	}

	function createReply(buzzId) {
		//
		var lText = buzzerClient.getPlainText(buzzText.textDocument);
		if (lText.length === 0) {
			handleError("E_BUZZ_IS_EMPTY", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZ_IS_EMPTY"));
			sending = false;
			return;
		}

		if (buzzerClient.getBuzzBodySize(lText) >= buzzerClient.getBuzzBodyMaxSize()) {
			handleError("E_BUZZ_IS_TOO_BIG", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZ_IS_TOO_BIG"));
			sending = false;
			return;
		}

		//
		createProgressBar.indeterminate = true;
		createProgressBar.visible = true;

		//
		replyCommand.buzzBody = lText;
		var lBuzzers = buzzerClient.extractBuzzers(replyCommand.buzzBody);

		for (var lIdx = 0; lIdx < lBuzzers.length; lIdx++) {
			replyCommand.addBuzzer(lBuzzers[lIdx]);
		}

		if (buzzId) replyCommand.buzzId = buzzId;

		replyCommand.process();
	}

	function handleError(code, message) {
		if (code === "E_CHAINS_ABSENT") return;
		if (message === "UNKNOWN_REFTX" || code === "E_TX_NOT_SENT") {
			//buzzerClient.resync();
			controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"), true);
		} else {
			controller.showError(message, true);
		}
	}
}
