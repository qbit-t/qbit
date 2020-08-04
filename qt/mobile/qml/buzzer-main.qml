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
				x: parent.width / 2 - width / 2
				y: parent.height / 2 - height / 2
				symbol: Fonts.bellSym
			}
		}
		TabButton {
			QuarkSymbolLabel {
				x: parent.width / 2 - width / 2
				y: parent.height / 2 - height / 2
				symbol: Fonts.chatsSym
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
			if (currentIndex == 1 /*global*/) {
				headerBar.showBottomLine = false;
				buzzfeedGlobal.start();
			} else {
				headerBar.showBottomLine = true;
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
		Item {}
		Item {}
		Item {}
	}
}
