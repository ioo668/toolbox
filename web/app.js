const API = new URLSearchParams(location.search).get("api") || "http://127.0.0.1:8787";
const FALLBACK_CSV = "../data/ledger.csv";

const yuan = (cents) => (cents / 100).toLocaleString("zh-CN", { minimumFractionDigits: 2 });

async function json(path) {
  const res = await fetch(`${API}${path}`, { cache: "no-store" });
  if (!res.ok) throw new Error(`${res.status} ${res.statusText}`);
  return res.json();
}

function parseCsv(text) {
  return text
    .split(/\r?\n/)
    .filter((line) => line.trim() && !line.startsWith("#"))
    .slice(1)
    .map((line) => {
      const [date, kind, amount, category, ...rest] = line.split(",");
      return {
        date: date.trim(),
        kind: kind.trim(),
        amountCents: Number(amount),
        category: category.trim(),
        note: rest.join(",").trim(),
      };
    });
}

function summarise(entries) {
  const totals = { incomeCents: 0, expenseCents: 0, categoriesByExpense: {} };
  for (const e of entries) {
    if (e.kind === "income") totals.incomeCents += e.amountCents;
    else {
      totals.expenseCents += e.amountCents;
      totals.categoriesByExpense[e.category] = (totals.categoriesByExpense[e.category] || 0) + e.amountCents;
    }
  }
  totals.balanceCents = totals.incomeCents - totals.expenseCents;
  return totals;
}

function renderTotals(s) {
  document.getElementById("income").textContent = yuan(s.incomeCents);
  document.getElementById("expense").textContent = yuan(s.expenseCents);
  const balance = document.getElementById("balance");
  balance.textContent = yuan(s.balanceCents);
  balance.style.color = s.balanceCents >= 0 ? "var(--income)" : "var(--expense)";
}

function renderCategories(cats) {
  const list = document.getElementById("categories");
  const entries = Object.entries(cats).sort((a, b) => b[1] - a[1]);
  const max = entries.length ? entries[0][1] : 1;
  list.replaceChildren(...entries.map(([name, cents]) => {
    const li = document.createElement("li");
    const label = document.createElement("span");
    label.textContent = name;
    const track = document.createElement("i");
    track.style.width = `${Math.max(2, (cents / max) * 100)}%`;
    const value = document.createElement("b");
    value.textContent = yuan(cents);
    li.append(label, track, value);
    return li;
  }));
}

function renderRows(entries) {
  const body = document.getElementById("rows");
  const sorted = [...entries].sort((a, b) => b.date.localeCompare(a.date));
  body.replaceChildren(...sorted.map((e) => {
    const tr = document.createElement("tr");
    const cells = [e.date, e.category, e.note || "—"];
    const tds = cells.map((text) => {
      const td = document.createElement("td");
      td.textContent = text;
      return td;
    });
    const amount = document.createElement("td");
    amount.className = `num ${e.kind}`;
    amount.textContent = `${e.kind === "expense" ? "−" : "+"}${yuan(e.amountCents)}`;
    tr.append(...tds, amount);
    return tr;
  }));
}

async function load() {
  const source = document.getElementById("source");
  try {
    const [summary, entries] = await Promise.all([json("/api/summary"), json("/api/entries")]);
    source.textContent = `live · ${API} · ${entries.length} entries`;
    renderTotals(summary);
    renderCategories(summary.categoriesByExpense);
    renderRows(entries);
  } catch (err) {
    source.textContent = `offline fallback (${err.message}) · reading ${FALLBACK_CSV}`;
    const res = await fetch(FALLBACK_CSV);
    const entries = parseCsv(await res.text());
    const summary = summarise(entries);
    renderTotals(summary);
    renderCategories(summary.categoriesByExpense);
    renderRows(entries);
  }
}

load();
