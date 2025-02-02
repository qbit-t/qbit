﻿import QtQuick 2.12
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

	// adjust height by virtual keyboard
	followKeyboard: true

	property bool globalFeedAdjusted: false

	prevPageHandler: function() {
		//
		if (navigatorBar.currentIndex - 1 >= 0) {
			navigatorBar.currentIndex--;
			return false;
		}

		return true;
	}

	Component.onCompleted: {
		//
		buzzerApp.unlockOrientation();
		closePageHandler = closePage;
		activatePageHandler = activatePage;

		if (buzzerClient.buzzerDAppReady) {
			infoLoaderCommand.process(buzzerClient.name);
		}

		if (!checkBuzzerInfo.running) checkBuzzerInfo.start();
	}

	onControllerChanged: {
		controller.rootComponent = buzzermain_;
	}

	property string prevOrientation: "none"

	onHeightChanged: {
		//
		console.info("[main/onHeightChanged(0)]: height = " + height + ", isPortrait = " + buzzerApp.isPortrait());
		//
		if (prevOrientation === "none" || buzzerApp.isPortrait() && prevOrientation !== "portrait" ||
											!buzzerApp.isPortrait() && prevOrientation === "portrait") {
			//
			console.info("[main/onHeightChanged(1)]: height = " + height + ", prevOrientation = " + prevOrientation);
			//
			if (buzzerApp.isPortrait()) {
				controller.resetPagesView(null);
				prevOrientation = "portrait";
			} else {
				controller.resetPagesView(rightView);
				prevOrientation = "landscape";
			}

			leftLane.width = buzzermain_.leftWidth();
		}

		console.info("[main/onHeightChanged(3)]: navigatorBar.contentHeight = " + navigatorBar.contentHeight + ", buzzfeedPresonal.height = " + buzzfeedPresonal.height);
		controller.workAreaHeight = buzzfeedPresonal.height;
	}

	function collapse() {
		//
		leftLane.width = buzzermain_.leftWidth();
		splitView.showSplitter();
	}

	function expand() {
		//
		leftLane.width = 0;
		splitView.hideSplitter();
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
		controller.mainToolBar = headerBar;
		controller.adjustPageBackground();
		//
		buzzermain_.activated = true;

		//
		//buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
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

		function onScaleFactorChanged() {
			if (buzzerApp.isDesktop) {
				navigatorBar.contentHeight = 48.0 * buzzerClient.scaleFactor;
				controller.bottomBarHeight = navigatorBar.contentHeight;
				controller.workAreaHeight = buzzfeedPresonal.height;
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

			// NOTICE: try to get permissions
			// 1. post notifications
			// 2. camera
			if (Qt.platform.os == "android") {
				buzzerApp.checkPermissionPostNotifications();
				buzzerApp.checkPermissionCamera();
			}

			// NOTICE: start notificator service for the first time
			var lValue = buzzerClient.getProperty("Client.runService");
			if (lValue !== "true") {
				// try to start
				buzzerApp.startNotificator();
				buzzerClient.setProperty("Client.runService", "true");
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
		if (buzzerApp.isPortrait()) return buzzerApp.deviceWidth();
		//
		var lLeftWidth = buzzerClient.getProperty("Client.leftPaneWidth");
		if (lLeftWidth !== "" && lLeftWidth !== "0") return parseInt(lLeftWidth);
		//
		return buzzerApp.isPortrait() ? buzzerApp.deviceWidth() : buzzerApp.deviceWidth() * 0.30;
	}

	SplitView {
		id: splitView
		anchors.fill: parent

		function showSplitter() {
		}

		function hideSplitter() {
		}

		handleDelegate: Rectangle {
			width: 1
			height: splitView.height
			color: leftLane.width != 0 ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden") :
										 "black"
		}

		Rectangle {
			id: leftLane
			x: 0
			y: 0
			width: leftWidth()
			height: splitView.height
			color: "transparent"

			onWidthChanged: {
				if (!buzzerApp.isPortrait() && width != 0) buzzerClient.setProperty("Client.leftPaneWidth", width);
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

				onToggleLimited: {
					navigatorBar.toggleLimited();
					buzzerApp.isLimited = !buzzerApp.isLimited;
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

				onHeightChanged:  {
					controller.bottomBarHeight = height;
				}

				Component.onCompleted: {
					contentHeight = 48.0 * buzzerClient.scaleFactor;
					controller.bottomBarHeight = contentHeight;
				}

				TabButton {
					QuarkSymbolLabel {
						id: homeLabel
						x: parent.width / 2 - width / 2
						y: parent.height / 2 - height / 2
						symbol: Fonts.homeSym
						font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 5)) : (defaultFontPointSize + 2)
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
						font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 5)) : (defaultFontPointSize + 2)
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
						font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 5)) : (defaultFontPointSize + 2)
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
						font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 5)) : (defaultFontPointSize + 2)
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
					id: walletTab
					parent: buzzerApp.getLimited() ? null : navigatorBar
					QuarkSymbolLabel {
						x: parent.width / 2 - width / 2
						y: parent.height / 2 - height / 2
						symbol: Fonts.walletSym
						font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 5)) : (defaultFontPointSize + 2)
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

					console.info("onCurrentIndexChanged = " + currentIndex)
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

					onHeightChanged: controller.workAreaHeight = buzzfeedPresonal.height
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
