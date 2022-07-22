// QuarkSearchField.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4

import "qrc:/fonts"

Rectangle
{
    id: searchField

    Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
    Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
    Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
    Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
    Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

	color: "transparent"
	height: calculatedHeight

	property string searchText: "";
	property string preeditText: "";
	property string placeHolder: buzzerApp.getLocalization(buzzerClient.locale, "Markets.Filter.Search");
    property int calculatedHeight: 0;
	property real defaultFontPointSize: buzzerApp.defaultFontSize() + 3
	property real fontPointSize: defaultFontPointSize
	property int itemSpacing: 3;
	property int spacingLeft: 0
	property int spacingRight: 0
	property var cancelSymbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
	property var placeholderTextColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")

    property bool clearButton: true;
	property var innerField: field

    signal textCleared();
	signal innerTextChanged(var text);

	function toBegin() {
		field.ensureVisible(0);
	}

    function reset()
    {
        searchText = "";
        field.text = "";
        textCleared();
    }

	function setText(value) {
		field.text = value;
	}

    onWidthChanged:
    {
		field.width = searchField.width - ((clearButton ? cancel.width + 15 : 0) + spacingRight + spacingLeft);
    }

    QuarkTextField
    {
        id: field
		font.pointSize: fontPointSize
		width: searchField.width - ((searchField.clearButton ? cancel.width + 15 : 0) + spacingRight + spacingLeft)
        clip: true

		Material.accent: searchField.Material.accent
		Material.background: searchField.Material.background
		Material.foreground: searchField.Material.foreground
		Material.primary: searchField.Material.primary

        placeholderText: searchField.placeHolder
		text: searchText
		mouseSelectionMode: TextInput.SelectCharacters
		inputMask: ""
		echoMode: TextInput.Normal

		placeholderTextColor: searchField.placeholderTextColor

		x: spacingLeft

		onActiveFocusChanged: {
			if (activeFocus) {
				cancel.color = field.Material.accent;
			} else {
				cancel.color = cancelSymbolColor;
			}
		}

		onDisplayTextChanged:
		{
			searchText = displayText;
			//console.log("inner/displayTextChanged = " + displayText);
		}

        onTextEdited:
        {
			searchText = text;
			//console.log("inner/textEdited = " + text);
        }

        onTextChanged:
        {
			searchText = text;
			innerTextChanged(text);
			//console.log("inner/textChanged = " + text);
        }

        onHeightChanged:
        {
            searchField.calculatedHeight = height;
        }

		onPreeditTextChanged:
		{
			//console.log("inner/preeditTextChanged = " + preeditText);
		}
    }

    QuarkLabel
    {
        id: cancel
		x: spacingLeft + (field.width + 5)
        y: field.y + 10
        //renderType: Text.NativeRendering
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: Fonts.closeSym
		color: cancelSymbolColor
        visible: searchField.clearButton

        font.family: Fonts.icons
        font.weight: Font.Normal
		font.pointSize: fontPointSize + 2

        onHeightChanged:
        {
            searchField.calculatedHeight = height;
        }
    }

    MouseArea
    {
        x: cancel.x - 10
        y: cancel.y - 10

        width: cancel.width + 10
        height: cancel.height + 10

        enabled: searchField.clearButton

        onClicked:
        {
            searchText = "";
            field.text = "";
            textCleared();
        }
    }
}

