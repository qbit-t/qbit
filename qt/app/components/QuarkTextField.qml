// QuarkTextField.qml

import QtQuick 2.15
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.12
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4

TextField
{
    id: textField

    Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
    Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
    Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
    Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
    Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

	property var hintColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.hint");

	background: Rectangle {
		y: textField.height - height - textField.bottomPadding + 8
		implicitWidth: 120
		height: textField.activeFocus || textField.hovered ? 2 : 1
		color: textField.activeFocus ? textField.Material.accentColor :
									   (textField.hovered ? textField.Material.foreground :
															textField.hintColor)
	}

	antialiasing: buzzerApp.isDesktop ? false : antialiasing
	//property var fontFamily: Qt.platform.os == "windows" ? "Segoe UI Emoji" : "Noto Color Emoji N"
	//font.family: buzzerApp.isDesktop ? fontFamily : font.family
}

