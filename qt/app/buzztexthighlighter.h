#ifndef BUZZTEXT_HIGHLIGHTER_H
#define BUZZTEXT_HIGHLIGHTER_H

#include <QObject>
#include <QSyntaxHighlighter>
#include <QQuickTextDocument>
#include <QRegularExpression>

namespace buzzer {

class BuzzTextHighlighter : public QSyntaxHighlighter {
	Q_OBJECT

	Q_PROPERTY(QQuickTextDocument* textDocument READ textDocument WRITE setTextDocument NOTIFY textDocumentChanged)

public:
	BuzzTextHighlighter(QObject* parent = nullptr);

	QQuickTextDocument* textDocument() { return textDocument_; }
	void setTextDocument(QQuickTextDocument* document) {
		//
		textDocument_ = document;
		QSyntaxHighlighter::setDocument(textDocument_->textDocument());
	}

	Q_INVOKABLE void tryHighlightBlock(const QString& /*text*/, int /*start*/);

protected:
	void highlightBlock(const QString &text) override;

signals:
	void textDocumentChanged();
	void matched(int start, int length, QString match);

private:
	struct HighlightingRule {
		QRegularExpression pattern;
		QTextCharFormat format;
	};

	QVector<HighlightingRule> highlightingRules_;

	QRegularExpression buzzerExpression;
	QRegularExpression tagExpression;

	QTextCharFormat buzzerFormat;
	QTextCharFormat tagFormat;

private:
	QQuickTextDocument* textDocument_;
};

} // buzzer

#endif // BUZZTEXT_HIGHLIGHTER_H
