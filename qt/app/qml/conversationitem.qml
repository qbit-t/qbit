import QtQuick 2.15
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

import "qrc:/lib/numberFunctions.js" as NumberFunctions

Item {
	id: conversationitem_

	property int calculatedHeight: calculateHeightInternal()

	property var timestamp_: timestamp
	property var ago_: ago
	property var localDateTime_: localDateTime
	property var score_: score
	property int side_: side
	property var conversationId_: conversationId
	property var conversationChainId_: conversationChainId
	property var creatorId_: creatorId
	property var creatorInfoId_: creatorInfoId
	property var creatorInfoChainId_: creatorInfoChainId
	property var counterpartyId_: counterpartyId
	property var counterpartyInfoId_: counterpartyInfoId
	property var counterpartyInfoChainId_: counterpartyInfoChainId
	property var events_: events
	property var self_: self

	property bool dynamic_: dynamic
	property bool onChain_: onChain

	property var controller_
	property var conversationsModel_
	property var listView_

	readonly property int spaceLeft_: 15
	readonly property int spaceTop_: 12
	readonly property int spaceRight_: 15
	readonly property int spaceBottom_: 12
	readonly property int spaceAvatarBuzz_: 10
	readonly property int spaceItems_: 5
	readonly property int spaceMedia_: 10
	readonly property int spaceSingleMedia_: 8
	readonly property int spaceMediaIndicator_: 15
	readonly property int spaceHeader_: 5
	readonly property int spaceRightMenu_: 15
	readonly property int spaceStats_: -5
	readonly property int spaceLine_: 4
	readonly property int spaceThreaded_: 33
	readonly property int spaceThreadedItems_: 4
	readonly property real defaultFontSize: buzzerApp.defaultFontSize()

	readonly property int sideCreator_: 0
	readonly property int sideCounterparty_: 1

	readonly property int conversationPending_: 0
	readonly property int conversationAccepted_: 1
	readonly property int conversationDeclined_: 2

	signal calculatedHeightModified(var value);

	onCalculatedHeightChanged: {
		calculatedHeightModified(calculatedHeight);
	}

	Component.onCompleted: {
		avatarDownloadCommand.process();

		if (!onChain_ && dynamic_) checkOnChain.start();
	}

	onEvents_Changed: {
		//
		if (conversationState() !== conversationPending_)
			decryptBodyCommand.process(conversationId_);
	}

	onConversationId_Changed: {
		//
		if (conversationState() !== conversationPending_)
			decryptBodyCommand.process(conversationId_);
	}

	function calculateHeightInternal() {
		var lCalculatedInnerHeight = bottomLine.y1;
		return lCalculatedInnerHeight > avatarImage.displayHeight ?
										   lCalculatedInnerHeight : avatarImage.displayHeight;
	}

	function calculateHeight() {
		calculatedHeight = calculateHeightInternal();
		return calculatedHeight;
	}

	function conversationState() {
		//
		if (events_.length >= 1) {
			var lInfo = events_[0];
			if (lInfo.type === buzzerClient.tx_BUZZER_ACCEPT_CONVERSATION())
				return conversationAccepted_;
			else if (lInfo.type === buzzerClient.tx_BUZZER_DECLINE_CONVERSATION())
				return conversationDeclined_;
		}

		return conversationPending_;
	}

	function conversationMessage() {
		//
		var lConversationState = conversationState();
		//
		if (events_.length >= 1) {
			if (lConversationState === conversationPending_) {
				if (events_.length === 1 && events_[0].type === buzzerClient.tx_BUZZER_ACCEPT_CONVERSATION())
					return buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.conversation.new");
				return buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.conversation.message.new");
			} else if (lConversationState === conversationAccepted_) {
				if (events_.length === 1) return buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.conversation.new");
				if (events_[1].decryptedBody === "") decryptBodyCommand.process(conversationId_);
				return events_[1].decryptedBody;
			} else if (lConversationState === conversationDeclined_) {
				return buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.conversation.declined");
			}
		}

		if (lConversationState === conversationPending_)
			return buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.conversation.pending");

		if (conversationState() !== conversationPending_)
			decryptBodyCommand.process(conversationId_);
		return "";
	}

	function conversationFrom() {
		//
		var lConversationState = conversationState();
		//
		if (events_.length >= 1) {
			if (lConversationState === conversationPending_) {
				return "";
			} else if (lConversationState === conversationAccepted_) {
				if (events_.length === 1) return "";
				if (events_[1].buzzerId === buzzerClient.getCurrentBuzzerId())
					return buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.conversation.you");
				return buzzerClient.getBuzzerName(events_[1].buzzerInfoId) + ":";
			} else if (lConversationState === conversationDeclined_) {
				return "";
			}
		}

		return "";
	}

	function initialize() {
		//
		decryptBodyCommand.model = conversationsModel_;

		if (conversationState() !== conversationPending_)
			decryptBodyCommand.process(conversationId_);
	}

	//
	// avatar download
	//
	BuzzerCommands.DownloadMediaCommand {
		id: avatarDownloadCommand
		url: buzzerClient.getBuzzerAvatarUrl(side_ === sideCreator_ ? creatorInfoId_ : counterpartyInfoId_)
		localFile: buzzerClient.getTempFilesPath() + "/" +
				   buzzerClient.getBuzzerAvatarUrlHeader(side_ === sideCreator_ ? creatorInfoId_ : counterpartyInfoId_)
		preview: true
		skipIfExists: true

		onProcessed: {
			// tx, previewFile, originalFile
			avatarImage.source = "file://" + previewFile
		}
	}

	BuzzerComponents.ImageQx {
		id: avatarImage

		x: spaceLeft_
		y: spaceTop_
		width: avatarImage.displayWidth
		height: avatarImage.displayHeight
		fillMode: Image.PreserveAspectCrop
		mipmap: true
		radius: avatarImage.displayWidth

		property bool rounded: false //!
		property int displayWidth: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 50 : 50
		property int displayHeight: displayWidth

		autoTransform: true

		/*
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
		*/

		MouseArea {
			id: buzzerInfoClick
			anchors.fill: parent

			onClicked: {
				controller_.openBuzzfeedByBuzzer(buzzerClient.getBuzzerName(side_ === sideCreator_ ? creatorInfoId_ : counterpartyInfoId_));
			}
		}
	}

	//
	// score
	//
	QuarkRoundProgress {
		id: imageFrame
		x: avatarImage.x - 2
		y: avatarImage.y - 2
		size: avatarImage.displayWidth + 4
		colorCircle: getColor()
		colorBackground: "transparent"
		arcBegin: 0
		arcEnd: 360
		lineWidth: buzzerClient.scaleFactor * 3
		beginAnimation: false
		endAnimation: false

		function getColor() {
			var lScoreBase = buzzerClient.getTrustScoreBase() / 10;
			var lIndex = score_ / lScoreBase;

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

	//
	// header
	//

	QuarkLabel {
		id: buzzerAliasControl
		x: avatarImage.x + avatarImage.width + spaceAvatarBuzz_
		y: avatarImage.y
		text: buzzerClient.getBuzzerAlias(side_ === sideCreator_ ? creatorInfoId_ : counterpartyInfoId_)
		font.bold: true
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : defaultFontPointSize
	}

	QuarkLabelRegular {
		id: buzzerNameControl
		x: buzzerAliasControl.x + buzzerAliasControl.width + spaceItems_
		y: avatarImage.y
		width: stateSymbol.x - (x + spaceItems_)
		elide: Text.ElideRight
		text: buzzerClient.getBuzzerName(side_ === sideCreator_ ? creatorInfoId_ : counterpartyInfoId_)
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : defaultFontPointSize
	}

	QuarkLabel {
		id: agoControl
		x: parent.width - (width + spaceRightMenu_)
		y: avatarImage.y
		text: ago_
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - 2)) : defaultFontPointSize
	}

	QuarkSymbolLabel {
		id: stateSymbol
		x: agoControl.x - (spaceItems_ + width)
		y: avatarImage.y + 3
		symbol: getSymbol()
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - 10)) : (buzzerApp.defaultFontSize() + 1)
		color: getColor()
		visible: getVisible()

		function getVisible() {
			return conversationState() === conversationPending_ || conversationState() === conversationDeclined_;
		}

		function getSymbol() {
			return conversationState() === conversationPending_ ? Fonts.userClockSym : Fonts.userBanSym;
		}

		function getColor() {
			if (conversationState() === conversationPending_)
				return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.conversation.pending");
			return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.conversation.declined");
		}

		function adjust() {
			symbol = getSymbol();
			color = getColor();
			visible = getVisible();
		}
	}

	QuarkSymbolLabel {
		id: onChainSymbol
		x: agoControl.x + agoControl.width - width
		y: stateSymbol.y + stateSymbol.height + spaceItems_
		symbol: !onChain_ ? Fonts.clockSym : Fonts.checkedCircleSym //linkSym
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - 10)) : (buzzerApp.defaultFontSize() + 1)
		color: !onChain_ ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzz.wait") :
						   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzz.done");

		visible: dynamic_
	}

	BuzzerCommands.LoadTransactionCommand {
		id: checkOnChainCommand

		onProcessed: {
			// tx, chain
			conversationsModel_.setOnChain(index);
		}

		onTransactionNotFound: {
			checkOnChain.start();
		}

		onError: {
			checkOnChain.start();
		}
	}

	BuzzerCommands.DecryptMessageBodyCommand {
		id: decryptBodyCommand
		model: conversationsModel_

		onProcessed: {
			// pkey, body
			bodyControl.message = body.replace(/(\r\n|\n|\r)/gm, " ");
		}

		onError: {
			// TODO: add 'x' mark to message
		}
	}

	Timer {
		id: checkOnChain
		interval: 2000
		repeat: false
		running: false

		onTriggered: {
			checkOnChainCommand.process(conversationId_, conversationChainId_);
		}
	}

	//
	// body
	//

	QuarkLabel {
		id: fromControl
		x: avatarImage.x + avatarImage.width + spaceAvatarBuzz_
		y: buzzerAliasControl.y + buzzerAliasControl.height + spaceHeader_
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link")
		text: conversationFrom()
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : defaultFontPointSize
	}

	TextMetrics	{
		property real defaultFontPointSize: buzzerApp.isDesktop ? buzzerApp.defaultFontSize() : (buzzerApp.defaultFontSize() + 5)
		id: bodyControlMetrics
		elide: Text.ElideRight
		text: bodyControl.message
		elideWidth: parent.width - (bodyControl.x + (buzzerApp.isDesktop ? 2 * spaceRight_ : spaceRight_))
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : bodyControl.defaultFontPointSize
		//property var fontFamily: Qt.platform.os == "windows" ? "Segoe UI Emoji" : "Noto Color Emoji N"
		//font.family: buzzerApp.isDesktop ? fontFamily : font.family
	}

	QuarkLabel {
		id: bodyControl
		x: fromControl.x + fromControl.width + (fromControl.text === "" ? 0 : spaceItems_)
		y: fromControl.y
		text: bodyControlMetrics.elidedText + (bodyControlMetrics.elidedText !== message && buzzerApp.isDesktop ? "..." : "")
		font.italic: conversationState() === conversationPending_
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : defaultFontPointSize

		property var message: getText()

		function getText() {
			//
			var lMessage = conversationMessage();
			return (lMessage !== "" ? lMessage : "...");
		}
	}

	//
	// bottom line
	//

	QuarkHLine {
		id: bottomLine
		x1: 0
		y1: avatarImage.y + avatarImage.height + spaceBottom_
		x2: parent.width
		y2: avatarImage.y + avatarImage.height + spaceBottom_
		penWidth: 1
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		visible: true

		onY1Changed: {
			calculateHeight();
		}
	}

	//
	// handlers
	//

	function handleError(code, message) {
		if (code === "E_CHAINS_ABSENT") return;
		if (message === "UNKNOWN_REFTX" || code === "E_TX_NOT_SENT") {
			//buzzerClient.resync();
			controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"));
		} else {
			controller_.showError(message);
		}
	}
}
