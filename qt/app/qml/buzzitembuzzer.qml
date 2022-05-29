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
	property var buzzerInfoId_
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
	readonly property real defaultFontSize: 11

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
			buzzerInfoId_ = infoLoaderCommand.infoId;

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

			imageFrame.visible = true;
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
			if (originalFile === "") avatarImage.source = "file://" + previewFile;
			else avatarImage.source = "file://" + originalFile;
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
			if (originalFile === "") headerImage.source = "file://" + previewFile;
			else headerImage.source = "file://" + originalFile;
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
					/*
					if (is54()) {
						sourceClipRect = Qt.rect(getX(), getY(), getWidth(), parent.height);
					} else {
						fillMode = Image.PreserveAspectCrop;
						height = parent.height;
					}
					*/
					fillMode = Image.PreserveAspectCrop;
					height = parent.height;
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
		visible: false

		function getColor() {
			var lScoreBase = buzzerClient.getTrustScoreBase() / 10;
			var lIndex = realScore_ / lScoreBase;

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
		y: headerContainer.y + headerContainer.height - height / 2 + getY() // headerImage.y + headerImage.height - height / 2 + getY()
		width: avatarImage.displayWidth
		height: avatarImage.displayHeight
		fillMode: Image.PreserveAspectCrop
		mipmap: true

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
		y: headerContainer.height + spaceTop_ // headerImage.height + spaceTop_
		Layout.alignment: Qt.AlignHCenter
		symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 14) : symbolFontPointSize

		//spacing: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 5) : 10
		//padding: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 9) : 18
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
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 3)) : (defaultFontPointSize + 2)
	}

	QuarkLabelRegular {
		id: buzzerNameControl
		x: spaceLeft_
		y: buzzerAliasControl.y + buzzerAliasControl.height + spaceItems_
		width: parent.width - (x + spaceRight_)
		elide: Text.ElideRight
		text: buzzer_
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 3)) : (defaultFontPointSize + 2)
	}
	MouseArea {
		x: buzzerNameControl.x - spaceItems_
		y: buzzerNameControl.y - spaceItems_
		width: buzzerNameControl.width + 2 * spaceItems_
		height: buzzerNameControl.height + 2 * spaceItems_
		cursorShape: Qt.PointingHandCursor

		onClicked: {
			clipboard.setText(buzzer_);
		}
	}

	QuarkLabelRegular {
		id: buzzerIdControl
		x: spaceLeft_
		y: buzzerNameControl.y + buzzerNameControl.height + spaceItems_
		width: parent.width - (x + spaceRight_ + copySymbol.width + spaceItems_)
		elide: Text.ElideRight

		text: /*"0x" +*/ buzzerId_
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
		//font.pointSize: 12
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 3)) : (defaultFontPointSize + 2)
	}
	MouseArea {
		x: buzzerIdControl.x - spaceItems_
		y: buzzerIdControl.y - spaceItems_
		width: buzzerIdControl.width + 2 * spaceItems_
		height: buzzerIdControl.height + 2 * spaceItems_
		cursorShape: Qt.PointingHandCursor

		onClicked: {
			clipboard.setText(buzzerInfoId_);
		}
	}

	QuarkSymbolLabel {
		id: copySymbol
		x: parent.width - (spaceRight_ + width)
		y: buzzerIdControl.y - buzzerClient.scaleFactor * 2
		symbol: Fonts.externalLinkSym
		// color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		//font.pointSize: 14
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzeritem_.defaultFontSize + 3)) : defaultFontSize
	}
	MouseArea {
		x: copySymbol.x - spaceItems_
		y: copySymbol.y - spaceItems_
		width: copySymbol.width + 2 * spaceItems_
		height: copySymbol.height + 2 * spaceItems_
		cursorShape: Qt.PointingHandCursor

		onClicked: {
			// clipboard.setText(buzzerId_);
			var lUrl = buzzerApp.getExploreTxRaw();
			lUrl = lUrl.replace("{tx}", buzzerId_);
			Qt.openUrlExternally(lUrl);
		}
	}

	QuarkToolButton {
		id: menuControl
		x: parent.width - width - (buzzerApp.isDesktop ? 0 : spaceItems_)
		y: spaceItems_
		symbol: buzzerApp.isDesktop ? Fonts.shevronDownSym : Fonts.elipsisVerticalSym
		visible: true
		labelYOffset: /*buzzerApp.isDesktop ? 0 :*/ 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter
		opacity: 0.7
		symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 14) : symbolFontPointSize

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
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 18) : 24
		visible: true
		fillTo: 8
		useSign: false
		x: spaceLeft_
		y: buzzerIdControl.y + buzzerIdControl.height + (buzzerApp.isDesktop ? (buzzerClient.scaleFactor * spaceTop_) : spaceTop_)
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
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzeritem_.defaultFontSize + 3)) : endorseSymbol.defaultFontSize
	}

	QuarkLabel {
		id: endorsementsNumber
		//font.pointSize: 12
		visible: true
		//fillTo: 0
		//useSign: false
		x: endorseSymbol.x + endorseSymbol.width + buzzerClient.scaleFactor * spaceItems_ + Math.max(endorsementsNumber.width, mistrustsNumber.width) - width
		y: endorseSymbol.y + endorseSymbol.height / 2 - height / 2 + buzzerClient.scaleFactor * 3
		text: endorsements_ === undefined ? "00000000" : "" + endorsements_
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 3)) : (defaultFontPointSize + 2)
	}

	QuarkSymbolLabel {
		id: expandEndorsementsSymbol
		x: endorsementsNumber.x + endorsementsNumber.width + buzzerClient.scaleFactor * spaceItems_
		y: endorseSymbol.y + buzzerClient.scaleFactor * 1
		symbol: Fonts.externalLinkSym
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzeritem_.defaultFontSize + 2)) : expandEndorsementsSymbol.defaultFontSize
	}

	MouseArea {
		id: endorsementsClick
		x: endorsementsNumber.x
		y: endorsementsNumber.y - spaceItems_
		width: expandEndorsementsSymbol.x + expandEndorsementsSymbol.width
		height: endorsementsNumber.height + 2 * spaceItems_
		cursorShape: Qt.PointingHandCursor

		onClicked: {
			//
			controller_.openBuzzfeedBuzzerEndorsements(buzzer_);
		}
	}

	// mistrusts
	QuarkSymbolLabel {
		id: mistrustSymbol
		x: spaceLeft_
		y: endorseSymbol.y + endorseSymbol.height + buzzerClient.scaleFactor * spaceItems_ + buzzerClient.scaleFactor * 5
		symbol: Fonts.mistrustSym
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.0")
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzeritem_.defaultFontSize + 3)) : mistrustSymbol.defaultFontSize
	}

	QuarkLabel {
		id: mistrustsNumber
		//font.pointSize: 12
		visible: true
		//fillTo: 0
		//useSign: false
		x: mistrustSymbol.x + mistrustSymbol.width + (buzzerClient.scaleFactor * spaceItems_) + Math.max(endorsementsNumber.width, mistrustsNumber.width) - width
		y: mistrustSymbol.y + mistrustSymbol.height / 2 - height / 2 - buzzerClient.scaleFactor * 2
		text: mistrusts_ === undefined ? "00000000" : "" + mistrusts_
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 3)) : (defaultFontPointSize + 2)
	}

	QuarkSymbolLabel {
		id: expandMistrustsSymbol
		x: mistrustsNumber.x + mistrustsNumber.width + (buzzerClient.scaleFactor * spaceItems_)
		y: mistrustSymbol.y - buzzerClient.scaleFactor * 2
		symbol: Fonts.externalLinkSym
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzeritem_.defaultFontSize + 2)) : expandMistrustsSymbol.defaultFontSize
	}

	MouseArea {
		id: mistrustsClick
		x: mistrustsNumber.x
		y: mistrustsNumber.y - spaceItems_
		width: expandMistrustsSymbol.x + expandMistrustsSymbol.width
		height: mistrustsNumber.height + 2 * spaceItems_
		cursorShape: Qt.PointingHandCursor

		onClicked: {
			//
			controller_.openBuzzfeedBuzzerMistrusts(buzzer_);
		}
	}

	//
	// description
	//

	QuarkLabel {
		id: descriptionText
		x: spaceLeft_
		y: mistrustSymbol.y + mistrustSymbol.height + (buzzerApp.isDesktop ? (buzzerClient.scaleFactor * spaceTop_) : spaceTop_)
		width: parent.width - (spaceLeft_ + spaceRight_)
		text: description_
		wrapMode: Text.Wrap
		textFormat: Text.RichText
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 4)) : (defaultFontPointSize + 3)

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
	// following
	//

	QuarkLabel {
		id: followingControl
		x: spaceLeft_
		y: descriptionText.y + descriptionText.height + spaceTop_
		text: NumberFunctions.numberToCompact(following_).toString()
		// color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 3)) : (defaultFontPointSize + 2)
	}

	QuarkLabel {
		id: followingText
		x: followingControl.x + followingControl.width + spaceItems_
		y: followingControl.y
		text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.following")
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 3)) : (defaultFontPointSize + 2)
	}

	MouseArea {
		id: followingClick
		x: followingControl.x
		y: followingControl.y - spaceItems_
		width: followingText.x + followingText.width
		height: followingText.height + 2 * spaceItems_

		onClicked: {
			//
			controller_.openBuzzfeedBuzzerFollowing(buzzer_);
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
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 3)) : (defaultFontPointSize + 2)
	}

	QuarkLabel {
		id: followersText
		x: followersControl.x + followersControl.width + spaceItems_
		y: followingControl.y
		text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.followers")
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 3)) : (defaultFontPointSize + 2)
	}

	MouseArea {
		id: followersClick
		x: followersControl.x
		y: followersControl.y - spaceItems_
		width: followersText.x + followersText.width
		height: followersText.height + 2 * spaceItems_

		onClicked: {
			//
			controller_.openBuzzfeedBuzzerFollowers(buzzer_);
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
		width: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 150) : 150
		visible: false
		//freeSizing: true

		model: ListModel { id: menuModel }

		onAboutToShow: prepare()

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
			} else if (key === "conversation") {
				//
				var lId = buzzerClient.getConversationsList().locateConversation(buzzer_);
				// conversation found
				if (lId !== "") {
					openBuzzerConversation(lId);
				} else {
					createConversation.process(buzzer_);
				}
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

			menuModel.append({
				key: "conversation",
				keySymbol: Fonts.conversationMessageSym,
				name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.conversation")});
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
				//buzzerClient.resync();
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"), true);
			} else if (code === "E_INSUFFICIENT_QTT_BALANCE") {
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_INSUFFICIENT_QTT_BALANCE"), true);
			} else {
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_ENDORSE"), true);
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
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"), true);
			} else if (code === "E_INSUFFICIENT_QTT_BALANCE") {
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_INSUFFICIENT_QTT_BALANCE"), true);
			} else {
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_MISTRUST"), true);
			}
		}
	}

	BuzzerCommands.CreateConversationCommand {
		id: createConversation

		onProcessed: {
			// open conversation
			openConversation.start();
		}

		onError: {
			// code, message
			handleError(code, message);
		}
	}

	Timer {
		id: openConversation
		interval: 500
		repeat: false
		running: false

		onTriggered: {
			//
			var lId = buzzerClient.getConversationsList().locateConversation(buzzer_);
			// conversation found
			if (lId !== "") {
				openBuzzerConversation(lId);
			} else {
				start();
			}
		}
	}

	function openBuzzerConversation(conversationId) {
		//
		var lConversation = buzzerClient.locateConversation(conversationId);
		if (lConversation) {
			controller_.openConversation(conversationId, lConversation, buzzerClient.getConversationsList());
		}
	}

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
