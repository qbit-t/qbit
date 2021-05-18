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
	id: buzzitembuzzerlight_

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
	readonly property real defaultFontSize: 11

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
	// buzzer state
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

		function getScore() {
			//
			if (eventInfos_.length > 0) {
				return eventInfos_[0].score;
			}

			return score_;
		}
	}

	//
	// buzzer avatar
	//

	Image {
		id: avatarImage

		x: spaceLeft_
		y: spaceTop_ // + headerInfo.getHeight()
		width: avatarImage.displayWidth
		height: avatarImage.displayHeight
		fillMode: Image.PreserveAspectCrop

		property bool rounded: true
		property int displayWidth: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 50 : 50
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
				controller_.openBuzzfeedByBuzzer(getBuzzerName());
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
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : defaultFontPointSize

		function getBuzzerAlias() {
			//
			if (eventInfos_.length > 0) {
				return buzzerClient.getBuzzerAlias(eventInfos_[0].buzzerInfoId);
			}

			return buzzerClient.getBuzzerAlias(publisherInfoId_);
		}
	}

	QuarkLabelRegular {
		id: buzzerNameControl
		x: buzzerAliasControl.x + buzzerAliasControl.width + spaceItems_
		y: avatarImage.y
		width: parent.width - x - (agoControl.width + spaceItems_ * 2 + menuControl.width + spaceItems_) - spaceRightMenu_
		elide: Text.ElideRight
		text: getBuzzerName()
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : defaultFontPointSize

		function getBuzzerName() {
			//
			if (eventInfos_.length > 0) {
				return buzzerClient.getBuzzerName(eventInfos_[0].buzzerInfoId);
			}

			return buzzerClient.getBuzzerName(publisherInfoId_);
		}
	}

	QuarkLabelRegular {
		id: buzzerIdControl
		x: buzzerAliasControl.x
		y: buzzerNameControl.y + buzzerNameControl.height + spaceItems_
		width: parent.width - (x + spaceRightMenu_)
		elide: Text.ElideRight

		text: /*"0x" +*/ getBuzzerId()
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : defaultFontPointSize
		//font.pointSize: 12

		function getBuzzerId() {
			if (eventInfos_.length > 0) {
				return eventInfos_[0].buzzerId;
			}

			return publisherId_;
		}
	}

	QuarkLabel {
		id: agoControl
		x: menuControl.x - width - spaceItems_ * 2
		y: avatarImage.y
		text: ago_
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - 2)) : (defaultFontPointSize - 2)
	}

	QuarkSymbolLabel {
		id: menuControl
		x: parent.width - width - spaceRightMenu_
		y: avatarImage.y
		symbol: Fonts.shevronDownSym
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - 7)) : 12
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
	}
	MouseArea {
		x: agoControl.x
		y: menuControl.y - spaceTop_
		width: agoControl.width + menuControl.width + spaceRightMenu_
		height: agoControl.height + spaceRightMenu_
		cursorShape: Qt.PointingHandCursor

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
	// description
	//

	QuarkLabel {
		id: descriptionText
		x: buzzerAliasControl.x
		y: buzzerIdControl.y + buzzerIdControl.height + spaceTop_
		width: parent.width - (x + spaceRightMenu_)
		text: getDescription()
		wrapMode: Text.Wrap
		textFormat: Text.RichText
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : defaultFontPointSize

		function getDescription() {
			//
			if (eventInfos_.length > 0) {
				return buzzerClient.getBuzzerDescription(eventInfos_[0].buzzerInfoId);
			}

			return buzzerClient.getBuzzerDescription(publisherInfoId_);
		}

		MouseArea {
			anchors.fill: parent
			cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
			acceptedButtons: Qt.NoButton
		}

		onLinkActivated: {
			//
			if (link[0] === '@') {
				// buzzer
				controller_.openBuzzfeedByBuzzer(link);
			} else if (link[0] === '#') {
				// tag
				controller_.openBuzzfeedByTag(link);
			} else {
				Qt.openUrlExternally(link);
			}
		}
	}

	//
	// bottom line
	//

	QuarkHLine {
		id: bottomLine
		x1: 0
		y1: descriptionText.y + descriptionText.height + spaceBottom_
		x2: parent.width
		y2: descriptionText.y + descriptionText.height + spaceBottom_
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
		width:  buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 150) : 150
		visible: false

		model: ListModel { id: menuModel }

		Component.onCompleted: prepare()

		onClick: {
			//
			if (key === "subscribe") {
				buzzerSubscribeCommand.process(buzzerNameControl.getBuzzerName());
			} else if (key === "unsubscribe") {
				buzzerUnsubscribeCommand.process(buzzerNameControl.getBuzzerName());
			} else if (key === "endorse") {
				buzzerEndorseCommand.process(buzzerNameControl.getBuzzerName());
			} else if (key === "mistrust") {
				buzzerMistrustCommand.process(buzzerNameControl.getBuzzerName());
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

			if (!buzzerClient.subscriptionExists(buzzerIdControl.getBuzzerId())) {
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
