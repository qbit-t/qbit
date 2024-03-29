// QuarkTextEdit.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4

import "qrc:/fonts"

TextEdit
{
    id: textEdit

    Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
    Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
    Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
    Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
    Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

	antialiasing: buzzerApp.isDesktop ? false : antialiasing
	//property var fontFamily: Qt.platform.os == "windows" ? "Segoe UI Emoji" : "Noto Color Emoji N"
	//font.family: buzzerApp.isDesktop ? fontFamily : font.family

	//font.kerning: buzzerApp.isDesktop ? false : true
	//font.preferShaping: buzzerApp.isDesktop ? false : true
	//font.hintingPreference: buzzerApp.isDesktop ? Font.PreferNoHinting : font.hintingPreference

	property real defaultFontPointSize_: buzzerApp.isDesktop ? buzzerApp.defaultFontSize() : (buzzerApp.defaultFontSize() + 5)
	property real defaultFontPointSize: defaultFontPointSize_
	font.pointSize: defaultFontPointSize
}

