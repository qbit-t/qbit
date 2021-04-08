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
	property var closePageHandler: null;
	property var activatePageHandler: null;
	property bool stacked: false;
	property string alias: "";

	property string statusBarColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.statusBar")
	property string navigationBarColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.navigationBar")

    property int topOffset: Qt.platform.os === "ios" ? Screen.height - Screen.desktopAvailableHeight - statusBar.extraPadding : 0

	function updateStakedInfo(key, alias, caption) {
		page_.key = key;
		page_.alias = alias;
		page_.caption = caption;
		page_.stacked = true;

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
		theme: buzzerClient.themeSelector === "dark" ? StatusBar.Dark : StatusBar.Light
    }

    function updateStatusBar()
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

