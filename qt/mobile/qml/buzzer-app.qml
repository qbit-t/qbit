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

	color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background");

	onWidthChanged: {
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
	}

	Component.onCompleted:
	{
		buzzerApp.lockPortraitOrientation();
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
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
				var lValue = buzzerClient.getProperty("Client.preventClose");
                if (lValue === "" || lValue === "false")
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
        theme: buzzerClient.themeSelector === "dark" ? StatusBar.Dark : StatusBar.Light
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

    function addPage(page)
    {
        pagesView.push(page, StackView.Transition);
        page.updateStatusBar();
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
				buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
				suspended = true;

				//
				//buzzerApp.resumeNotifications();
            }
            else if(Qt.application.state === 4 /* active */)
            {
                console.log("Main window waking up...");
				buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
                suspended = false;

                if (localNotificator) localNotificator.reset();

				//
				//buzzerApp.pauseNotifications();

				// try to re-force change background
				adjustTimer.start();
			}
        }
    }

    Connections
    {
        target: buzzerApp

        onDeviceTokenUpdated:
        {
            registerDeviceId(token);
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
                //easing.type: Easing.Linear
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

		BuzzerHeader {
			id: myBuzzer
			width: parent.width
			controller_: window
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

					QuarkLabel {
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
			buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
		}
	}

    Timer
    {
        id: startUpTimer
        interval: 1200
        repeat: false
        running: true

        onTriggered:
        {
            var lComponent = null;
            var lPage = null;

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

