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

		if (buzzerClient.buzzerDAppReady) {
			infoLoaderCommand.process(buzzerClient.name);
		}

		if (!checkBuzzerInfo.running) checkBuzzerInfo.start();

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
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Window.background"));
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
			//infoLoaderCommand.process(buzzerClient.name);
			//if (!checkBuzzerInfo.running) checkBuzzerInfo.start();
		}

		function onBuzzerDAppReadyChanged() {
			if (buzzerClient.buzzerDAppReady) {
				infoLoaderCommand.process(buzzerClient.name);
				if (!checkBuzzerInfo.running) checkBuzzerInfo.start();
			}
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

		function onThemeChanged() {
			buzzermain_.updateStatusBar();
		}
	}

	//
	// buzzer info loader
	//
	BuzzerCommands.LoadBuzzerInfoCommand {
		id: infoLoaderCommand

		property var count_: 0;

		onProcessed: {
			// info
			console.log("[infoLoaderCommand]: infoLoaderCommand.buzzerId = " + infoLoaderCommand.buzzerId);

			// stop info checker
			checkBuzzerInfo.stop();

			// reset
			buzzerClient.avatar = "";

			// name
			loadTrustScoreCommand.process(infoLoaderCommand.buzzerId + "/" + infoLoaderCommand.buzzerChainId);

			// start score checker
			checkTrustScore.start();

			if (infoLoaderCommand.avatarId !== "0000000000000000000000000000000000000000000000000000000000000000") {
				avatarDownloadCommand.url = infoLoaderCommand.avatarUrl;
				avatarDownloadCommand.localFile = buzzerClient.getTempFilesPath() + "/" + infoLoaderCommand.avatarId;
				avatarDownloadCommand.process();
			}
		}

		onError: {
			//
			console.log("[infoLoaderCommand]: error = " + code + ", message = " + message);
			//
			if (code == "E_BUZZER_NOT_FOUND") {
				if (count_++ < 5) {
					checkBuzzerInfo.start();
				} else count_ = 0;
			}

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
			checkTrustScore.start(); // re-start
		}
	}

	Timer {
		id: checkBuzzerInfo
		interval: 2000
		repeat: false
		running: false

		onTriggered: {
			//
			console.log("[checkBuzzerInfo]: checking buzzer info...");
			infoLoaderCommand.process(buzzerClient.name);
			checkBuzzerInfo.start(); // re-start
		}
	}

	BuzzerCommands.LoadBuzzerTrustScoreCommand {
		id: loadTrustScoreCommand

		property var count_: 0;

		onProcessed: {
			//
			if (endorsements === 0) {
				if (count_++ < 5) checkTrustScore.start();
			} else {
				console.log("[loadTrustScoreCommand]: setting endorsements = " + endorsements + ", mistrusts = " + mistrusts);
				checkTrustScore.stop();
				buzzerClient.setTrustScore(endorsements, mistrusts);
				headerBar.adjustTrustScore();

				count_ = 0;
			}
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
			} else if (navigatorBar.currentIndex === 2) { // events
				eventsfeedPersonal.externalPull();
			}
		}

		onGoDrawer: {
			controller.openDrawer();
		}

		onToggleLimited: {
			navigatorBar.toggleLimited();
		}
	}

	TabBar {
		id: navigatorBar
		x: 0
		y: parent.height - (height)
		width: parent.width
		height: 50 + (bottomOffset > 30 ? bottomOffset : 0)
		currentIndex: 0
		position: TabBar.Footer

		TabButton {
			QuarkSymbolLabel {
				x: parent.width / 2 - width / 2
				y: parent.height / 2 - height / 2
				symbol: Fonts.homeSym
				font.pointSize: bottomOffset > 30 ? defaultFontSize_ + 3 : defaultFontSize_
			}

			onClicked: {
				if (!navigatorBar.indexTransfered && navigatorBar.currentIndex == 0 /*feed*/) {
					// go up
					buzzfeedPresonal.externalTop();
				} else navigatorBar.indexTransfered = false;
			}

			onDoubleClicked: {
				if (!navigatorBar.indexTransfered && navigatorBar.currentIndex == 0 /*feed*/) {
					// go up
					buzzfeedPresonal.externalPull();
				} else navigatorBar.indexTransfered = false;
			}
		}
		TabButton {
			QuarkSymbolLabel {
				x: parent.width / 2 - width / 2
				y: parent.height / 2 - height / 2
				symbol: Fonts.hashSym
				font.pointSize: bottomOffset > 30 ? defaultFontSize_ + 3 : defaultFontSize_
			}

			onClicked: {
				if (!navigatorBar.indexTransfered && navigatorBar.currentIndex == 1 /*global*/) {
					// go up
					buzzfeedGlobal.externalTop();
				} else navigatorBar.indexTransfered = false;
			}

			onDoubleClicked: {
				if (!navigatorBar.indexTransfered && navigatorBar.currentIndex == 1 /*global*/) {
					// go up
					buzzfeedGlobal.externalPull();
				} else navigatorBar.indexTransfered = false;
			}
		}
		TabButton {
			QuarkSymbolLabel {
				id: eventsSymbol
				x: parent.width / 2 - width / 2
				y: parent.height / 2 - height / 2
				symbol: Fonts.bellSym
				font.pointSize: bottomOffset > 30 ? defaultFontSize_ + 3 : defaultFontSize_
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

			onClicked: {
				if (!navigatorBar.indexTransfered && navigatorBar.currentIndex == 2 /*events*/) {
					// go up
					eventsfeedPersonal.externalTop();
				} else navigatorBar.indexTransfered = false;
			}

			onDoubleClicked: {
				if (!navigatorBar.indexTransfered && navigatorBar.currentIndex == 2 /*events*/) {
					// go up
					eventsfeedPersonal.externalPull();
				} else navigatorBar.indexTransfered = false;
			}
		}
		TabButton {
			QuarkSymbolLabel {
				id: messagesSymbol
				x: parent.width / 2 - width / 2
				y: parent.height / 2 - height / 2
				symbol: Fonts.chatsSym
				font.pointSize: bottomOffset > 30 ? defaultFontSize_ + 3 : defaultFontSize_
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
			id: walletTab
			parent: buzzerApp.getLimited() ? null : navigatorBar
			QuarkSymbolLabel {
				x: parent.width / 2 - width / 2
				y: parent.height / 2 - height / 2
				symbol: Fonts.walletSym
				font.pointSize: bottomOffset > 30 ? defaultFontSize_ + 3 : defaultFontSize_
			}
		}

		function toggleLimited() {
			if (walletTab.parent) walletTab.parent = null;
			else walletTab.parent = navigatorBar;
		}

		property bool startUp: false
		property bool indexTransfered: false

		onCurrentIndexChanged: {
			//
			if (!startUp) indexTransfered = true;
			else startUp = false;
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
			} else {
				buzzerApp.unlockOrientation();
			}

			if (globalMediaPlayerController.isCurrentInstancePlaying()) {
				globalMediaPlayerController.showCurrentPlayer(null);
			}
		}

		Component.onCompleted: {
			if (!buzzerClient.haveSubscriptions()) {
				startUp = true;
				currentIndex = 1; // global
			}
		}

		Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Market.tabActive");
		Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Market.tabInactive");
		Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Market.tabBackground");
	}

	QuarkHLine {
		id: topLine
		x1: 0
		y1: navigatorBar.y
		x2: parent.width
		y2: navigatorBar.y
		penWidth: 1
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Panel.top.separator") // Market.tabBackground
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
			mediaPlayerController: globalMediaPlayerController
		}
		Conversations {
			id: conversations
			x: 0
			y: headerBar.y + headerBar.height
			width: buzzermain_.width
			height: navigatorBar.y - (headerBar.y + headerBar.height)
			controller: buzzermain_.controller
			mediaPlayerController: globalMediaPlayerController
		}
		Wallets {
			id: wallets
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
