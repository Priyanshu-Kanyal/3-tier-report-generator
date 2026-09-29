package com.reportgen.config;

import com.reportgen.model.ReportOutput;
import com.reportgen.model.ReportRequest;
import com.reportgen.model.ReportStatus;
import com.reportgen.model.ReportTemplate;
import com.reportgen.repository.ReportOutputRepository;
import com.reportgen.repository.ReportRequestRepository;
import com.reportgen.repository.ReportTemplateRepository;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.boot.CommandLineRunner;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import java.time.OffsetDateTime;

@Configuration
public class DatabaseSeeder {

    private static final Logger log = LoggerFactory.getLogger(DatabaseSeeder.class);

    @Bean
    public CommandLineRunner seedDatabase(ReportTemplateRepository templateRepo,
                                         ReportRequestRepository requestRepo,
                                         ReportOutputRepository outputRepo) {
        return args -> {
            if (templateRepo.count() == 0) {
                log.info("Database is empty. Seeding initial report templates...");

                // Template 1: Sales Summary
                ReportTemplate salesTemplate = ReportTemplate.builder()
                        .name("Sales Summary")
                        .description("Regional sales report over a date range")
                        .templateSchema("{\n" +
                                "  \"title\": \"Sales Summary Report\",\n" +
                                "  \"fields\": [\n" +
                                "    {\"name\": \"region\", \"type\": \"string\", \"label\": \"Region\"},\n" +
                                "    {\"name\": \"start_date\", \"type\": \"date\", \"label\": \"Start Date\"},\n" +
                                "    {\"name\": \"end_date\", \"type\": \"date\", \"label\": \"End Date\"}\n" +
                                "  ],\n" +
                                "  \"columns\": [\"date\", \"product\", \"quantity\", \"revenue\"]\n" +
                                "}")
                        .outputFormats("{PDF,XLSX}")
                        .build();
                salesTemplate = templateRepo.save(salesTemplate);

                // Template 2: Inventory Status
                ReportTemplate inventoryTemplate = ReportTemplate.builder()
                        .name("Inventory Status")
                        .description("Current stock levels by warehouse")
                        .templateSchema("{\n" +
                                "  \"title\": \"Inventory Status Report\",\n" +
                                "  \"fields\": [\n" +
                                "    {\"name\": \"warehouse\", \"type\": \"string\", \"label\": \"Warehouse\"}\n" +
                                "  ],\n" +
                                "  \"columns\": [\"sku\", \"product_name\", \"quantity_on_hand\", \"reorder_level\"]\n" +
                                "}")
                        .outputFormats("{PDF}")
                        .build();
                inventoryTemplate = templateRepo.save(inventoryTemplate);

                // Seed initial request #1
                ReportRequest seedRequest = ReportRequest.builder()
                        .template(salesTemplate)
                        .parameters("{\"region\": \"West\", \"start_date\": \"2026-01-01\", \"end_date\": \"2026-01-31\"}")
                        .status(ReportStatus.DONE)
                        .completedAt(OffsetDateTime.now())
                        .build();
                seedRequest = requestRepo.save(seedRequest);

                // Seed initial output #1
                byte[] samplePdf = ("%PDF-1.4\n1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n" +
                        "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n" +
                        "3 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Contents 4 0 R /Resources << /Font << /F1 5 0 R >> >> >>\nendobj\n" +
                        "4 0 obj\n<< /Length 120 >>\nstream\nBT /F1 18 Tf 50 700 Td (Sales Summary Report - Seed Output) Tj ET\nendstream\nendobj\n" +
                        "5 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>\nendobj\n" +
                        "xref\n0 6\n0000000000 65535 f \n0000000009 00000 n \n0000000058 00000 n \n0000000115 00000 n \n0000000252 00000 n \n0000000424 00000 n \n" +
                        "trailer\n<< /Size 6 /Root 1 0 R >>\nstartxref\n503\n%%EOF\n").getBytes();

                ReportOutput seedOutput = ReportOutput.builder()
                        .request(seedRequest)
                        .format("PDF")
                        .fileData(samplePdf)
                        .build();
                outputRepo.save(seedOutput);

                log.info("Database seeding complete. Seeded 2 templates, 1 request, and 1 output.");
            }
        };
    }
}
