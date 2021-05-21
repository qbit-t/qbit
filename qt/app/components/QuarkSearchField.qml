// QuarkSearchField.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4

import "qrc:/fonts"

Item
{
    id: searchField

    Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
    Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
    Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
    Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
    Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

	property string searchText: "";
	property string preeditText: "";
	property string placeHolder: buzzerApp.getLocalization(buzzerClient.locale, "Markets.Filter.Search");
    property int calculatedHeight: 0;
	property real fontPointSize: 14
	property int itemSpacing: 3;

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
		field.width = searchField.width - (clearButton ? cancel.width + 15 : 0);
    }

    QuarkTextField
    {
        id: field
		font.pointSize: fontPointSize
		width: searchField.width - (searchField.clearButton ? cancel.width + 15 : 0)
        clip: true

        placeholderText: searchField.placeHolder
		text: searchText
		mouseSelectionMode: TextInput.SelectCharacters

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
		}
    }

    QuarkLabel
    {
        id: cancel
        x: field.width + 5
        y: field.y + 10
        //renderType: Text.NativeRendering
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: Fonts.closeSym
        color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.hidden");
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

