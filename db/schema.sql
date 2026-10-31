PRAGMA journal_mode = WAL;
PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS account (
    id            INTEGER PRIMARY KEY,
    name          TEXT    NOT NULL UNIQUE,
    base_currency TEXT    NOT NULL DEFAULT 'CNY'
);

CREATE TABLE IF NOT EXISTS entry (
    id           INTEGER PRIMARY KEY,
    account_id   INTEGER NOT NULL REFERENCES account (id) ON DELETE CASCADE,
    recorded_on  TEXT    NOT NULL CHECK (length (recorded_on) = 10),
    kind         TEXT    NOT NULL CHECK (kind IN ('income', 'expense')),
    amount_cents INTEGER NOT NULL CHECK (amount_cents >= 0),
    category     TEXT    NOT NULL,
    note         TEXT,
    imported_via TEXT    NOT NULL DEFAULT 'manual'
);

CREATE INDEX IF NOT EXISTS entry_recorded_on_idx ON entry (recorded_on);
CREATE INDEX IF NOT EXISTS entry_account_category_idx ON entry (account_id, category);

CREATE VIEW IF NOT EXISTS daily_totals AS
SELECT account_id,
       recorded_on,
       kind,
       SUM (amount_cents) AS amount_cents,
       COUNT (*)          AS entries
FROM   entry
GROUP  BY account_id, recorded_on, kind;

CREATE VIEW IF NOT EXISTS category_totals AS
SELECT account_id,
       kind,
       category,
       SUM (amount_cents) AS amount_cents,
       COUNT (*)          AS entries
FROM   entry
GROUP  BY account_id, kind, category;
