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

    function showError(error)
    {
        var lComponent;
        var lPage;

        if (suspended) return;

        if (typeof(error) !== "string")
        {
            if (error.code !== lastErrorCodeDialog)
            {
                lastErrorCodeDialog = error.code;

                lComponent = Qt.createComponent("errordialog.qml");
                lPage = lComponent.createObject(window);
                lPage.show(error, errorDialogAccepted);
            }
        }
		else if (lastErrorMessageDialog !== error)
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

	property int lastErrorCodeDialog: 0;
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
                suspended = true;
            }
            else if(Qt.application.state === 4 /* active */)
            {
                console.log("Main window waking up...");
				buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
                suspended = false;

                if (localNotificator) localNotificator.reset();
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

