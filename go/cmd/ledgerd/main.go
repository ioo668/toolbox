package main

import (
	"encoding/csv"
	"encoding/json"
	"flag"
	"fmt"
	"log"
	"net/http"
	"os"
	"sort"
	"strconv"
	"strings"
)

type Entry struct {
	Date     string `json:"date"`
	Kind     string `json:"kind"`
	Amount   int64  `json:"amountCents"`
	Category string `json:"category"`
	Note     string `json:"note"`
}

type DayTotal struct {
	Date    string `json:"date"`
	Income  int64  `json:"incomeCents"`
	Expense int64  `json:"expenseCents"`
}

type Summary struct {
	Entries      int              `json:"entries"`
	IncomeCents  int64            `json:"incomeCents"`
	ExpenseCents int64            `json:"expenseCents"`
	BalanceCents int64            `json:"balanceCents"`
	Categories   map[string]int64 `json:"categoriesByExpense"`
	Daily        []DayTotal       `json:"daily"`
}

func load(path string) ([]Entry, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer f.Close()

	records, err := csv.NewReader(f).ReadAll()
	if err != nil {
		return nil, err
	}

	entries := make([]Entry, 0, len(records))
	for i, rec := range records {
		if len(rec) < 5 {
			return nil, fmt.Errorf("line %d: expected 5 columns, got %d", i+1, len(rec))
		}
		if i == 0 && strings.EqualFold(strings.TrimSpace(rec[0]), "recorded_on") {
			continue
		}
		amount, err := strconv.ParseInt(strings.TrimSpace(rec[2]), 10, 64)
		if err != nil {
			return nil, fmt.Errorf("line %d: bad amount %q", i+1, rec[2])
		}
		kind := strings.TrimSpace(rec[1])
		if kind != "income" && kind != "expense" {
			return nil, fmt.Errorf("line %d: bad kind %q", i+1, kind)
		}
		entries = append(entries, Entry{
			Date:     strings.TrimSpace(rec[0]),
			Kind:     kind,
			Amount:   amount,
			Category: strings.TrimSpace(rec[3]),
			Note:     strings.TrimSpace(rec[4]),
		})
	}
	return entries, nil
}

func day(date string, index map[string]*DayTotal) *DayTotal {
	d, ok := index[date]
	if !ok {
		d = &DayTotal{Date: date}
		index[date] = d
	}
	return d
}

func summarize(entries []Entry) Summary {
	s := Summary{
		Entries:    len(entries),
		Categories: map[string]int64{},
	}
	byDay := map[string]*DayTotal{}

	for _, e := range entries {
		if e.Kind == "income" {
			s.IncomeCents += e.Amount
			day(e.Date, byDay).Income += e.Amount
			continue
		}
		s.ExpenseCents += e.Amount
		s.Categories[e.Category] += e.Amount
		day(e.Date, byDay).Expense += e.Amount
	}
	s.BalanceCents = s.IncomeCents - s.ExpenseCents

	for _, d := range byDay {
		s.Daily = append(s.Daily, *d)
	}
	sort.Slice(s.Daily, func(i, j int) bool { return s.Daily[i].Date < s.Daily[j].Date })
	return s
}

func writeJSON(w http.ResponseWriter, value any) {
	w.Header().Set("Content-Type", "application/json; charset=utf-8")
	w.Header().Set("Access-Control-Allow-Origin", "*")
	if err := json.NewEncoder(w).Encode(value); err != nil {
		log.Printf("encode: %v", err)
	}
}

func main() {
	path := flag.String("data", "data/ledger.csv", "ledger CSV to serve")
	addr := flag.String("addr", ":8787", "listen address")
	flag.Parse()

	entries, err := load(*path)
	if err != nil {
		log.Fatalf("load %s: %v", *path, err)
	}
	summary := summarize(entries)

	mux := http.NewServeMux()
	mux.HandleFunc("/api/summary", func(w http.ResponseWriter, r *http.Request) {
		writeJSON(w, summary)
	})
	mux.HandleFunc("/api/entries", func(w http.ResponseWriter, r *http.Request) {
		writeJSON(w, entries)
	})
	mux.HandleFunc("/healthz", func(w http.ResponseWriter, r *http.Request) {
		fmt.Fprintln(w, "ok")
	})

	log.Printf("ledgerd serving %d entries on %s", len(entries), *addr)
	if err := http.ListenAndServe(*addr, mux); err != nil {
		log.Fatal(err)
	}
}
