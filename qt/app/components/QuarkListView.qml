// QuarkListView.qml

import QtQuick 2.15
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4

import "qrc:/fonts"

ListView
{
    id: listView

    Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
    Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
    Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background");
    Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
    Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

    property bool pullReady: header.refresh;
    property int currentIndexAtTop: 0;
    property real pullScale: 0.0;
    property real pullHeight: 0.0;
    property bool usePull: false;
	property int feedDelta: 12;
	property bool useFeed: true;

	signal feedReady();

    highlightFollowsCurrentItem: true;

    onContentYChanged:
    {
		currentIndexAtTop = indexAt(1, contentY);

		if (useFeed && model && (currentIndexAtTop + feedDelta > model.count /*|| lEffectiveBottom < 30*/)) {
			//console.log("[feed/onContentYChanged/fired]: lEffectiveBottom = " + lEffectiveBottom);
			feedReady();
		}

		if (!usePull) return;

		var lHeight = contentY - originY;
		var lCoeff = 0.1;
        if (lHeight < 0) pullHeight = -lHeight;
        else pullHeight = 0.0;

        pullScale = lHeight * lCoeff;

		//console.log("[ListView]: pullHeight = " + pullHeight + ", pullScale = " +
		//			pullScale + ", currentIndexAtTop = " + currentIndexAtTop + ", contentY = " + contentY);

		//var lEffectiveBottom = ((contentHeight - contentY) * 100.0 / contentHeight);
		//console.log("[feed/onContentYChanged]: lEffectiveBottom = " + lEffectiveBottom + ", currentIndexAtTop = " + currentIndexAtTop
		//			+ ", total = " + (currentIndexAtTop + feedDelta) + ", model.count = " + model.count);
    }

    Item
    {
        id: header
        height: pullHeight
        width: parent.width

		y: listView.headerItem && listView.headerPositioning != ListView.InlineHeader ? listView.headerItem.height : 0
        visible: usePull && listView.currentIndexAtTop < 0

		property bool refresh: usePull ? (state == "pulled" ? true : false) : false

        Row
        {
            spacing: 6
            height: childrenRect.height
            anchors.centerIn: parent

            QuarkLabel
            {
                id: arrow
                text: Fonts.circleArrowUpSym
                font.family: Fonts.icons
                font.weight: Font.Normal
				//font.pixelSize: getPixelSize()
				font.pointSize: getPixelSize() < 0 ? 1 : getPixelSize()
                //renderType: Text.NativeRendering
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                transformOrigin: Item.Center
                Behavior on rotation { NumberAnimation { duration: 200 } }
				visible: getPixelSize() > 1

                function getPixelSize()
                {
					var lSize = 3 * (-pullScale);
					if (lSize < 1) return 1;
                    if (lSize < 30) return lSize;
                    return 30;
                }
            }
        }

        states:
        [
            State
            {
                name: "base"; when: listView.currentIndexAtTop < 0 && listView.pullScale > -12
                PropertyChanges { target: arrow; rotation: 180 }
            },
            State
            {
				name: "pulled"; when: listView.currentIndexAtTop < 0 && listView.pullScale <= -12
                PropertyChanges { target: arrow; rotation: 0 }
            }
        ]
    }

	ScrollIndicator.vertical: ScrollIndicator {
	}
}

