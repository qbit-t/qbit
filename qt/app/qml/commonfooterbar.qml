import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Dialogs 1.1

import "qrc:/backend"
import "qrc:/fonts"
import "qrc:/components"
import "qrc:/models"

import "qrc:/lib/dateFunctions.js" as DateHelper
import "qrc:/lib/numberFunctions.js" as NumberFunctions

import "qrc:/qml"

QuarkFooterBar
{
    id: footerBar
    x: 0
    height: adjustHeight()
    width: parent.width

    property bool topPage: false
    property int extraWidth: 100
    property int calculatedWidth: walletButton.width + strategiesButton.width + ordersButton.width + historyButton.width +
                                  accountButton.width + extraWidth

    property var controller: null;

    onControllerChanged:
    {
        height = adjustHeight();
    }

    function adjustHeight()
    {
        return 50 + (Qt.platform.os === "ios" && controller && controller.extraPadding > 20 ? 10 : 0);
    }

    RowLayout
    {
        id: row

        property bool marketsAdded: false;

        Layout.fillWidth: true
        width: parent.width

        QuarkToolButton
        {
            id: walletButton
            symbol: getSymbol()
            Material.background: "transparent"
            visible: true
            labelYOffset: 3

            property bool enabled_: controller && !controller.containsPage("allaccounts") || symbol === Fonts.analyticsSym
            symbolColor: enabled_ ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
                                    buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")

            Layout.alignment: Qt.AlignHCenter

            onClicked:
            {
                if (enabled_)
                {
                    if (symbol === Fonts.analyticsSym)
                    {
                        if (controller)
                            controller.unwindToTop();
                    }
                    else
                    {
                        var lWallet = controller.createPage("qrc:/qml/allaccounts.qml");
                        // parametrize
                        controller.addPage(lWallet);
                        lWallet.startPage();
                    }
                }
            }

            function getSymbol()
            {
                if (footerBar.topPage) return Fonts.walletSym;
                if (symbol === Fonts.analyticsSym) return Fonts.analyticsSym;
                if (controller && controller.containsPage("allaccounts") && !row.marketsAdded)
                {
                    row.marketsAdded = true;
                    return Fonts.analyticsSym;
                }

                return Fonts.walletSym;
            }
        }
        QuarkToolButton
        {
            id: strategiesButton
            symbol: getSymbol()
            Material.background: "transparent"
            visible: true
            labelYOffset: 3

            property bool enabled_: controller && !controller.containsPage("strategies") || symbol === Fonts.analyticsSym
            symbolColor: enabled_ ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
                                    buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")

            Layout.alignment: Qt.AlignHCenter

            onClicked:
            {
                if (enabled_)
                {
                    if (symbol === Fonts.analyticsSym)
                    {
                        if (controller)
                            controller.unwindToTop();
                    }
                    else
                    {
                        var lStrategies = controller.createPage("qrc:/qml/strategies.qml");
                        // parametrize
                        controller.addPage(lStrategies);
                        lStrategies.startPage();
                    }
                }
            }

            function getSymbol()
            {
                if (footerBar.topPage) return Fonts.botSym;
                if (symbol === Fonts.analyticsSym) return Fonts.analyticsSym;
                if (controller && controller.containsPage("strategies") && !row.marketsAdded)
                {
                    row.marketsAdded = true;
                    return Fonts.analyticsSym;
                }

                return Fonts.botSym;
            }
        }
        QuarkToolButton
        {
            id: ordersButton
            symbol: getSymbol()
            Material.background: "transparent"
            visible: true
            labelYOffset: 3

            property bool enabled_: controller && !controller.containsPage("allorders") || symbol === Fonts.analyticsSym
            symbolColor: enabled_ ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
                                    buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")

            Layout.alignment: Qt.AlignHCenter

            onClicked:
            {
                if (enabled_)
                {
                    if (symbol === Fonts.analyticsSym)
                    {
                        if (controller)
                            controller.unwindToTop();
                    }
                    else
                    {
                        var lOrdering = controller.createPage("qrc:/qml/allorders.qml");
                        // parametrize
                        controller.addPage(lOrdering);
                        lOrdering.startPage();
                    }
                }
            }

            function getSymbol()
            {
                if (footerBar.topPage) return Fonts.listSym;
                if (symbol === Fonts.analyticsSym) return Fonts.analyticsSym;
                if (controller && controller.containsPage("allorders") && !row.marketsAdded)
                {
                    row.marketsAdded = true;
                    return Fonts.analyticsSym;
                }

                return Fonts.listSym;
            }
        }
        QuarkToolButton
        {
            id: historyButton
            symbol: getSymbol()
            Material.background: "transparent"
            visible: true
            labelYOffset: 3

            property bool enabled_: controller && !controller.containsPage("history") || symbol === Fonts.analyticsSym
            symbolColor: enabled_ ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
                                    buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")

            Layout.alignment: Qt.AlignHCenter

            onClicked:
            {
                if (enabled_)
                {
                    if (symbol === Fonts.analyticsSym)
                    {
                        if (controller)
                            controller.unwindToTop();
                    }
                    else
                    {
                        var lHistory = controller.createPage("qrc:/qml/history.qml");
                        // parametrize
                        controller.addPage(lHistory);
                        lHistory.startPage();
                    }
                }
            }

            function getSymbol()
            {
                if (footerBar.topPage) return Fonts.historySym;
                if (symbol === Fonts.analyticsSym) return Fonts.analyticsSym;
                if (controller && controller.containsPage("history") && !row.marketsAdded)
                {
                    row.marketsAdded = true;
                    return Fonts.analyticsSym;
                }

                return Fonts.historySym;
            }
        }
        QuarkToolButton
        {
            id: accountButton
            symbol: Fonts.userSym
            Material.background: "transparent"
            visible: true
            labelYOffset: 3

            property bool enabled_: controller && !controller.containsPage("myaccount")
            symbolColor: enabled_ ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
                                    buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")

            Layout.alignment: Qt.AlignHCenter

            onClicked:
            {
                if (enabled_)
                {
                    var lAccount = controller.createPage("qrc:/qml/myaccount.qml");
                    // parametrize
                    controller.addPage(lAccount);
                    lAccount.startPage();
                }
            }
        }
    }
}
