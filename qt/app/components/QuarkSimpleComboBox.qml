// QuarkSimpleComboBox.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4

ComboBox
{
    id: comboBox

    Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
    Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
    Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
    Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
    Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

    textRole: "name"

	//property int fontPixelSize: 14;
	property int fontPointSize: 14;
    property int popUpWidth: comboBox.width;
	property int itemLeftPadding: 10;
	property int itemTopPadding: 10;
	property int itemHorizontalAlignment: Text.AlignRight;
    property bool popUpAlignLeft: true;
    property var keySymbols: [];
    property int keyTopPadding: 0;
    property bool readOnly: false;

	//property var fontFamily: Qt.platform.os == "windows" ? "Segoe UI Emoji" : "Noto Color Emoji N"
	//font.family: buzzerApp.isDesktop ? fontFamily : font.family
	font.pointSize: fontPointSize

    onPopUpWidthChanged:
    {
        popup.adjustPosition();
    }

    onPopUpAlignLeftChanged:
    {
        popup.adjustPosition();
    }

    onWidthChanged:
    {
        popup.adjustPosition();
    }

    delegate: ItemDelegate
    {
        id: listDelegate
        width: popUpWidth
        contentItem: Rectangle
        {
            color: comboBox.highlightedIndex === index ?
                       buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.highlight"):
                       buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
            border.color: "transparent"
            anchors.fill: parent

            QuarkLabel
            {
                id: textLabel
				x: itemLeftPadding
				y: itemTopPadding //+ (index === 0 ? 0 : 8)
				text: getText()
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
				horizontalAlignment: itemHorizontalAlignment
                Material.background: "transparent"
				font.pointSize: fontPointSize
                function getText()
                {
                    return name;
                }
            }
        }

        highlighted: comboBox.highlightedIndex === index
    }

	popup: QuarkPopup
    {
        x: adjustPosition()
        y: comboBox.height - 1
        width: popUpWidth
        implicitHeight: contentItem.implicitHeight
        padding: 0
        enabled: !comboBox.readOnly

		elevationWidth: (width * 60) / 100
		elevationHeight: (height * 60) / 100

        Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
        Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
        Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background");
        Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
        Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

        contentItem: QuarkListView
        {
            clip: true
            implicitHeight: contentHeight
            model: comboBox.popup.visible ? comboBox.delegateModel : null
            currentIndex: comboBox.highlightedIndex

            ScrollIndicator.vertical: ScrollIndicator { }
        }

        function adjustPosition()
        {
            x = comboBox.popUpAlignLeft == true ? 0 : (comboBox.width - comboBox.popUpWidth);
            return x;
        }
    }
}

