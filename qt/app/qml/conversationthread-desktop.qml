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
	id: conversationthread_
	key: "conversationthread"
	stacked: true

	property var infoDialog;
	property var controller;
	property bool listen: false;
	property var buzzesThread_;
	property var conversation_;
	property var conversations_;
	property var messageId_;

	//
	// TODO: recheck!
	property bool accepted_: false;

	readonly property int spaceLeft_: 15
	readonly property int spaceTop_: 12
	readonly property int spaceRight_: 15
	readonly property int spaceBottom_: 12
	readonly property int spaceItems_: 5
	readonly property real defaultFontSize: 11

	readonly property int conversationPending_: 0
	readonly property int conversationAccepted_: 1
	readonly property int conversationDeclined_: 2

	readonly property int sideCreator_: 0
	readonly property int sideCounterparty_: 1

	property bool sending: false
	property bool atTheBottom: false

	property bool isCreator_: conversation_ !== undefined &&
							  buzzerClient.getCurrentBuzzerId() === conversation_.creatorId

	Connections {
		target: buzzesThread_

		function onCountChanged() {
			console.log("[onCountChanged]: atTheBottom = " + atTheBottom);
			if (atTheBottom) {
				list.positionViewAtEnd();
			}
		}
	}

	function start(conversationId, conversation, conversations, messageId) {
		//
		buzzesThread_ = buzzerClient.createConversationMessagesList();
		//
		messageId_ = messageId;
		conversation_ = conversation;
		conversations_ = conversations;
		modelLoader.conversationId = conversationId;
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
	}

	function showItem(messageId) {
		if (messageId !== undefined) {
			var lIndex = buzzesThread_.locateIndex(messageId);
			if (lIndex !== -1) {
				list.positionViewAtIndex(lIndex, ListView.Contain);
			}
		}
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
		stopPage();
		controller.popPage();
		destroy(1000);
	}

	function activatePage() {
		buzzText.external_ = false;
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
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
		back.width = width;
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

		function onBuzzerDAppResumed() {
			if (buzzerClient.buzzerDAppReady) {
				modelLoader.processAndMerge(true);
			}
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
		model: buzzesThread_

		property bool dataReceived: false
		property bool dataRequested: false
		property bool initialFeed: false

		onProcessed: {
			console.log("[buzzfeed]: loaded");
			dataReceived = true;
			dataRequested = false;
			waitDataTimer.done();

			if (initialFeed) {
				initialFeed = false;
				editBuzzTimer.start();
			}

			if (messageId_ !== undefined) {
				var lIndex = buzzesThread_.locateIndex(messageId_);
				if (lIndex !== -1) {
					list.positionViewAtIndex(lIndex, ListView.Contain);
				}

				messageId_ = undefined;
			}
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
			buzzesThread_.resetModel();
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
		property int defaultHeight_: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 50) : 45
		property int buttonsHeight_: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 50) : 45

		id: buzzThreadToolBar
		height: getHeight()
		width: parent.width

		property int totalHeight: height

		function getHeight() {
			if (conversationState() === conversationPending_ && !isCreator_)
				return defaultHeight_ + buttonsHeight_; // + spaceBottom_;
			return defaultHeight_;
		}

		Component.onCompleted: {
		}

		QuarkToolButton	{
			id: cancelButton
			y: buzzThreadToolBar.defaultHeight_ / 2 - height / 2
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
			y: buzzThreadToolBar.defaultHeight_ / 2 - height / 2

			width: avatarImage.displayWidth
			height: avatarImage.displayHeight
			fillMode: Image.PreserveAspectCrop
			mipmap: true

			property bool rounded: true
			property int displayWidth: (buzzThreadToolBar.defaultHeight_ - buzzerClient.scaleFactor * 15)
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
			text: buzzerClient.getBuzzerAlias(getBuzzerInfoId())
			font.bold: true
			width: parent.width - (x + spaceRight_)
			elide: Text.ElideRight
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link")
			font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : defaultFontPointSize
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
				font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 12) : 16
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
				font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 12) : 16
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

		QuarkToolButton {
			id: menuControl
			x: parent.width - width //- spaceItems_
			y: buzzThreadToolBar.defaultHeight_ / 2 - height / 2
			Material.background: "transparent"
			symbol: Fonts.elipsisVerticalSym
			visible: true
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
			id: middleLine
			x1: 0
			y1: buzzThreadToolBar.defaultHeight_
			x2: parent.width
			y2: buzzThreadToolBar.defaultHeight_
			penWidth: 1
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
			visible: conversationState() === conversationPending_ && !isCreator_
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

	///
	// thread
	///

	Image {
		id: back
		fillMode: Image.PreserveAspectFit
		width: parent.width
		x: 0
		y: buzzThreadToolBar.y + buzzThreadToolBar.height
		Layout.alignment: Qt.AlignCenter
		source: "../images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "conversations.back")
	}

	QuarkListView {
		id: list
		x: 0
		y: buzzThreadToolBar.y + buzzThreadToolBar.height
		width: parent.width
		height: parent.height - (buzzThreadToolBar.y + buzzThreadToolBar.height + messageContainer.height - 1)
		//contentHeight: 1000
		usePull: true
		clip: true

		cacheBuffer: 10000
		//displayMarginBeginning: 5000
		//displayMarginEnd: 5000

		model: buzzesThread_

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
				modelLoader.restart();
			}
		}

		onFeedReady: {
			//
			modelLoader.feed();
		}

		Component.onCompleted: {
			list.positionViewAtEnd();
			atTheBottom = true;
		}

		delegate: Item {
			id: itemDelegate

			property var buzzItem;

			/*
			onClicked: {
				// TODO: actions
			}
			*/

			onWidthChanged: {
				if (buzzItem) {
					buzzItem.width = list.width;
					itemDelegate.height = buzzItem.calculateHeight();
				}
			}

			Component.onCompleted: {
				var lSource = "qrc:/qml/conversationmessageitem.qml";
				var lComponent = Qt.createComponent(lSource);
				if (lComponent.status === Component.Error) {
					controller.showError(lComponent.errorString());
				} else {
					buzzItem = lComponent.createObject(itemDelegate);

					buzzItem.accepted_ = conversationState() === conversationAccepted_;
					buzzItem.width = list.width;
					buzzItem.controller_ = conversationthread_.controller;
					buzzItem.listView_ = list;
					buzzItem.model_ = buzzesThread_;
					buzzItem.conversationId_ = modelLoader.conversationId;

					itemDelegate.height = buzzItem.calculateHeight();
					itemDelegate.width = list.width;

					buzzItem.calculatedHeightModified.connect(itemDelegate.calculatedHeightModified);
				}
			}

			function calculatedHeightModified(value) {
				itemDelegate.height = value;
			}
		}
	}

	//
	// write message
	//

	Rectangle {
		id: messageContainer
		x: 0
		y: parent.height - (height) //buzzThreadToolBar.height +
		width: parent.width
		height: buzzText.lineCount > 1 ? replyEditorContainer.getHeight() + spaceTop_ + spaceBottom_:
										 controller.bottomBarHeight
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background")

		QuarkHLine {
			id: topLine
			x1: 0
			y1: 0
			x2: parent.width
			y2: 0
			penWidth: 1
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
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
			symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : symbolFontPointSize

			x: 0
			y: controller.bottomBarHeight / 2 - height / 2

			onClicked: {
				//
				if (!sending && buzzesThread_.count > 0) {
					//
					var lComponent = null;
					var lPage = null;

					//
					buzzText.external_ = true;

					lComponent = Qt.createComponent("qrc:/qml/buzzeditor-desktop.qml");
					if (lComponent.status === Component.Error) {
						showError(lComponent.errorString());
					} else {
						lPage = lComponent.createObject(controller);
						lPage.controller = controller;
						lPage.initializeMessage(buzzerClient.getPlainText(buzzText.textDocument),
												buzzerClient.getCounterpartyKey(modelLoader.conversationId),
												modelLoader.conversationId);
						buzzText.clear();
						addPage(lPage);
					}
				}
			}
		}

		QuarkToolButton {
			id: addEmojiButton
			Material.background: "transparent"
			visible: true
			symbolColor: !sending ?
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.peopleEmojiSym
			symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : symbolFontPointSize

			x: richEditorButton.x + richEditorButton.width // + spaceItems_
			y: controller.bottomBarHeight / 2 - height / 2

			onClicked: {
				emojiPopup.popup(addEmojiButton.x, messageContainer.y - (emojiPopup.height));
			}
		}

		Rectangle {
			id: replyEditorContainer
			x: addEmojiButton.x + addEmojiButton.width
			y:  buzzText.lineCount > 1 ? buzzerClient.scaleFactor * spaceTop_ :
										 parent.height / 2 - height / 2
			height: getHeight()
			width: parent.width - (addEmojiButton.width + sendButton.width +
								   richEditorButton.width + hiddenCountFrame.width + spaceItems_ + spaceRight_)
			radius: 4
			border.color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
			color: "transparent"

			function getHeight() {
				return buzzText.contentHeight + 2 * (spaceItems_);
			}

			QuarkTextEdit {
				id: buzzText
				x: spaceItems_
				y: spaceItems_
				width: parent.width - spaceItems_
				wrapMode: Text.Wrap
				textFormat: Text.RichText
				color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
				font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 12) : defaultFontPointSize
				selectionColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.selected")
				selectByMouse: true

				property bool external_: false

				onLengthChanged: {
					// TODO: may by too expensive
					var lText = buzzerClient.getPlainText(buzzText.textDocument);
					countProgress.adjust(buzzerClient.getBuzzBodySize(lText) + preeditText.length);
					buzzersList.close();
					tagsList.close();
				}

				onPreeditTextChanged: {
					buzzersList.close();
					tagsList.close();
				}

				onFocusChanged: {
					editBuzzTimer.start();
				}

				property bool ctrlEnter: false

				Keys.onPressed: {
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

			layer.enabled: true
			layer.effect: OpacityMask {
				maskSource: Item {
					width: replyEditorContainer.width
					height: replyEditorContainer.height

					Rectangle {
						anchors.centerIn: parent
						width: replyEditorContainer.width
						height: replyEditorContainer.height
						radius: 4
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
			//symbolFontPointSize: 20
			symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : 20

			x: hiddenCountFrame.x - width - spaceItems_
			y: controller.bottomBarHeight / 2 - height / 2

			onClicked: {
				sendButton.send();
			}

			function send() {
				if (!sending) {
					//
					if (buzzesThread_.count > 0) {
						var lBottomItem = list.itemAtIndex(buzzesThread_.count - 1);
						if (lBottomItem !== null) {
							atTheBottom = (lBottomItem.y < list.contentY + list.contentHeight);
						} else {
							atTheBottom = false;
						}
					}

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
			y: controller.bottomBarHeight / 2 - size / 2
			size: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 24) : 24
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.hidden")
			background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background")
			penWidth: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 3) : 3
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

	QuarkEmojiPopup {
		id: emojiPopup
		width: buzzerClient.scaleFactor * 360
		height: buzzerClient.scaleFactor * 300

		onEmojiSelected: {
			//
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

	QuarkPopupMenu {
		id: buzzersList
		width: 170
		visible: false

		model: ListModel { id: buzzersModel }

		property var matched;

		onClick: {
			//
			buzzText.remove(buzzText.cursorPosition - matched.length, buzzText.cursorPosition);
			buzzText.insert(buzzText.cursorPosition, key);
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
			buzzText.remove(buzzText.cursorPosition - matched.length, buzzText.cursorPosition);
			buzzText.insert(buzzText.cursorPosition, key);
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
				var lPosition = buzzText.cursorPosition;
				if (/*(lPosition === start || lPosition === start + length) &&*/ buzzText.preeditText != " ") {
					if (match[0] === '@')
						searchBuzzers.process(match);
					else if (match[0] === '#')
						searchTags.process(match);
				}
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
