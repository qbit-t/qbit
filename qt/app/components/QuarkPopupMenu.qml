// QuarkPopupMenu.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4

QuarkPopup
{
	id: popupBox

	Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
	Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
	Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background");
	Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
	Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

	property var menuHighlightColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.highlight")
	property var menuBackgroundColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.background")
	property var menuForegroundColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")

	property var model;
	signal click(var key);

	implicitHeight: contentItem.implicitHeight
	padding: 0

	contentItem: QuarkListView
	{
		id: list
		clip: true
		implicitHeight: contentHeight
		model: popupBox.model

		delegate: ItemDelegate
		{
			id: listDelegate
			width: popupBox.width
			leftPadding: 0
			rightPadding: 0
			topPadding: 0
			bottomPadding: 0
			clip: false

			onClicked: {
				popupBox.close();
				click(key);
			}

			Binding {
				target: background
				property: "color"
				value: listDelegate.highlighted ?
						   menuHighlightColor:
						   menuBackgroundColor;
			}

			contentItem: Rectangle {
				width: popupBox.width
				color: "transparent"
				border.color: "transparent"
				anchors.fill: parent

				QuarkSymbolLabel
				{
					id: symbolLabel
					x: 15
					y: parent.height / 2 - height / 2
					verticalAlignment: Text.AlignVCenter
					Material.background: "transparent"
					Material.foreground: menuForegroundColor
					visible: true
					symbol: keySymbol
					font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 12) : defaultFontSize
				}

				TextMetrics
				{
					id: metrics
					elide: Text.ElideRight
					text: name
					elideWidth: popupBox.width - ((keySymbol ? (buzzerClient.scaleFactor * 40) : 15) + 10)
					font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 11) : 16
				}

				QuarkLabel
				{
					id: textLabel
					x: keySymbol ? (buzzerClient.scaleFactor * 40) : 15
					y: parent.height / 2 - height / 2
					width: metrics.elideWidth // popupBox.width - ((keySymbol ? (buzzerClient.scaleFactor * 40) : 15) + 10)
					text: metrics.elidedText
					//maximumLineCount: 1
					Material.background: "transparent"
					Material.foreground: menuForegroundColor
					visible: true
					font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 11) : 16
				}
			}
		}

		ScrollIndicator.vertical: ScrollIndicator { }
	}
}

