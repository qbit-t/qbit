import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 1.4
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

	property bool globalFeedAdjusted: false

	Component.onCompleted: {
		//
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

	onControllerChanged: {
		controller.rootComponent = buzzermain_;
	}

	function closePage() {
		stopPage();
		destroy(500);
		controller.popPage();
	}

	function activatePage() {
		//
		var lInitialize = controller.mainToolBar === undefined;
		//
		controller.pagesView = rightView;
		controller.mainToolBar = headerBar;
		//
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
		headerBar.activate();

		//
		if (lInitialize) {
			// common static check
			if (!buzzerClient.haveSubscriptions() && !buzzermain_.globalFeedAdjusted) {
				navigatorBar.startUp = true;
				navigatorBar.currentIndex = 1; // global
				buzzermain_.globalFeedAdjusted = true;
			}

			//
			conversations.start();
		}
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
			infoLoaderCommand.process(buzzerClient.name);
		}

		function onBuzzerDAppReadyChanged() {
			infoLoaderCommand.process(buzzerClient.name);
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

		function onScaleFactorChanged() {
			if (buzzerApp.isDesktop) navigatorBar.contentHeight = 48 * buzzerClient.scaleFactor;
		}
	}

	//
	// buzzer info loader
	//
	BuzzerCommands.LoadBuzzerInfoCommand {
		id: infoLoaderCommand

		property var count_: 0;

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
		}
	}

	Timer {
		id: checkBuzzerInfo
		interval: 1000
		repeat: false
		running: false

		onTriggered: {
			//
			console.log("[checkBuzzerInfo]: checking buzzer info...");
			infoLoaderCommand.process(buzzerClient.name);
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
				buzzerClient.setTrustScore(endorsements, mistrusts);
				headerBar.adjustTrustScore();

				// switch pages - dynamic check
				if (following == 0 && !buzzermain_.globalFeedAdjusted) {
					navigatorBar.startUp = true;
					navigatorBar.currentIndex = 1; // switch to global
					buzzermain_.globalFeedAdjusted = true;
				}

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

	function leftWidth() {
		//
		var lLeftWidth = buzzerClient.getProperty("Client.leftPaneWidth");
		if (lLeftWidth !== "") return parseInt(lLeftWidth);
		return 400;
	}

	SplitView {
		id: splitView
		anchors.fill: parent

		handleDelegate: Rectangle {
			width: 1
			height: splitView.height
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		}

		Rectangle {
			id: leftLane
			x: 0
			y: 0
			width: leftWidth()
			height: splitView.height
			color: "transparent"

			onWidthChanged: {
				buzzerClient.setProperty("Client.leftPaneWidth", width);
			}

			//
			// toolbar
			//
			BuzzerDesktopToolBar {
				id: headerBar
				halfLine: leftLane.width
				width: leftLane.width

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
				width: leftLane.width
				currentIndex: 0
				position: TabBar.Footer

				onYChanged: {
					controller.bottomBarHeight = height;
				}

				Component.onCompleted: {
					contentHeight = contentHeight * buzzerClient.scaleFactor;
				}

				TabButton {
					QuarkSymbolLabel {
						id: homeLabel
						x: parent.width / 2 - width / 2
						y: parent.height / 2 - height / 2
						symbol: Fonts.homeSym
						font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : (defaultFontPointSize + 2)
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
						font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : (defaultFontPointSize + 2)
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
						font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : (defaultFontPointSize + 2)
					}

					Rectangle {
						id: newEventsDot
						x: eventsSymbol.x + eventsSymbol.width + (buzzerApp.isDesktop ? buzzerClient.scaleFactor * 3 : 3)
						y: eventsSymbol.y - (buzzerApp.isDesktop ? buzzerClient.scaleFactor * 3 : 3)
						width: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 6 : 6
						height: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 6 : 6
						radius: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 3 : 3
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
						font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : (defaultFontPointSize + 2)
					}

					Rectangle {
						id: newMessagesDot
						x: messagesSymbol.x + messagesSymbol.width + (buzzerApp.isDesktop ? buzzerClient.scaleFactor * 3 : 3)
						y: messagesSymbol.y - (buzzerApp.isDesktop ? buzzerClient.scaleFactor * 3 : 3)
						width: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 6 : 6
						height: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 6 : 6
						radius: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 3 : 3
						color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Market.tabActive");
						visible: false
					}
				}
				TabButton {
					QuarkSymbolLabel {
						x: parent.width / 2 - width / 2
						y: parent.height / 2 - height / 2
						symbol: Fonts.walletSym
						font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : (defaultFontPointSize + 2)
					}
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
						headerBar.resetSearchText();
						buzzfeedGlobal.disconnect();
						conversations.disconnect();
					}

					//
					if (currentIndex == 1 /*global*/) {
						if (!buzzerApp.isDesktop) headerBar.showBottomLine = true;
						headerBar.resetSearchText();
						buzzfeedGlobal.start();
						conversations.disconnect();
					}

					//
					if (currentIndex == 2 /*events*/) {
						headerBar.showBottomLine = true;
						newEventsDot.visible = false;
						headerBar.resetSearchText();
						buzzfeedGlobal.disconnect();
						conversations.disconnect();
					}

					if (currentIndex == 3 /*conversations*/) {
						headerBar.resetSearchText();
						if (!buzzerApp.isDesktop) headerBar.showBottomLine = true;
						newMessagesDot.visible = false;
						conversations.resetModel();
						//conversations.start();
						buzzfeedGlobal.disconnect();
					}

					if (currentIndex == 4 /*wallet*/) {
						headerBar.resetSearchText();
						headerBar.showBottomLine = true;
						buzzerApp.lockPortraitOrientation();
						buzzfeedGlobal.disconnect();
						conversations.disconnect();
					} else {
						buzzerApp.unlockOrientation();
					}

					if (globalMediaPlayerController.isCurrentInstancePlaying()) {
						globalMediaPlayerController.showCurrentPlayer(null);
					}
				}

				//Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Market.tabActive");
				//Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Market.tabInactive");

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
				color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
				visible: true
			}

			StackLayout {
				id: buzzerPages
				currentIndex: navigatorBar.currentIndex

				Buzzfeed {
					id: buzzfeedPresonal
					x: 0
					y: headerBar.totalHeight + 1
					width: leftLane.width
					height: navigatorBar.y - (headerBar.totalHeight + 1)
					controller: buzzermain_.controller
					mediaPlayerController: globalMediaPlayerController
				}
				BuzzfeedGlobal {
					id: buzzfeedGlobal
					x: 0
					y: headerBar.totalHeight + 1
					width: leftLane.width
					height: navigatorBar.y - (headerBar.totalHeight + 1)
					controller: buzzermain_.controller
					mediaPlayerController: globalMediaPlayerController
				}
				Eventsfeed {
					id: eventsfeedPersonal
					x: 0
					y: headerBar.totalHeight + 1
					width: leftLane.width
					height: navigatorBar.y - (headerBar.totalHeight + 1)
					controller: buzzermain_.controller
					mediaPlayerController: globalMediaPlayerController
				}
				Conversations {
					id: conversations
					x: 0
					y: headerBar.totalHeight + 1
					width: leftLane.width
					height: navigatorBar.y - (headerBar.totalHeight + 1)
					controller: buzzermain_.controller
					mediaPlayerController: globalMediaPlayerController
				}
				Wallets {
					id: wallets
					x: 0
					y: headerBar.totalHeight + 1
					width: leftLane.width
					height: navigatorBar.y - (headerBar.totalHeight)
					controller: buzzermain_.controller
				}
			}
		}

		BuzzerStackView {
			id: rightView
		}
	}

	//
	function handleError(code, message) {
		if (code === "E_CHAINS_ABSENT") return;
		if (message === "UNKNOWN_REFTX" || code === "E_TX_NOT_SENT") {
			//buzzermain_.controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"));
		} else {
			//buzzermain_.controller.showError(message);
		}
	}
}
