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
	id: buzzendorse_

	property int calculatedHeight: calculateHeightInternal()

	property var type_: type
	property var buzzId_: buzzId
	property var buzzChainId_: buzzChainId
	property var timestamp_: timestamp
	property var ago_: ago
	property var score_: score
	property var buzzerId_: buzzerId
	property var buzzerInfoId_: buzzerInfoId
	property var buzzerInfoChainId_: buzzerInfoChainId
	property var buzzBody_: buzzBody
	property var buzzMedia_: buzzMedia
	property var replies_: replies
	property var rebuzzes_: rebuzzes
	property var likes_: likes
	property var rewards_: rewards
	property var originalBuzzId_: originalBuzzId
	property var originalBuzzChainId_: originalBuzzChainId
	property var hasParent_: hasParent
	property var value_: value
	property var buzzInfos_: buzzInfos
	property var buzzerName_: buzzerName
	property var buzzerAlias_: buzzerAlias
	property var childrenCount_: childrenCount
	property var hasPrevSibling_: hasPrevSibling
	property var hasNextSibling_: hasNextSibling
	property var prevSiblingId_: prevSiblingId
	property var nextSiblingId_: nextSiblingId
	property var firstChildId_: firstChildId
	property var wrapped_: wrapped
	property var childLink_: false
	property var parentLink_: false
	property var self_: self
	property var rootId_
	property bool mistrusted_: false
	property bool endorsed_: false

	property var controller_: controller
	property var buzzfeedModel_: buzzfeedModel
	property var listView_

	property var publisherScore_: 0;
	property var publisherBuzzerId_: buzzInfos_[0].buzzerId;
	property var publisherBuzzerInfoId_: buzzInfos_[0].buzzerInfoId;
	property var publisherBuzzerInfoChainId_: buzzInfos_[0].buzzerInfoChainId;

	readonly property int spaceLeft_: 15
	readonly property int spaceTop_: 12
	readonly property int spaceRight_: 15
	readonly property int spaceBottom_: 12
	readonly property int spaceAvatarBuzz_: 10
	readonly property int spaceItems_: 5
	readonly property int spaceHeader_: 5
	readonly property int spaceRightMenu_: 15
	readonly property int spaceStats_: -5
	readonly property int spaceLine_: 4

	signal calculatedHeightModified(var value);

	onCalculatedHeightChanged: {
		calculatedHeightModified(value);
	}

	Component.onCompleted: {
		avatarDownloadCommand.process();
		loadTrustScoreCommand.process(publisherBuzzerId_ + "/" + publisherBuzzerInfoChainId_);
	}

	function calculateHeightInternal() {
		var lCalculatedInnerHeight = spaceTop_ + headerInfo.getHeight() + spaceHeader_ + buzzerAliasControl.height +
										bodyControl.height + spaceItems_ + spaceItems_ + 1;
		return lCalculatedInnerHeight > avatarImage.displayHeight ?
										   lCalculatedInnerHeight : avatarImage.displayHeight;
	}

	/*
	function initialize() {
		//
		avatarDownloadCommand.url = buzzerClient.getBuzzerAvatarUrl(publisherBuzzerInfoId_)
		avatarDownloadCommand.localFile = buzzerClient.getTempFilesPath() + "/" + buzzerClient.getBuzzerAvatarUrlHeader(buzzerInfoId_)
		avatarDownloadCommand.process();
	}
	*/

	function calculateHeight() {
		calculatedHeight = calculateHeightInternal();
		return calculatedHeight;
	}

	//
	// avatar download
	//

	BuzzerCommands.DownloadMediaCommand {
		id: avatarDownloadCommand
		url: buzzerClient.getBuzzerAvatarUrl(publisherBuzzerInfoId_)
		localFile: buzzerClient.getTempFilesPath() + "/" + buzzerClient.getBuzzerAvatarUrlHeader(publisherBuzzerInfoId_)
		preview: true
		skipIfExists: true

		onProcessed: {
			// tx, previewFile, originalFile
			avatarImage.source = "file://" + previewFile
		}
	}

	//
	// header info
	//
	Rectangle {
		id: headerInfo
		color: "transparent"
		x: 0
		y: spaceTop_
		width: parent.width
		height: actionText.height + spaceHeader_ - 2
		visible: getVisible()

		function getVisible() {
			return type_ === buzzerClient.tx_BUZZER_ENDORSE_TYPE() ||
					type_ === buzzerClient.tx_BUZZER_MISTRUST_TYPE();
		}

		function getHeight() {
			if (getVisible()) return height;
			return 0;
		}

		QuarkSymbolLabel {
			id: actionSymbol
			x: avatarImage.x + avatarImage.width - width
			y: -1
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			font.pointSize: 12
			symbol: getSymbol()

			function getSymbol() {
				if (type_ === buzzerClient.tx_BUZZER_ENDORSE_TYPE()) {
					return Fonts.endorseSym;
				} else if (type_ === buzzerClient.tx_BUZZER_MISTRUST_TYPE()) {
					return Fonts.mistrustSym;
				}

				return "";
			}
		}

		QuarkLabelRegular {
			id: actionText
			x: avatarImage.x + avatarImage.width + spaceAvatarBuzz_
			y: -2
			font.pointSize: 12
			width: parent.width - x - spaceRight_
			elide: Text.ElideRight
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			text: getText()

			function getText() {
				var lInfo;
				var lString;
				var lResult;

				if (type_ === buzzerClient.tx_BUZZER_ENDORSE_TYPE()) {
					lString = buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.buzz.endorse.one");
					return lString.replace("{alias}", buzzerAlias_);
				} else if (type_ === buzzerClient.tx_BUZZER_MISTRUST_TYPE()) {
					lString = buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.buzz.mistrust.one");
					return lString.replace("{alias}", buzzerAlias_);
				}

				return "";
			}
		}
	}

	QuarkRoundState {
		id: imageFrame
		x: avatarImage.x - 2
		y: avatarImage.y - 2
		size: avatarImage.displayWidth + 4
		color: getColor()
		background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background")

		function getColor() {
			var lScoreBase = buzzerClient.getTrustScoreBase() / 10;
			var lIndex = publisherScore_ / lScoreBase;

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

		x: spaceLeft_
		y: spaceTop_ + headerInfo.getHeight()
		width: avatarImage.displayWidth
		height: avatarImage.displayHeight
		fillMode: Image.PreserveAspectCrop

		property bool rounded: true
		property int displayWidth: 50
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
					lPage.start(buzzerClient.resolveBuzzerName(publisherBuzzerInfoId_));
					addPage(lPage);
				}
			}
		}
	}

	QuarkSymbolLabel {
		id: endorseSymbol
		x: avatarImage.x + avatarImage.displayWidth / 2 - width / 2
		y: avatarImage.y + avatarImage.displayHeight + spaceItems_
		symbol: endorsed_ ? Fonts.endorseSym : Fonts.mistrustSym
		font.pointSize: 14
		color: endorsed_ ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.endorsed") :
						   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.mistrusted");

		visible: endorsed_ || mistrusted_
	}

	//
	// header
	//

	QuarkLabel {
		id: buzzerAliasControl
		x: avatarImage.x + avatarImage.width + spaceAvatarBuzz_
		y: avatarImage.y
		text: buzzerClient.resolveBuzzerAlias(publisherBuzzerInfoId_)
		font.bold: true
	}

	QuarkLabelRegular {
		id: buzzerNameControl
		x: buzzerAliasControl.x + buzzerAliasControl.width + spaceItems_
		y: avatarImage.y
		width: parent.width - x - (agoControl.width + spaceItems_ * 2 + menuControl.width + spaceItems_) - spaceRightMenu_
		elide: Text.ElideRight
		text: buzzerClient.resolveBuzzerName(publisherBuzzerInfoId_)
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
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
			//
			if (headerMenu.visible) headerMenu.close();
			else { headerMenu.prepare(); headerMenu.open(); }
		}
	}

	//
	// body
	//

	Rectangle {
		id: bodyControl
		x: avatarImage.x + avatarImage.width + spaceAvatarBuzz_
		y: buzzerAliasControl.y + buzzerAliasControl.height + spaceHeader_
		width: parent.width - (avatarImage.x + avatarImage.width + spaceAvatarBuzz_) - spaceRight_
		height: buzzerIdControl.height + spaceItems_ + scoreControl.height + spaceItems_ + buzzText.height
		clip: true

		border.color: "transparent"
		color: "transparent"

		QuarkLabelRegular {
			id: buzzerIdControl
			x: 0
			y: 0
			width: parent.width - spaceRight_
			elide: Text.ElideRight

			text: /*"0x" +*/ publisherBuzzerId_
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
			font.pointSize: 12
		}

		QuarkNumberLabel {
			id: scoreControl
			font.pointSize: 24
			visible: true
			fillTo: 1
			useSign: true
			sign: type_ === buzzerClient.tx_BUZZER_ENDORSE_TYPE() ? "+" : "-"
			x: 0
			y: buzzerIdControl.y + buzzerIdControl.height + spaceItems_
			number: value_ / buzzerClient.getTrustScoreBase()

			numberColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector,
								type_ === buzzerClient.tx_BUZZER_ENDORSE_TYPE() ? "Buzzer.endorsed" : "Buzzer.mistrusted")
			mantissaColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector,
								type_ === buzzerClient.tx_BUZZER_ENDORSE_TYPE() ? "Buzzer.endorsed" : "Buzzer.mistrusted")
			zeroesColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.endorsed",
								type_ === buzzerClient.tx_BUZZER_ENDORSE_TYPE() ? "Buzzer.endorsed" : "Buzzer.mistrusted")
			unitsColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.endorsed",
								type_ === buzzerClient.tx_BUZZER_ENDORSE_TYPE() ? "Buzzer.endorsed" : "Buzzer.mistrusted")
		}

		QuarkText {
			id: buzzText
			x: 0
			y: scoreControl.y + scoreControl.height + spaceItems_
			width: parent.width
			text: buzzerClient.resolveBuzzerDescription(publisherBuzzerInfoId_)
			wrapMode: Text.Wrap

			/*
			TextMetrics {
				id: buzzBodyMetrics
				font.family: buzzText.font.family

				text: buzzBody
			}
			*/
		}
	}

	QuarkHLine {
		id: bottomLine
		x1: 0
		y1: bodyControl.y + bodyControl.height + spaceItems_ + spaceItems_
		x2: parent.width
		y2: bodyControl.y + bodyControl.height + spaceItems_ + spaceItems_
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
				buzzerSubscribeCommand.process(buzzerClient.resolveBuzzerName(publisherBuzzerInfoId_));
			} else if (key === "unsubscribe") {
				buzzerUnsubscribeCommand.process(buzzerClient.resolveBuzzerName(publisherBuzzerInfoId_));
			} else if (key === "endorse") {
				buzzerEndorseCommand.process(buzzerClient.resolveBuzzerName(publisherBuzzerInfoId_));
			} else if (key === "mistrust") {
				buzzerMistrustCommand.process(buzzerClient.resolveBuzzerName(publisherBuzzerInfoId_));
			} else if (key === "copytx") {
				clipboard.setText(buzzId_);
			}
		}

		function prepare() {
			//
			menuModel.clear();

			//
			if (buzzerId_ !== buzzerClient.getCurrentBuzzerId()) {
				menuModel.append({
					key: "endorse",
					keySymbol: Fonts.endorseSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.endorse")});

				menuModel.append({
					key: "mistrust",
					keySymbol: Fonts.mistrustSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.mistrust")});

				if (!buzzerClient.subscriptionExists(buzzerId_)) {
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

			menuModel.append({
				key: "copytx",
				keySymbol: Fonts.clipboardSym,
				name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.copytransaction")});
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
			endorsed_ = true;
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
			mistrusted_ = true;
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

	BuzzerCommands.LoadBuzzerTrustScoreCommand {
		id: loadTrustScoreCommand
		onProcessed: {
			if (endorsements > mistrusts) {
				publisherScore_ = endorsements - mistrusts;
			} else {
				publisherScore_ = 0;
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
