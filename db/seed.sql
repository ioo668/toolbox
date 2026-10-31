INSERT OR IGNORE INTO account (id, name, base_currency) VALUES (1, 'personal', 'CNY');

INSERT INTO entry (account_id, recorded_on, kind, amount_cents, category, note, imported_via)
VALUES
    (1, '2026-10-03', 'income',  120000, 'other',         'freelance invoice 07',  'seed'),
    (1, '2026-10-04', 'expense',   4350, 'food',          'groceries',             'seed'),
    (1, '2026-10-11', 'expense', 158000, 'housing',       'storage VPS monthly',   'seed'),
    (1, '2026-10-26', 'income',   99000, 'other',         'consulting call',       'seed');
