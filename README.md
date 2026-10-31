# toolbox

一个手写记账台账的小项目，同一个数据源（`data/ledger.csv` / `var/ledger.db`）由几种语言各管一段。
A hand-written ledger toolbox: one data source, several languages each owning one stage of it.

| 目录 | 语言 | 作用 |
| --- | --- | --- |
| `db/` | SQL | SQLite 表结构、视图、常用查询 |
| `c/` | C | CSV → SQL 导入器，只校验不入库，无第三方依赖 |
| `go/` | Go | `ledgerd`，把台账以 JSON 接口对外提供 |
| `java/` | Java | 按月汇总、按类别画条形报表 |
| `python/` | Python | 汇总 + 脏数据体检（重复行、未来日期、未知类别） |
| `web/` | HTML / CSS / JS | 看板，接口通就实时，不通回落到读 CSV |
| `docker/` | Dockerfile / Compose | 打包 `ledgerd` 并和 nginx 一起起 |
| `scripts/` | Shell | 数据库快照、校验、保留最近 N 份 |
| `.github/workflows/` | YAML | 每种语言各跑一个 job |

## Quick start

```sh
# 1. build the database
mkdir -p var
sqlite3 var/ledger.db < db/schema.sql
sqlite3 var/ledger.db < db/seed.sql
python python/stats.py data/ledger.csv --check   # or java / make -C c
```

```sh
# 2. serve it
cd go && go run ./cmd/ledgerd -data ../data/ledger.csv -addr :8787
```

```sh
# 3. open the dashboard
python -m http.server 8000 --directory web
# http://localhost:8000  (fallback reads ../data/ledger.csv, so a plain
#                         static server works even with ledgerd stopped)
```

## Per-language commands

```sh
make -C c check                                  # C:   rows emitted
cd go && go vet ./... && go build ./cmd/ledgerd   # Go
javac -d target java/Report.java && java -cp target Report data/ledger.csv  # Java
python python/stats.py data/ledger.csv --check    # Python
docker compose -f docker/docker-compose.yml up --build  # Docker
```

## Amounts

Money is always integer cents — never floats. `recorded_on` is `YYYY-MM-DD` text so the
views group without date functions, which keeps the schema portable across SQLite versions.

## Status

Schema, seed, Java report and the Python checker run locally and are exercised by CI.
The C and Go parts need `gcc`/`go` and are only checked by CI, not on the machine they
were written on; `docker/` likewise. Nothing here phones home, and `data/ledger.csv`
is synthetic sample data.
