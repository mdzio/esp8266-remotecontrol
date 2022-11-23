remove-item -path firmware\data\*
copy-item webapp\* -exclude mithril.js -destination firmware\data
(get-content firmware\data\index.htm) -replace 'mithril.js', 'mithril.min.js' | set-content firmware\data\index.htm
