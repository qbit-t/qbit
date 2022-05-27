// QuarkLabelRegular.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4

import "qrc:/fonts"

Label {
	id: label

	Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
	Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
	Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
	Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
	Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

	//property var fontFamily: "Noto Sans" //Qt.platform.os == "windows" ? "Segoe UI Emoji" : "Noto Color Emoji N"
	//property var fontFamily: Qt.platform.os == "windows" ? "Segoe UI Emoji" : "Noto Color Emoji N"
	//font.family: buzzerApp.isDesktop ? fontFamily : font.family

	//antialiasing: buzzerApp.isDesktop ? false : antialiasing
	//font.kerning: buzzerApp.isDesktop ? false : font.kerning

	property real defaultFontPointSize: buzzerApp.isDesktop ? 11 : 16
	font.pointSize: defaultFontPointSize
}
