import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.3
import QtQuick.Controls 1.4
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Window 2.15

import StatusBar 0.1

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

ApplicationWindow
{
    id: window
	width: 460
	height: 620
    visible: true
    title: "buzzer"

    property string onlineCount: "";
	property int bottomBarHeight: 0;
	property string activePageBackground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Window.background")

	color: activePageBackground

	onWidthChanged: {
		buzzerApp.setBackgroundColor(activePageBackground);
	}

	Component.onCompleted:
	{
		buzzerApp.lockPortraitOrientation();
		buzzerApp.setBackgroundColor(activePageBackground);
	}

	onClosing:
    {
        if (Qt.platform.os == "android")
        {
            close.accepted = false;
            if (getDepth() > 1)
            {
                pagesView.get(pagesView.depth-1).closePageHandler();
            }
            else
            {
				//
				var lTryExit = true;
				var lPage = pagesView.get(pagesView.depth-1);
				if (lPage && lPage.prevPageHandler) {
					lTryExit = lPage.prevPageHandler();
				}

				//
				var lValue = buzzerClient.getProperty("Client.preventClose");
				if (lTryExit && lValue === "false")
                    close.accepted = true;
            }
        }
    }

    flags: Qt.Window | Qt.MaximizeUsingFullscreenGeometryHint

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

    function getDepth()
    {
		if (buzzerClient.configured())
		{
			for (var lIdx = pagesView.depth - 1; lIdx >= 0; lIdx--)
			{
				if (pagesView.get(lIdx).key === "buzzermain") return pagesView.depth - lIdx;
			}

			return 0;
		}

		return pagesView.depth - 1;
    }

	function addPage(page) {
        pagesView.push(page, StackView.Transition);
        page.updateStatusBar();
    }

	function activatePage(key, itemId) {
		for (var lIdx = pagesView.depth - 1; lIdx >= 0; lIdx--) {
			var lPage = pagesView.get(lIdx);
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

	function unwindToTop()
    {
        var lArray = [];
        for (var lIdx = pagesView.depth - 1; lIdx >= 0; lIdx--)
        {
            var lPage = pagesView.get(lIdx);
			if (lPage.key !== "buzzfeed")
            {
                lArray.push(lPage);
            }
            else break;
        }

        for (var lI = 0; lI < lArray.length; lI++)
        {
            lArray[lI].closePage();
        }
    }

    function popPage(page)
    {
        pagesView.pop();

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

    function containsPage(key)
    {
        for (var lIdx = 0; lIdx < pagesView.depth; lIdx++)
        {
            if (pagesView.get(lIdx).key === key) return true;
        }

        return false;
    }

	function openThread(buzzChainId, buzzId, buzzerAlias, buzzBody) {
		//
		var lComponent = null;
		var lPage = null;

		lComponent = Qt.createComponent("qrc:/qml/buzzfeedthread.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(window);
			lPage.controller = window;

			lPage.updateStakedInfo(buzzId);
			lPage.start(buzzChainId, buzzId);

			addPage(lPage);
		}
	}

	function openMedia(pkey, index, media, player, instance, buzzId, buzzerAlias, buzzBody) {
		//
		var lComponent = null;
		var lPage = null;

		lComponent = Qt.createComponent("qrc:/qml/buzzmedia.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(window);
			lPage.controller = window;

			lPage.buzzMedia_ = media;
			lPage.mediaPlayerController = player;
			lPage.initialize(pkey, index, instance, buzzBody);

			var lTitle = "Media";
			if (buzzBody !== "") lTitle = buzzBody.replace(/(\r\n|\n|\r)/gm, " ");
			lPage.updateStakedInfo(buzzId + "-media", buzzerAlias, lTitle);

			addPage(lPage);
		}
	}

	function openBuzzEditor(text) {
		//
		var lComponent = null;
		var lPage = null;

		lComponent = Qt.createComponent("qrc:/qml/buzzeditor.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(window);
			lPage.controller = window;

			lPage.updateStakedInfo(buzzerClient.generateRandom(), buzzerClient.alias, buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.editor.buzz.caption"));
			if (text) lPage.initializeBuzz(text);

			addPage(lPage);
		}
	}

	function openReplyEditor(self, buzzfeedModel, text) {
		//
		var lComponent = null;
		var lPage = null;

		lComponent = Qt.createComponent("qrc:/qml/buzzeditor.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(window);
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

		lComponent = Qt.createComponent("qrc:/qml/buzzeditor.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(window);
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

		lComponent = Qt.createComponent("qrc:/qml/buzzeditor.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(window);
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

		lComponent = Qt.createComponent("qrc:/qml/buzzfeedbuzzer.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(window);
			lPage.controller = window;

			lPage.start(buzzerName);

			addPage(lPage);
		}
	}

	function openBuzzfeedByTag(tag) {
		//
		var lComponent = null;
		var lPage = null;

		lComponent = Qt.createComponent("qrc:/qml/buzzfeedtag.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(window);
			lPage.controller = window;

			lPage.start(tag);

			addPage(lPage);
		}
	}

	function openBuzzfeedBuzzerEndorsements(buzzer) {
		//
		var lComponent = null;
		var lPage = null;

		lComponent = Qt.createComponent("qrc:/qml/buzzfeedendorsements.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(window);
			lPage.controller = window;

			lPage.start(buzzer);

			addPage(lPage);
		}
	}

	function openBuzzfeedBuzzerMistrusts(buzzer) {
		//
		var lComponent = null;
		var lPage = null;

		lComponent = Qt.createComponent("qrc:/qml/buzzfeedmistrusts.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(window);
			lPage.controller = window;

			lPage.start(buzzer);

			addPage(lPage);
		}
	}

	function openBuzzfeedBuzzerFollowers(buzzer) {
		//
		var lComponent = null;
		var lPage = null;

		lComponent = Qt.createComponent("qrc:/qml/buzzfeedfollowers.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(window);
			lPage.controller = window;
			lPage.start(buzzer);

			addPage(lPage);
		}
	}

	function openBuzzfeedBuzzerFollowing(buzzer) {
		//
		var lComponent = null;
		var lPage = null;

		lComponent = Qt.createComponent("qrc:/qml/buzzfeedfollowing.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(window);
			lPage.controller = window;
			lPage.start(buzzer);

			addPage(lPage);
		}
	}

	function openConversation(conversationId, conversation, conversationModel, messageId) {
		//
		var lComponent = null;
		var lPage = null;

		// locate and activate
		if (activatePage(conversationId, messageId)) return;

		lComponent = Qt.createComponent("qrc:/qml/conversationthread.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(window);
			lPage.controller = window;

			//
			var lCounterpart = conversation.counterpartyInfoId;
			if (conversation.side === 0) {
				lCounterpart = conversation.creatorInfoId;
			}

			lPage.updateStakedInfo(conversationId);
			addPage(lPage);

			lPage.start(conversationId, conversation, conversationModel, messageId);
		}
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
		target: buzzerClient

		function onThemeChanged() {
			window.activePageBackground = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Window.background")
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
	}

    function registerDeviceId(token)
    {
		if (buzzerClient.getProperty("Client.deviceId") === "")
		{
			buzzerClient.setProperty("Client.deviceId", token);
        }
    }

    StackView
    {
        id: pagesView
        anchors.fill: parent

        delegate: StackViewDelegate
        {
            popTransition: StackViewTransition
            {
                PropertyAnimation
                {
                    target: enterItem
                    property: "x"
                    from: -pagesView.width
                    to: 0
                    duration: 100
                }
                PropertyAnimation
                {
                    target: exitItem
                    property: "x"
                    from: 0
                    to: pagesView.width
                    duration: 100
                }
            }

            pushTransition: StackViewTransition
            {
                PropertyAnimation
                {
                    target: enterItem
                    property: "x"
                    from: pagesView.width
                    to: 0
                    duration: 100
                }
                PropertyAnimation
                {
                    target: exitItem
                    property: "x"
                    from: 0
                    to: -pagesView.width
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
					width: parent.width - parent.width / 2
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
		width: 0.75 * window.width
		height: window.height

		onAboutToShow: {
			myBuzzer.initialize();
			drawerMenu.prepare();
		}

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

					//
					var lComponent = null;
					var lPage = null;

					if (key === "createNewBuzzer" || key === "editBuzzer") {
						lComponent = Qt.createComponent("qrc:/qml/buzzercreateupdate.qml");
						if (lComponent.status === Component.Error) {
							showError(lComponent.errorString());
						} else {
							lPage = lComponent.createObject(window);
							lPage.controller = window;
							lPage.initialize(key === "createNewBuzzer" ? "CREATE" : "UPDATE");

							addPage(lPage);
						}
					} else if (key === "qbitKey") {
						lComponent = Qt.createComponent("qrc:/qml/buzzerqbitkey.qml");
						if (lComponent.status === Component.Error) {
							showError(lComponent.errorString());
						} else {
							lPage = lComponent.createObject(window);
							lPage.controller = window;

							addPage(lPage);
						}
					} else if (key === "personalFeed") {
						lComponent = Qt.createComponent("qrc:/qml/buzzfeedbuzzer.qml");
						if (lComponent.status === Component.Error) {
							showError(lComponent.errorString());
						} else {
							lPage = lComponent.createObject(window);
							lPage.controller = window;
							lPage.start(buzzerClient.name);

							addPage(lPage);
						}
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
						x: 15
						y: parent.height / 2 - height / 2
						verticalAlignment: Text.AlignVCenter
						Material.background: "transparent"
						Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
						visible: true
						symbol: keySymbol
					}

					QuarkLabelRegular {
						id: textLabel
						x: keySymbol ? 45 : 15
						y: parent.height / 2 - height / 2
						width: drawerMenu.width - (symbolLabel.x + symbolLabel.width + 5 + 10)
						text: name
						elide: Text.ElideRight
						verticalAlignment: Text.AlignVCenter
						Material.background: "transparent"
						Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
						visible: true
					}
				}
			}

			ScrollIndicator.vertical: ScrollIndicator { }

			function prepare() {
				if (menuModel_.count) return;

				menuModel_.append({
					key: "createNewBuzzer",
					keySymbol: Fonts.userPlusSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.create.new")});
				menuModel_.append({
					key: "editBuzzer",
					keySymbol: Fonts.userEditSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.edit.buzzer")});
				menuModel_.append({
					key: "qbitKey",
					keySymbol: Fonts.secretSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.qbit.key")});
				menuModel_.append({
					key: "personalFeed",
					keySymbol: Fonts.flashSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.personal.feed")});
				menuModel_.append({
					key: "network",
					keySymbol: Fonts.networkSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.network")});
				menuModel_.append({
					key: "quickHelp",
					keySymbol: Fonts.helpSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.quick.help")});
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
		interval: 400
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

					addPage(lPage);
				} else {
					lComponent = Qt.createComponent("qrc:/qml/buzzer-main.qml");
					if (lComponent.status === Component.Error) {
						showError(lComponent.errorString());
					} else {
						lPage = lComponent.createObject(window);
						lPage.controller = window;

						addPage(lPage);
					}
				}
			} else {
				lComponent = Qt.createComponent("qrc:/qml/setupinfo.qml");
				if (lComponent.status === Component.Error) {
					showError(lComponent.errorString());
				} else {
					lPage = lComponent.createObject(window);
					lPage.controller = window;

					addPage(lPage);
				}
			}
        }
    }
}

