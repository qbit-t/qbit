import QtQuick 2.15
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
	id: conversationthreadfeed_
	key: "conversationthread"
	stacked: buzzerApp.isDesktop

	property var infoDialog;
	property var controller;
	property bool listen: false;
	property var conversationThread_;
	property var conversation_;
	property var conversations_;
	property var message_;
	property var mediaPlayerController: buzzerApp.sharedMediaPlayerController()

	//
	// TODO: recheck!
	property bool accepted_: false;

	readonly property int spaceLeft_: 15
	readonly property int spaceTop_: 12
	readonly property int spaceRight_: 15
	readonly property int spaceBottom_: 12
	readonly property int spaceItems_: 5

	readonly property int conversationPending_: 0
	readonly property int conversationAccepted_: 1
	readonly property int conversationDeclined_: 2

	readonly property int sideCreator_: 0
	readonly property int sideCounterparty_: 1

	property bool sending: false
	property bool atTheBottom: true

	property bool isCreator_: conversation_ !== undefined &&
							  buzzerClient.getCurrentBuzzerId() === conversation_.creatorId

	// adjust height by virtual keyboard
	followKeyboard: true

	function start(conversationId, conversation, conversations, message) {
		//
		conversationThread_ = buzzerClient.createConversationMessagesList();
		//
		conversation_ = conversation;
		conversations_ = conversations;
		message_ = message;
		modelLoader.conversationId = conversationId;
		conversationsPlayer.key = conversationId;
		list.model = conversationThread_;
		console.log("[conversationthread/start]: conversationId = " + conversationId);

		/*
		if (conversationState() === conversationAccepted_ || buzzerClient.getCurrentBuzzerId() === conversation_.creatorId) {
			console.log("[start]: try load pkey...");
			keyLoader.process(conversationId);
		}
		*/

		keyLoader.process(conversationId);

		//
		avatarDownloadCommand.process();
		switchDataTimer.start();

		// muting notificaions
		buzzerApp.pauseNotifications(buzzerClient.getBuzzerName(getBuzzerInfoId()));
	}

	function showItem(message) {
		message_ = message;
		toTheTimer.start();
	}

	function conversationState() {
		//
		if (accepted_) return conversationAccepted_;
		//
		if (conversation_ !== undefined && conversation_.eventInfos.length >= 1) {
			var lInfo = conversation_.eventInfos[0];
			if (lInfo.type === buzzerClient.tx_BUZZER_ACCEPT_CONVERSATION())
				return conversationAccepted_;
			else if (lInfo.type === buzzerClient.tx_BUZZER_DECLINE_CONVERSATION())
				return conversationDeclined_;
		}

		return conversationPending_;
	}

	function getBuzzerInfoId() {
		//
		if (conversation_ === undefined) return "";

		//
		if (conversation_.side === sideCreator_) {
			return conversation_.creatorInfoId;
		}

		return conversation_.counterpartyInfoId;
	}

	function getScore () {
		//
		if (conversation_ === undefined) return 0;

		//
		return conversation_.score;
	}

	Component.onCompleted: {
		closePageHandler = closePage;
		activatePageHandler = activatePage;
	}

	function closePage() {
		// enabling notificaions
		buzzerApp.resumeNotifications(buzzerClient.getBuzzerName(getBuzzerInfoId()));

		//
		if (mediaPlayerController && mediaPlayerController.isCurrentInstancePlaying()) {
			mediaPlayerController.disableContinousPlayback();
			mediaPlayerController.popVideoInstance();
			mediaPlayerController.showCurrentPlayer(null);
		}

		//
		list.unbind();

		//
		stopPage();
		controller.popPage(conversationthreadfeed_);
		destroy(1000);
	}

	function activatePage() {
		buzzText.external_ = false;
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Window.background"));
		//
		buzzText.forceActiveFocus();
	}

	function onErrorCallback(error)	{
		controller.showError(error);
	}

	function stopPage() {
		//
		modelLoader.reset();
	}

	onWidthChanged: {
		orientationChangedTimer.start();
	}

	onHeightChanged: {
		back.height = height;
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
				modelLoader.restart();
				listen = false;
			}
		}

		function onBuzzerDAppSuspended() {
			// enabling notificaions
			buzzerApp.resumeNotifications(buzzerClient.getBuzzerName(getBuzzerInfoId()));
		}

		function onBuzzerDAppResumed() {
			if (buzzerClient.buzzerDAppReady) {
				modelLoader.processAndMerge(true);
				// muting notificaions
				buzzerApp.pauseNotifications(buzzerClient.getBuzzerName(getBuzzerInfoId()));
			}
		}

		function onThemeChanged() {
			//
			conversationsPlayer.terminate();
			if (conversationThread_) conversationThread_.resetModel();
		}
	}

	BuzzerCommands.LoadCounterpartyKeyCommand {
		id: keyLoader

		onProcessed: {
			console.log("[keyLoader]: key loaded");
		}

		onError: {
			console.log("[keyLoader/error]: " + message);
		}
	}

	BuzzerCommands.LoadConversationMessagesCommand {
		id: modelLoader
		model: conversationThread_

		property bool dataReceived: false
		property bool dataRequested: false
		property bool initialFeed: false

		onProcessed: {
			console.log("[conversationfeed]: loaded");
			dataReceived = true;
			dataRequested = false;
			waitDataTimer.done();
			list.reuseItems = true;

			if (initialFeed) {
				initialFeed = false;
				toTheTimer.start();
			}
		}

		onError: {
			// code, message
			console.log("[conversationfeed/error]: " + code + " | " + message);
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
				initialFeed = true;
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
			// console.log("[conversationfeed/feed]: feeding");
			if (!start() && !model.noMoreData && !dataRequested) {
				dataRequested = true;
				modelLoader.process(true);
			}
		}
	}

	BuzzerCommands.AcceptConversationCommand {
		id: acceptCommand
		model: conversations_

		onProcessed: {
			//
			accepted_ = true;
			buzzThreadToolBar.height = buzzThreadToolBar.getHeight();
			acceptButton.visible = false;
			declineButton.visible = false;
			// reset / refetch
			conversationThread_.resetModel();
		}

		onError: {
			//
			handleError(code, message);
		}
	}

	BuzzerCommands.DeclineConversationCommand {
		id: declineCommand
		model: conversations_

		onProcessed: {
			//
			accepted_ = false;
			closePage();
		}

		onError: {
			//
			handleError(code, message);
		}
	}

	//
	// decline dialog
	//

	YesNoDialog	{
		id: declineConversationDialog

		onAccepted:	{
			//
			declineCommand.process(modelLoader.conversationId);
		}

		onRejected: {
			declineConversationDialog.close();
		}
	}

	//
	// toolbar
	//

	QuarkToolBar {
		property int defaultHeight_: (buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 50) : 55) + topOffset
		property int buttonsHeight_: (buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 50) : 55) + topOffset

		id: buzzThreadToolBar
		height: getHeight()
		width: parent.width

		property int totalHeight: height

		function getHeight() {
			if (conversationState() === conversationPending_ && !isCreator_)
				return defaultHeight_ + buttonsHeight_ + spaceBottom_;
			return defaultHeight_;
		}

		Component.onCompleted: {
		}

		QuarkRoundSymbolButton {
			id: cancelButton
			x: spaceItems_
			y: parent.height / 2 - height / 2 + topOffset / 2
			symbol: Fonts.leftArrowSym
			fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 5)) : buzzerApp.defaultFontSize() + 7
			radius: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultRadius - 7)) : (defaultRadius - 7)
			color: "transparent"
			textColor: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
			opacity: 1.0

			onClick: {
				closePage();
			}
		}

		QuarkRoundSymbolButton {
			id: menuControl
			x: parent.width - width - spaceItems_
			y: parent.height / 2 - height / 2 + topOffset / 2
			symbol: Fonts.elipsisVerticalSym
			fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 5)) : buzzerApp.defaultFontSize() + 7
			radius: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultRadius - 7)) : (defaultRadius - 7)
			color: "transparent"
			textColor: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
			opacity: 1.0
			visible: buzzerApp.isDesktop

			onClick: {
				if (headerMenu.visible) headerMenu.close();
				else { headerMenu.prepare(); headerMenu.open(); }
			}
		}

		//
		// avatar download
		//
		BuzzerCommands.DownloadMediaCommand {
			id: avatarDownloadCommand
			url: buzzerClient.getBuzzerAvatarUrl(getBuzzerInfoId())
			localFile: buzzerClient.getTempFilesPath() + "/" +
					   buzzerClient.getBuzzerAvatarUrlHeader(getBuzzerInfoId())
			preview: true
			skipIfExists: true

			onProcessed: {
				// tx, previewFile, originalFile
				avatarImage.source = "file://" + previewFile
			}
		}

		//
		// score
		//
		QuarkRoundState {
			id: imageFrame
			x: avatarImage.x - 2
			y: avatarImage.y - 2
			size: avatarImage.displayWidth + 4
			color: getColor()
			background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background")

			function getColor() {
				var lScoreBase = buzzerClient.getTrustScoreBase() / 10;
				var lIndex = getScore() / lScoreBase;

				// TODO: consider to use 4 basic colours:
				// 0 - red
				// 1 - 4 - orange
				// 5 - green
				// 6 - 9 - teal
				// 10 -

				switch(Math.round(lIndex)) {
					case 0: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.0");
					case 1: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.1");
					case 2: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.2");
					case 3: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.3");
					case 4: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.4");
					case 5: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.5");
					case 6: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.6");
					case 7: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.7");
					case 8: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.8");
					case 9: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.9");
					default: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.10");
				}
			}
		}

		Image {
			id: avatarImage

			x: cancelButton.x + cancelButton.width + spaceItems_
			y: buzzThreadToolBar.defaultHeight_ / 2 - height / 2 + topOffset / 2

			width: avatarImage.displayWidth
			height: avatarImage.displayHeight
			fillMode: Image.PreserveAspectCrop

			property bool rounded: true
			property int displayWidth: buzzThreadToolBar.defaultHeight_ - (15 + topOffset)
			property int displayHeight: displayWidth

			autoTransform: true

			layer.enabled: rounded
			layer.effect: OpacityMask {
				maskSource: Item {
					width: avatarImage.displayWidth
					height: avatarImage.displayHeight

					Rectangle {
						anchors.centerIn: parent
						width: avatarImage.displayWidth
						height: avatarImage.displayHeight
						radius: avatarImage.displayWidth
					}
				}
			}

			MouseArea {
				id: buzzerInfoClick
				anchors.fill: parent
				cursorShape: Qt.PointingHandCursor

				onClicked: {
					//
					controller.openBuzzfeedByBuzzer(buzzerClient.getBuzzerName(getBuzzerInfoId()));
				}
			}
		}

		QuarkLabel {
			id: buzzerAliasControl
			x: avatarImage.x + avatarImage.width + spaceLeft_
			y: avatarImage.y + (avatarImage.height / 2 - height / 2)
			text: buzzerClient.getBuzzerAlias(getBuzzerInfoId()) + " " + buzzerClient.getBuzzerName(getBuzzerInfoId())
			font.bold: true
			width: parent.width - (x + spaceRight_)
			elide: Text.ElideRight
			color: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.link")
			font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : defaultFontPointSize

			property real defaultFontSize: buzzerApp.defaultFontSize()
		}

		//
		// Accept
		//

		QuarkButton {
			id: acceptButton
			x: parent.width / 2 - ((parent.width / 2 - parent.width / 5) + spaceBottom_)
			y: buzzThreadToolBar.defaultHeight_ + buzzThreadToolBar.buttonsHeight_ / 2 - height / 2
			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.conversation.accept")
			Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.4")
			visible: conversationState() === conversationPending_ && !isCreator_
			width: parent.width / 2 - parent.width / 5
			height: buzzThreadToolBar.buttonsHeight_

			Layout.alignment: Qt.AlignHCenter

			enabled: true

			contentItem: QuarkText {
				text: acceptButton.text
				font.pointSize: 16
				color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")
				horizontalAlignment: Text.AlignHCenter
				verticalAlignment: Text.AlignVCenter
				elide: Text.ElideRight
			}

			onClicked: {
				//
				console.log("[acceptButton]: clicked");
				acceptCommand.process(modelLoader.conversationId);
			}
		}

		//
		// Decline
		//

		QuarkButton {
			id: declineButton
			x: parent.width / 2 + spaceItems_
			y: buzzThreadToolBar.defaultHeight_ + buzzThreadToolBar.buttonsHeight_ / 2 - height / 2
			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.conversation.decline")
			Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.1")
			visible: conversationState() === conversationPending_ && !isCreator_
			width: parent.width / 2 - parent.width / 5
			height: buzzThreadToolBar.buttonsHeight_

			Layout.alignment: Qt.AlignHCenter

			enabled: true

			contentItem: QuarkText {
				text: declineButton.text
				font.pointSize: 16
				color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")
				horizontalAlignment: Text.AlignHCenter
				verticalAlignment: Text.AlignVCenter
				elide: Text.ElideRight
			}

			onClicked: {
				//
				declineConversationDialog.show(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.conversation.decline.confirm"));
			}
		}

		QuarkHLine {
			id: bottomLine
			x1: 0
			y1: parent.height
			x2: parent.width
			y2: parent.height
			penWidth: 1
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Panel.bottom.separator")
			visible: true
		}
	}

	///
	// thread
	///

	Image {
		id: back
		fillMode: Image.Tile
		width: parent.width
		x: 0
		y: buzzThreadToolBar.y + buzzThreadToolBar.height
		source: "../images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "desktop.back.tile")
	}

	property real posX
	property real posY

	property real gposX
	property real gposY

	QuarkListView {
		id: list
		x: 0
		y: buzzThreadToolBar.y + buzzThreadToolBar.height
		width: parent.width
		height: parent.height - (buzzThreadToolBar.y + buzzThreadToolBar.height + messageContainer.height - 1)
		usePull: false
		clip: true
		highlightFollowsCurrentItem: true
		highlightMoveDuration: -1
		highlightMoveVelocity: -1
		//cacheBuffer: 800
		reuseItems: true
		rotation: 180
		feedDelta: 20

		//
		// TODO: pass through cursor share from the list items
		//
		MouseArea {
			id: mouseArea
			enabled: false
			anchors.fill: parent
			propagateComposedEvents: true
			onPositionChanged: {
				//
				var positionInRoot = mapToItem(list, mouse.x, mouse.y);
				posX = positionInRoot.x;
				posY = positionInRoot.y;

				var globalPosition = mapToGlobal(mouse.x, mouse.y);
				gposX = globalPosition.x;
				gposY = globalPosition.y;
			}

			onWheel: {
				buzzerApp.sendWheelEvent(list, posX, posY, gposX, gposY, wheel.pixelDelta.y * (-1), wheel.angleDelta.y * (-1), wheel.buttons, wheel.modifiers, true);
			}

			onPressed: {
				enabled = false;
				mouse.accepted = false;
			}
		}

		WheelHandler {
			enabled: buzzerApp.isDesktop

			onWheel: {
				if (buzzerApp.isDesktop) mouseArea.enabled = true;
			}
		}


		ScrollIndicator.vertical: scrollIndicator

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
					lProcessable = (lForwardItem.y + lForwardItem.height) > list.contentY + list.height && (lForwardItem.y + lForwardItem.height) - (list.contentY + list.height) >= (cacheBuffer * 0.9);

					if (!lProcessable || lVisible) {
						lForwardItem.unbindCommonControls();
					}
				}
			}
		}

		function toTheBottom() {
		}

		function toTheMessage(message) {
			//
			if (message && conversationThread_) {
				// try locate index
				var lIdx = conversationThread_.locateIndex(message);
				console.log("[toTheMessage]: message = " + message + ", index = " + lIdx);
				if (lIdx >= 0) {
					list.positionViewAtIndex(lIdx, ListView.Beginning);
				}
			}
		}

		onContentHeightChanged: {
		}

		function isBottomItemVisible() {
			var lBottomItem = list.itemAtIndex(model.count - 1);
			if (lBottomItem) {
				return lBottomItem.isFullyVisible;
			}

			return false;
		}

		function isTopItemVisible() {
			var lTopItem = list.itemAtIndex(0);
			if (lTopItem) {
				return lTopItem.isFullyVisible;
			}

			return false;
		}

		onContentYChanged: {
			//
			atTheBottom = isTopItemVisible();

			//
			//console.log("[onContentYChanged]: indexAtTop = " + indexAt(1, contentY));

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
						lProcessable = (lBackItem.y + lBackItem.height) < list.contentY && list.contentY - (lBackItem.y + lBackItem.height) >= (cacheBuffer * 0.9);
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
						lProcessable = (lForwardItem.y + lForwardItem.height) > list.contentY + list.height && (lForwardItem.y + lForwardItem.height) - (list.contentY + list.height) >= (cacheBuffer * 0.9);
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
				list.undind();
				modelLoader.restart();
			}
		}

		onFeedReady: {
			//
			modelLoader.feed();
		}

		Component.onCompleted: {
		}

		delegate: Item {
			id: itemDelegate

			rotation: 180

			property var buzzItem;
			property bool isFullyVisible: y >= list.contentY && y + height < list.contentY + list.height
			property var adjustValue: []

			ListView.onPooled: {
				unbindItem();
				unbindCommonControls();
			}

			ListView.onReused: {
				bindItem();
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
				var lSource = "qrc:/qml/conversationmessageitem.qml";
				var lComponent = Qt.createComponent(lSource);
				if (lComponent.status === Component.Error) {
					controller.showError(lComponent.errorString());
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
				buzzItem.forceChangeCursorShape.connect(itemDelegate.changeCursorShape);
				buzzItem.sharedMediaPlayer_ = conversationthreadfeed_.mediaPlayerController;
				buzzItem.accepted_ = conversationState() === conversationAccepted_;
				buzzItem.width = list.width;
				buzzItem.controller_ = conversationthreadfeed_.controller;
				buzzItem.listView_ = list;
				buzzItem.model_ = conversationThread_;
				buzzItem.conversationId_ = modelLoader.conversationId;

				//itemDelegate.height = buzzItem.calculateHeight();
				itemDelegate.width = list.width;

				buzzItem.adjust();
			}

			function calculatedHeightModified(value) {
				//
				itemDelegate.height = value;
				itemDelegate.buzzItem.height = value;
				itemDelegate.adjustValue.push(value);
			}

			function changeCursorShape(shape) {
				//
				mouseArea.cursorShape = shape;
			}

			function unbindItem() {
				if (itemDelegate.buzzItem)
					itemDelegate.buzzItem.calculatedHeightModified.disconnect(itemDelegate.calculatedHeightModified);
			}

			function unbindCommonControls() {
				if (itemDelegate.buzzItem)
					itemDelegate.buzzItem.unbindCommonControls();
			}

			function forceVisibilityCheck(check) {
				if (itemDelegate.buzzItem) {
					itemDelegate.buzzItem.forceVisibilityCheck(check);
				}
			}
		}

		Rectangle {
			id: viewPort
			x: 0
			y: parent.height - conversationsPlayer.height
			width: parent.width
			height: conversationsPlayer.height
			color: "transparent"
			//rotation: 180
		}
	}

	ScrollIndicator {
		id: scrollIndicator
		rotation: 180
		active: true
		anchors {
			left: list.left
			top: list.top
			bottom: list.bottom
			leftMargin: list.width - width
		}
	}

	//
	BuzzItemMediaPlayer {
		id: conversationsPlayer
		x: 0
		y: bottomLine.y1
		width: parent.width
		mediaPlayerController: conversationthreadfeed_.mediaPlayerController
		overlayParent: list
		overlayRect: Qt.rect(viewPort.x, viewPort.y, viewPort.width, viewPort.height)
		inverse: true
	}

	//
	QuarkRoundSymbolButton {
		id: goBottom
		x: list.width - (width + spaceItems_ * 2)
		y: list.y + list.height - (height + spaceItems_ * 2)

		property real defaultFontSize: buzzerApp.defaultFontSize()

		symbol: Fonts.arrowBottomSym
		fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 7)) : (buzzerApp.defaultFontSize() + 7)
		radius: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultRadius)) : defaultRadius
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.highlight")
		opacity: 0.8

		enabled: true

		visible: !atTheBottom

		onClick: {
			list.positionViewAtBeginning();
		}
	}

	//
	// write message
	//

	Rectangle {
		id: messageContainer
		x: 0
		y: parent.height - (height)
		width: parent.width
		height: buzzerApp.isDesktop ? (buzzText.lineCount > 1 ? replyEditorContainer.getHeight() + spaceTop_ + spaceBottom_:
																controller.bottomBarHeight) :
									  (buzzText.contentHeight + spaceTop_ + spaceBottom_ + 2 * spaceItems_ + adjustedOffset)
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Market.tabBackground")

		property var adjustedOffset: conversationthreadfeed_.keyboardHeight == 0 && bottomOffset > 30 ? bottomOffset : 0

		QuarkHLine {
			id: topLine
			x1: 0
			y1: 0
			x2: parent.width
			y2: 0
			penWidth: 1
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, buzzerApp.isDesktop ? "Material.disabledHidden" : "Panel.top.separator")
			visible: true
		}

		QuarkToolButton {
			id: richEditorButton
			Material.background: "transparent"
			visible: true
			labelYOffset: buzzerApp.isDesktop ? 0 : 2
			labelXOffset: buzzerApp.isDesktop ? 0 : -3
			symbolColor: !sending ?
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.richEditSym
			symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 7)) : defaultSymbolFontPointSize

			x: 0
			y: parent.height - (height + messageContainer.adjustedOffset + (buzzerApp.isDesktop ? 0 : spaceItems_ - 2))

			onClicked: {
				//
				if (!sending /*&& buzzesThread_.count > 0*/) {
					//
					buzzText.external_ = true;
					//
					controller.openMessageEditor(buzzerClient.getPlainText(buzzText.textDocument),
												 buzzerClient.getCounterpartyKey(modelLoader.conversationId),
												 modelLoader.conversationId, buzzerClient.getBuzzerAlias(getBuzzerInfoId()),
												 conversationthreadfeed_.keyboardHeight);
				}
			}
		}

		QuarkToolButton {
			id: addEmojiButton
			Material.background: "transparent"
			visible: buzzerApp.isDesktop
			symbolColor: !sending ?
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.peopleEmojiSym
			symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 7)) : defaultSymbolFontPointSize

			x: richEditorButton.x + richEditorButton.width // + spaceItems_
			y: parent.height - (height + (buzzerApp.isDesktop ? 0 : spaceItems_ - 2))

			onClicked: {
				emojiPopup.popup(addEmojiButton.x, messageContainer.y - (emojiPopup.height));
			}
		}

		Rectangle {
			id: replyEditorContainer
			x: addEmojiButton.visible ? addEmojiButton.x + addEmojiButton.width : richEditorButton.x + richEditorButton.width
			y: buzzText.contentHeight > richEditorButton.height / 2 ? spaceTop_ :
																	  richEditorButton.y + richEditorButton.height / 2 - buzzText.contentHeight / 2
			height: buzzText.contentHeight
			width: parent.width - (sendButton.width + richEditorButton.width +
										(addEmojiButton.visible ? addEmojiButton.width : 0) +
											hiddenCountFrame.width + spaceItems_ + spaceRight_)
			border.color: "transparent"
			color: "transparent"

			function getHeight() {
				return buzzText.contentHeight + 2 * (spaceItems_);
			}

			Rectangle {
				id: quasiCursor

				x: 0
				y: 3

				width: 1
				height: parent.height - 6
				visible: !buzzerApp.isDesktop

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
				//x: spaceItems_
				//y: spaceItems_
				//height: 1000 //parent.height - spaceItems_
				width: parent.width - spaceItems_
				wrapMode: Text.Wrap
				textFormat: Text.PlainText //Text.RichText
				color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
				font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 1)) : (defaultFontPointSize + 1)
				selectionColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.selected")
				selectByMouse: true
				focus: buzzerApp.isDesktop

				Component.onCompleted: {
					if (Qt.platform.os === "ios") buzzerApp.setupImEventFilter(buzzText);
				}

				QuarkLabelRegular {
					id: placeHolder

					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
					anchors.fill: parent
					anchors.leftMargin: spaceItems_

					elide: Text.ElideRight

					text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.editor.message")
					color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.textDisabled")
					font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 1)) : (defaultFontPointSize + 1)

					visible: !buzzText.activeFocus && !buzzText.hasText_
				}

				property bool external_: false

				onActiveFocusChanged: {
					if (!buzzerApp.isDesktop) quasiCursor.visible = !activeFocus;
				}

				onLengthChanged: {
					// TODO: may be too expensive
					var lText = buzzerClient.getPlainText(buzzText.textDocument);
					hasText_ = preeditText.length || lText.length;
					countProgress.adjust(buzzerClient.getBuzzBodySize(lText) + preeditText.length);
					buzzersList.close();
					tagsList.close();
				}

				property bool buzzerStarted_: false;
				property bool tagStarted_: false;
				property bool hasText_: false;

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

					var lText = "";
					if (buzzerStarted_ || tagStarted_) {
						lText = buzzerClient.getPlainText(buzzText.textDocument);
						highlighter.tryHighlightBlock(lText.slice(0, cursorPosition) + preeditText + lText.slice(cursorPosition), 0);
					}

					hasText_ = preeditText.length || lText.length;
				}

				onFocusChanged: {
					//editBuzzTimer.start();
				}

				onEditingFinished: {
					if (!external_ && !buzzerApp.isDesktop) buzzText.forceActiveFocus();
				}

				property bool ctrlEnter: false

				Keys.onPressed: {
					//
					if (!buzzerApp.isDesktop) return;
					//
					if (event.key === Qt.Key_Return && event.modifiers !== Qt.ControlModifier && !ctrlEnter) {
						event.accepted = true;
						ctrlEnter = false;
						sendButton.send();
					} else if (event.key === Qt.Key_Return && event.modifiers === Qt.ControlModifier) {
						event.accepted = false;
						ctrlEnter = true;
						keyEmitter.keyPressed(buzzText, Qt.Key_Return);
					} else {
						ctrlEnter = false;
					}
				}
			}
		}

		QuarkToolButton {
			id: sendButton
			Material.background: "transparent"
			visible: true
			labelYOffset: buzzerApp.isDesktop ? 0 : 2
			labelXOffset: buzzerApp.isDesktop ? 0 : -3
			symbolColor: !sending ?
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.sendSym
			symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 7)) : (buzzerApp.defaultFontSize() + 9)

			x: hiddenCountFrame.x - width - spaceItems_
			y: parent.height - (height + messageContainer.adjustedOffset + (buzzerApp.isDesktop ? 0 : spaceItems_ - 2))

			onClicked: {
				sendButton.send();
			}

			function send() {
				if (!sending) {
					//
					/*
					if (list.count > 0) {
						var lBottomItem = list.itemAtIndex(list.count - 1);
						if (lBottomItem !== null) {
							atTheBottom = lBottomItem.isFullyVisible;
						}
					}
					*/

					//
					sending = true;
					//
					createMessage();
				}
			}
		}

		QuarkRoundState {
			id: hiddenCountFrame
			x: parent.width - (size + spaceRight_)
			y: parent.height - (size + messageContainer.adjustedOffset + spaceBottom_ + (buzzerApp.isDesktop ? 0 : spaceItems_ - 2))
			size: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 26) : 24
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
			lineWidth: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 3) : 3

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
		interval: 200
		repeat: false
		running: false

		onTriggered: {
			//
			list.toTheBottom();
		}
	}

	Timer {
		id: toTheTimer
		interval: 500
		repeat: false
		running: false

		onTriggered: {
			//
			list.toTheMessage(message_);

			//
			if (buzzerApp.isDesktop) buzzText.forceActiveFocus();
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

			//
			var lPlayerController = buzzerApp.sharedMediaPlayerController();
			if (lPlayerController && lPlayerController.isCurrentInstancePlaying()) {
				console.log("[onTriggered]: show current player = " + conversationsPlayer);
				lPlayerController.showCurrentPlayer(null);
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

	QuarkEmojiPopup {
		id: emojiPopup
		width: buzzerClient.scaleFactor * 360
		height: buzzerClient.scaleFactor * 300

		onEmojiSelected: {
			//
			buzzText.forceActiveFocus();
			buzzText.insert(buzzText.cursorPosition, emoji);
		}

		function popup(nx, ny) {
			//
			x = nx;
			y = ny;

			open();
		}
	}

	QuarkPopupMenu {
		id: headerMenu
		x: parent.width - width - spaceRight_
		y: menuControl.y + menuControl.height + spaceItems_
		width: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 350) : 350
		visible: false

		model: ListModel { id: menuModel }

		onAboutToShow: prepare()

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
			lText = lText.slice(0, lPos) + buzzText.preeditText + lText.slice(lPos);

			for (var lIdx = lPos; lIdx >= 0; lIdx--) {
				if (lText[lIdx] === '@') {
					break;
				}
			}

			if (buzzerApp.isDesktop) {
				//
				buzzText.remove(lIdx, lPos);
				buzzText.insert(lIdx, key);

				buzzText.cursorPosition = lIdx + key.length;
			} else {
				var lNewText = lText.slice(0, lIdx);
				if (lNewText.length && lNewText[lNewText.length-1] !== ' ') lNewText += " ";
				lNewText += key + lText.slice(lPos + buzzText.preeditText.length);

				buzzText.clear();

				var lParagraphs = lNewText.split("\n");
				for (lIdx = 0; lIdx < lParagraphs.length; lIdx++) {
					buzzText.append(lParagraphs[lIdx]);
				}

				buzzText.cursorPosition = lPos + key.length;
			}
		}

		function popup(match, nx, ny, buzzers) {
			//
			if (buzzers.length === 0) return;
			if (buzzers.length === 1 && match === buzzers[0]) return;
			//
			matched = match;
			//
			if (nx + replyEditorContainer.x + width > messageContainer.width) x = messageContainer.width - width;
			else x = nx + replyEditorContainer.x;

			buzzersModel.clear();

			for (var lIdx = 0; lIdx < buzzers.length; lIdx++) {
				buzzersModel.append({
					key: buzzers[lIdx],
					keySymbol: "",
					name: buzzers[lIdx]});
			}

			var lNewY = ny + messageContainer.y + spaceTop_;
			y = lNewY - ((buzzers.length) * itemHeight + spaceTop_);

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
			lText = lText.slice(0, lPos) + buzzText.preeditText + lText.slice(lPos);

			for (var lIdx = lPos; lIdx >= 0; lIdx--) {
				if (lText[lIdx] === '#') {
					break;
				}
			}

			if (buzzerApp.isDesktop) {
				//
				buzzText.remove(lIdx, lPos);
				buzzText.insert(lIdx, key);

				buzzText.cursorPosition = lIdx + key.length;
			} else {
				var lNewText = lText.slice(0, lIdx);
				if (lNewText.length && lNewText[lNewText.length-1] !== ' ') lNewText += " ";
				lNewText += key + lText.slice(lPos + buzzText.preeditText.length);

				buzzText.clear();

				var lParagraphs = lNewText.split("\n");
				for (lIdx = 0; lIdx < lParagraphs.length; lIdx++) {
					buzzText.append(lParagraphs[lIdx]);
				}

				buzzText.cursorPosition = lPos + key.length;
			}
		}

		function popup(match, nx, ny, tags) {
			//
			if (tags.length === 0) return;
			if (tags.length === 1 && match === tags[0]) return;
			//
			matched = match;
			//
			if (nx + replyEditorContainer.x + width > messageContainer.width) x = messageContainer.width - width;
			else x = nx + replyEditorContainer.x;

			tagsModel.clear();

			for (var lIdx = 0; lIdx < tags.length; lIdx++) {
				tagsModel.append({
					key: tags[lIdx],
					keySymbol: "",
					name: tags[lIdx]});
			}

			var lNewY = ny + messageContainer.y + spaceTop_;
			y = lNewY - ((tags.length) * itemHeight + spaceTop_);

			open();
		}
	}

	BuzzerCommands.SearchEntityNamesCommand {
		id: searchBuzzers

		onProcessed: {
			// pattern, entities
			var lRect = buzzText.positionToRectangle(buzzText.cursorPosition);
			buzzersList.popup(pattern, buzzText.x + lRect.x + spaceItems_, lRect.y + lRect.height, entities);
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
			tagsList.popup(pattern, buzzText.x + lRect.x + spaceItems_, lRect.y + lRect.height, tags);
		}

		onError: {
			controller.showError(message);
		}
	}

	BuzzerComponents.BuzzTextHighlighter {
		id: highlighter
		textDocument: buzzText.textDocument

		onMatched: {
			//
			var lText = buzzerClient.getPlainText(buzzText.textDocument);
			if (!buzzerApp.isDesktop) {
				lText = lText.slice(0, buzzText.cursorPosition) + buzzText.preeditText + lText.slice(buzzText.cursorPosition, lText.length);
			}

			var lPosition = buzzText.cursorPosition;
			if (lText.slice(lPosition - length, lPosition) === match ||
					(!buzzerApp.isDesktop && lText.slice(lPosition-1, (lPosition-1) + length) === match) ||
					(!buzzerApp.isDesktop && lText.length >= lPosition + length &&
											lText.slice(lPosition, lPosition + length) === match)) {
				if (match[0] === '@')
					searchBuzzers.process(match);
				else if (match[0] === '#')
					searchTags.process(match);
			}
		}
	}

	BuzzerCommands.ConversationMessageCommand {
		id: messageCommand

		onProcessed: {
			sending = false;
			createProgressBar.visible = false;
			buzzText.clear();
		}

		onRetry: {
			// was specific error, retrying...
			createProgressBar.indeterminate = true;
			retryConversationMessageCommand.start();
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
		id: retryConversationMessageCommand
		interval: 1000
		repeat: false
		running: false

		onTriggered: {
			messageCommand.reprocess();
		}
	}

	function createMessage() {
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

		if (lText.includes("/>") || lText.includes("</")) { // TODO: implement more common approach
			handleError("E_BUZZ_UNSUPPORTED_HTML", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZ_UNSUPPORTED_HTML"));
			sending = false;
			return;
		}

		//
		createProgressBar.indeterminate = true;
		createProgressBar.visible = true;

		//
		messageCommand.conversation = modelLoader.conversationId;
		messageCommand.messageBody = lText;
		var lBuzzers = buzzerClient.extractBuzzers(messageCommand.messageBody);

		for (var lIdx = 0; lIdx < lBuzzers.length; lIdx++) {
			messageCommand.addBuzzer(lBuzzers[lIdx]);
		}

		messageCommand.process();
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
