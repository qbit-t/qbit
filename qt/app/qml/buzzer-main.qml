import QtQuick 2.9
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

QuarkPage
{
	id: buzzermain_
	key: "buzzermain"

	prevPageHandler: function() {
		//
		if (navigatorBar.currentIndex - 1 >= 0) {
			navigatorBar.currentIndex--;
			return false;
		}

		return true;
	}

	Component.onCompleted: {
		buzzerApp.unlockOrientation();
		closePageHandler = closePage;
		activatePageHandler = activatePage;

		if (buzzerClient.cacheReady) {
			infoLoaderCommand.process(buzzerClient.name);
		}

		// NOTICE: start notificator service for the first time
		var lValue = buzzerClient.getProperty("Client.runService");
		if (lValue !== "true") {
			buzzerApp.startNotificator();
			buzzerClient.setProperty("Client.runService", "true");
		}
	}

	function closePage() {
		stopPage();
		destroy(500);
		controller.popPage();
	}

	function activatePage() {
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
		headerBar.activate();
	}

	function onErrorCallback(error)	{
		controller.showError(error);
	}

    //
    // stop processing
    //
	function stopPage() {
    }

	//
	// initial commands
	//
	Connections {
		target: buzzerClient

		function onCacheReadyChanged() {
			infoLoaderCommand.process(buzzerClient.name); //loadTrustScoreCommand.process();
		}

		function onBuzzerDAppReadyChanged() {
			infoLoaderCommand.process(buzzerClient.name); //loadTrustScoreCommand.process();
		}

		function onNewEvents() {
			if (navigatorBar.currentIndex != 2 /*events*/) {
				newEventsDot.visible = true;
			}
		}

		function onNewMessages() {
			if (navigatorBar.currentIndex != 3 /*conversations*/) {
				newMessagesDot.visible = true;
			}
		}
	}

	//
	// buzzer info loader
	//
	BuzzerCommands.LoadBuzzerInfoCommand {
		id: infoLoaderCommand

		onProcessed: {
			// reset
			buzzerClient.avatar = "";

			// name
			loadTrustScoreCommand.process(infoLoaderCommand.buzzerId + "/" + infoLoaderCommand.buzzerChainId);

			if (infoLoaderCommand.avatarId !== "0000000000000000000000000000000000000000000000000000000000000000") {
				avatarDownloadCommand.url = infoLoaderCommand.avatarUrl;
				avatarDownloadCommand.localFile = buzzerClient.getTempFilesPath() + "/" + infoLoaderCommand.avatarId;
				avatarDownloadCommand.process();
			}
		}

		onError: {
			handleError(code, message);
		}
	}

	Timer {
		id: checkTrustScore
		interval: 1000
		repeat: false
		running: false

		onTriggered: {
			//
			console.log("[checkTrustScore]: checking trust score...");
			loadTrustScoreCommand.process();
		}
	}

	BuzzerCommands.LoadBuzzerTrustScoreCommand {
		id: loadTrustScoreCommand

		property var count_: 0;

		onProcessed: {
			buzzerClient.setTrustScore(endorsements, mistrusts);
			headerBar.adjustTrustScore();
			count_ = 0;
		}

		onError: {
			if (count_++ < 5) {
				checkTrustScore.start();
			} else count_ = 0;
		}
	}

	BuzzerCommands.DownloadMediaCommand {
		id: avatarDownloadCommand
		preview: false // TODO: consider to use full image
		skipIfExists: true

		onProcessed: {
			// tx, previewFile, originalFile, orientation, duration, size, type
			buzzerClient.avatar = originalFile;
		}

		onError: {
			handleError(code, message);
		}
	}

	//
	// Shared media player controller
	//
	BuzzerMediaPlayerController {
		id: globalMediaPlayerController
		controller: buzzermain_.controller
	}

	//
	// toolbar
	//
	BuzzerToolBar {
		id: headerBar
		extraOffset: topOffset

		onGoHome: {
			//
			if (navigatorBar.currentIndex === 0) { // personal
				buzzfeedPresonal.externalPull();
			} else if (navigatorBar.currentIndex === 1) { // global
				buzzfeedGlobal.externalPull();
			}
		}

		onGoDrawer: {
			controller.openDrawer();
		}
	}

	TabBar {
		id: navigatorBar
		x: 0
		y: parent.height - (height)
		width: parent.width
		currentIndex: 0
		position: TabBar.Footer

		TabButton {
			QuarkSymbolLabel {
				x: parent.width / 2 - width / 2
				y: parent.height / 2 - height / 2
				symbol: Fonts.homeSym
			}
		}
		TabButton {
			QuarkSymbolLabel {
				x: parent.width / 2 - width / 2
				y: parent.height / 2 - height / 2
				symbol: Fonts.hashSym
			}
		}
		TabButton {
			QuarkSymbolLabel {
				id: eventsSymbol
				x: parent.width / 2 - width / 2
				y: parent.height / 2 - height / 2
				symbol: Fonts.bellSym
			}

			Rectangle {
				id: newEventsDot
				x: eventsSymbol.x + eventsSymbol.width + 3
				y: eventsSymbol.y - 3
				width: 6
				height: 6
				radius: 3
				color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Market.tabActive");
				visible: false
			}
		}
		TabButton {
			QuarkSymbolLabel {
				id: messagesSymbol
				x: parent.width / 2 - width / 2
				y: parent.height / 2 - height / 2
				symbol: Fonts.chatsSym
			}

			Rectangle {
				id: newMessagesDot
				x: messagesSymbol.x + messagesSymbol.width + 3
				y: messagesSymbol.y - 3
				width: 6
				height: 6
				radius: 3
				color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Market.tabActive");
				visible: false
			}
		}
		TabButton {
			QuarkSymbolLabel {
				x: parent.width / 2 - width / 2
				y: parent.height / 2 - height / 2
				symbol: Fonts.walletSym
			}
		}

		onCurrentIndexChanged: {
			//
			if (currentIndex == 0 /*feed*/) {
				headerBar.showBottomLine = true;
			}

			//
			if (currentIndex == 1 /*global*/) {
				headerBar.showBottomLine = false;
				buzzfeedGlobal.start();
			}

			//
			if (currentIndex == 2 /*events*/) {
				headerBar.showBottomLine = true;
				newEventsDot.visible = false;
			}

			if (currentIndex == 3 /*conversations*/) {
				headerBar.showBottomLine = false;
				newMessagesDot.visible = false;
				conversations.resetModel();
			}

			if (currentIndex == 4 /*wallet*/) {
				headerBar.showBottomLine = false;
				buzzerApp.lockPortraitOrientation();
				walletQbit.init();
			} else {
				buzzerApp.unlockOrientation();
			}

			if (globalMediaPlayerController.isCurrentInstancePlaying()) {
				globalMediaPlayerController.showCurrentPlayer();
			}
		}

		Component.onCompleted: {
			if (!buzzerClient.haveSubscriptions()) {
				currentIndex = 1; // global
			}
		}

		Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Market.tabActive");
		Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Market.tabInactive");
	}

	QuarkHLine {
		id: topLine
		x1: 0
		y1: navigatorBar.y
		x2: parent.width
		y2: navigatorBar.y
		penWidth: 1
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		visible: true
	}

	StackLayout {
		id: buzzerPages
		currentIndex: navigatorBar.currentIndex

		Buzzfeed {
			id: buzzfeedPresonal
			x: 0
			y: headerBar.y + headerBar.height
			width: buzzermain_.width
			height: navigatorBar.y - (headerBar.y + headerBar.height)
			controller: buzzermain_.controller
			mediaPlayerController: globalMediaPlayerController
		}
		BuzzfeedGlobal {
			id: buzzfeedGlobal
			x: 0
			y: headerBar.y + headerBar.height
			width: buzzermain_.width
			height: navigatorBar.y - (headerBar.y + headerBar.height)
			controller: buzzermain_.controller
			mediaPlayerController: globalMediaPlayerController
		}
		Eventsfeed {
			id: eventsfeedPersonal
			x: 0
			y: headerBar.y + headerBar.height
			width: buzzermain_.width
			height: navigatorBar.y - (headerBar.y + headerBar.height)
			controller: buzzermain_.controller
		}
		Conversations {
			id: conversations
			x: 0
			y: headerBar.y + headerBar.height
			width: buzzermain_.width
			height: navigatorBar.y - (headerBar.y + headerBar.height)
			controller: buzzermain_.controller
		}
		Wallet {
			id: walletQbit
			x: 0
			y: headerBar.y + headerBar.height
			width: buzzermain_.width
			height: navigatorBar.y - (headerBar.y + headerBar.height)
			controller: buzzermain_.controller
		}
	}

	function handleError(code, message) {
		if (code === "E_CHAINS_ABSENT") return;
		if (message === "UNKNOWN_REFTX" || code === "E_TX_NOT_SENT") {
			//buzzermain_.controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"));
		} else {
			//buzzermain_.controller.showError(message);
		}
	}
}
