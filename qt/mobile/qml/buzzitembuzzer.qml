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
	id: buzzeritem_

	property int calculatedHeight: calculateHeightInternal()

	property var buzzer_
	property var endorsements_
	property var mistrusts_
	property var score_
	property var realScore_
	property var alias_
	property var description_
	property var buzzerId_
	property var controller_
	property var buzzfeedModel_
	property var listView_
	property int following_: 0
	property int followers_: 0

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

	function initialize(buzzer) {
		buzzer_ = buzzer;
		infoLoaderCommand.process(buzzer_);
	}

	function calculateHeightInternal() {
		return bottomLine.y1;
	}

	function calculateHeight() {
		calculatedHeight = calculateHeightInternal();
		return calculatedHeight;
	}

	//
	// buzzer info loader
	//
	BuzzerCommands.LoadBuzzerInfoCommand {
		id: infoLoaderCommand

		onProcessed: {
			// name
			alias_ = infoLoaderCommand.alias;
			description_ = buzzerClient.decorateBuzzBody(infoLoaderCommand.description);
			buzzerId_ = infoLoaderCommand.buzzerId;

			trustScoreLoader.process(infoLoaderCommand.buzzerId + "/" + infoLoaderCommand.buzzerChainId);

			avatarDownloadCommand.url = infoLoaderCommand.avatarUrl;
			avatarDownloadCommand.localFile = buzzerClient.getTempFilesPath() + "/" + infoLoaderCommand.avatarId;
			avatarDownloadCommand.process();

			headerDownloadCommand.url = infoLoaderCommand.headerUrl;
			headerDownloadCommand.localFile = buzzerClient.getTempFilesPath() + "/" + infoLoaderCommand.headerId;
			headerDownloadCommand.process();
		}

		onError: {
			handleError(code, message);
		}
	}

	//
	// buzzer score loader
	//
	BuzzerCommands.LoadBuzzerTrustScoreCommand {
		id: trustScoreLoader

		onProcessed: {
			// endorsements, mistrusts, following, followers
			endorsements_ = endorsements - buzzerClient.getTrustScoreBase();
			mistrusts_ = mistrusts - buzzerClient.getTrustScoreBase();
			score_ = endorsements / mistrusts;

			following_ = following;
			followers_ = followers;

			if (endorsements > mistrusts) realScore_ = endorsements - mistrusts;
			else realScore_ = 0;
		}

		onError: {
			handleError(code, message);
		}
	}

	//
	// avatar download
	//

	BuzzerCommands.DownloadMediaCommand {
		id: avatarDownloadCommand
		preview: true // TODO: consider to use full image
		skipIfExists: true

		onProcessed: {
			// tx, previewFile, originalFile
			avatarImage.source = "file://" + previewFile
		}

		onError: {
			handleError(code, message);
		}
	}

	//
	// header download
	//

	BuzzerCommands.DownloadMediaCommand {
		id: headerDownloadCommand
		preview: true // TODO: consider to use full image
		skipIfExists: true

		onProcessed: {
			// tx, previewFile, originalFile
			headerImage.source = "file://" + previewFile
		}

		onError: {
			handleError(code, message);
		}
	}

	//
	// header info
	//

	onWidthChanged: {
		//headerImage.adjust();
	}

	Rectangle {
		id: headerContainer
		x: 0
		y: 0
		width: parent.width
		height: 250
		color: "transparent"
		clip: true

		Image {
			id: headerImage

			width: parent.width

			onStatusChanged: {
				//
				if (status == Image.Ready) {
					//
					if (is54()) {
						sourceClipRect = Qt.rect(getX(), getY(), getWidth(), parent.height);
					} else {
						fillMode = Image.PreserveAspectCrop;
						height = parent.height;
					}
				}
			}

			function is54() {
				// 5:4 ratio
				var lCoeff = parseFloat(sourceSize.width) / parseFloat(sourceSize.height);
				return sourceSize.width > sourceSize.height &&
						lCoeff - 1.3333 < 0.001 &&
						lCoeff - 1.3333 >= 0.0;
			}

			function getX() {
				if (is54()) {
					var lDelta = (sourceSize.width - parent.width) / 2;
					return lDelta;
				}

				return 0;
			}

			function getY() {
				if (is54() /*&&
						parent.height < height*/) {
					return sourceSize.height / 2 - height / 2;
				}

				return 0;
			}

			function getWidth() {
				if (is54()) {
					var lDelta = (sourceSize.width - parent.width);
					console.log("[getWidth]: " + (sourceSize.width - lDelta));
					return sourceSize.width - lDelta;
				}

				return sourceSize.width;
			}

			autoTransform: true
		}
	}

	QuarkRoundState {
		id: imageFrame
		x: avatarImage.x - 3
		y: avatarImage.y - 3
		penWidth: 4
		size: avatarImage.displayWidth + 6
		color: getColor()
		background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background")

		function getColor() {
			var lScoreBase = buzzerClient.getTrustScoreBase() / 10;
			var lIndex = realScore_ / lScoreBase;

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
	}

	Image {
		id: avatarImage

		x: spaceLeft_
		y: headerImage.y + headerImage.height - height / 2 + getY()
		width: avatarImage.displayWidth
		height: avatarImage.displayHeight
		fillMode: Image.PreserveAspectCrop

		property bool rounded: true
		property int displayWidth: 120
		property int displayHeight: displayWidth

		function getY() {
			// console.log("headerImage.y = " + headerImage.y + ", headerImage.paintedHeight = " + headerImage.paintedHeight + ", headerImage.height = " + headerImage.height);
			return 0;
		}

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
	}

	QuarkToolButton {
		id: subscribeControl
		x: parent.width - width - spaceItems_
		y: headerImage.height + spaceTop_
		Layout.alignment: Qt.AlignHCenter

		spacing: 10
		padding: 18

		visible: buzzerId_ !== buzzerClient.getCurrentBuzzerId()

		text: !buzzerClient.subscriptionExists(buzzerId_) ? buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.subscribe") :
															buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.unsubscribe")
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")

		onClicked: {
			if (!buzzerClient.subscriptionExists(buzzerId_)) {
				buzzerSubscribeCommand.process(buzzer_);
			} else {
				buzzerUnsubscribeCommand.process(buzzer_);
			}

			checkSubscriptionState.start();
		}

		Timer {
			id: checkSubscriptionState
			interval: 500
			repeat: false
			running: false

			onTriggered: {
				var lText = subscribeControl.text;
				subscribeControl.text = !buzzerClient.subscriptionExists(buzzerId_) ?
							buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.subscribe") :
							buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.unsubscribe");
				if (lText !== subscribeControl.text) {
					checkSubscriptionState.start();
				}
			}
		}
	}

	//
	// buzzer info
	//

	QuarkLabel {
		id: buzzerAliasControl
		x: spaceLeft_
		y: avatarImage.y + avatarImage.displayHeight + spaceTop_
		width: parent.width - (x + spaceRight_)
		elide: Text.ElideRight
		text: alias_
		font.bold: true
	}

	QuarkLabel {
		id: buzzerNameControl
		x: spaceLeft_
		y: buzzerAliasControl.y + buzzerAliasControl.height + spaceItems_
		width: parent.width - (x + spaceRight_)
		elide: Text.ElideRight
		text: buzzer_
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
	}

	QuarkLabel {
		id: buzzerIdControl
		x: spaceLeft_
		y: buzzerNameControl.y + buzzerNameControl.height + spaceItems_
		width: parent.width - (x + spaceRight_ + copySymbol.width + spaceItems_)
		elide: Text.ElideRight

		text: /*"0x" +*/ buzzerId_
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
		//font.pointSize: 12
	}

	QuarkSymbolLabel {
		id: copySymbol
		x: parent.width - (spaceRight_ + width)
		y: buzzerIdControl.y - 2
		symbol: Fonts.clipboardSym
		// color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		//font.pointSize: 14

		MouseArea {
			x: parent.x - spaceItems_
			y: parent.y - spaceItems_
			width: parent.width + 2 * spaceItems_
			height: parent.height + 2 * spaceItems_

			onClicked: {
				clipboard.setText(buzzerId_);
			}
		}
	}

	QuarkToolButton {
		id: menuControl
		x: parent.width - width - spaceItems_
		y: spaceItems_
		symbol: Fonts.elipsisVerticalSym
		visible: true
		labelYOffset: 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter
		opacity: 0.7

		onClicked: {
			if (buzzerId_ === buzzerClient.getCurrentBuzzerId()) return; // no actions
			if (headerMenu.visible) headerMenu.close();
			else { headerMenu.prepare(); headerMenu.open(); }
		}
	}

	//
	// trust score
	//

	QuarkNumberLabel {
		id: scoreControl
		font.pointSize: 24
		visible: true
		fillTo: 8
		useSign: false
		x: spaceLeft_
		y: buzzerIdControl.y + buzzerIdControl.height + spaceTop_
		number: score_ ? parseFloat(score_) : 0.0

		numberColor: imageFrame.getColor()
		mantissaColor: imageFrame.getColor()
		zeroesColor: imageFrame.getColor()
		unitsColor: imageFrame.getColor()
	}

	// endorsements
	QuarkSymbolLabel {
		id: endorseSymbol
		x: spaceLeft_
		y: scoreControl.y + scoreControl.height + spaceItems_
		// font.pointSize: 12
		symbol: Fonts.endorseSym
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.4")
	}

	QuarkLabel {
		id: endorsementsNumber
		//font.pointSize: 12
		visible: true
		//fillTo: 0
		//useSign: false
		x: endorseSymbol.x + endorseSymbol.width + spaceItems_ + Math.max(endorsementsNumber.width, mistrustsNumber.width) - width
		y: endorseSymbol.y
		text: endorsements_ === undefined ? "00000000" : "" + endorsements_
	}

	QuarkSymbolLabel {
		id: expandEndorsementsSymbol
		x: endorsementsNumber.x + endorsementsNumber.width + spaceItems_
		y: endorseSymbol.y
		symbol: Fonts.externalLinkSym
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		font.pointSize: 14
	}

	MouseArea {
		id: endorsementsClick
		x: endorsementsNumber.x
		y: endorsementsNumber.y - spaceItems_
		width: expandEndorsementsSymbol.x + expandEndorsementsSymbol.width
		height: endorsementsNumber.height + 2 * spaceItems_

		onClicked: {
			//
			var lComponent = null;
			var lPage = null;

			lComponent = Qt.createComponent("qrc:/qml/buzzfeedendorsements.qml");
			if (lComponent.status === Component.Error) {
				showError(lComponent.errorString());
			} else {
				lPage = lComponent.createObject(controller);
				lPage.controller = controller;
				lPage.start(buzzer_);
				addPage(lPage);
			}
		}
	}

	// mistrusts
	QuarkSymbolLabel {
		id: mistrustSymbol
		x: spaceLeft_
		y: endorseSymbol.y + endorseSymbol.height + spaceItems_ + 5
		symbol: Fonts.mistrustSym
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.0")
	}

	QuarkLabel {
		id: mistrustsNumber
		//font.pointSize: 12
		visible: true
		//fillTo: 0
		//useSign: false
		x: mistrustSymbol.x + mistrustSymbol.width + spaceItems_ + Math.max(endorsementsNumber.width, mistrustsNumber.width) - width
		y: mistrustSymbol.y
		text: mistrusts_ === undefined ? "00000000" : "" + mistrusts_
	}

	QuarkSymbolLabel {
		id: expandMistrustsSymbol
		x: mistrustsNumber.x + mistrustsNumber.width + spaceItems_ + 5
		y: mistrustSymbol.y
		symbol: Fonts.externalLinkSym
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		font.pointSize: 14
	}

	MouseArea {
		id: mistrustsClick
		x: mistrustsNumber.x
		y: mistrustsNumber.y - spaceItems_
		width: expandMistrustsSymbol.x + expandMistrustsSymbol.width
		height: mistrustsNumber.height + 2 * spaceItems_

		onClicked: {
			//
			var lComponent = null;
			var lPage = null;

			lComponent = Qt.createComponent("qrc:/qml/buzzfeedmistrusts.qml");
			if (lComponent.status === Component.Error) {
				showError(lComponent.errorString());
			} else {
				lPage = lComponent.createObject(controller);
				lPage.controller = controller;
				lPage.start(buzzer_);
				addPage(lPage);
			}
		}
	}

	//
	// description
	//

	QuarkLabel {
		id: descriptionText
		x: spaceLeft_
		y: mistrustSymbol.y + mistrustSymbol.height + spaceTop_
		width: parent.width - (spaceLeft_ + spaceRight_)
		text: description_
		wrapMode: Text.Wrap
		textFormat: Text.RichText

		onLinkActivated: {
			var lComponent = null;
			var lPage = null;
			//
			if (link[0] === '@') {
				// buzzer
				lComponent = Qt.createComponent("qrc:/qml/buzzfeedbuzzer.qml");
				if (lComponent.status === Component.Error) {
					showError(lComponent.errorString());
				} else {
					lPage = lComponent.createObject(controller);
					lPage.controller = controller;
					lPage.start(link);
					addPage(lPage);
				}
			} else if (link[0] === '#') {
				// tag
				lComponent = Qt.createComponent("qrc:/qml/buzzfeedtag.qml");
				if (lComponent.status === Component.Error) {
					showError(lComponent.errorString());
				} else {
					lPage = lComponent.createObject(controller);
					lPage.controller = controller;
					lPage.start(link);
					addPage(lPage);
				}
			} else {
				Qt.openUrlExternally(link);
			}
		}
	}

	//
	// following
	//

	QuarkLabel {
		id: followingControl
		x: spaceLeft_
		y: descriptionText.y + descriptionText.height + spaceTop_
		text: NumberFunctions.numberToCompact(following_).toString()
		// color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
	}

	QuarkLabel {
		id: followingText
		x: followingControl.x + followingControl.width + spaceItems_
		y: followingControl.y
		text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.following")
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
	}

	MouseArea {
		id: followingClick
		x: followingControl.x
		y: followingControl.y - spaceItems_
		width: followingText.x + followingText.width
		height: followingText.height + 2 * spaceItems_

		onClicked: {
			//
			var lComponent = null;
			var lPage = null;

			lComponent = Qt.createComponent("qrc:/qml/buzzfeedfollowing.qml");
			if (lComponent.status === Component.Error) {
				showError(lComponent.errorString());
			} else {
				lPage = lComponent.createObject(controller);
				lPage.controller = controller;
				lPage.start(buzzer_);
				addPage(lPage);
			}
		}
	}

	//
	// followers
	//

	QuarkLabel {
		id: followersControl
		x: followingText.x + followingText.width + spaceLeft_
		y: followingControl.y
		text: NumberFunctions.numberToCompact(followers_).toString()
		// color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
	}

	QuarkLabel {
		id: followersText
		x: followersControl.x + followersControl.width + spaceItems_
		y: followingControl.y
		text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.followers")
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
	}

	MouseArea {
		id: followersClick
		x: followersControl.x
		y: followersControl.y - spaceItems_
		width: followersText.x + followersText.width
		height: followersText.height + 2 * spaceItems_

		onClicked: {
			//
			var lComponent = null;
			var lPage = null;

			lComponent = Qt.createComponent("qrc:/qml/buzzfeedfollowers.qml");
			if (lComponent.status === Component.Error) {
				showError(lComponent.errorString());
			} else {
				lPage = lComponent.createObject(controller);
				lPage.controller = controller;
				lPage.start(buzzer_);
				addPage(lPage);
			}
		}
	}

	//
	// bottom line
	//

	QuarkHLine {
		id: bottomLine
		x1: 0
		y1: followingControl.y + followingControl.height + spaceBottom_
		x2: parent.width
		y2: followingControl.y + followingControl.height + spaceBottom_
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
				buzzerSubscribeCommand.process(buzzer_);
			} else if (key === "unsubscribe") {
				buzzerUnsubscribeCommand.process(buzzer_);
			} else if (key === "endorse") {
				buzzerEndorseCommand.process(buzzer_);
			} else if (key === "mistrust") {
				buzzerMistrustCommand.process(buzzer_);
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
