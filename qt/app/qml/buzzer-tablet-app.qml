import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.3
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.15
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Window 2.15

import StatusBar 0.1

import app.buzzer.components 1.0 as QuarkComponents

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

ApplicationWindow {
    id: window
	width: 920
	height: 740
    visible: true
	title: "Buzzer"

    property string onlineCount: "";
	property var pagesView: null;
	property real bottomBarHeight: 0;
	property real workAreaHeight: 0;
	property var mainToolBar;
	property string activePageBackground: getPageBackground()
	property var rootComponent;

	color: activePageBackground

	function getPageBackground() {
		return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector,
								  (!buzzerApp.isTablet ? "Window.background" :
														 (buzzerApp.isPortrait() ?
															  "Window.background" :	mainStatusBar.enabled ? "Page.background" :
																											"Material.statusBar")));
	}

	function adjustPageBackground() {
		activePageBackground = getPageBackground();
	}

	onWidthChanged: {
		activePageBackground = getPageBackground();
		buzzerApp.setBackgroundColor(activePageBackground);
	}

	onHeightChanged: {
		console.info("[app/onHeightChanged]: height = " + height);
		rootComponent.width = width;
		rootComponent.height = height;
	}

	Component.onCompleted: {
		//
		buzzerApp.setBackgroundColor(activePageBackground);
		// default
		pagesView = pagesViewLocal;
	}

	onClosing: {
		if (Qt.platform.os == "android") {
			console.info("[onClosing]: closing...");
			close.accepted = false;
			if (getDepth() >= 1) {
				closeCurrentPage();
			} else {
				//
				var lTryExit = tryExit();
				//
				var lValue = buzzerClient.getProperty("Client.preventClose");
				if (lTryExit && lValue === "false")
					close.accepted = true;
			}
		}
	}

	flags: Qt.Window | Qt.MaximizeUsingFullscreenGeometryHint

	Connections {
		target: buzzerClient
		function onThemeChanged() {
			window.activePageBackground = getPageBackground();
		}
	}

	Connections
	{
		target: Qt.application
		onStateChanged:
		{
			if(Qt.application.state === 0 /* suspended */ ||
			   Qt.application.state === 1 /* hidden */ ||
			   Qt.application.state === 2 /* inactive */ )
			{
				console.log("Main window suspending...");
				buzzerApp.setBackgroundColor(activePageBackground);
				suspended = true;
			}
			else if(Qt.application.state === 4 /* active */)
			{
				console.log("Main window waking up...");
				buzzerApp.setBackgroundColor(activePageBackground);
				suspended = false;

				if (localNotificator) localNotificator.reset();

				// try to re-force change background
				adjustTimer.start();
			}
		}
	}

	Connections
	{
		target: buzzerApp

		function onExternalActivityCalled(type, chain, tx, buzzer) {
			//
			console.log("[onExternalActivityCalled]: type = " + type + ", chain = " + chain + ", tx = " + tx + ", buzzer = " + buzzer);
			//
			if (type === buzzerClient.tx_BUZZER_SUBSCRIBE_TYPE() ||
				type === buzzerClient.tx_BUZZER_ENDORSE_TYPE() ||
				type === buzzerClient.tx_BUZZER_MISTRUST_TYPE()) {
				//
				openBuzzfeedByBuzzer(buzzer);
			} else if (type === buzzerClient.tx_BUZZER_CONVERSATION() ||
						type === buzzerClient.tx_BUZZER_ACCEPT_CONVERSATION() ||
						type === buzzerClient.tx_BUZZER_DECLINE_CONVERSATION() ||
						type === buzzerClient.tx_BUZZER_MESSAGE() ||
						type === buzzerClient.tx_BUZZER_MESSAGE_REPLY()) {
				//
				var lConversationId = buzzerClient.getConversationsList().locateConversation(buzzer);
				var lConversation = buzzerClient.locateConversation(lConversationId);
				if (lConversation) {
					openConversation(lConversationId, lConversation, buzzerClient.getConversationsList(), tx);
				}
			} else {
				//
				openThread(chain, tx);
			}
		}

		function onGlobalGeometryChanged(width, height) {
			//
			var lWidth = width;
			if (Qt.platform.os !== "ios") lWidth = width / Screen.devicePixelRatio;
			var lHeight = height;
			if (Qt.platform.os !== "ios") lHeight = height / Screen.devicePixelRatio;
			//
			if (rootComponent)
				console.log("[onGlobalGeometryChanged]: width = " + lWidth + ", height = " + lHeight + ", rootComponent.width = " + rootComponent.width + ", rootComponent.height = " + rootComponent.height);
		}
	}

	StatusBar
	{
		id: mainStatusBar
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Splash.statusBar")
		navigationBarColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Splash.navigationBar")
		theme: buzzerClient.statusBarTheme === "dark" ? 1 : 0
		navigationTheme: buzzerClient.themeSelector === "dark" ? 1 : 0
	}

	function createPage(source)
    {
        var lComponent = Qt.createComponent(source);
        var lPage = lComponent.createObject(window);
        lPage.controller = window;
        return lPage;
    }

	//
	// facade methods
	//

	Connections {
		target: buzzerClient

		function onTryOpenThread(buzzChainId, buzzId, buzzerAlias, buzzBody) {
			//
			window.requestActivate();
			window.showNormal();
			window.show();
			window.raise();
			//
			openThread(buzzChainId, buzzId, buzzerAlias, buzzBody);
		}

		function onTryOpenBuzzfeedByBuzzer(buzzer) {
			//
			window.requestActivate();
			window.showNormal();
			window.show();
			window.raise();
			//
			openBuzzfeedByBuzzer(buzzer);
		}

		function onTryOpenConversation(id, conversationId, conversation, list) {
			//
			window.requestActivate();
			window.showNormal();
			window.show();
			window.raise();
			//
			openConversation(conversationId, conversation, list, id);
		}
	}

	function resetPagesView(view) {
		//
		try {
			unwindToTop();
		} catch(err) {
			console.error(err);
		}

		//
		if (!view) pagesView = pagesViewLocal;
		else pagesView = view;
	}

	function getDepth() {
		//
		var lIdx;
		if (pagesView === pagesViewLocal) {
			if (buzzerClient.configured()) {
				for (lIdx = pagesView.depth - 1; lIdx >= 0; lIdx--)	{
					if (pagesView.get(lIdx).key === "buzzermain") return pagesView.depth - lIdx;
				}
				return 0;
			}
			return pagesView.depth - 1;
		}

		if (buzzerClient.configured())
			return pagesView.count;
		return 0;
    }

	function addPage(page) {
		if (pagesView === pagesViewLocal) {
			pagesView.push(page, StackView.Transition);
			page.updateStatusBar();
			return;
		}

		pagesView.addItem(page);
		pagesView.setCurrentIndex(pagesView.count-1);
		page.updateStatusBar();
		if (page.activatePageHandler) page.activatePageHandler();
	}

	function activatePage(key, itemId) {
		var lIdx;
		var lPage;
		if (pagesView === pagesViewLocal) {
			for (lIdx = pagesView.depth - 1; lIdx >= 0; lIdx--) {
				lPage = pagesView.get(lIdx);
				if (lPage.key === key) {
					//
					if (lPage.activatePageHandler) lPage.activatePageHandler();
					if (itemId) lPage.showItem(itemId);
					//
					return true;
				}
			}

			return false;
		}

		// drop focus
		if (buzzerApp.isTablet) {
			for (lIdx = pagesView.count - 1; lIdx >= 1; lIdx--) {
				lPage = pagesView.itemAt(lIdx);
				try {
					lPage.releaseFocus = true;
				} catch(err) {
					console.error("[activatePage/error]: " + err);
				}
			}
		}

		// try to find page
		for (lIdx = pagesView.count - 1; lIdx >= 0; lIdx--) {
			lPage = pagesView.itemAt(lIdx);
			if (lPage.key === key) {
				//
				pagesView.currentIndex = lIdx;
				buzzerClient.setTopId(key);
				//
				if (lPage.activatePageHandler) lPage.activatePageHandler();
				if (itemId) lPage.showItem(itemId);
				//
				return true;
			}
		}

		return false;
	}

	function enumStakedPages() {
		var lArray = [];
		for (var lIdx = pagesView.count - 1; lIdx >= 0; lIdx--) {
			var lPage = pagesView.itemAt(lIdx);
			if (lPage.stacked === true) {
				lArray.push(lPage);
			}
		}

		return lArray;
	}

	function tryExit() {
		//
		var lPage;
		//
		if (!pagesView) return;
		//
		if (pagesView === pagesViewLocal) {
			if (pagesView.depth - 1 >= 0) {
				lPage = pagesView.get(pagesView.depth - 1);
				if (lPage && lPage.prevPageHandler) {
					return lPage.prevPageHandler();
				}
			}

			return false;
		}

		if (pagesView.count - 1 >= 0) {
			lPage = pagesView.itemAt(pagesView.count - 1);
			if (lPage && lPage.prevPageHandler) {
				return lPage.prevPageHandler();
			}
		}

		return false;
	}

	function closeCurrentPage() {
		//
		var lPage;
		//
		if (!pagesView) return;
		//
		if (pagesView === pagesViewLocal) {
			if (pagesView.depth - 1 >= 0) {
				lPage = pagesView.get(pagesView.depth - 1);
				if (lPage.key !== "buzzermain") {
					if (lPage.closePageHandler) lPage.closePageHandler();
				}
			}

			return;
		}

		if (pagesView.count - 1 >= 0) {
			lPage = pagesView.itemAt(pagesView.count - 1);
			if (lPage.key !== "buzzermain") {
				if (lPage.closePageHandler) lPage.closePageHandler();
			}
		}
	}

	function unwindToTop() {
		var lI;
		var lIdx;
		var lPage;
		var lArray = [];

		if (!pagesView) return;

		if (pagesView === pagesViewLocal) {
			for (lIdx = pagesView.depth - 1; lIdx >= 0; lIdx--) {
				lPage = pagesView.get(lIdx);
				if (lPage.key !== "buzzermain") {
					lArray.push(lPage);
				}
				else break;
			}

			for (lI = 0; lI < lArray.length; lI++) {
				if (lArray[lI].closePageHandler) lArray[lI].closePageHandler();
			}

			return;
		}

		for (lIdx = pagesView.count - 1; lIdx >= 0; lIdx--) {
			lPage = pagesView.itemAt(lIdx);
			if (lPage.key !== "buzzermain") {
                lArray.push(lPage);
            }
            else break;
        }

		for (lI = 0; lI < lArray.length; lI++) {
			if (lArray[lI].closePageHandler) lArray[lI].closePageHandler();
        }
    }

	function popPage(page) {
		//
		var lPage;
		//
		if (pagesView === pagesViewLocal) {
			pagesView.pop();

			lPage = pagesView.get(pagesView.depth - 1);
			try {
				lPage.updateStatusBar();
			} catch (ex) {
				console.error("[app/error]: " + ex);
			}
			if (lPage.activatePageHandler) lPage.activatePageHandler();
			return;
		}

		//
		if (page === undefined) {
			if (pagesView.count - 1 > 0) {
				lPage = pagesView.itemAt(pagesView.count - 1);
				pagesView.removeItem(lPage);
				pagesView.currentIndex = pagesView.count - 1;
			}
		} else {
			for (var lIdx = pagesView.count - 1; lIdx >= 0; lIdx--) {
				lPage = pagesView.itemAt(lIdx);
				if (lPage.key === page.key) {
					//
					pagesView.removeItem(page);
					if (pagesView.count > 1) {
						if (lIdx - 1 === 0) pagesView.currentIndex = 1;
						else if (lIdx - 1 > 0) pagesView.currentIndex = lIdx - 1;
						else pagesView.currentIndex = 0;
					} else pagesView.currentIndex = 0;

					break;
				}
			}
		}

		var lNewPage = pagesView.itemAt(pagesView.currentIndex);
		if (lNewPage) {
			buzzerClient.setTopId(lNewPage.key); // reset
			try {
				lNewPage.updateStatusBar();
			} catch (err) {
				console.error("[app/error]: " + err);
			}
			if (lNewPage.activatePageHandler) lNewPage.activatePageHandler();
		} else {
			buzzerClient.setTopId(""); // reset
		}
    }

	function popNonStacked() {
		//
		if (pagesView === pagesViewLocal) return;

		//
		var lArray = [];
		for (var lIdx = pagesView.count - 1; lIdx > 0; lIdx--) {
			var lPage = pagesView.itemAt(lIdx);
			if (lPage.stacked === false) {
				//
				lArray.push(lPage);
			} else {
				//
				break;
			}
		}

		//
		for (var lI = 0; lI < lArray.length; lI++) {
			pagesView.removeItem(lArray[lI]);
		}

		//
		pagesView.currentIndex = pagesView.count - 1;
		var lNewPage = pagesView.itemAt(pagesView.currentIndex);
		if (lNewPage) {
			buzzerClient.setTopId(lNewPage.key); // reset
		} else {
			buzzerClient.setTopId(""); // reset
		}
	}

	function isTop() {
		return pagesViewLocal.depth - 1;
	}

	function openThread(buzzChainId, buzzId, buzzerAlias, buzzBody) {
		//
		var lComponent = null;
		var lPage = null;

		// pop no-stacked page(s)
		popNonStacked();

		// locate and activate
		if (activatePage(buzzId)) return;

		lComponent = buzzerApp.isDesktop ? Qt.createComponent("qrc:/qml/buzzfeedthread.qml", pagesView) :
										   Qt.createComponent("qrc:/qml/buzzfeedthread.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(pagesView);
			lPage.controller = window;

			lPage.updateStakedInfo(buzzId, buzzerAlias, buzzBody ? buzzerClient.unMarkdownBuzzBodyLimited(buzzBody, 200).replace(/(\r\n|\n|\r)/gm, " ") : "");
			lPage.start(buzzChainId, buzzId);

			addPage(lPage);
		}
	}

	function openMedia(pkey, index, media, player, instance, buzzId, buzzerAlias, buzzBody) {
		//
		var lComponent = null;
		var lPage = null;

		// pop no-stacked page(s)
		popNonStacked();

		// locate and activate
		if (activatePage(buzzId + "-media")) return;

		lComponent = buzzerApp.isDesktop ? Qt.createComponent("qrc:/qml/buzzmedia.qml", rootComponent) :
										   Qt.createComponent("qrc:/qml/buzzmedia.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(rootComponent);
			lPage.controller = window;

			lPage.buzzMedia_ = media;
			lPage.mediaPlayerController = player;
			lPage.initialize(pkey, index, instance, buzzId, buzzBody);

			var lTitle = "Media";
			if (buzzBody !== "") lTitle = buzzerClient.unMarkdownBuzzBodyLimited(buzzBody, -1).replace(/(\r\n|\n|\r)/gm, " ");
			lPage.updateStakedInfo(buzzId + "-media", buzzerAlias, lTitle);

			addPage(lPage);
		}
	}

	function openBuzzEditor(text) {
		//
		var lComponent = null;
		var lPage = null;

		// pop no-stacked page(s)
		popNonStacked();

		lComponent = buzzerApp.isDesktop ? Qt.createComponent("qrc:/qml/buzzeditor-desktop.qml", rootComponent) :
										   Qt.createComponent("qrc:/qml/buzzeditor.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(rootComponent);
			lPage.controller = window;

			lPage.updateStakedInfo(buzzerClient.generateRandom(), buzzerClient.alias, buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.editor.buzz.caption"));
			lPage.initializeBuzz(text);

			addPage(lPage);
		}
	}

	function openReplyEditor(self, buzzfeedModel, text) {
		//
		var lComponent = null;
		var lPage = null;

		// pop no-stacked page(s)
		popNonStacked();

		lComponent = buzzerApp.isDesktop ? Qt.createComponent("qrc:/qml/buzzeditor-desktop.qml", rootComponent) :
										   Qt.createComponent("qrc:/qml/buzzeditor.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(rootComponent);
			lPage.controller = window;

			lPage.updateStakedInfo(buzzerClient.generateRandom(), buzzerClient.alias, buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.editor.reply.caption"));
			lPage.initializeReply(self, buzzfeedModel, text);

			addPage(lPage);
		}
	}

	function openRebuzzEditor(self, buzzfeedModel, index) {
		//
		var lComponent = null;
		var lPage = null;

		// pop no-stacked page(s)
		popNonStacked();

		lComponent = buzzerApp.isDesktop ? Qt.createComponent("qrc:/qml/buzzeditor-desktop.qml", rootComponent) :
										   Qt.createComponent("qrc:/qml/buzzeditor.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(rootComponent);
			lPage.controller = window;

			lPage.updateStakedInfo(buzzerClient.generateRandom(), buzzerClient.alias, buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.editor.rebuzz.caption"));
			lPage.initializeRebuzz(self, buzzfeedModel, index);

			addPage(lPage);
		}
	}

	function openMessageEditor(text, key, conversation, counterparty) {
		//
		var lComponent = null;
		var lPage = null;

		// pop no-stacked page(s)
		popNonStacked();

		lComponent = buzzerApp.isDesktop ? Qt.createComponent("qrc:/qml/buzzeditor-desktop.qml", rootComponent) :
										   Qt.createComponent("qrc:/qml/buzzeditor.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(rootComponent);
			lPage.controller = window;

			lPage.updateStakedInfo(buzzerClient.generateRandom(), buzzerClient.alias, buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.editor.message.caption") + counterparty);
			lPage.initializeMessage(text, key, conversation);
			addPage(lPage);
		}
	}

	function openBuzzfeedByBuzzer(buzzerName) {
		//
		var lComponent = null;
		var lPage = null;

		// pop no-stacked page(s)
		popNonStacked();

		// locate and activate
		if (activatePage(buzzerName)) return;

		lComponent = Qt.createComponent("qrc:/qml/buzzfeedbuzzer.qml", rootComponent);
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(rootComponent);
			lPage.controller = window;

			lPage.updateStakedInfo(buzzerName, buzzerName, "...");
			lPage.start(buzzerName);

			addPage(lPage);
		}
	}

	function openBuzzfeedByTag(tag) {
		//
		var lComponent = null;
		var lPage = null;

		// pop no-stacked page(s)
		popNonStacked();

		// locate and activate
		if (activatePage(tag)) return;

		//
		lComponent = Qt.createComponent("qrc:/qml/buzzfeedtag.qml", rootComponent);
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(rootComponent);
			lPage.controller = window;

			lPage.updateStakedInfo(tag, tag, "...");
			lPage.start(tag);

			addPage(lPage);
		}
	}

	function openBuzzfeedBuzzerEndorsements(buzzer) {
		//
		var lComponent = null;
		var lPage = null;

		// pop no-stacked page(s)
		popNonStacked();

		// locate and activate
		if (activatePage(buzzer + "-endorsements")) return;

		lComponent = Qt.createComponent("qrc:/qml/buzzfeedendorsements.qml", rootComponent);
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(rootComponent);
			lPage.controller = window;

			lPage.updateStakedInfo(buzzer + "-endorsements", buzzer, buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.endorsers"));
			lPage.start(buzzer);

			addPage(lPage);
		}
	}

	function openBuzzfeedBuzzerMistrusts(buzzer) {
		//
		var lComponent = null;
		var lPage = null;

		// pop no-stacked page(s)
		popNonStacked();

		// locate and activate
		if (activatePage(buzzer + "-mistrusts")) return;

		lComponent = Qt.createComponent("qrc:/qml/buzzfeedmistrusts.qml", rootComponent);
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(rootComponent);
			lPage.controller = window;

			lPage.updateStakedInfo(buzzer + "-mistrusts", buzzer, buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.mistrusters"));
			lPage.start(buzzer);

			addPage(lPage);
		}
	}

	function openBuzzfeedBuzzerFollowers(buzzer) {
		//
		var lComponent = null;
		var lPage = null;

		// pop no-stacked page(s)
		popNonStacked();

		// locate and activate
		if (activatePage(buzzer + "-followers")) return;

		lComponent = Qt.createComponent("qrc:/qml/buzzfeedfollowers.qml", rootComponent);
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(rootComponent);
			lPage.controller = window;
			lPage.start(buzzer);

			lPage.updateStakedInfo(buzzer + "-followers", buzzer, buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.followers"));
			addPage(lPage);
		}
	}

	function openBuzzfeedBuzzerFollowing(buzzer) {
		//
		var lComponent = null;
		var lPage = null;

		// pop no-stacked page(s)
		popNonStacked();

		// locate and activate
		if (activatePage(buzzer + "-following")) return;

		lComponent = Qt.createComponent("qrc:/qml/buzzfeedfollowing.qml", rootComponent);
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(rootComponent);
			lPage.controller = window;
			lPage.start(buzzer);

			lPage.updateStakedInfo(buzzer + "-following", buzzer, buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.following"));
			addPage(lPage);
		}
	}

	function openConversation(conversationId, conversation, conversationModel, messageId) {
		//
		var lComponent = null;
		var lPage = null;

		// pop no-stacked page(s)
		popNonStacked();

		// locate and activate
		if (activatePage(conversationId, messageId)) return;

		lComponent = Qt.createComponent("qrc:/qml/conversationthread.qml", pagesView);
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(pagesView);
			lPage.controller = window;

			//
			var lCounterpart = conversation.counterpartyInfoId;
			if (conversation.side === 0) {
				lCounterpart = conversation.creatorInfoId;
			}

			lPage.updateStakedInfo(conversationId, buzzerClient.getBuzzerAlias(lCounterpart) + " " + buzzerClient.getBuzzerName(lCounterpart), buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.conversation.title"));
			addPage(lPage);

			lPage.start(conversationId, conversation, conversationModel, messageId);
		}
	}

	//
	// inner methods
	//

	function addPageLocal(page)
	{
		pagesViewLocal.push(page, StackView.Transition);
		page.updateStatusBar();
		if (page.activatePageHandler) page.activatePageHandler();
	}

	function popPageLocal(page)
	{
		pagesViewLocal.pop();

		var lPage = pagesView.get(pagesView.depth - 1);
		lPage.updateStatusBar();
		if (lPage.activatePageHandler) lPage.activatePageHandler();
	}

	function errorDialogAccepted()
    {
        lastErrorCodeDialog = 0;
    }

	function showError(error, force)
    {
        var lComponent;
        var lPage;

        if (suspended) return;

        if (typeof(error) !== "string")
        {
			if (error.code !== lastErrorCodeDialog || force)
            {
                lastErrorCodeDialog = error.code;

                lComponent = Qt.createComponent("errordialog.qml");
                lPage = lComponent.createObject(window);
                lPage.show(error, errorDialogAccepted);
            }
        }
		else if (lastErrorMessageDialog !== error || force)
        {
			lastErrorMessageDialog = error;
			lComponent = Qt.createComponent("errordialog.qml");
            lPage = lComponent.createObject(window);
            lPage.showText(error);
        }
    }

	function containsPage(key) {
		for (var lIdx = 0; lIdx < pagesView.depth; lIdx++) {
			if (pagesView.get(lIdx).key === key) return true;
		}
		return false;
    }

	function openDrawer() {
		myBuzzer.initialize();
		drawerMenu.prepare();
		drawer.open();
	}

	function closeDrawer() {
		drawer.close();
	}

	property var lastErrorCodeDialog: "";
	property var lastErrorMessageDialog: "";
	property bool suspended: false;
    property int extraPadding: 0;

    StackView
    {
		id: pagesViewLocal
		//anchors.fill: parent
		x: parent.x
		y: parent.y
		width: parent.width
		height: parent.height

        delegate: StackViewDelegate
        {
            popTransition: StackViewTransition
            {
                PropertyAnimation
                {
                    target: enterItem
                    property: "x"
					from: -pagesViewLocal.width
                    to: 0
                    duration: 100
                }
                PropertyAnimation
                {
                    target: exitItem
                    property: "x"
                    from: 0
					to: pagesViewLocal.width
                    duration: 100
                }
            }

            pushTransition: StackViewTransition
            {
                PropertyAnimation
                {
                    target: enterItem
                    property: "x"
					from: pagesViewLocal.width
                    to: 0
                    duration: 100
                }
                PropertyAnimation
                {
                    target: exitItem
                    property: "x"
                    from: 0
					to: -pagesViewLocal.width
                    duration: 100
                }
            }
        }

        initialItem: QuarkPage
        {
            id: welcome

			PropertyAnimation
			{
				id: apearing
				target: welcomeRect
				property: "opacity"
				from: 0.0
				to: 1.0
				duration: 600

				onFinished: {
					//
					// REAL start-up
					startUpTimer.start();
				}
			}

			Rectangle
            {
                id: welcomeRect
                anchors.fill: parent

                border.color: "transparent"
                color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background");

                Image
                {
                    id: welcomeImage
					width: parent.width - parent.width / 2 > 300 ? 300 : parent.width - parent.width / 2
                    fillMode: Image.PreserveAspectFit
                    mipmap: true
					x: parent.width / 2 - (width) / 2
                    y: parent.height / 2 - paintedHeight / 2
					source: "../images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "buzzer.splash")
                }

                Component.onCompleted:
                {
					apearing.start();
                }
            }
        }
    }

	Drawer
	{
		Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
		Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
		Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
		Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
		Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

		id: drawer
		width: buzzerClient.scaleFactor * 400
		height: window.height
		interactive: opened

		BuzzerHeader {
			id: myBuzzer
			width: parent.width
			controller_: window
			infoDialog: drawerInfoDialog
		}

		InfoDialog {
			id: drawerInfoDialog
			bottom: 50
		}

		QuarkListView {
			id: drawerMenu
			x: 0
			y: myBuzzer.y + myBuzzer.calculatedHeight
			height: parent.height - y
			width: parent.width
			model: ListModel { id: menuModel_ }
			clip: true

			delegate: ItemDelegate {
				id: listDelegate
				width: drawerMenu.width
				leftPadding: 0
				rightPadding: 0
				topPadding: 0
				bottomPadding: 0
				clip: false

				onClicked: {
					//
					drawer.close();

					// pop no-stacked page(s)
					popNonStacked();

					//
					var lComponent = null;
					var lPage = null;

					if (key === "createNewBuzzer") {
						lComponent = Qt.createComponent("qrc:/qml/buzzercreatelink.qml");
						if (lComponent.status === Component.Error) {
							showError(lComponent.errorString());
						} else {
							lPage = lComponent.createObject(window);
							lPage.controller = window;

							addPage(lPage);
						}
					} else if (key === "editBuzzer") {
						lComponent = Qt.createComponent("qrc:/qml/buzzercreateupdate.qml");
						if (lComponent.status === Component.Error) {
							showError(lComponent.errorString());
						} else {
							lPage = lComponent.createObject(window);
							lPage.controller = window;
							lPage.initialize(key === "createNewBuzzer" ? "CREATE" : "UPDATE");

							addPage(lPage);
						}
					} else if (key === "removeBuzzer") {
						lComponent = Qt.createComponent("qrc:/qml/buzzerremove.qml");
						if (lComponent.status === Component.Error) {
							showError(lComponent.errorString());
						} else {
							lPage = lComponent.createObject(window);
							lPage.controller = window;

							addPage(lPage);
						}
					} else if (key === "qbitKey") {
						lComponent = Qt.createComponent("qrc:/qml/buzzerqbitkey.qml");
						if (lComponent.status === Component.Error) {
							showError(lComponent.errorString());
						} else {
							lPage = lComponent.createObject(window);
							lPage.controller = window;
							lPage.showKeys = true;

							addPage(lPage);
						}
					} else if (key === "personalFeed") {
						//
						window.openBuzzfeedByBuzzer(buzzerClient.name);
					} else if (key === "network") {
						lComponent = Qt.createComponent("qrc:/qml/peers.qml");
						if (lComponent.status === Component.Error) {
							showError(lComponent.errorString());
						} else {
							lPage = lComponent.createObject(window);
							lPage.controller = window;

							addPage(lPage);
						}
					} else if (key === "quickHelp") {
						lComponent = Qt.createComponent("qrc:/qml/buzzerquickhelp.qml");
						if (lComponent.status === Component.Error) {
							showError(lComponent.errorString());
						} else {
							lPage = lComponent.createObject(window);
							lPage.controller = window;

							addPage(lPage);
						}
					}
				}

				Binding {
					target: background
					property: "color"
					value: listDelegate.highlighted ?
							   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.highlight"):
							   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
				}

				contentItem: Rectangle {
					width: drawerMenu.width
					color: "transparent"
					border.color: "transparent"
					anchors.fill: parent

					QuarkSymbolLabel {
						id: symbolLabel
						x: 15 + leftOffset
						y: parent.height / 2 - height / 2
						verticalAlignment: Text.AlignVCenter
						Material.background: "transparent"
						Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
						visible: true
						symbol: keySymbol
						font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 1)) : defaultFontSize
					}

					QuarkLabelRegular {
						id: textLabel
						x: keySymbol ? (buzzerClient.scaleFactor * 40) : 15
						y: parent.height / 2 - height / 2
						width: drawerMenu.width - ((keySymbol ? (buzzerClient.scaleFactor * 40) : 15) + 10)
						text: name
						elide: Text.ElideRight
						verticalAlignment: Text.AlignVCenter
						Material.background: "transparent"
						Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
						visible: true
						font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * buzzerApp.defaultFontSize()) : defaultFontPointSize
					}
				}
			}

			ScrollIndicator.vertical: ScrollIndicator { }

			function prepare() {
				if (menuModel_.count) return;

				menuModel_.append({
					key: "createNewBuzzer",
					keySymbol: Fonts.userPlusSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.create.new"),
					leftOffset: 0});
				menuModel_.append({
					key: "editBuzzer",
					keySymbol: Fonts.userEditSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.edit.buzzer"),
					leftOffset: 0});
				//if (Qt.platform.os == "ios" || Qt.platform.os == "osx")
				menuModel_.append({
					key: "removeBuzzer",
					keySymbol: Fonts.userMinusSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.remove.current"),
					leftOffset: 0});
				menuModel_.append({
					key: "qbitKey",
					keySymbol: Fonts.secretSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.qbit.key"),
					leftOffset: 0});
				menuModel_.append({
					key: "personalFeed",
					keySymbol: Fonts.flashSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.personal.feed"),
					leftOffset: buzzerClient.scaleFactor * 2});
				menuModel_.append({
					key: "network",
					keySymbol: Fonts.networkSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.network"),
					leftOffset: (-1) * (buzzerClient.scaleFactor * 2)});
				menuModel_.append({
					key: "quickHelp",
					keySymbol: Fonts.helpSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.quick.help"),
					leftOffset: 0});
			}
		}
	}

	Timer
	{
		id: adjustTimer
		interval: 500
		repeat: false
		running: false

		onTriggered: {
			buzzerApp.setBackgroundColor(activePageBackground);
		}
	}

    Timer
    {
        id: startUpTimer
		interval: 1000
        repeat: false
		running: false

        onTriggered:
        {
            var lComponent = null;
            var lPage = null;

			mainStatusBar.enabled = false;

			if (buzzerClient.configured())
			{
				if (buzzerClient.pinAccessConfigured())
				{
					lComponent = Qt.createComponent("qrc:/qml/pin.qml");
					lPage = lComponent.createObject(window);
					lPage.pinCaption = buzzerApp.getLocalization(buzzerClient.locale, "EnterPin.Title");
					lPage.pinSubmit = buzzerApp.getLocalization(buzzerClient.locale, "Button.Unlock");
					lPage.nextPage = "buzzfeed.qml";
					lPage.pinAction = lPage.actionUnlock;
					lPage.controller = window;

					addPageLocal(lPage);
				} else {
					lComponent = Qt.createComponent("qrc:/qml/buzzer-main-tablet.qml");
					if (lComponent.status === Component.Error) {
						showError(lComponent.errorString());
					} else {
						lPage = lComponent.createObject(window);
						lPage.controller = window;

						addPageLocal(lPage);
					}
				}
			} else {
				lComponent = Qt.createComponent("qrc:/qml/setupinfo.qml");
				if (lComponent.status === Component.Error) {
					showError(lComponent.errorString());
				} else {
					lPage = lComponent.createObject(window);
					lPage.controller = window;

					addPageLocal(lPage);
				}
			}
        }
    }
}

