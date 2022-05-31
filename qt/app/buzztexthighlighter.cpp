#include "buzztexthighlighter.h"
#include "iapplication.h"
#include "client.h"

using namespace buzzer;

BuzzTextHighlighter::BuzzTextHighlighter(QObject* parent): QSyntaxHighlighter(parent) {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());
	QString lLinkColor = gApplication->getColor(lClient->theme(), lClient->themeSelector(), "Material.link");

	//
	buzzerFormat.setFontWeight(QFont::Normal);
	buzzerFormat.setForeground(QBrush(QColor(lLinkColor)));

	tagFormat.setFontWeight(QFont::Normal);
	tagFormat.setForeground(QBrush(QColor(lLinkColor)));

	HighlightingRule lRule;
	lRule.pattern = QRegularExpression("@[A-Za-z0-9]+");
	lRule.format = buzzerFormat;
	highlightingRules_.append(lRule);

	/*
	lRule.pattern = QRegularExpression("@+");
	lRule.format = buzzerFormat;
	highlightingRules_.append(lRule);
	*/

	lRule.pattern = QRegularExpression("#[\\w\u0400-\u04FF]+"); // add support for another languages
	lRule.format = tagFormat;
	highlightingRules_.append(lRule);

	/*
	lRule.pattern = QRegularExpression("#+");
	lRule.format = tagFormat;
	highlightingRules_.append(lRule);
	*/

	lRule.pattern = QRegularExpression("REGEX_UNIVERSAL_URL_PATTERN");
	lRule.format = tagFormat;
	highlightingRules_.append(lRule);

	/*
	lRule.pattern = QRegularExpression("((/data/user)/\\S+)");
	lRule.format = QTextCharFormat();
	highlightingRules_.append(lRule);
	*/

	/*
	lRule.pattern = QRegularExpression("((?:buzz|msg|qbit)://\\S+)");
	lRule.format = tagFormat;
	highlightingRules_.append(lRule);
	*/
}

void BuzzTextHighlighter::tryHighlightBlock(const QString& text, int start) {
	//
	QString lText = text.mid(start, text.length()-1);
	highlightInternal(text, start);
}

void BuzzTextHighlighter::highlightBlock(const QString& text) {
	//
	highlightInternal(text, 0);
}

void BuzzTextHighlighter::highlightInternal(const QString &text, int start) {
	//
	for (const HighlightingRule &rule : qAsConst(highlightingRules_)) {
		QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
		while (matchIterator.hasNext()) {
			QRegularExpressionMatch match = matchIterator.next();
			if (rule.format.isValid()) setFormat(match.capturedStart(), match.capturedLength(), rule.format);
			// repositioning
			emit matched(start + match.capturedStart(), match.capturedLength(), text.mid(match.capturedStart(), match.capturedLength()), text);
		}
	}
}
