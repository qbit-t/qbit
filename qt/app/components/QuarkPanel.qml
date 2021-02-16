// QuarkPanel.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4

Rectangle
{
    id: panel

    Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
    Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
    Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
    Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
    Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

    border.color: "transparent"
    gradient: Gradient
    {
        GradientStop { position: 0; color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Panel.gradient1"); }
        GradientStop { position: 0.7; color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Panel.gradient2"); }
        GradientStop { position: 1; color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Panel.gradient3"); }
    }
}

