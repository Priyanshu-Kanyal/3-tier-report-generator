package com.reportgen.controller;

import com.reportgen.dto.TemplateResponseDto;
import com.reportgen.service.ReportService;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.List;

@RestController
@RequestMapping("/api/templates")
@CrossOrigin(origins = "*")
@Tag(name = "Report Templates", description = "Endpoints for managing and inspecting report templates and schemas")
public class TemplateController {

    private final ReportService reportService;

    public TemplateController(ReportService reportService) {
        this.reportService = reportService;
    }

    @GetMapping
    @Operation(summary = "List all report templates")
    public ResponseEntity<List<TemplateResponseDto>> listTemplates() {
        return ResponseEntity.ok(reportService.getAllTemplates());
    }

    @GetMapping("/{id}")
    @Operation(summary = "Get a single template by ID including its JSON schema")
    public ResponseEntity<TemplateResponseDto> getTemplate(@PathVariable Long id) {
        return ResponseEntity.ok(reportService.getTemplateById(id));
    }
}
