// QuarkPage.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4
import StatusBar 0.1
import QtQuick.Window 2.12

Page
{
    id: page_

    property string nextPage;
    property var controller;
	property string key: "";
	property string caption: "";
	property string captionOriginal: "";
	property var closePageHandler: null;
	property var activatePageHandler: null;
	property var prevPageHandler: null;
	property bool stacked: false;
	property string alias: "";
	property string extraInfo: "";
	property bool activated: false;
	property bool releaseFocus: true;

	property string statusBarColor: getStatusBarColor()
	property string navigationBarColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.navigationBar")
	property string statusBarTheme: buzzerClient.statusBarTheme
	property string navigatorTheme: buzzerClient.themeSelector

    property int topOffset: Qt.platform.os === "ios" ? Screen.height - Screen.desktopAvailableHeight - statusBar.extraPadding : 0
    property int bottomOffset: Qt.platform.os === "ios" ? statusBar.extraBottomPadding : 0

	onExtraInfoChanged: {
		page_.caption = page_.captionOriginal + page_.extraInfo;
	}

	onHeightChanged: {
		if (page_.activated) {
			console.info("[page/onHeightChanged]: height = " + height + ", buzzerApp.isPortrait() = " + buzzerApp.isPortrait());
			page_.updateStatusBar();
		}
	}

	function getStatusBarColor() {
		return buzzerApp.isTablet ? (buzzerApp.isPortrait() ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.statusBar") :
															  buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.statusBar")) :
									buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.statusBar");
	}

	function updateStakedInfo(key, alias, caption) {
		page_.key = key;
		page_.alias = alias ? alias : "";
		page_.caption = caption ? caption : "";
		page_.stacked = true;
		page_.captionOriginal = page_.caption;

		buzzerClient.setTopId(key);
	}

	// reflect theme changes
	Connections
	{
		target: buzzerClient
		function onThemeChanged()
		{
			Material.theme = buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
			Material.accent = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
			Material.background = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background");
			Material.foreground = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
			Material.primary = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");
		}
	}

	Connections
	{
		target: Qt.application
		function onStateChanged()
		{
			if (!buzzerApp.isDesktop) {
				if(Qt.application.state !== 4 /* inactive */) {
					console.log("[Application]: suspend client");
					buzzerApp.suspendClient();
				} else {
					console.log("[Application]: resume client");
					buzzerApp.resumeClient();
				}
			}
		}
	}

	property bool followKeyboard: false
	property int parentHeightDelta: 0
	property int keyboardHeight: 0
	property bool keyboardVisible: false

	Connections {
		target: buzzerApp

		function onKeyboardHeightChanged(height) {
			//
			keyboardVisible = height > 0;

			//
			if (followKeyboard) {
				//
				var lKeyboardHeight = height;
				if (Qt.platform.os !== "ios") lKeyboardHeight = height / Screen.devicePixelRatio;
				keyboardHeight = lKeyboardHeight;

				//
				page_.height = page_.parent.height - lKeyboardHeight;
				
				//
				console.log("[onKeyboardHeightChanged]: parent.height = " + page_.parent.height + ", lKeyboardHeight = " + lKeyboardHeight + ", Screen.devicePixelRatio = " + Screen.devicePixelRatio);
			}
		}
	}

    signal dataReady(var data);
	signal pageClosed();

    function stopPage()
    {
        console.log("[QuarkPage/stopPage]: not implemented!");
    }

    onControllerChanged:
    {
        console.log("EXTRAPADDING:" + statusBar.extraPadding);
        if (controller) controller.extraPadding = statusBar.extraPadding;
    }

    StatusBar
    {
        id: statusBar
        color: page_.statusBarColor
        navigationBarColor: page_.navigationBarColor
		theme: page_.statusBarTheme === "dark" ? 1 : 0
		navigationTheme: page_.navigatorTheme === "dark" ? 1 : 0
    }

    function updateStatusBar()
    {
		statusBarColor = getStatusBarColor();
        statusBar.color = statusBarColor;
        statusBar.navigationBarColor = navigationBarColor;
    }

	function tryUpdateStatusBar()
	{
		statusBar.color = statusBarColor;
		statusBar.navigationBarColor = navigationBarColor;
	}

	Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
	Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
	Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background");
	Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
	Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");
}

