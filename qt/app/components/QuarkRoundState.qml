// QuarkRoundState.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1

import QtQuick 2.0
import QtQml 2.2

Item {
	Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
    Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
    Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
    Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
    Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

	id: root

	width: size
    height: size

	property int size: 200
	property int penWidth: 2
	property string color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary")
	property string background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background")

	// reflect theme changes
	Connections {
		target: buzzerClient
		function onThemeChanged() {
			Material.theme = buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
			Material.accent = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
			Material.background = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background");
			Material.foreground = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
			Material.primary = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");
		}
	}

	Rectangle {
		id: frame
		width: root.size
		height: root.size
		radius: width / 2
		color: root.color

		Rectangle {
			id: innerFrame
			x: penWidth
			y: penWidth
			width: parent.width - penWidth * 2
			height: parent.height - penWidth * 2
			radius: width / 2

			color: root.background
		}
	}
}
