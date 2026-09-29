package com.reportgen.controller;

import com.reportgen.dto.CreateReportRequestDto;
import com.reportgen.dto.ReportResponseDto;
import com.reportgen.model.ReportOutput;
import com.reportgen.service.ReportService;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.validation.Valid;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.List;

@RestController
@RequestMapping("/api/reports")
@CrossOrigin(origins = "*")
@Tag(name = "Reports", description = "Endpoints for submitting report requests, checking status, and downloading outputs")
public class ReportController {

    private final ReportService reportService;

    public ReportController(ReportService reportService) {
        this.reportService = reportService;
    }

    @PostMapping
    @Operation(summary = "Submit a report request (stores PENDING in DB, calls C++ rendering service, persists output, marks DONE)")
    public ResponseEntity<ReportResponseDto> createReport(@Valid @RequestBody CreateReportRequestDto dto) {
        ReportResponseDto response = reportService.createReport(dto);
        return ResponseEntity.status(HttpStatus.CREATED).body(response);
    }

    @GetMapping
    @Operation(summary = "List all report requests")
    public ResponseEntity<List<ReportResponseDto>> listReports() {
        return ResponseEntity.ok(reportService.getAllReports());
    }

    @GetMapping("/{id}")
    @Operation(summary = "Get report request details and generation status")
    public ResponseEntity<ReportResponseDto> getReport(@PathVariable Long id) {
        return ResponseEntity.ok(reportService.getReportById(id));
    }

    @GetMapping("/{id}/output")
    @Operation(summary = "Fetch and download the generated report binary file (PDF or Excel)")
    public ResponseEntity<byte[]> getReportOutput(@PathVariable Long id) {
        ReportOutput output = reportService.getReportOutput(id);

        String format = (output.getFormat() != null) ? output.getFormat().toUpperCase() : "PDF";
        String contentType = "application/pdf";
        String extension = "pdf";

        if ("XLSX".equals(format) || "EXCEL".equals(format)) {
            contentType = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
            extension = "xlsx";
        } else if ("CSV".equals(format)) {
            contentType = "text/csv";
            extension = "csv";
        }

        HttpHeaders headers = new HttpHeaders();
        headers.setContentType(MediaType.parseMediaType(contentType));
        headers.setContentDispositionFormData("attachment", "report_" + id + "." + extension);
        headers.setContentLength(output.getFileData() != null ? output.getFileData().length : 0);

        return ResponseEntity.ok()
                .headers(headers)
                .body(output.getFileData());
    }
}
