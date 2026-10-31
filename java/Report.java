import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

public class Report {

    record Row(String date, String kind, long amountCents, String category, String note) {}

    public static void main(String[] args) throws IOException {
        Path csv = Path.of(args.length > 0 ? args[0] : "data/ledger.csv");
        List<Row> rows = new ArrayList<>();

        for (String line : Files.readAllLines(csv)) {
            if (line.isBlank() || line.startsWith("#")) continue;
            String[] parts = line.split(",", 5);
            if (parts.length < 5 || parts[0].equals("recorded_on")) continue;
            rows.add(new Row(parts[0].trim(), parts[1].trim(),
                    Long.parseLong(parts[2].trim()), parts[3].trim(), parts[4].trim()));
        }

        long income = 0;
        long expense = 0;
        Map<String, Long> byCategory = new TreeMap<>();
        Map<String, long[]> byMonth = new LinkedHashMap<>();

        for (Row row : rows) {
            String month = row.date().substring(0, 7);
            long[] bucket = byMonth.computeIfAbsent(month, k -> new long[2]);
            if (row.kind().equals("income")) {
                income += row.amountCents();
                bucket[0] += row.amountCents();
            } else {
                expense += row.amountCents();
                bucket[1] += row.amountCents();
                byCategory.merge(row.category(), row.amountCents(), Long::sum);
            }
        }

        System.out.printf("ledger report - %d entries%n", rows.size());
        System.out.println("=".repeat(46));
        System.out.printf("%-10s %12s %12s %12s%n", "month", "income", "expense", "net");
        byMonth.forEach((month, bucket) -> System.out.printf("%-10s %12s %12s %12s%n",
                month, money(bucket[0]), money(bucket[1]), money(bucket[0] - bucket[1])));
        System.out.println("-".repeat(46));
        System.out.printf("%-10s %12s %12s %12s%n", "TOTAL", money(income), money(expense), money(income - expense));

        System.out.println();
        System.out.println("expense by category");
        long totalExpense = expense;
        long net = income - expense;
        byCategory.entrySet().stream()
                .sorted((a, b) -> Long.compare(b.getValue(), a.getValue()))
                .forEach(e -> System.out.printf("  %-14s %10s %s%n", e.getKey(), money(e.getValue()),
                        bar(e.getValue(), totalExpense)));

        if (net < 0) {
            System.out.printf("%nnet deficit for this ledger: %s%n", money(-net));
        }
    }

    private static String money(long cents) {
        return String.format("%,.2f", cents / 100.0);
    }

    private static String bar(long value, long total) {
        int width = total == 0 ? 0 : (int) Math.round(20.0 * value / total);
        return "#".repeat(Math.max(0, width));
    }
}
