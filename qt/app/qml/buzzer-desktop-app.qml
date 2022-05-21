import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.3
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.15
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Window 2.15

import app.buzzer.components 1.0 as QuarkComponents

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

ApplicationWindow {
    id: window
	width: 920
	height: 740
    visible: true
	title: "Buzzer"

    property string onlineCount: "";
	property var pagesView;
	property int bottomBarHeight: 0;
	property var mainToolBar;
	property string activePageBackground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Window.background")

	color: activePageBackground

	onWidthChanged: {
		buzzerApp.setBackgroundColor(activePageBackground);
		buzzerClient.setProperty("Client.width", width);
	}

	onHeightChanged: {
		buzzerClient.setProperty("Client.height", height);
	}

	Component.onCompleted: {
		buzzerApp.setBackgroundColor(activePageBackground);

		var lWidth = buzzerClient.getProperty("Client.width");
		if (lWidth !== "") width = parseInt(lWidth);
		else width = 920;

		var lHeight = buzzerClient.getProperty("Client.height");
		if (lHeight !== "") height = parseInt(lHeight);
		else height = 740;
	}

	onClosing:
	{
	}

	flags: Qt.Window |
		   Qt.MaximizeUsingFullscreenGeometryHint |
		   Qt.FramelessWindowHint |
		   Qt.WindowMinimizeButtonHint

	Keys.onPressed: {
		console.log("[event]: " + event);
		if (event.key === Qt.Key_Escape)
			closePage();
	}

	toolBar: Rectangle {
		id: windowBar
		width: parent.width
		height: buzzerClient.scaleFactor * 25
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background.transparent")

		MouseArea {
			id: topArea
			height: 5
			anchors {
				top: parent.top
				left: parent.left
				right: parent.right
			}

			cursorShape: Qt.SizeVerCursor

			property var fixedBottom;

			onPressed: {
				fixedBottom = window.y + window.height;
			}

			onMouseYChanged: {
				var lPoint = mapToGlobal(mouse.x, mouse.y);
				var lY = window.y;
				window.setY(lPoint.y);

				var lHeight = window.height - (lPoint.y - lY);
				lHeight -= ((lY + lHeight) - fixedBottom);
				window.setHeight(lHeight);
			}
		}

		MouseArea {
			id: moveArea
			y: topArea.height
			height: parent.height - topArea.height
			width: parent.width

			onPressed: {
				previousX = mouseX;
				previousY = mouseY;
			}

			onMouseXChanged: {
				var dx = mouseX - previousX;
				window.setX(window.x + dx);
			}

			onMouseYChanged: {
				var dy = mouseY - previousY;
				window.setY(window.y + dy);
			}
		}

		Rectangle {
			id: minButton
			x: parent.width - (3 * parent.height)
			width: parent.height
			height: parent.height
			color: "transparent"

			QuarkSymbolLabel {
				id: minButtonLabel
				symbol: Fonts.minimizeSym
				font.pointSize: buzzerClient.scaleFactor * 9
				color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")

				x: parent.width / 2 - width / 2
				y: parent.height / 2 - height / 2

				Connections {
					target: buzzerClient
					function onThemeChanged() {
						minButtonLabel.color =
								buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
					}
				}
			}

			MouseArea {
				id: minButtonArea
				anchors.fill: parent
				hoverEnabled: true

				onEntered: {
					minButton.color = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.hidden")
					minButtonLabel.color = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground");
				}

				onExited: {
					minButton.color = "transparent"
					minButtonLabel.color = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
				}

				onPressed: {
					minButton.color = Qt.darker(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.hidden"), 1.1);
				}

				onReleased: {
					minButton.color = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.hidden");
				}

				onClicked: {
					window.showMinimized();
				}
			}
		}

		Rectangle {
			id: maxButton
			x: parent.width - (2 * parent.height)
			width: parent.height
			height: parent.height
			color: "transparent"

			QuarkSymbolLabel {
				id: maxButtonLabel
				symbol: window.visibility === Window.Maximized ? Fonts.restoreSym : Fonts.maximizeSym
				font.pointSize: buzzerClient.scaleFactor * 9
				color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")

				x: parent.width / 2 - width / 2
				y: parent.height / 2 - height / 2

				Connections {
					target: buzzerClient
					function onThemeChanged() {
						maxButtonLabel.color =
								buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
					}
				}
			}

			MouseArea {
				id: maxButtonArea
				anchors.fill: parent
				hoverEnabled: true

				onEntered: {
					maxButton.color = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.hidden")
					maxButtonLabel.color = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground");
				}

				onExited: {
					maxButton.color = "transparent"
					maxButtonLabel.color = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
				}

				onPressed: {
					maxButton.color = Qt.darker(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.hidden"), 1.1);
				}

				onReleased: {
					maxButton.color = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.hidden");
				}

				onClicked: {
					if (window.visibility === Window.Maximized) {
						window.showNormal();
					} else {
						window.showMaximized();
					}
				}
			}
		}

		Rectangle {
			id: closeButton
			x: parent.width - parent.height
			width: parent.height
			height: parent.height
			color: "transparent"

			QuarkSymbolLabel {
				id: closeButtonLabel
				symbol: Fonts.cancelSym
				font.pointSize: buzzerClient.scaleFactor * 12
				color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")

				x: parent.width / 2 - width / 2
				y: parent.height / 2 - height / 2

				Connections {
					target: buzzerClient
					function onThemeChanged() {
						closeButtonLabel.color =
								buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
					}
				}
			}

			MouseArea {
				id: closeButtonArea
				anchors.fill: parent
				hoverEnabled: true

				onEntered: {
					closeButton.color = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.downBackground")
					closeButtonLabel.color = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground");
				}

				onExited: {
					closeButton.color = "transparent"
					closeButtonLabel.color = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
				}

				onPressed: {
					closeButton.color = Qt.darker(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.downBackground"), 1.1);
				}

				onReleased: {
					closeButton.color = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.downBackground");
				}

				onClicked: {
					window.close();
				}
			}
		}

		//
		// work-a-round: frame for frameless window
		//

		QuarkComponents.Line {
			id: headerLeftLine
			x1: 0
			y1: 0
			y2: parent.height
			x2: 0
			penWidth: 1
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
			visible: true
		}

		QuarkComponents.Line {
			id: headerRightLine
			x1: parent.width-1
			y1: 0
			y2: parent.height
			x2: parent.width-1
			penWidth: 1
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
			visible: true
		}

		QuarkComponents.Line {
			id: headerTopLine
			x1: 0
			y1: 0
			y2: 0
			x2: parent.width
			penWidth: 1
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
			visible: true
		}
	}

    function createPage(source)
    {
        var lComponent = Qt.createComponent(source);
        var lPage = lComponent.createObject(window);
        lPage.controller = window;
        return lPage;
    }

	//
	// facade methods
	//

	Connections {
		target: buzzerClient

		function onTryOpenThread(buzzChainId, buzzId, buzzerAlias, buzzBody) {
			//
			window.requestActivate();
			window.showNormal();
			window.show();
			window.raise();
			//
			openThread(buzzChainId, buzzId, buzzerAlias, buzzBody);
		}

		function onTryOpenBuzzfeedByBuzzer(buzzer) {
			//
			window.requestActivate();
			window.showNormal();
			window.show();
			window.raise();
			//
			openBuzzfeedByBuzzer(buzzer);
		}

		function onTryOpenConversation(id, conversationId, conversation, list) {
			//
			window.requestActivate();
			window.showNormal();
			window.show();
			window.raise();
			//
			openConversation(conversationId, conversation, list, id);
		}
	}

	function getDepth() {
		if (buzzerClient.configured()) {
			for (var lIdx = pagesView.count - 1; lIdx >= 0; lIdx--) {
				if (pagesView.itemAt(lIdx).key === "buzzermain")
					return pagesView.depth - lIdx;
			}

			return 0;
		}

		return pagesView.depth - 1;
    }

	function addPage(page) {
		pagesView.addItem(page);
		pagesView.setCurrentIndex(pagesView.count-1);
		//page.visible = true;
		page.updateStatusBar();
		if (page.activatePageHandler) page.activatePageHandler();
	}

	function activatePage(key, itemId) {
		for (var lIdx = pagesView.count - 1; lIdx >= 0; lIdx--) {
			var lPage = pagesView.itemAt(lIdx);
			if (lPage.key === key) {
				//
				pagesView.currentIndex = lIdx;
				buzzerClient.setTopId(key);
				//
				if (lPage.activatePageHandler) lPage.activatePageHandler();
				if (itemId) lPage.showItem(itemId);
				//
				return true;
			}
		}

		return false;
	}

	function enumStakedPages() {
		var lArray = [];
		for (var lIdx = pagesView.count - 1; lIdx >= 0; lIdx--) {
			var lPage = pagesView.itemAt(lIdx);
			if (lPage.stacked === true) {
				lArray.push(lPage);
			}
		}

		return lArray;
	}

	function unwindToTop() {
        var lArray = [];
		for (var lIdx = pagesView.count - 1; lIdx >= 0; lIdx--) {
			var lPage = pagesView.itemAt(lIdx);
			if (lPage.key !== "buzzfeed") {
                lArray.push(lPage);
            }
            else break;
        }

		for (var lI = 0; lI < lArray.length; lI++) {
            lArray[lI].closePage();
        }
    }

	function popPage(page) {
		//
		var lPage;
		//
		if (page === undefined) {
			if (pagesView.count - 1 > 0) {
				lPage = pagesView.itemAt(pagesView.count - 1);
				pagesView.removeItem(lPage);
				pagesView.currentIndex = pagesView.count - 1;
			}
		} else {
			for (var lIdx = pagesView.count - 1; lIdx >= 0; lIdx--) {
				lPage = pagesView.itemAt(lIdx);
				if (lPage.key === page.key) {
					//
					pagesView.removeItem(page);
					if (pagesView.count > 1) {
						if (lIdx - 1 === 0) pagesView.currentIndex = 1;
						else if (lIdx - 1 > 0) pagesView.currentIndex = lIdx - 1;
						else pagesView.currentIndex = 0;
					} else pagesView.currentIndex = 0;

					break;
				}
			}
		}

		var lNewPage = pagesView.itemAt(pagesView.currentIndex);
		if (lNewPage) {
			buzzerClient.setTopId(lNewPage.key); // reset
			lNewPage.updateStatusBar();
			if (lNewPage.activatePageHandler) lNewPage.activatePageHandler();
		} else {
			buzzerClient.setTopId(""); // reset
		}
    }

	function popNonStacked() {
		//
		var lArray = [];
		for (var lIdx = pagesView.count - 1; lIdx > 0; lIdx--) {
			var lPage = pagesView.itemAt(lIdx);
			if (lPage.stacked === false) {
				//
				lArray.push(lPage);
			} else {
				//
				break;
			}
		}

		//
		for (var lI = 0; lI < lArray.length; lI++) {
			pagesView.removeItem(lArray[lI]);
		}

		//
		pagesView.currentIndex = pagesView.count - 1;
		var lNewPage = pagesView.itemAt(pagesView.currentIndex);
		if (lNewPage) {
			buzzerClient.setTopId(lNewPage.key); // reset
		} else {
			buzzerClient.setTopId(""); // reset
		}
	}

	function isTop() {
		return pagesViewLocal.depth - 1;
	}

	function openThread(buzzChainId, buzzId, buzzerAlias, buzzBody) {
		//
		var lComponent = null;
		var lPage = null;

		// pop no-stacked page(s)
		popNonStacked();

		// locate and activate
		if (activatePage(buzzId)) return;

		lComponent = buzzerApp.isDesktop ? Qt.createComponent("qrc:/qml/buzzfeedthread.qml", window) :
										   Qt.createComponent("qrc:/qml/buzzfeedthread.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(window);
			lPage.controller = window;

			lPage.updateStakedInfo(buzzId, buzzerAlias, buzzBody.replace(/(\r\n|\n|\r)/gm, ""));
			lPage.start(buzzChainId, buzzId);

			addPage(lPage);
		}
	}

	function openMedia(pkey, index, media, player, instance, buzzId, buzzerAlias, buzzBody) {
		//
		var lComponent = null;
		var lPage = null;

		// pop no-stacked page(s)
		popNonStacked();

		// locate and activate
		if (activatePage(buzzId + "-media")) return;

		lComponent = buzzerApp.isDesktop ? Qt.createComponent("qrc:/qml/buzzmedia.qml", window) :
										   Qt.createComponent("qrc:/qml/buzzmedia.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(window);
			lPage.controller = window;

			lPage.buzzMedia_ = media;
			lPage.mediaPlayerController = player;
			lPage.initialize(pkey, index, instance, buzzId, buzzBody);

			var lTitle = "Media";
			if (buzzBody !== "") lTitle = buzzBody.replace(/(\r\n|\n|\r)/gm, "");
			lPage.updateStakedInfo(buzzId + "-media", buzzerAlias, lTitle);

			addPage(lPage);
		}
	}

	function openBuzzfeedByBuzzer(buzzerName) {
		//
		var lComponent = null;
		var lPage = null;

		// pop no-stacked page(s)
		popNonStacked();

		// locate and activate
		if (activatePage(buzzerName)) return;

		lComponent = Qt.createComponent("qrc:/qml/buzzfeedbuzzer.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(window);
			lPage.controller = window;

			lPage.updateStakedInfo(buzzerName, buzzerName, "...");
			lPage.start(buzzerName);

			addPage(lPage);
		}
	}

	function openBuzzfeedByTag(tag) {
		//
		var lComponent = null;
		var lPage = null;

		// pop no-stacked page(s)
		popNonStacked();

		// locate and activate
		if (activatePage(tag)) return;

		lComponent = Qt.createComponent("qrc:/qml/buzzfeedtag.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(window);
			lPage.controller = window;

			lPage.updateStakedInfo(tag, tag, "...");
			lPage.start(tag);

			addPage(lPage);
		}
	}

	function openBuzzfeedBuzzerEndorsements(buzzer) {
		//
		var lComponent = null;
		var lPage = null;

		// pop no-stacked page(s)
		popNonStacked();

		// locate and activate
		if (activatePage(buzzer + "-endorsements")) return;

		lComponent = Qt.createComponent("qrc:/qml/buzzfeedendorsements.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(window);
			lPage.controller = window;

			lPage.updateStakedInfo(buzzer + "-endorsements", buzzer, buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.endorsers"));
			lPage.start(buzzer);

			addPage(lPage);
		}
	}

	function openBuzzfeedBuzzerMistrusts(buzzer) {
		//
		var lComponent = null;
		var lPage = null;

		// pop no-stacked page(s)
		popNonStacked();

		// locate and activate
		if (activatePage(buzzer + "-mistrusts")) return;

		lComponent = Qt.createComponent("qrc:/qml/buzzfeedmistrusts.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(window);
			lPage.controller = window;

			lPage.updateStakedInfo(buzzer + "-mistrusts", buzzer, buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.mistrusters"));
			lPage.start(buzzer);

			addPage(lPage);
		}
	}

	function openBuzzfeedBuzzerFollowers(buzzer) {
		//
		var lComponent = null;
		var lPage = null;

		// pop no-stacked page(s)
		popNonStacked();

		// locate and activate
		if (activatePage(buzzer + "-followers")) return;

		lComponent = Qt.createComponent("qrc:/qml/buzzfeedfollowers.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(window);
			lPage.controller = window;
			lPage.start(buzzer);

			lPage.updateStakedInfo(buzzer + "-followers", buzzer, buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.followers"));
			addPage(lPage);
		}
	}

	function openBuzzfeedBuzzerFollowing(buzzer) {
		//
		var lComponent = null;
		var lPage = null;

		// pop no-stacked page(s)
		popNonStacked();

		// locate and activate
		if (activatePage(buzzer + "-following")) return;

		lComponent = Qt.createComponent("qrc:/qml/buzzfeedfollowing.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(window);
			lPage.controller = window;
			lPage.start(buzzer);

			lPage.updateStakedInfo(buzzer + "-following", buzzer, buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.following"));
			addPage(lPage);
		}
	}

	function openConversation(conversationId, conversation, conversationModel, messageId) {
		//
		var lComponent = null;
		var lPage = null;

		// pop no-stacked page(s)
		popNonStacked();

		// locate and activate
		if (activatePage(conversationId, messageId)) return;

		lComponent = Qt.createComponent("qrc:/qml/conversationthread.qml", window);
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(window);
			lPage.controller = window;

			//
			var lCounterpart = conversation.counterpartyInfoId;
			if (conversation.side === 0) {
				lCounterpart = conversation.creatorInfoId;
			}

			lPage.updateStakedInfo(conversationId, buzzerClient.getBuzzerAlias(lCounterpart), buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.conversation.title"));
			addPage(lPage);

			lPage.start(conversationId, conversation, conversationModel, messageId);
		}
	}

	//
	// inner methods
	//

	function addPageLocal(page)
	{
		pagesViewLocal.push(page, StackView.Transition);
		page.updateStatusBar();
		if (page.activatePageHandler) page.activatePageHandler();
	}

	function popPageLocal(page)
	{
		pagesViewLocal.pop();

		var lPage = pagesView.get(pagesView.depth - 1);
		lPage.updateStatusBar();
		if (lPage.activatePageHandler) lPage.activatePageHandler();
	}

	function errorDialogAccepted()
    {
        lastErrorCodeDialog = 0;
    }

	function showError(error, force)
    {
        var lComponent;
        var lPage;

        if (suspended) return;

        if (typeof(error) !== "string")
        {
			if (error.code !== lastErrorCodeDialog || force)
            {
                lastErrorCodeDialog = error.code;

                lComponent = Qt.createComponent("errordialog.qml");
                lPage = lComponent.createObject(window);
                lPage.show(error, errorDialogAccepted);
            }
        }
		else if (lastErrorMessageDialog !== error || force)
        {
			lastErrorMessageDialog = error;
			lComponent = Qt.createComponent("errordialog.qml");
            lPage = lComponent.createObject(window);
            lPage.showText(error);
        }
    }

    function containsPage(key)
    {
        for (var lIdx = 0; lIdx < pagesView.depth; lIdx++)
        {
            if (pagesView.get(lIdx).key === key) return true;
        }

        return false;
    }

	function openDrawer() {
		myBuzzer.initialize();
		drawerMenu.prepare();
		drawer.open();
	}

	function closeDrawer() {
		drawer.close();
	}

	property var lastErrorCodeDialog: "";
	property var lastErrorMessageDialog: "";
	property bool suspended: false;
    property int extraPadding: 0;

    StackView
    {
		id: pagesViewLocal
        anchors.fill: parent

        delegate: StackViewDelegate
        {
            popTransition: StackViewTransition
            {
                PropertyAnimation
                {
                    target: enterItem
                    property: "x"
					from: -pagesViewLocal.width
                    to: 0
                    duration: 100
                }
                PropertyAnimation
                {
                    target: exitItem
                    property: "x"
                    from: 0
					to: pagesViewLocal.width
                    duration: 100
                }
            }

            pushTransition: StackViewTransition
            {
                PropertyAnimation
                {
                    target: enterItem
                    property: "x"
					from: pagesViewLocal.width
                    to: 0
                    duration: 100
                }
                PropertyAnimation
                {
                    target: exitItem
                    property: "x"
                    from: 0
					to: -pagesViewLocal.width
                    duration: 100
                }
            }
        }

        initialItem: QuarkPage
        {
            id: welcome

            Rectangle
            {
                id: welcomeRect
                anchors.fill: parent

                border.color: "transparent"
                color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background");

                Image
                {
                    id: welcomeImage
					width: parent.width - parent.width / 2 > 300 ? 300 : parent.width - parent.width / 2
                    fillMode: Image.PreserveAspectFit
                    mipmap: true
					x: parent.width / 2 - (width) / 2
                    y: parent.height / 2 - paintedHeight / 2
					source: "../images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "buzzer.splash")
                }

                Component.onCompleted:
                {
					// apearing.start();
                }
            }
        }
    }

	Drawer
	{
		Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
		Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
		Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
		Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
		Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

		id: drawer
		width: buzzerClient.scaleFactor * 400
		height: window.height
		interactive: opened

		BuzzerHeader {
			id: myBuzzer
			width: parent.width
			controller_: window
			infoDialog: drawerInfoDialog
		}

		InfoDialog {
			id: drawerInfoDialog
			bottom: 50
		}

		QuarkListView {
			id: drawerMenu
			x: 0
			y: myBuzzer.y + myBuzzer.calculatedHeight
			height: parent.height - y
			width: parent.width
			model: ListModel { id: menuModel_ }
			clip: true

			delegate: ItemDelegate {
				id: listDelegate
				width: drawerMenu.width
				leftPadding: 0
				rightPadding: 0
				topPadding: 0
				bottomPadding: 0
				clip: false

				onClicked: {
					//
					drawer.close();

					// pop no-stacked page(s)
					popNonStacked();

					//
					var lComponent = null;
					var lPage = null;

					if (key === "createNewBuzzer" || key === "editBuzzer") {
						lComponent = Qt.createComponent("qrc:/qml/buzzercreateupdate-desktop.qml");
						if (lComponent.status === Component.Error) {
							showError(lComponent.errorString());
						} else {
							lPage = lComponent.createObject(window);
							lPage.controller = window;
							lPage.initialize(key === "createNewBuzzer" ? "CREATE" : "UPDATE");

							addPage(lPage);
						}
					} else if (key === "qbitKey") {
						lComponent = Qt.createComponent("qrc:/qml/buzzerqbitkey.qml");
						if (lComponent.status === Component.Error) {
							showError(lComponent.errorString());
						} else {
							lPage = lComponent.createObject(window);
							lPage.controller = window;

							addPage(lPage);
						}
					} else if (key === "personalFeed") {
						//
						window.openBuzzfeedByBuzzer(buzzerClient.name);
					} else if (key === "network") {
						lComponent = Qt.createComponent("qrc:/qml/peers.qml");
						if (lComponent.status === Component.Error) {
							showError(lComponent.errorString());
						} else {
							lPage = lComponent.createObject(window);
							lPage.controller = window;

							addPage(lPage);
						}
					} else if (key === "quickHelp") {
						lComponent = Qt.createComponent("qrc:/qml/buzzerquickhelp.qml");
						if (lComponent.status === Component.Error) {
							showError(lComponent.errorString());
						} else {
							lPage = lComponent.createObject(window);
							lPage.controller = window;

							addPage(lPage);
						}
					}
				}

				Binding {
					target: background
					property: "color"
					value: listDelegate.highlighted ?
							   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.highlight"):
							   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
				}

				contentItem: Rectangle {
					width: drawerMenu.width
					color: "transparent"
					border.color: "transparent"
					anchors.fill: parent

					QuarkSymbolLabel {
						id: symbolLabel
						x: 15 + leftOffset
						y: parent.height / 2 - height / 2
						verticalAlignment: Text.AlignVCenter
						Material.background: "transparent"
						Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
						visible: true
						symbol: keySymbol
						font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 12) : defaultFontSize
					}

					QuarkLabelRegular {
						id: textLabel
						x: keySymbol ? (buzzerClient.scaleFactor * 40) : 15
						y: parent.height / 2 - height / 2
						width: drawerMenu.width - ((keySymbol ? (buzzerClient.scaleFactor * 40) : 15) + 10)
						text: name
						elide: Text.ElideRight
						verticalAlignment: Text.AlignVCenter
						Material.background: "transparent"
						Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
						visible: true
						font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 11) : defaultFontPointSize
					}
				}
			}

			ScrollIndicator.vertical: ScrollIndicator { }

			function prepare() {
				if (menuModel_.count) return;

				menuModel_.append({
					key: "createNewBuzzer",
					keySymbol: Fonts.userPlusSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.create.new"),
					leftOffset: 0});
				menuModel_.append({
					key: "editBuzzer",
					keySymbol: Fonts.userEditSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.edit.buzzer"),
					leftOffset: 0});
				menuModel_.append({
					key: "qbitKey",
					keySymbol: Fonts.secretSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.qbit.key"),
					leftOffset: 0});
				menuModel_.append({
					key: "personalFeed",
					keySymbol: Fonts.flashSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.personal.feed"),
					leftOffset: buzzerClient.scaleFactor * 2});
				menuModel_.append({
					key: "network",
					keySymbol: Fonts.networkSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.network"),
					leftOffset: (-1) * (buzzerClient.scaleFactor * 2)});
				menuModel_.append({
					key: "quickHelp",
					keySymbol: Fonts.helpSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.quick.help"),
					leftOffset: 0});
			}
		}
	}

	Timer
	{
		id: adjustTimer
		interval: 500
		repeat: false
		running: false

		onTriggered: {
			buzzerApp.setBackgroundColor(activePageBackground);
		}
	}

    Timer
    {
        id: startUpTimer
		interval: 1000
        repeat: false
        running: true

        onTriggered:
        {
            var lComponent = null;
            var lPage = null;

			if (buzzerClient.configured())
			{
				if (buzzerClient.pinAccessConfigured())
				{
					lComponent = Qt.createComponent("qrc:/qml/pin.qml");
					lPage = lComponent.createObject(window);
					lPage.pinCaption = buzzerApp.getLocalization(buzzerClient.locale, "EnterPin.Title");
					lPage.pinSubmit = buzzerApp.getLocalization(buzzerClient.locale, "Button.Unlock");
					lPage.nextPage = "buzzfeed.qml";
					lPage.pinAction = lPage.actionUnlock;
					lPage.controller = window;

					addPageLocal(lPage);
				} else {
					lComponent = Qt.createComponent("qrc:/qml/buzzer-main-desktop.qml");
					if (lComponent.status === Component.Error) {
						showError(lComponent.errorString());
					} else {
						lPage = lComponent.createObject(window);
						lPage.controller = window;

						addPageLocal(lPage);
					}
				}
			} else {
				lComponent = Qt.createComponent("qrc:/qml/setupinfo-desktop.qml");
				if (lComponent.status === Component.Error) {
					showError(lComponent.errorString());
				} else {
					lPage = lComponent.createObject(window);
					lPage.controller = window;

					addPageLocal(lPage);
				}
			}
        }
    }

	//
	// resizing
	//

	property int previousX;
	property int previousY;

	MouseArea {
		id: bottomArea
		height: 5
		anchors {
			bottom: parent.bottom
			left: parent.left
			right: parent.right
		}
		cursorShape: Qt.SizeVerCursor

		onPressed: {
		}

		onMouseYChanged: {
			var lPoint = mapToGlobal(mouse.x, mouse.y);
			window.setHeight(lPoint.y - window.y);
		}
	}

	MouseArea {
		id: leftArea
		width: 5
		anchors {
			top: parent.top
			bottom: bottomArea.top
			left: parent.left
		}
		cursorShape: Qt.SizeHorCursor

		property var fixedLeft;

		onPressed: {
			fixedLeft = window.x + window.width;
		}

		onMouseXChanged: {
			var lPoint = mapToGlobal(mouse.x, mouse.y);
			var lX = window.x;
			window.setX(lPoint.x);

			var lWidth = window.width - (lPoint.x - lX);
			lWidth -= ((lX + lWidth) - fixedLeft);
			window.setWidth(lWidth);
		}
	}

	MouseArea {
		id: rightArea
		width: 5
		anchors {
			top: parent.top
			bottom: bottomArea.top
			right: parent.right
		}
		cursorShape:  Qt.SizeHorCursor

		onPressed: {
		}

		property var previousPoint;

		onMouseXChanged: {
			var lPoint = mapToGlobal(mouse.x, mouse.y);
			window.setWidth(lPoint.x - window.x);
		}
	}

	//
	// work-a-round: frame for frameless window
	//

	QuarkComponents.Line {
		id: leftLine
		x1: 0
		y1: 0
		x2: 0
		y2: parent.height
		penWidth: 1
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		visible: true
	}

	QuarkComponents.Line {
		id: rightLine
		x1: parent.width-1
		y1: 0
		x2: parent.width-1
		y2: parent.height
		penWidth: 1
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		visible: true
	}

	QuarkComponents.Line {
		id: bottomLine
		x1: 0
		y1: parent.height-1
		x2: parent.width
		y2: parent.height-1
		penWidth: 1
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		visible: true
	}
}

