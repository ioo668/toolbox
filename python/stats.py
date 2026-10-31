#!/usr/bin/env python3
"""Summarise a ledger CSV and flag rows that look wrong."""

import argparse
import csv
import json
import sys
from collections import defaultdict
from datetime import date, datetime

KNOWN_CATEGORIES = {"food", "transport", "housing", "tools", "entertainment", "other"}
VALID_KINDS = ("income", "expense")


def parse_rows(handle):
    reader = csv.DictReader(handle)
    for index, row in enumerate(reader, start=2):
        yield index, row


def summarise(rows):
    income = 0
    expense = 0
    by_category = defaultdict(int)
    by_month = defaultdict(lambda: {"income": 0, "expense": 0})

    for row in rows:
        bucket = by_month[row["recorded_on"][:7]]
        if row["kind"] == "income":
            income += row["amount_cents"]
            bucket["income"] += row["amount_cents"]
        else:
            expense += row["amount_cents"]
            bucket["expense"] += row["amount_cents"]
            by_category[row["category"]] += row["amount_cents"]

    return {
        "entries": len(rows),
        "incomeCents": income,
        "expenseCents": expense,
        "balanceCents": income - expense,
        "categories": dict(sorted(by_category.items(), key=lambda kv: -kv[1])),
        "months": {k: v for k, v in sorted(by_month.items())},
    }


def find_problems(rows, today):
    problems = []
    seen = set()
    for row in rows:
        key = (row["recorded_on"], row["kind"], row["amount_cents"], row["category"], row["note"])
        stamp = row["_line"]
        try:
            recorded = datetime.strptime(row["recorded_on"], "%Y-%m-%d").date()
        except ValueError:
            problems.append(f"line {stamp}: unparseable date {row['recorded_on']!r}")
            continue
        if recorded > today:
            problems.append(f"line {stamp}: dated in the future ({recorded})")
        if row["kind"] not in VALID_KINDS:
            problems.append(f"line {stamp}: unknown kind {row['kind']!r}")
        if row["amount_cents"] <= 0:
            problems.append(f"line {stamp}: non-positive amount {row['amount_cents']}")
        if row["category"] not in KNOWN_CATEGORIES:
            problems.append(f"line {stamp}: category {row['category']!r} is not in the known set")
        if key in seen:
            problems.append(f"line {stamp}: duplicate of an earlier row")
        seen.add(key)
    return problems


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv_file", nargs="?", default="data/ledger.csv")
    parser.add_argument("--check", action="store_true", help="exit non-zero when problems are found")
    args = parser.parse_args(argv)

    with open(args.csv_file, newline="", encoding="utf-8") as handle:
        rows = []
        for line_no, row in parse_rows(handle):
            row = {k: (v or "").strip() for k, v in row.items()}
            row["amount_cents"] = int(row["amount_cents"])
            row["_line"] = line_no
            rows.append(row)

    problems = find_problems(rows, date.today())
    report = {"summary": summarise(rows), "problems": problems}
    json.dump(report, sys.stdout, ensure_ascii=False, indent=2)
    print()

    if problems and args.check:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
