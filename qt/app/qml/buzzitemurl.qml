﻿import QtQuick 2.15
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Dialogs 1.1
import QtGraphicalEffects 1.0
import Qt.labs.folderlistmodel 2.11
import QtMultimedia 5.8

import app.buzzer.components 1.0 as BuzzerComponents
import app.buzzer.commands 1.0 as BuzzerCommands

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

import "qrc:/lib/numberFunctions.js" as NumberFunctions

Item {
	id: buzzitemurl_

	property int calculatedHeight: spaceItems_;
	property int calculatedWidth: 500
	property var lastUrl_: lastUrl
	property var controller_: controller

	readonly property int spaceLeft_: 15
	readonly property int spaceTop_: 12
	readonly property int spaceRight_: 15
	readonly property int spaceBottom_: 12
	readonly property int spaceAvatarBuzz_: 10
	readonly property int spaceItems_: 5
	readonly property int spaceHeader_: 5
	readonly property int spaceRightMenu_: 15
	readonly property int spaceStats_: -5
	readonly property int spaceLine_: 4

	signal calculatedHeightModified(var value);
	onCalculatedHeightChanged: {
		//console.log("onCalculatedHeightChanged(url) = " + calculatedHeight);
		calculatedHeightModified(calculatedHeight);
	}

	function initialize() {
		sourceInfo.url = lastUrl_;
		sourceInfo.process();
	}

	//
	// make view
	//

	// rich style
	Rectangle {
		//
		id: infoContainer
		x: 0
		y: 0
		color: "transparent"
		width: calculatedWidth
		visible: false //sourceInfo.type === 2

		clip: true

		height: getHeight()

		function getHeight() {
			if (!infoContainer.visible) return spaceItems_;
			return infoImage.height + spaceTop_ + infoTitle.height + spaceItems_ +
					infoDescription.height + spaceItems_ + infoSite.height + spaceBottom_;
		}

		Image {
			id: infoImage
			x: 0
			y: 0
			asynchronous: true
			autoTransform: true
			opacity: 0.0

			width: calculatedWidth
			fillMode: Image.PreserveAspectFit
			mipmap: true

			transitions: Transition {
				NumberAnimation {
					property: "opacity"; duration: 400
				}
			}

			states: State {
				name: "source-changed"; when: infoImage.status === Image.Ready
				PropertyChanges { target: infoImage; opacity: 1.0; }
			}

			onStatusChanged: {
				if (infoImage.status === Image.Ready) {
					infoContainer.visible = true;
					infoContainer.height = infoContainer.getHeight();
					buzzitemurl_.calculatedHeight = infoContainer.height;

					//console.log("Ready for " + sourceInfo.image);
				}
			}
		}

		QuarkLabel {
			id: infoTitle
			x: spaceLeft_
			y: spaceTop_ + infoImage.y + infoImage.height
			text: sourceInfo.title
			width: parent.width - (spaceLeft_ + spaceRight_)
			elide: Text.ElideRight
		}

		QuarkLabel {
			id: infoDescription
			x: spaceLeft_
			y: infoTitle.y + infoTitle.height + spaceItems_
			width: parent.width - (spaceLeft_ + spaceRight_)
			text: sourceInfo.description
			wrapMode: Text.Wrap
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		}

		QuarkSymbolLabel {
			id: infoLink
			x: spaceLeft_
			y: infoDescription.y + infoDescription.height + spaceItems_ + 3
			symbol: Fonts.externalLinkSym
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			font.pointSize: 12
		}

		QuarkLabel {
			id: infoSite
			x: infoLink.x + infoLink.width + spaceItems_
			y: infoDescription.y + infoDescription.height + spaceItems_
			width: parent.width - (spaceRight_ + x)
			text: sourceInfo.host
			elide: Text.ElideRight
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		}

		layer.enabled: true
		layer.effect: OpacityMask {
			maskSource: Item {
				width: infoContainer.width
				height: infoContainer.height

				Rectangle {
					anchors.centerIn: parent
					width: infoContainer.width
					height: infoContainer.height
					radius: 8
				}
			}
		}

		MouseArea {
			id: linkClick
			x: infoContainer.x
			y: infoContainer.y
			width: infoContainer.width
			height: infoContainer.height

			ItemDelegate {
				id: linkClicked
				x: infoContainer.x
				y: infoContainer.y
				width: infoContainer.width
				height: infoContainer.height

				onClicked: {
					Qt.openUrlExternally(lastUrl_);
				}
			}
		}
	}

	Rectangle {
		//
		id: frameContainer
		x: infoContainer.x
		y: infoContainer.y
		color: "transparent"
		border.color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		width: infoContainer.width
		height: infoContainer.height
		visible: infoContainer.visible
		radius: 8
	}

	//
	// url info
	//

	BuzzerComponents.WebSourceInfo {
		id: sourceInfo

		onProcessed: {
			//
			if (type > 0) {
				//console.log("host = " + sourceInfo.host);
				//console.log("image = " + sourceInfo.image);
				infoImage.source = sourceInfo.image;
			}
		}
		onError: {
			console.log("[WebSourceInfo/error]: " + message);
		}
	}
}