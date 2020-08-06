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
						   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.highlight"):
						   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.background");
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
					Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")
					visible: true
					symbol: keySymbol
				}

				QuarkLabel
				{
					id: textLabel
					x: symbolLabel.x + symbolLabel.width + 5
					y: parent.height / 2 - height / 2
					width: popupBox.width - (symbolLabel.x + symbolLabel.width + 5 + 10)
					text: name
					elide: Text.ElideRight
					verticalAlignment: Text.AlignVCenter
					Material.background: "transparent"
					Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")
					visible: true
				}
			}
		}

		ScrollIndicator.vertical: ScrollIndicator { }
	}
}

