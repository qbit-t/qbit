import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.3
import QtQuick.Controls 1.4
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Window 2.15

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

ApplicationWindow
{
    id: window
	width: 920
	height: 740
    visible: true
	title: "Buzzer"

    property string onlineCount: "";
	property var pagesView;
	property int bottomBarHeight: 0;
	property var mainToolBar;

	color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background");

	onWidthChanged: {
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
		buzzerClient.setProperty("Client.width", width);
	}

	onHeightChanged: {
		buzzerClient.setProperty("Client.height", height);
	}

	Component.onCompleted: {
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));

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

    flags: Qt.Window | Qt.MaximizeUsingFullscreenGeometryHint

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

	function activatePage(key) {
		for (var lIdx = pagesView.count - 1; lIdx >= 0; lIdx--) {
			var lPage = pagesView.itemAt(lIdx);
			if (lPage.key === key) {
				//
				pagesView.currentIndex = lIdx;
				if (lPage.activatePageHandler) lPage.activatePageHandler();
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
		console.log("[popPage]: page = " + page);
		for (var lIdx = pagesView.count - 1; lIdx >= 0; lIdx--) {
			var lPage = pagesView.itemAt(lIdx);
			console.log("[popPage]: page key = " + page.key + ", key = " + lPage.key);
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

		var lNewPage = pagesView.itemAt(pagesView.currentIndex);
		if (lNewPage) {
			lNewPage.updateStatusBar();
			if (lNewPage.activatePageHandler) lNewPage.activatePageHandler();
		}
    }

	function isTop() {
		return pagesView.count - 1;
	}

	function openThread(buzzChainId, buzzId, buzzerAlias, buzzBody) {
		//
		var lComponent = null;
		var lPage = null;

		// locate and activate
		if (activatePage(buzzId)) return;

		lComponent = buzzerApp.isDesktop ? Qt.createComponent("qrc:/qml/buzzfeedthread-desktop.qml", window) :
										   Qt.createComponent("qrc:/qml/buzzfeedthread.qml");
		if (lComponent.status === Component.Error) {
			showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(window);
			lPage.controller = window;

			lPage.updateStakedInfo(buzzId, buzzerAlias, buzzBody);
			lPage.start(buzzChainId, buzzId);

			addPage(lPage);
		}
	}

	function openBuzzfeedByBuzzer(buzzerName) {
		//
		var lComponent = null;
		var lPage = null;

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

	//
	// inner methods
	//

	function addPageLocal(page)
	{
		pagesViewLocal.push(page, StackView.Transition);
		page.updateStatusBar();
		if (page.activatePageHandler) page.activatePageHandler();
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

    Connections
    {
        target: Qt.application
        onStateChanged:
        {
            if(Qt.application.state === 0 /* suspended */ ||
               Qt.application.state === 1 /* hidden */ ||
               Qt.application.state === 2 /* inactive */ )
            {
                console.log("Main window suspending...");
				buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
				suspended = true;

				//
				//buzzerApp.resumeNotifications();
            }
            else if(Qt.application.state === 4 /* active */)
            {
                console.log("Main window waking up...");
				buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
                suspended = false;

                if (localNotificator) localNotificator.reset();

				//
				//buzzerApp.pauseNotifications();

				// try to re-force change background
				adjustTimer.start();
			}
        }
    }

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

            PropertyAnimation
            {
                id: apearing
                target: welcomeRect
                property: "opacity"
                from: 0.0
                to: 1.0
                duration: 600
                //easing.type: Easing.Linear
            }

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
                    apearing.start();
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
		width: 0.75 * window.width
		height: window.height

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

					//
					var lComponent = null;
					var lPage = null;

					if (key === "createNewBuzzer" || key === "editBuzzer") {
						lComponent = Qt.createComponent("qrc:/qml/buzzercreateupdate.qml");
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
						lComponent = Qt.createComponent("qrc:/qml/buzzfeedbuzzer.qml");
						if (lComponent.status === Component.Error) {
							showError(lComponent.errorString());
						} else {
							lPage = lComponent.createObject(window);
							lPage.controller = window;
							lPage.start(buzzerClient.name);

							addPage(lPage);
						}
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
						x: 15
						y: parent.height / 2 - height / 2
						verticalAlignment: Text.AlignVCenter
						Material.background: "transparent"
						Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
						visible: true
						symbol: keySymbol
					}

					QuarkLabelRegular {
						id: textLabel
						x: keySymbol ? 45 : 15
						y: parent.height / 2 - height / 2
						width: drawerMenu.width - (symbolLabel.x + symbolLabel.width + 5 + 10)
						text: name
						elide: Text.ElideRight
						verticalAlignment: Text.AlignVCenter
						Material.background: "transparent"
						Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
						visible: true
					}
				}
			}

			ScrollIndicator.vertical: ScrollIndicator { }

			function prepare() {
				if (menuModel_.count) return;

				menuModel_.append({
					key: "createNewBuzzer",
					keySymbol: Fonts.userPlusSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.create.new")});
				menuModel_.append({
					key: "editBuzzer",
					keySymbol: Fonts.userEditSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.edit.buzzer")});
				menuModel_.append({
					key: "qbitKey",
					keySymbol: Fonts.secretSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.qbit.key")});
				menuModel_.append({
					key: "personalFeed",
					keySymbol: Fonts.flashSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.personal.feed")});
				menuModel_.append({
					key: "network",
					keySymbol: Fonts.networkSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.network")});
				menuModel_.append({
					key: "quickHelp",
					keySymbol: Fonts.helpSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.quick.help")});
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
			buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
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
}

