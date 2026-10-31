FROM golang:1.23-alpine AS build
WORKDIR /src
COPY go/go.mod ./go/
COPY go/cmd ./go/cmd/
RUN cd go && CGO_ENABLED=0 go build -o /out/ledgerd ./cmd/ledgerd

FROM eclipse-temurin:21-jdk-alpine AS report
WORKDIR /src
COPY java/Report.java ./
RUN javac -d /out Report.java

FROM alpine:3.20
RUN apk add --no-cache sqlite bash curl && adduser -D -u 10001 ledger
WORKDIR /app
COPY --from=build /out/ledgerd /app/ledgerd
COPY --from=report /out/ /app/report/
COPY db/ /app/db/
COPY data/ledger.csv /app/data/ledger.csv
COPY scripts/backup.sh /app/scripts/
USER ledger
EXPOSE 8787
HEALTHCHECK --interval=30s --timeout=3s CMD curl -fsS http://127.0.0.1:8787/healthz || exit 1
CMD ["/app/ledgerd", "-data", "/app/data/ledger.csv", "-addr", ":8787"]
