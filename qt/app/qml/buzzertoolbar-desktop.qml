import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Dialogs 1.1
import QtGraphicalEffects 1.0

import "qrc:/fonts"
import "qrc:/components"

import app.buzzer.commands 1.0 as BuzzerCommands

//
// toolbar
//
QuarkToolBar
{
	id: buzzerToolBar
	height: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 50 : 45
	width: parent.width

	property int extraOffset: 0;
	property int totalHeight: height
	property bool showBottomLine: true;
	property int halfLine: 0
	readonly property int spaceAvatarBuzz_: 10
	readonly property int spaceItems_: 5

	property string searchText: "";
	property string searchPlaceHolder: "";
	property bool searchVisible: false;

	onSearchTextChanged: {
		search.setText(searchText);
	}

	function setSearchText(text, placeholder) {
		//
		search.prevText_ = text;
		search.setText("");
		searchText = text;
		searchPlaceHolder = placeholder;
		searchVisible = true;
	}

	function resetSearchText() {
		//
		searchVisible = false;
		searchText = "";
		searchPlaceHolder = "";
	}

	/*
	background: Rectangle
	{
		color: buzzerApp.isDesktop ?
				   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden") :
				   "transparent"
		anchors.fill: parent
	}
	*/

	signal goHome();
	signal goDrawer();

	signal searchTextEdited(var searchText);
	signal searchTextCleared();

	Component.onCompleted: {
	}

	function activate()	{
	}

	function adjustTrustScore() {
		imageFrame.adjust();
	}

	Connections {
		id: connectedState
		target: buzzerClient

		property var endorsements_;
		property var mistrusts_;

		function onTrustScoreChanged(endorsements, mistrusts) {
			// raw updates - try to get actual (expected to be a bit less, than 'hot' updated)
			endorsements_ = endorsements;
			mistrusts_ = mistrusts;

			console.log("[onTrustScoreChanged]: starting checker for endorsements = " + endorsements + ", mistrusts = " + mistrusts);
			checkTrustScore.start();
		}

		function onCacheReadyChanged() {
			openDrawer.enabled = buzzerClient.buzzerDAppReady === true;
			networkButton.ready(buzzerClient.buzzerDAppReady);
		}

		function onBuzzerDAppReadyChanged() {
			if (buzzerClient.buzzerDAppReady) {
				openDrawer.enabled = buzzerClient.buzzerDAppReady === true;
			}

			networkButton.ready(buzzerClient.buzzerDAppReady);
		}

		function onThemeChanged() {
			networkButton.ready(buzzerClient.buzzerDAppReady);
		}
	}

	Timer {
		id: checkTrustScore
		interval: 2000
		repeat: false
		running: false

		onTriggered: {
			//
			console.log("[checkTrustScore]: checking trust score...");
			updateTrustScoreCommand.process();
		}
	}

	BuzzerCommands.LoadBuzzerTrustScoreCommand {
		id: updateTrustScoreCommand

		property var count_: 0;

		onProcessed: {
			//
			if (endorsements >= connectedState.endorsements_ && mistrusts >= connectedState.mistrusts_) {
				// set actual trust score
				buzzerClient.setTrustScore(endorsements, mistrusts);
				imageFrame.adjust();
			} else if (count_ < 5) {
				console.log("[onTrustScoreChanged]: current endorsements = " + endorsements + ", mistrusts = " + mistrusts);
				checkTrustScore.start(); // repeat
				count_++;
			} else count_ = 0;
		}
	}

	QuarkRoundState {
		id: imageFrame
		x: avatarImage.x - 2
		y: avatarImage.y - 2
		size: avatarImage.displayWidth + 4
		color: getColor()
		background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background")

		function adjust() {
			color = getColor();
		}

		function getColor() {
			//
			if (!buzzerClient.endorsements && !buzzerClient.mistrusts) {
				return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
			}

			var lScoreBase = buzzerClient.getTrustScoreBase() / 10;
			var lIndex = buzzerClient.trustScore / lScoreBase;

			console.log("index = " + Math.round(lIndex) + ", ts = " + buzzerClient.trustScore +
						", endorsements = " + buzzerClient.endorsements + ", mistrusts = " + buzzerClient.mistrusts);
			// TODO: consider to use 4 basic colours:
			// 0 - red
			// 1 - 4 - orange
			// 5 - green
			// 6 - 9 - teal
			// 10 -

			switch(Math.round(lIndex)) {
				case 0: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.0");
				case 1: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.1");
				case 2: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.2");
				case 3: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.3");
				case 4: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.4");
				case 5: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.5");
				case 6: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.6");
				case 7: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.7");
				case 8: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.8");
				case 9: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.9");
				default: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.10");
			}
		}
	}

	Image {
		id: avatarImage

		x: 15
		y: parent.height / 2 - height / 2

		width: avatarImage.displayWidth
		height: avatarImage.displayHeight
		fillMode: Image.PreserveAspectCrop
		mipmap: true

		property bool rounded: true
		property int displayWidth: buzzerToolBar.height - 15
		property int displayHeight: displayWidth

		autoTransform: true

		source: "file://" + buzzerClient.avatar

		layer.enabled: rounded
		layer.effect: OpacityMask {
			maskSource: Item {
				width: avatarImage.displayWidth
				height: avatarImage.displayHeight

				Rectangle {
					anchors.centerIn: parent
					width: avatarImage.displayWidth
					height: avatarImage.displayHeight
					radius: avatarImage.displayWidth
				}
			}
		}
	}

	MouseArea
	{
		id: openDrawer
		x: imageFrame.x - 10
		y: imageFrame.y - 10
		width: imageFrame.width + 20
		height: imageFrame.height + 20
		enabled: buzzerClient.buzzerDAppReady === true
		cursorShape: Qt.PointingHandCursor

		onClicked: {
			goDrawer();
		}
	}

	QuarkSearchField {
		id: search
		width: parent.width - (x + networkButton.width + themeButton.width)
		placeHolder: searchPlaceHolder
		fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 11) : fontPointSize
		visible: searchVisible
		clearButton: false

		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.statusBar")

		Material.theme: buzzerClient.statusBarTheme == "dark" ? Material.Dark : Material.Light;
		Material.accent: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
		Material.background: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
		Material.foreground: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
		Material.primary: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

		cancelSymbolColor: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		placeholderTextColor: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")

		x: avatarImage.x + avatarImage.width + 15
		y: avatarImage.y + avatarImage.height / 2 - calculatedHeight / 2

		property var prevText_: ""

		onSearchTextChanged: {
			//
			// console.log("[onSearchTextChanged]: prev = '" + prevText_ + "', text = '" + searchText + "'");
			if (prevText_ !== searchText) {
				prevText_ = searchText;
				searchTextEdited(searchText);
			}

			if (searchText === "")
				searchTextCleared();
		}

		onInnerTextChanged: {
			if (text === "") searchTextCleared();
		}

		onTextCleared: {
			//
			searchTextCleared();
		}
	}

	QuarkToolButton
	{
		id: networkButton
		symbol: Fonts.networkSym
		Material.background: "transparent"
		visible: true
		labelYOffset: buzzerApp.isDesktop ? 0 : 3
		//labelYOffset: buzzerApp.isDesktop ? (buzzerClient.scaleFactor > 1.0 ? 1 : 2) : 3
		symbolColor: buzzerClient.buzzerDAppReady ?
						 buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
						 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.peer.pending")
		Layout.alignment: Qt.AlignHCenter
		symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : symbolFontPointSize

		x: themeButton.x - width + 5
		y: avatarImage.y + avatarImage.height / 2 - height / 2

		onClicked: {
			//
			var lComponent = null;
			var lPage = null;
			lComponent = Qt.createComponent("qrc:/qml/peers.qml");
			if (lComponent.status === Component.Error) {
				showError(lComponent.errorString());
			} else {
				// pop no-stacked page(s)
				popNonStacked();

				lPage = lComponent.createObject(window);
				lPage.controller = window;

				addPage(lPage);
			}
		}

		function ready(isReady) {
			if (isReady) symbolColor = buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
			else symbolColor = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.peer.pending");
		}
	}

	QuarkToolButton
	{
		id: themeButton
		symbol: getSymbol()
		Material.background: "transparent"
		visible: true
		labelYOffset: buzzerApp.isDesktop ? 0 : 3
		//labelYOffset: buzzerApp.isDesktop ? (buzzerClient.scaleFactor > 1.0 ? 1 : 2) : 3
		symbolColor: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter
		symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : symbolFontPointSize

		x: parent.width - width// - 8
		y: avatarImage.y + avatarImage.height / 2 - height / 2

		onClicked:
		{
			//
			if (symbol === Fonts.sunSym)
			{
				buzzerClient.setTheme("Nova", "light");
				buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
				buzzerClient.save();
			}
			else
			{
				buzzerClient.setTheme("Darkmatter", "dark");
				buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
				buzzerClient.save();
			}
		}

		function getSymbol()
		{
			if (buzzerClient.themeSelector == "light") return Fonts.moonSym;
			return Fonts.sunSym;
		}
	}

	QuarkHLine {
		id: bottomLine
		x1: 0
		y1: parent.height
		x2: parent.width
		y2: parent.height
		penWidth: 1
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Panel.bottom.separator") // "Material.disabledHidden"
		visible: showBottomLine
	}
}
