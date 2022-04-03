import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.3
import QtQuick.Controls 1.4
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Window 2.15

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

SwipeView
{
	id: pagesView
	clip: true
	interactive: false

	onWidthChanged: {
		decentralized.width = width;
	}

	Image {
			id: decentralized
			x: 0
			y: 0
			width: pagesView.width
			Layout.alignment: Qt.AlignCenter
			source: "../images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "desktop.back")
			fillMode: Image.PreserveAspectCrop
	}
}
