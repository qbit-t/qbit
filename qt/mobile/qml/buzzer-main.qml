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

	Component.onCompleted: {
		buzzerApp.unlockOrientation();
		closePageHandler = closePage;
		activatePageHandler = activatePage;

		if (buzzerClient.cacheReady) {
			loadTrustScoreCommand.process();
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
			loadTrustScoreCommand.process();
		}

		function onBuzzerDAppReadyChanged() {
			loadTrustScoreCommand.process();
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

	BuzzerCommands.LoadBuzzerTrustScoreCommand {
		id: loadTrustScoreCommand
		onProcessed: {
			buzzerClient.setTrustScore(endorsements, mistrusts);
			headerBar.adjustTrustScore();
		}
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
		}
		BuzzfeedGlobal {
			id: buzzfeedGlobal
			x: 0
			y: headerBar.y + headerBar.height
			width: buzzermain_.width
			height: navigatorBar.y - (headerBar.y + headerBar.height)
			controller: buzzermain_.controller
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
}
