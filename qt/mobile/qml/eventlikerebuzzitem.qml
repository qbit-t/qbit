﻿import QtQuick 2.15
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
	id: eventLikeRebuzzItem_

	property int calculatedHeight: calculateHeightInternal()

	property var type_: type
	property var timestamp_: timestamp
	property var ago_: ago
	property var localDateTime_: localDateTime
	property var score_: score
	property var buzzId_: buzzId
	property var buzzChainId_: buzzChainId
	property var publisherId_: publisherId
	property var publisherInfoId_: publisherInfoId
	property var publisherInfoChainId_: publisherInfoChainId
	property var buzzBody_: buzzBody
	property var buzzMedia_: buzzMedia
	property var eventInfos_: eventInfos
	property var mention_: mention
	property var value_: value
	property var self_: self

	property var controller_: controller
	property var eventsfeedModel_
	property var listView_

	readonly property int spaceLeft_: 15
	readonly property int spaceTop_: 12
	readonly property int spaceRight_: 15
	readonly property int spaceBottom_: 12
	readonly property int spaceAvatarBuzz_: 10
	readonly property int spaceItems_: 5
	readonly property int spaceMedia_: 10
	readonly property int spaceMediaIndicator_: 15
	readonly property int spaceHeader_: 5
	readonly property int spaceRightMenu_: 15
	readonly property int spaceStats_: -5
	readonly property int spaceLine_: 4
	readonly property int spaceThreaded_: 33
	readonly property int spaceThreadedItems_: 4

	signal calculatedHeightModified(var value);

	onCalculatedHeightChanged: {
		calculatedHeightModified(calculatedHeight);
	}

	Component.onCompleted: {
		avatarDownloadCommand.process();
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

	//
	// avatar download
	//

	BuzzerCommands.DownloadMediaCommand {
		id: avatarDownloadCommand
		url: getAvatar()
		localFile: buzzerClient.getTempFilesPath() + "/" + buzzerClient.getBuzzerAvatarUrlHeader(getBuzzerInfoId())
		preview: true
		skipIfExists: true

		onProcessed: {
			// tx, previewFile, originalFile
			avatarImage.source = "file://" + previewFile
		}

		function getBuzzerInfoId() {
			if (eventInfos_.length > 0) {
				return eventInfos_[0].buzzerInfoId;
			}

			return publisherInfoId_;
		}

		function getAvatar() {
			//
			if (eventInfos_.length > 0) {
				return buzzerClient.getBuzzerAvatarUrl(eventInfos_[0].buzzerInfoId);
			}

			return buzzerClient.getBuzzerAvatarUrl(publisherInfoId_);
		}
	}

	//
	// event (like, rebuzz)
	//

	QuarkSymbolLabel {
		id: actionLabel
		x: spaceLeft_
		y: spaceTop_
		symbol: type_ === buzzerClient.tx_BUZZ_LIKE_TYPE() ? Fonts.heartSym : Fonts.rebuzzSym
		font.pointSize: 22
		color: type_ === buzzerClient.tx_BUZZ_LIKE_TYPE() ?
				   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.event.like") :
				   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.event.rebuzz")
	}

	//
	// info[0] buzzer state
	//

	QuarkRoundState {
		id: imageFrame
		x: avatarImage.x - 1
		y: avatarImage.y - 1
		size: avatarImage.displayWidth + 2
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

			switch(parseInt(lIndex)) {
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

		function getScore() {
			//
			if (eventInfos_.length > 0) {
				return eventInfos_[0].score;
			}

			return score_;
		}
	}

	//
	// info[0] buzzer avatar
	//

	Image {
		id: avatarImage

		x: spaceLeft_ + actionLabel.width + spaceAvatarBuzz_
		y: spaceTop_
		width: avatarImage.displayWidth
		height: avatarImage.displayHeight
		fillMode: Image.PreserveAspectCrop

		property bool rounded: true
		property int displayWidth: 30
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
				// buzzer
				var lComponent = null;
				var lPage = null;

				lComponent = Qt.createComponent("qrc:/qml/buzzfeedbuzzer.qml");
				if (lComponent.status === Component.Error) {
					showError(lComponent.errorString());
				} else {
					lPage = lComponent.createObject(controller);
					lPage.controller = controller;
					lPage.start(getBuzzerName());
					addPage(lPage);
				}
			}

			function getBuzzerName() {
				if (eventInfos_.length > 0) {
					return buzzerClient.getBuzzerName(eventInfos_[0].buzzerInfoId);
				}

				return buzzerClient.getBuzzerName(publisherInfoId_);
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
		text: getBuzzerAlias()
		font.bold: true

		function getBuzzerAlias() {
			//
			if (eventInfos_.length > 0) {
				return buzzerClient.getBuzzerAlias(eventInfos_[0].buzzerInfoId);
			}

			return buzzerClient.getBuzzerAlias(publisherInfoId_);
		}
	}

	QuarkLabel {
		id: buzzerNameControl
		x: buzzerAliasControl.x + buzzerAliasControl.width + spaceItems_
		y: avatarImage.y
		text: getBuzzerName()
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");

		function getBuzzerName() {
			//
			if (eventInfos_.length > 0) {
				return buzzerClient.getBuzzerName(eventInfos_[0].buzzerInfoId);
			}

			return buzzerClient.getBuzzerName(publisherInfoId_);
		}
	}

	QuarkLabel {
		id: buzzerInfoControl
		x: buzzerAliasControl.x
		y: buzzerAliasControl.y + buzzerAliasControl.height + 3
		width: parent.width - (x + spaceRight_)
		elide: Text.ElideRight
		font.pointSize: 14
		text: getText()

		function getText() {
			var lInfo;
			var lString;
			var lResult;

			if (type_ === buzzerClient.tx_BUZZ_LIKE_TYPE()) {
				if (eventInfos_.length > 1) {
					lString = buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.event.like.many");
					return lString.replace("{count}", eventInfos_.length);
				} else if (eventInfos_.length === 1) {
					lString = buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.event.like.one");
					return lString;
				}
			} else if (type_ === buzzerClient.tx_REBUZZ_TYPE() /*|| buzzerClient.tx_REBUZZ_REPLY_TYPE()*/) {
				if (eventInfos_.length > 1) {
					lString = buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.event.rebuzz.many");
					return lString.replace("{count}", eventInfos_.length);
				} else if (eventInfos_.length === 1) {
					lString = buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.event.rebuzz.one");
					return lString;
				}
			}

			return "?";
		}
	}

	QuarkLabel {
		id: agoControl
		x: menuControl.x - width - spaceItems_ * 2
		y: avatarImage.y
		text: ago_
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
	}

	QuarkSymbolLabel {
		id: menuControl
		x: parent.width - width - spaceRightMenu_
		y: avatarImage.y
		symbol: Fonts.shevronDownSym
		font.pointSize: 12
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
	}
	MouseArea {
		x: agoControl.x
		y: menuControl.y - spaceTop_
		width: agoControl.width + menuControl.width + spaceRightMenu_
		height: agoControl.height + spaceRightMenu_

		onClicked: {
			if (getBuzzerId() === buzzerClient.getCurrentBuzzerId()) return; // no actions
			if (headerMenu.visible) headerMenu.close();
			else { headerMenu.prepare(); headerMenu.open(); }
		}

		function getBuzzerId() {
			if (eventInfos_.length > 0) {
				return eventInfos_[0].buzzerId;
			}

			return publisherId_;
		}
	}

	//
	// body
	//

	Rectangle {
		id: bodyControl
		x: avatarImage.x
		y: ((avatarImage.y + avatarImage.height) > (buzzerInfoControl.y + buzzerInfoControl.height) ?
				avatarImage.y + avatarImage.height : buzzerInfoControl.y + buzzerInfoControl.height) + spaceHeader_
		width: parent.width - (x + spaceRight_)
		height: getHeight() //buzzText.height

		border.color: "transparent"
		color: "transparent"

		Component.onCompleted: {
			// expand();
			eventLikeRebuzzItem_.calculateHeight();
		}

		property var wrappedItem_;

		onWidthChanged: {
			expand();

			if (wrappedItem_ && bodyControl.width > 0) {
				wrappedItem_.width = bodyControl.width;
			}

			eventLikeRebuzzItem_.calculateHeight();
		}

		onHeightChanged: {
			expand();
			eventLikeRebuzzItem_.calculateHeight();
		}

		function expand() {
			//
			var lSource;
			var lComponent;

			// if rebuzz and wapped
			if (!wrappedItem_) {
				// buzzText
				lSource = "qrc:/qml/buzzitemlight.qml";
				lComponent = Qt.createComponent(lSource);
				wrappedItem_ = lComponent.createObject(bodyControl);
				wrappedItem_.calculatedHeightModified.connect(innerHeightChanged);

				wrappedItem_.x = 0;
				wrappedItem_.y = 0;
				wrappedItem_.width = bodyControl.width;
				wrappedItem_.controller_ = eventLikeRebuzzItem_.controller_;

				//console.log("[WRAPPED]: wrapped_.buzzerId = " + wrapped_.buzzerId + ", wrapped_.buzzerInfoId = " + wrapped_.buzzerInfoId);

				wrappedItem_.timestamp_ = timestamp_;
				wrappedItem_.score_ = score_;
				wrappedItem_.buzzId_ = buzzId_;
				wrappedItem_.buzzChainId_ = buzzChainId_;

				wrappedItem_.buzzerId_ = publisherId_;
				wrappedItem_.buzzerInfoId_ = publisherInfoId_;
				wrappedItem_.buzzerInfoChainId_ = publisherInfoChainId_;

				wrappedItem_.buzzBody_ = buzzerClient.decorateBuzzBody(buzzBody_);
				wrappedItem_.buzzMedia_ = buzzMedia_;
				wrappedItem_.lastUrl_ = buzzerClient.extractLastUrl(buzzBody_);
				wrappedItem_.ago_ = buzzerClient.timestampAgo(timestamp_);
			}
		}

		function innerHeightChanged(value) {
			bodyControl.height = value;
			eventLikeRebuzzItem_.calculateHeight();
		}

		function getHeight() {
			return (wrappedItem_ ? wrappedItem_.calculatedHeight : 0);
		}
	}

	//
	// bottom line
	//

	QuarkHLine {
		id: bottomLine
		x1: 0
		y1: bodyControl.y + bodyControl.height + spaceBottom_
		x2: parent.width
		y2: bodyControl.y + bodyControl.height + spaceBottom_
		penWidth: 1
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		visible: true
	}

	//
	// menus
	//

	QuarkPopupMenu {
		id: headerMenu
		x: parent.width - width - spaceRight_
		y: menuControl.y + menuControl.height + spaceItems_
		width: 150
		visible: false

		model: ListModel { id: menuModel }

		Component.onCompleted: prepare()

		onClick: {
			//
			if (key === "subscribe") {
				buzzerSubscribeCommand.process(buzzerName_);
			} else if (key === "unsubscribe") {
				buzzerUnsubscribeCommand.process(buzzerName_);
			} else if (key === "endorse") {
				buzzerEndorseCommand.process(buzzerName_);
			} else if (key === "mistrust") {
				buzzerMistrustCommand.process(buzzerName_);
			}
		}

		function prepare() {
			//
			menuModel.clear();

			menuModel.append({
				key: "endorse",
				keySymbol: Fonts.endorseSym,
				name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.endorse")});

			menuModel.append({
				key: "mistrust",
				keySymbol: Fonts.mistrustSym,
				name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.mistrust")});

			if (!buzzerClient.subscriptionExists(getBuzzerId())) {
				menuModel.append({
					key: "subscribe",
					keySymbol: Fonts.subscribeSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.subscribe")});
			} else {
				menuModel.append({
					key: "unsubscribe",
					keySymbol: Fonts.unsubscribeSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.unsubscribe")});
			}
		}

		function getBuzzerId() {
			//
			if (eventInfos_.length > 0) {
				return eventInfos_[0].buzzerId;
			}

			return publisherId_;
		}
	}

	BuzzerCommands.BuzzerSubscribeCommand {
		id: buzzerSubscribeCommand

		onProcessed: {
		}
		onError: {
			handleError(code, message);
		}
	}

	BuzzerCommands.BuzzerUnsubscribeCommand {
		id: buzzerUnsubscribeCommand

		onProcessed: {
		}
		onError: {
			handleError(code, message);
		}
	}

	BuzzerCommands.BuzzerEndorseCommand {
		id: buzzerEndorseCommand

		onProcessed: {
		}
		onError: {
			if (code === "E_CHAINS_ABSENT") return;
			if (message === "UNKNOWN_REFTX" || code == "E_TX_NOT_SENT") {
				buzzerClient.resync();
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"));
			} else {
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_ENDORSE"));
			}
		}
	}

	BuzzerCommands.BuzzerMistrustCommand {
		id: buzzerMistrustCommand

		onProcessed: {
		}
		onError: {
			if (code === "E_CHAINS_ABSENT") return;
			if (message === "UNKNOWN_REFTX" || code == "E_TX_NOT_SENT") {
				buzzerClient.resync();
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"));
			} else {
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_MISTRUST"));
			}
		}
	}

	function handleError(code, message) {
		if (code === "E_CHAINS_ABSENT") return;
		if (message === "UNKNOWN_REFTX" || code === "E_TX_NOT_SENT") {
			buzzerClient.resync();
			controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"));
		} else {
			controller_.showError(message);
		}
	}
}
