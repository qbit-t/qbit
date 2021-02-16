// QuarkFavoriteButton.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1

import "qrc:/fonts"

QuarkLabel
{
    id: favButton

    property bool favorite: false

    text: favorite ? Fonts.thumbsSym : Fonts.circleAddSym
    font.pointSize: 14
    font.family: Fonts.icons
    font.weight: Font.Normal
    color: favorite ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
                      buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.textDisabled")

    MouseArea
    {
        x: 0
        y: 0
        width: favButton.width * 3
        height: favButton.width * 3

        onPressAndHold:
        {
            favorite = !favorite
        }
    }
}


