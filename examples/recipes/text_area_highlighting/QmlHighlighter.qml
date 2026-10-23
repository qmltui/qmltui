pragma ComponentBehavior: Bound
import QtQml
import QmlTui

QtObject {
    id: root

    property TextArea target: null
    property string searchTerm: ""

    property color keywordColor: "blue"
    property color typeColor: "cyan"
    property color stringColor: "green"
    property color numberColor: "magenta"
    property color commentColor: "#888888"
    property color searchBgColor: "yellow"

    property TextArea _activeTarget: null

    readonly property var _keywords: ({
            "pragma": true,
            "import": true,
            "property": true,
            "signal": true,
            "function": true,
            "true": true,
            "false": true,
            "null": true,
            "undefined": true,
            "var": true,
            "let": true,
            "const": true,
            "return": true,
            "if": true,
            "else": true,
            "for": true,
            "while": true,
            "as": true,
            "alias": true,
            "readonly": true,
            "required": true,
            "component": true,
            "default": true,
            "id": true
        })

    function rehighlight() {
        if (!target)
            return;
        applySyntaxHighlighting();
        applySearchHighlighting();
    }

    function applySyntaxHighlighting() {
        if (!target)
            return;
        const text = target.text;
        const tokens = tokenize(text);
        let ranges = [];

        for (let k = 0; k < tokens.length; ++k) {
            const tok = tokens[k];
            if (tok.type === "comment") {
                ranges.push({
                    start: tok.start,
                    length: tok.length,
                    color: root.commentColor,
                    italic: true
                });
            } else if (tok.type === "string") {
                ranges.push({
                    start: tok.start,
                    length: tok.length,
                    color: root.stringColor
                });
            } else if (tok.type === "number") {
                ranges.push({
                    start: tok.start,
                    length: tok.length,
                    color: root.numberColor
                });
            } else if (tok.type === "word") {
                if (root._keywords[tok.text] === true) {
                    ranges.push({
                        start: tok.start,
                        length: tok.length,
                        color: root.keywordColor,
                        bold: true
                    });
                } else if (tok.text.length > 0 && tok.text[0] >= 'A' && tok.text[0] <= 'Z') {
                    ranges.push({
                        start: tok.start,
                        length: tok.length,
                        color: root.typeColor,
                        bold: true
                    });
                }
            }
        }

        target.setHighlightLayer("syntax", ranges, target.textRevision, 0);
    }

    function applySearchHighlighting() {
        if (!target)
            return;
        if (!searchTerm) {
            target.clearHighlightLayer("search");
            return;
        }

        const text = target.text;
        const searchLower = searchTerm.toLowerCase();
        const textLower = text.toLowerCase();
        const termLen = searchTerm.length;
        let ranges = [];
        let index = 0;

        while ((index = textLower.indexOf(searchLower, index)) !== -1) {
            ranges.push({
                start: index,
                length: termLen,
                backgroundColor: root.searchBgColor,
                bold: true
            });
            index += termLen;
        }

        target.setHighlightLayer("search", ranges, target.textRevision, 10);
    }

    function tokenize(text) {
        let tokens = [];
        if (!text)
            return tokens;
        const len = text.length;
        let i = 0;

        while (i < len) {
            const ch = text[i];

            // Whitespace
            if (ch === ' ' || ch === '\t' || ch === '\n' || ch === '\r') {
                i++;
                continue;
            }

            // Single line comment: //
            if (ch === '/' && i + 1 < len && text[i + 1] === '/') {
                const start = i;
                i += 2;
                while (i < len && text[i] !== '\n') {
                    i++;
                }
                tokens.push({
                    start: start,
                    length: i - start,
                    type: "comment"
                });
                continue;
            }

            // Multi line comment: /* ... */
            if (ch === '/' && i + 1 < len && text[i + 1] === '*') {
                const start = i;
                i += 2;
                while (i + 1 < len && !(text[i] === '*' && text[i + 1] === '/')) {
                    i++;
                }
                if (i + 1 < len) {
                    i += 2;
                } else {
                    i = len;
                }
                tokens.push({
                    start: start,
                    length: i - start,
                    type: "comment"
                });
                continue;
            }

            // Strings: "...", '...', or `...`
            if (ch === '"' || ch === "'" || ch === '`') {
                const quote = ch;
                const start = i;
                i++;
                while (i < len) {
                    if (text[i] === '\\') {
                        i += 2;
                    } else if (text[i] === quote) {
                        i++;
                        break;
                    } else {
                        i++;
                    }
                }
                i = Math.min(i, len);
                tokens.push({
                    start: start,
                    length: i - start,
                    type: "string"
                });
                continue;
            }

            // Numbers: digits or negative number
            if ((ch >= '0' && ch <= '9') || (ch === '-' && i + 1 < len && text[i + 1] >= '0' && text[i + 1] <= '9')) {
                const start = i;
                if (ch === '-')
                    i++;
                if (i + 1 < len && text[i] === '0' && (text[i + 1] === 'x' || text[i + 1] === 'X')) {
                    i += 2;
                    while (i < len && ((text[i] >= '0' && text[i] <= '9') || (text[i] >= 'a' && text[i] <= 'f') || (text[i] >= 'A' && text[i] <= 'F'))) {
                        i++;
                    }
                } else {
                    while (i < len && ((text[i] >= '0' && text[i] <= '9') || text[i] === '.')) {
                        i++;
                    }
                }
                tokens.push({
                    start: start,
                    length: i - start,
                    type: "number"
                });
                continue;
            }

            // Identifiers / words
            if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch === '_' || ch === '$') {
                const start = i;
                while (i < len) {
                    const c = text[i];
                    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c === '_' || c === '$') {
                        i++;
                    } else {
                        break;
                    }
                }
                const word = text.substring(start, i);
                tokens.push({
                    start: start,
                    length: i - start,
                    type: "word",
                    text: word
                });
                continue;
            }

            i++;
        }

        return tokens;
    }

    onTargetChanged: {
        if (_activeTarget && _activeTarget !== target) {
            _activeTarget.textChanged.disconnect(root.rehighlight);
            _activeTarget.clearHighlightLayer("syntax");
            _activeTarget.clearHighlightLayer("search");
        }
        _activeTarget = target;
        if (_activeTarget) {
            _activeTarget.textChanged.connect(root.rehighlight);
        }
        root.rehighlight();
    }

    Component.onDestruction: {
        if (_activeTarget) {
            _activeTarget.textChanged.disconnect(root.rehighlight);
            _activeTarget.clearHighlightLayer("syntax");
            _activeTarget.clearHighlightLayer("search");
        }
    }

    onSearchTermChanged: root.applySearchHighlighting()
    onKeywordColorChanged: root.applySyntaxHighlighting()
    onTypeColorChanged: root.applySyntaxHighlighting()
    onStringColorChanged: root.applySyntaxHighlighting()
    onNumberColorChanged: root.applySyntaxHighlighting()
    onCommentColorChanged: root.applySyntaxHighlighting()
    onSearchBgColorChanged: root.applySearchHighlighting()
}
