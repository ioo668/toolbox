-- Balance per account, in cents.
SELECT account_id,
       SUM (CASE kind WHEN 'income' THEN amount_cents ELSE -amount_cents END) AS balance_cents
FROM   entry
GROUP  BY account_id;

-- Top spending categories for a month.
SELECT category, SUM (amount_cents) AS amount_cents, COUNT (*) AS entries
FROM   entry
WHERE  kind = 'expense' AND recorded_on BETWEEN :month_start AND :month_end
GROUP  BY category
ORDER  BY amount_cents DESC;

-- Rolling 7-day spend, newest first.
SELECT recorded_on, SUM (amount_cents) AS amount_cents
FROM   entry
WHERE  kind = 'expense'
GROUP  BY recorded_on
ORDER  BY recorded_on DESC
LIMIT  7;

-- Entries that were never tagged with a category we recognise.
SELECT id, recorded_on, note
FROM   entry
WHERE  category NOT IN ('food', 'transport', 'housing', 'tools', 'entertainment', 'other');
