// QuarkInfoBox.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4

import "qrc:/fonts"

Rectangle
{
    id: infoBox

    height: 50

    Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
    Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
    Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
    Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
    Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");
    border.color: "transparent"
    color: "transparent"

    property string symbol;
    property string image;
    property string symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
    property string text;
    property string placeholderText;
    property int symbolLeftPadding: 0;
    property int textLeftPadding: 5;
	property int textFontSize: 16;
    property int symbolFontSize: 20;
    property int imageWidth: 36;
    property bool clipboardButton: false;
	property bool helpButton: false;
	property bool pasteButton: false;
	property bool openUrl: false;
	property bool isNumber: false;
	property real number: -1000000000.0;
	property int fillTo: 4;
	property int itemRightPadding: -5;
	property int itemLeftPadding: 5;

	signal helpClicked();
	signal pasteClicked();
	signal textClicked();

    Rectangle
    {
        id: symbolRect

		width: infoBox.height
		height: infoBox.height
        clip: true

        border.color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.border");
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");

		QuarkLabel
        {
            id: symbolLabel
            anchors.fill: parent
            leftPadding: symbolLeftPadding
            x: symbolLeftPadding
            //renderType: Text.NativeRendering
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: infoBox.symbol
            color: symbolColor
            visible: !image.length && !openUrl

            font.family: Fonts.icons
            font.weight: Font.Normal
            font.pointSize: symbolFontSize
        }

        QuarkSymbolButton
        {
            x: -5
            y: -5
            width: parent.width + 10
            height: parent.height + 10
            visible: openUrl

            symbol: infoBox.symbol
            Material.background: "transparent"
            font.pointSize: symbolFontSize

            onClicked:
            {
                Qt.openUrlExternally(infoBox.text);
            }
        }

        Image
        {
            Layout.alignment: Qt.AlignCenter
            mipmap: true
            source: image
            visible: !symbol.length
            x: parent.width / 2 - imageWidth / 2
            y: parent.height / 2 - height / 2
            width: imageWidth
            //height: parent.height - 14
            fillMode: Image.PreserveAspectFit
        }
    }

    Rectangle
    {
        id: infoRect

        x: symbolRect.width - 1
		height: infoBox.height
        //width: infoBox.width - copyRect.getWidth() - symbolRect.width + 2
		width: infoBox.width - symbolRect.width - copyRect.width * clipboardButton -
			   helpRect.width * helpButton - pasteRect.width * pasteButton +
			   clipboardButton*1 + helpButton*1 + pasteButton*1 + 1

        border.color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.border");
        color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.background");

		QuarkLabelRegular
        {
            id: infoLabel
            leftPadding: textLeftPadding
            rightPadding: 5
			topPadding: 1
			anchors.fill: parent
            horizontalAlignment: Text.AlignLeft
			verticalAlignment: Text.AlignVCenter
			elide: Text.ElideRight

			font.pointSize: textFontSize

			visible: !isNumber

            text: !infoBox.text.length ? infoBox.placeholderText : infoBox.text
            color: !infoBox.text.length ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.textDisabled") :
                buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
        }

		QuarkPriceLabel
		{
			id: numberLabel

			trend: ""

			y: parent.height / 2 - calculatedHeight / 2
			x: itemLeftPadding

			number: infoBox.number
			//useNumberText: true

			fillTo: infoBox.fillTo
			verticalAlignment: Text.AlignVCenter
			//horizontalAlignment: Text.AlignHCenter
			//elide: Text.ElideRight
			font.pointSize: textFontSize

			visible: isNumber

			priceUpColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
			priceDownColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		}

		MouseArea
        {
            width: parent.width
            height: parent.height

            onClicked:
            {
                if (text !== "") {
                    if (infoLabel.elide == Text.ElideRight) infoLabel.elide = Text.ElideLeft;
                    else infoLabel.elide = Text.ElideRight;
                } else {
                    textClicked();
                }
            }
        }
    }

    Rectangle
    {
        id: copyRect

        x: infoRect.x + infoRect.width - 1
		width: infoBox.height
		height: infoBox.height
        visible: clipboardButton
        clip: true

        border.color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.border");
        color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");

        function getWidth()
        {
            if (clipboardButton) return width;
            return 0;
        }

        function getX()
        {
            return x;
        }

        QuarkSymbolButton
        {
            x: -5
            y: -5
            width: parent.width + 10
            height: parent.height + 10

            symbol: Fonts.clipboardSym
            Material.background: "transparent"
			font.pointSize: symbolFontSize

            onClicked:
            {
                clipboard.setText(infoLabel.text);
            }
        }
    }

    Rectangle
    {
        id: helpRect

        x: copyRect.getX() + copyRect.getWidth() - 1
		width: infoBox.height
		height: infoBox.height
        visible: helpButton
        clip: true

        border.color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.border");
        color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");

        function getWidth()
        {
            if (helpButton) return width;
            return 0;
        }

        QuarkSymbolButton
        {
            x: -5
            y: -5
            width: parent.width + 10
            height: parent.height + 10

            symbol: Fonts.helpSym
            Material.background: "transparent"
			font.pointSize: symbolFontSize

            onClicked:
            {
                helpClicked();
            }
        }
    }

	Rectangle
	{
		id: pasteRect

		x: copyRect.getX() + copyRect.getWidth() - 1
		width: infoBox.height
		height: infoBox.height
		visible: pasteButton
		clip: true

		border.color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.border");
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");

		function getWidth()
		{
			if (pasteButton) return width;
			return 0;
		}

		QuarkSymbolButton
		{
			x: -5
			y: -5
			width: parent.width + 10
			height: parent.height + 10

			symbol: Fonts.pasteSym
			Material.background: "transparent"
			font.pointSize: symbolFontSize

			onClicked:
			{
				infoBox.text = clipboard.getText();
				pasteClicked();
			}
		}
	}
}

