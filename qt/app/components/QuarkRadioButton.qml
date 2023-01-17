// QuarkRadioButton.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4

import "qrc:/fonts"

RadioButton {
	id: radioButton

	Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
	Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
	Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
	Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
	Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

	property real defaultFontPointSize: buzzerApp.isDesktop ? buzzerApp.defaultFontSize() : (buzzerApp.defaultFontSize() + 5)
	font.pointSize: defaultFontPointSize

	contentItem: QuarkLabel {
		id: label
		anchors.centerIn: parent
		leftPadding: (radioButton.indicator ? radioButton.indicator.width + radioButton.spacing : 0) *
					 (buzzerApp.isDesktop ? buzzerClient.scaleFactor : 1.0)
		text: radioButton.text
		font.pointSize: radioButton.font.pointSize
		width: parent.width - leftPadding
		wrapMode: Text.WordWrap
	}

	// reflect theme changes
	Connections	{
		target: buzzerClient
		function onThemeChanged() {
			Material.theme = buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
			Material.accent = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
			Material.background = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
			Material.foreground = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
			Material.primary = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");
		}
	}
}
