import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Dialogs 1.1

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

QuarkPage
{
	id: setupeula_
	key: "setupeula"

	readonly property int spaceLeft_: 20
	readonly property int spaceTop_: 12
	readonly property int spaceRight_: 20
	readonly property int spaceBottom_: 12
	readonly property int spaceAvatarBuzz_: 10
	readonly property int spaceItems_: 5
	readonly property int spaceMedia_: 10
	readonly property int spaceSingleMedia_: 8
	readonly property int spaceMediaIndicator_: 15
	readonly property int spaceHeader_: 5
	readonly property int spaceRightMenu_: 15
	readonly property int spaceStats_: -5
	readonly property int spaceLine_: 4
	readonly property int spaceThreaded_: 33
	readonly property int spaceThreadedItems_: 4
	readonly property var hederFontSize_: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 9)) : (buzzerApp.defaultFontSize() + 13)
	readonly property var bodyFontSize_: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 3)) : (buzzerApp.defaultFontSize() + 7)

	Component.onCompleted: {
		buzzerApp.lockPortraitOrientation();
        closePageHandler = closePage;
		activatePageHandler = activatePage;
    }

	function closePage() {
        stopPage();
        destroy(500);
        controller.popPage();
    }

	function activatePage()	{
		buzzerApp.lockPortraitOrientation();
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
		toolBar.activate();
	}

	function onErrorCallback(error) {
        controller.showError(error);
    }

    //
    // toolbar
    //
	SetupToolBarDesktop {
		id: toolBar
		extraOffset: topOffset
		x: 10
		width: parent.width - 20
	}

	//
	// Text
	//
	Flickable {
		 id: pageContainer
		 x: 0
		 y: toolBar.y + toolBar.height /* + 20 */
		 width: parent.width
		 height: parent.height - (toolBar.y + toolBar.height + 20 + nextButton.height + 10 /* + 20 */)
		 contentHeight: buzzerText5.y + buzzerText5.height + toolBar.height + spaceBottom_
		 clip: true

		 onDragStarted: {
		 }

		Image {
			id: buzzerImage
			x: spaceLeft_
			y: spaceTop_
			width: 50
			height: 50
			fillMode: Image.PreserveAspectFit
			source: "../images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "buzzer.icon")
			mipmap: true
		}

		QuarkLabel {
			id: buzzerHeader
			x: buzzerImage.x + buzzerImage.width + spaceLeft_
			y: spaceTop_ + buzzerImage.height / 2 - height / 2
			width: parent.width
			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.eula")
			font.pointSize: hederFontSize_
			wrapMode: Text.Wrap

			onLinkActivated: {
				Qt.openUrlExternally(link);
			}
		}

		QuarkLabel {
			id: buzzerText0
			x: buzzerImage.x
			y: buzzerImage.y + buzzerImage.height + spaceTop_
			width: parent.width - (spaceLeft_ + spaceRight_)
			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.eula.0")
			font.pointSize: bodyFontSize_
			wrapMode: Text.Wrap

			onLinkActivated: {
				Qt.openUrlExternally(link);
			}
		}

		QuarkLabel {
			id: buzzerText1
			x: buzzerText0.x
			y: buzzerText0.y + buzzerText0.height + spaceTop_
			width: parent.width - (spaceLeft_ + spaceRight_)
			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.eula.1")
			font.pointSize: bodyFontSize_
			wrapMode: Text.Wrap

			onLinkActivated: {
				Qt.openUrlExternally(link);
			}
		}

		QuarkLabel {
			id: buzzerText2
			x: buzzerText1.x
			y: buzzerText1.y + buzzerText1.height + spaceTop_
			width: parent.width - (spaceLeft_ + spaceRight_)
			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.eula.2")
			font.pointSize: bodyFontSize_
			wrapMode: Text.Wrap

			MouseArea {
				anchors.fill: parent
				cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
				acceptedButtons: Qt.NoButton
			}

			onLinkActivated: {
				//
				if (link[0] === '@') {
					var lId = buzzerClient.getConversationsList().locateConversation(link);
					// conversation found
					if (lId !== "") {
						openBuzzerConversation(lId);
					} else {
						createConversation.process(link);
					}
				} else Qt.openUrlExternally(link);
			}
		}

		QuarkLabel {
			id: buzzerText3
			x: buzzerText2.x
			y: buzzerText2.y + buzzerText2.height + spaceTop_
			width: parent.width - (spaceLeft_ + spaceRight_)
			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.eula.3")
			font.pointSize: bodyFontSize_
			wrapMode: Text.Wrap

			MouseArea {
				anchors.fill: parent
				cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
				acceptedButtons: Qt.NoButton
			}

			onLinkActivated: {
				//
				if (link[0] === '@') {
					var lId = buzzerClient.getConversationsList().locateConversation(link);
					// conversation found
					if (lId !== "") {
						openBuzzerConversation(lId);
					} else {
						createConversation.process(link);
					}
				} else Qt.openUrlExternally(link);
			}
		}

		QuarkLabel {
			id: buzzerText4
			x: buzzerText3.x
			y: buzzerText3.y + buzzerText3.height + spaceTop_
			width: parent.width - (spaceLeft_ + spaceRight_)
			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.eula.4")
			font.pointSize: bodyFontSize_
			wrapMode: Text.Wrap

			onLinkActivated: {
				Qt.openUrlExternally(link);
			}
		}

		QuarkLabel {
			id: buzzerText5
			x: buzzerText4.x
			y: buzzerText4.y + buzzerText4.height + spaceTop_
			width: parent.width - (spaceLeft_ + spaceRight_)
			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.eula.5")
			font.pointSize: bodyFontSize_
			wrapMode: Text.Wrap

			onLinkActivated: {
				Qt.openUrlExternally(link);
			}
		}
	}

	QuarkButton	{
		id: nextButton
		x: welcomeText.x + 2
		y: parent.height - height - 20
		text: buzzerApp.getLimited() ? buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.accept") : buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.next")
		visible: true
		width: welcomeText.width
		font.capitalization: Font.AllUppercase

		Layout.minimumWidth: 150
		Layout.alignment: Qt.AlignHCenter

		onClicked: {
			var lComponent = null;
			var lPage = null;

			lComponent = Qt.createComponent("qrc:/qml/setupqbit-desktop.qml");
			lPage = lComponent.createObject(window);
			lPage.controller = controller;

			addPageLocal(lPage);
		}
	}
}
