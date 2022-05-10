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
	id: buzzerheader_

	property int calculatedHeight: calculateHeightInternal()
	property var infoDialog

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
	readonly property real defaultFontSize: 11

	signal calculatedHeightModified(var value);

	onCalculatedHeightChanged: {
		calculatedHeightModified(calculatedHeight);
	}

	Component.onCompleted: {
	}

	function initialize() {
		buzzer_ = buzzerClient.name;
		infoLoaderCommand.process(buzzer_);
		balanceCommand.process();
		balanceQttCommand.process(buzzerApp.qttAsset()); // QTT
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

			if (infoLoaderCommand.avatarId !== "0000000000000000000000000000000000000000000000000000000000000000") {
				avatarDownloadCommand.url = infoLoaderCommand.avatarUrl;
				avatarDownloadCommand.localFile = buzzerClient.getTempFilesPath() + "/" + infoLoaderCommand.avatarId;
				avatarDownloadCommand.process();
			}

			if (infoLoaderCommand.headerId !== "0000000000000000000000000000000000000000000000000000000000000000") {
				headerDownloadCommand.url = infoLoaderCommand.headerUrl;
				headerDownloadCommand.localFile = buzzerClient.getTempFilesPath() + "/" + infoLoaderCommand.headerId;
				headerDownloadCommand.process();
			}
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
		preview: false // TODO: consider to use full image
		skipIfExists: true

		onProcessed: {
			// tx, previewFile, originalFile, orientation, duration, size, type
			avatarImage.source = "file://" + originalFile;
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
		preview: false // TODO: consider to use full image
		skipIfExists: true

		onProcessed: {
			// tx, previewFile, originalFile
			headerImage.source = "file://" + originalFile;
		}

		onError: {
			handleError(code, message);
		}
	}

	//
	// background
	//
	Rectangle {
		id: backgroundContainer
		x: 0
		y: 0
		width: parent.width
		height: bottomLine.y1
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background")
		clip: true
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
		height: 200 // 250
		color: "transparent"
		clip: true

		Image {
			id: headerImage

			width: parent.width
			mipmap: true

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
				var lResult = sourceSize.width > sourceSize.height &&
						lCoeff - 1.3333 < 0.001 &&
						lCoeff - 1.3333 >= 0.0;
				return lResult;
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
					return sourceSize.width - lDelta;
				}

				return sourceSize.width;
			}

			autoTransform: true
		}

		QuarkSimpleComboBox {
			id: localeCombo
			x: parent.width - width - 8
			y: 0
			width: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 50 : 40
			fontPointSize: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 22 : 24
			itemLeftPadding: buzzerApp.isDesktop ? /*buzzerClient.scaleFactor * */12 : 8
			itemTopPadding: buzzerApp.isDesktop ? /*buzzerClient.scaleFactor * */10 : 8
			itemHorizontalAlignment: Text.AlignHCenter
			leftPadding: buzzerApp.isDesktop ? 0 : -3

			Material.background: "transparent"

			model: ListModel { id: languageModel_ }

			Component.onCompleted: {
				prepare();
			}

			indicator: Canvas {
			}

			clip: false

			onActivated: {
				var lEntry = languageModel_.get(localeCombo.currentIndex);
				if (lEntry.name !== buzzerClient.locale) {
					buzzerClient.locale = lEntry.id;
					buzzerClient.save();
				}
			}

			function activate() {
				if (!languageModel_.count) prepare();
				else {
					var lLanguages = buzzerApp.getLanguages().split("|");
					var lCurrentIdx = 0;
					for (var lIdx = 0; lIdx < lLanguages.length; lIdx++) {
						if (lLanguages[lIdx].length) {
							var lPair = lLanguages[lIdx].split(",");
							if (lPair[1] === buzzerClient.locale) lCurrentIdx = lIdx;
						}
					}

					localeCombo.currentIndex = lCurrentIdx;
				}
			}

			function prepare() {
				if (languageModel_.count) return;

				var lLanguages = buzzerApp.getLanguages().split("|");
				var lCurrentIdx = 0;
				for (var lIdx = 0; lIdx < lLanguages.length; lIdx++) {
					if (lLanguages[lIdx].length) {
						var lPair = lLanguages[lIdx].split(",");
						languageModel_.append({ id: lPair[1], name: lPair[0] });

						if (lPair[1] === buzzerClient.locale) lCurrentIdx = lIdx;
					}
				}

				localeCombo.currentIndex = lCurrentIdx;
			}
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
		property int displayWidth: buzzerClient.scaleFactor * 120
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

	QuarkSymbolLabel {
		id: uiZoomSymbol
		x: scaleCombo.x - width
		y: scaleCombo.y + scaleCombo.height / 2 - height / 2
		symbol: Fonts.searchSym
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 12) : (defaultFontPointSize + 1)
		visible: buzzerApp.isDesktop
	}

	QuarkSimpleComboBox {
		id: scaleCombo
		x: parent.width - width - 8
		y: avatarImage.y + avatarImage.displayHeight // headerContainer.y + headerContainer.height + height + spaceItems_
		width: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 62 : 50
		fontPointSize: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 11 : 14
		itemLeftPadding: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 12 : 8
		itemTopPadding: buzzerApp.isDesktop ? /*buzzerClient.scaleFactor * */15 : 8
		itemHorizontalAlignment: Text.AlignHCenter
		visible: buzzerApp.isDesktop
		leftPadding: buzzerClient.scaleFactor

		Material.background: "transparent"

		model: ListModel { id: scaleModel_ }

		Component.onCompleted: {
			prepare();
		}

		indicator: Canvas {
		}

		onActivated: {
			var lEntry = scaleModel_.get(scaleCombo.currentIndex);
			if (lEntry.id !== buzzerClient.scale) {
				buzzerClient.scale = lEntry.id;
				buzzerClient.save();
			}
		}

		function activate() {
			if (!scaleModel_.count) prepare();
			else {
				for (var lIdx = 0; lIdx < scaleModel_.count; lIdx++) {
					if (scaleModel_.get(lIdx).id === buzzerClient.scale) {
						scaleCombo.currentIndex = lIdx;
						break;
					}
				}
			}
		}

		function prepare() {
			//
			if (scaleModel_.count) return;
			//
			scaleModel_.append({ id: "100", name: "100%" });
			scaleModel_.append({ id: "110", name: "110%" });
			scaleModel_.append({ id: "115", name: "115%" });
			scaleModel_.append({ id: "120", name: "120%" });
			scaleModel_.append({ id: "125", name: "125%" });
			scaleModel_.append({ id: "135", name: "135%" });
			scaleModel_.append({ id: "150", name: "150%" });
			scaleModel_.append({ id: "175", name: "175%" });
			scaleModel_.append({ id: "200", name: "200%" });

			for (var lIdx = 0; lIdx < scaleModel_.count; lIdx++) {
				if (scaleModel_.get(lIdx).id === buzzerClient.scale) {
					scaleCombo.currentIndex = lIdx;
					break;
				}
			}
		}
	}

	//
	// buzzer info
	//

	QuarkSymbolLabel {
		id: aliasSymbol
		x: spaceLeft_
		y: avatarImage.y + avatarImage.displayHeight + spaceTop_
		symbol: Fonts.userTagSym
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 12) : (defaultFontPointSize + 1)
	}

	QuarkLabel {
		id: buzzerAliasControl
		x: aliasSymbol.x + aliasSymbol.width + spaceItems_
		y: aliasSymbol.y
		width: parent.width - (x + spaceRight_ + (uiZoomSymbol.visible ? (parent.width - uiZoomSymbol.x) : 0))
		elide: Text.ElideRight
		text: alias_
		font.bold: true
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : defaultFontPointSize
	}

	//

	QuarkSymbolLabel {
		id: nameSymbol
		x: spaceLeft_
		y: aliasSymbol.y + aliasSymbol.height + spaceItems_
		symbol: Fonts.userAliasSym
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 12) : (defaultFontPointSize + 1)
	}

	QuarkLabelRegular {
		id: buzzerNameControl
		x: aliasSymbol.x + aliasSymbol.width + spaceItems_
		y: nameSymbol.y
		width: parent.width - (x + spaceRight_)
		elide: Text.ElideRight
		text: buzzer_
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : defaultFontPointSize
	}

	//

	QuarkSymbolLabel {
		id: idSymbol
		x: spaceLeft_
		y: nameSymbol.y + nameSymbol.height + spaceItems_
		symbol: Fonts.hashSym
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 12) : (defaultFontPointSize + 1)
	}

	QuarkLabelRegular {
		id: buzzerIdControl
		x: aliasSymbol.x + aliasSymbol.width + spaceItems_
		y: idSymbol.y
		width: parent.width - (x + spaceRight_ + copySymbol.width + spaceItems_)
		elide: Text.ElideRight

		text: /*"0x" +*/ buzzerId_
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : defaultFontPointSize
	}

	QuarkSymbolLabel {
		id: copySymbol
		x: parent.width - (spaceRight_ + width)
		y: buzzerIdControl.y - 2
		symbol: Fonts.externalLinkSym
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 12) : (defaultFontPointSize + 1)
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

	//

	QuarkSymbolLabel {
		id: walletSymbol
		x: spaceLeft_
		y: idSymbol.y + idSymbol.height + spaceItems_
		symbol: Fonts.walletSym
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 12) : (defaultFontPointSize + 1)
	}

	QuarkNumberLabel {
		id: availableNumber
		number: 0.00000000
		//font.pointSize: 30
		fillTo: 8
		mayCompact: true
		x: aliasSymbol.x + aliasSymbol.width + spaceItems_
		y: walletSymbol.y
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		numberColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		mantissaColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		zeroesColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		unitsColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : defaultFontPointSize
	}

	MouseArea {
		x: availableNumber.x
		y: availableNumber.y
		width: availableNumber.calculatedWidth
		height: availableNumber.calculatedHeight

		onClicked: {
			availableNumber.mayCompact = !availableNumber.mayCompact;
		}
	}

	QuarkLabel {
		id: qbitText1
		x: availableNumber.x + availableNumber.calculatedWidth + spaceItems_
		y: availableNumber.y
		text: "QBIT"
		font.pointSize: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 7 : 10
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
	}

	BuzzerCommands.BalanceCommand {
		id: balanceCommand

		onProcessed: {
			// amount, pending, scale
			availableNumber.number = amount;
		}

		onError: {
			controller_.showError({ code: code, message: message, component: "Balance" });
		}
	}

	//

	QuarkNumberLabel {
		id: sharesNumber
		number: 0
		//font.pointSize: 30
		fillTo: 1
		mayCompact: true
		x: aliasSymbol.x + aliasSymbol.width + spaceItems_
		y: availableNumber.y + availableNumber.height + spaceItems_
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		numberColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		mantissaColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		zeroesColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		unitsColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : defaultFontPointSize
	}

	MouseArea {
		x: sharesNumber.x
		y: sharesNumber.y
		width: sharesNumber.calculatedWidth
		height: sharesNumber.calculatedHeight

		onClicked: {
			sharesNumber.mayCompact = !sharesNumber.mayCompact;
		}
	}

	QuarkLabel {
		id: qbitText2
		x: sharesNumber.x + sharesNumber.calculatedWidth + spaceItems_
		y: sharesNumber.y
		text: "QTT"
		font.pointSize: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 7 : 10
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
	}

	QuarkSymbolLabel {
		id: questionSymbol
		x: qbitText2.x + qbitText2.width + spaceItems_
		y: qbitText2.y
		symbol: Fonts.questionCircleSym
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 12) : (defaultFontPointSize + 1)
	}

	MouseArea {
		x: questionSymbol.x - spaceItems_
		y: questionSymbol.y - spaceItems_
		width: questionSymbol.width + 2 * spaceItems_
		height: questionSymbol.height + 2 * spaceItems_

		onClicked: {
			//
			console.log("[questionCircleSym]: clicked");
			var lMsgs = [];
			lMsgs.push(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.qtt.help"));
			infoDialog.show(lMsgs);
		}
	}

	BuzzerCommands.BalanceCommand {
		id: balanceQttCommand

		onProcessed: {
			// amount, pending, scale
			sharesNumber.number = amount;
		}

		onError: {
			controller_.showError({ code: code, message: message, component: "Balance" });
		}
	}

	//
	// bottom line
	//

	QuarkHLine {
		id: bottomLine
		x1: 0
		y1: sharesNumber.y + sharesNumber.height + spaceBottom_ + spaceItems_
		x2: parent.width
		y2: sharesNumber.y + sharesNumber.height + spaceBottom_ + spaceItems_
		penWidth: 1
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		visible: true
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

