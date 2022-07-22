// QuarkSymbolLabel.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1

import "qrc:/fonts"

QuarkLabel
{
    id: symbolLabel

    property alias symbol: symbolLabel.text
	property real defaultFontSize_: buzzerApp.defaultFontSize() + 7
	property real defaultFontSize: defaultFontSize_

    font.family: Fonts.icons
    font.weight: Font.Normal
	font.pointSize: defaultFontSize

	antialiasing: buzzerApp.isDesktop ? false : antialiasing
}


