import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Dialogs 1.1

import app.buzzer.components 1.0 as BuzzerComponents
import app.buzzer.commands 1.0 as BuzzerCommands

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

QuarkPage
{
	id: setupaskqbit_
	key: "setupaskqbit"

	Component.onCompleted: {
		buzzerApp.lockPortraitOrientation();
        closePageHandler = closePage;
		activatePageHandler = activatePage;

		balanceCommand.process(); // try to get current balance
    }

	function closePage() {
        stopPage();
        controller.popPage();
		destroy(1000);
	}

	function activatePage() {
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
		toolBar.activate();
	}

	function onErrorCallback(error) {
        controller.showError(error);
    }

    //
    // toolbar
    //
	SetupToolBar {
		id: toolBar
		extraOffset: topOffset
	}

	//
	// Text
	//
	QuarkLabel {
		id: welcomeText
		x: 20
		y: toolBar.y + toolBar.totalHeight + 20
		width: parent.width-40
		wrapMode: Label.Wrap
		font.pointSize: 18

		text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.askqbits")
	}

	QuarkToolButton {
		id: askButton
		x: parent.width / 2 - width / 2
		y: welcomeText.y + welcomeText.height +
		   ((parent.height - (welcomeText.y + welcomeText.height + nextButton.height)) / 2 - height / 2) - 15
		width: parent.width - 140
		height: parent.width - 140
		visible: true
		labelYOffset: height / 2 - metrics.tightBoundingRect.height
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter
		radius: width / 2 - 10
		text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.askqbits.button");
		symbolFontPointSize: 24

		TextMetrics {
			id: metrics
			font: askButton.symbolFont
			text: askButton.text
		}

		onClicked: {
			//
			text = buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.askqbits.button.wait");
			waitTimer.start();

			// ask for qbits from network
			askForQbitsCommand.process();

			enabled = false;
		}
	}

	//
	Connections {
		target: buzzerClient

		function onWalletTransactionReceived() {
			// chain, tx
			// get balance
			balanceCommand.process();
		}
	}

	//
	BuzzerCommands.AskForQbitsCommand {
		id: askForQbitsCommand

		onProcessed: {
			console.log("AskForQbitsCommand - processed");
		}

		onError: {
			controller.showError({ code: code, message: message, component: "Setup" });
		}
	}

	//
	BuzzerCommands.BalanceCommand {
		id: balanceCommand

		onProcessed: {
			// amount, pending, scale
			if (amount > 0.0) {
				// stop waiting
				waitTimer.stop();
				progressBar.arcEnd = 360;

				// you have qbits
				askButton.enabled = false;
				nextButton.enabled = true;

				askButton.text = "";
				qbitBalance.number = amount;
				qbitBalance.visible = true;
			} else {
				//
				askButton.enabled = true;
				nextButton.enabled = false;
			}
		}

		onError: {
			controller.showError({ code: code, message: message, component: "Setup" });
		}
	}

	//
	QuarkRoundProgress {
		id: progressBar
		x: parent.width / 2 - (askButton.width + 20) / 2
		y: askButton.y - 10
		size: parent.width - 140 + 20
		colorCircle: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accentUltra");
		colorBackground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accentUltra");
		arcBegin: 0
		arcEnd: 0
		lineWidth: 5

		Timer {
			id: waitTimer
			running: false
			repeat: true
			interval: 100

			onTriggered: {
				progressBar.arcEnd += 5;
				if (progressBar.arcEnd >= 360) {
					running = false;
				}
			}
		}
	}

	//
	QuarkNumberLabel {
		id: qbitBalance
		font.pointSize: 34
		visible: false
		fillTo: 1
		unitsGap: 10
		units: "QBIT"
		x: parent.width / 2 - qbitBalance.calculatedWidth / 2
		y: askButton.y + askButton.height / 2 - qbitBalance.calculatedHeight / 2
	}

	QuarkButton	{
		id: nextButton
		x: welcomeText.x + 2
		y: parent.height - height - 20
		text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.next")
		visible: true
		enabled: false
		width: welcomeText.width

		Layout.minimumWidth: 150
		Layout.alignment: Qt.AlignHCenter

		onClicked: {
			var lComponent = null;
			var lPage = null;

			lComponent = Qt.createComponent("qrc:/qml/setupbuzzer.qml");
			if (lComponent.status === Component.Error) {
				controller.showError(lComponent.errorString());
			} else {
				lPage = lComponent.createObject(window);
				lPage.controller = controller;

				addPage(lPage);
			}
		}
	}
}
